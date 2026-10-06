#include "asset/EsmParser.h"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <set>
#include <stdexcept>

namespace mojave::asset {

namespace {

uint32_t ReadU32(const unsigned char* p) {
    return uint32_t(p[0]) | (uint32_t(p[1]) << 8) | (uint32_t(p[2]) << 16) |
           (uint32_t(p[3]) << 24);
}

uint16_t ReadU16(const unsigned char* p) {
    return uint16_t(p[0]) | (uint16_t(p[1]) << 8);
}

float ReadF32(const unsigned char* p) {
    uint32_t bits = ReadU32(p);
    float f;
    std::memcpy(&f, &bits, sizeof(f));
    return f;
}

std::string FourCC(const unsigned char* p) {
    return std::string(reinterpret_cast<const char*>(p), 4);
}

uint64_t Fnv1a(uint64_t h, const unsigned char* data, size_t n) {
    constexpr uint64_t kOffset = 1469598103934665603ull;
    constexpr uint64_t kPrime = 1099511628211ull;
    if (h == 0) h = kOffset;
    for (size_t i = 0; i < n; ++i) {
        h ^= data[i];
        h *= kPrime;
    }
    return h;
}

// Record header is 24 bytes: type(4) size(4) flags(4) formID(4) ts(2) vci(2) iv(2) unk(2)
// Citation: docs/formats/esm-records.md (UESP Tes4Mod record header).
constexpr size_t kRecordHeaderSize = 24;
constexpr size_t kGrupHeaderSize = 24;

} // namespace

EsmStats EsmParser::Parse(const fs::path& esmPath) {
    std::ifstream f(esmPath, std::ios::binary);
    if (!f) throw std::runtime_error("EsmParser: cannot open " + esmPath.string());

    f.seekg(0, std::ios::end);
    const uint64_t fileSize = static_cast<uint64_t>(f.tellg());
    f.seekg(0, std::ios::beg);
    if (fileSize < kRecordHeaderSize) throw std::runtime_error("EsmParser: file too small");

    std::vector<unsigned char> buf(kRecordHeaderSize);
    f.read(reinterpret_cast<char*>(buf.data()), kRecordHeaderSize);
    if (FourCC(buf.data()) != "TES4")
        throw std::runtime_error("EsmParser: missing TES4 header");

    const uint32_t headerDataSize = ReadU32(buf.data() + 4);
    const uint32_t headerFlags = ReadU32(buf.data() + 8);
    (void)headerFlags;

    EsmStats stats;
    stats.fileSize = fileSize;

    // Walk the header record's subrecords to find HEDR.
    std::vector<unsigned char> hdr(headerDataSize);
    f.read(reinterpret_cast<char*>(hdr.data()), headerDataSize);
    size_t off = 0;
    while (off + 6 <= headerDataSize) {
        const std::string sub = FourCC(hdr.data() + off);
        const uint32_t subSize = ReadU16(hdr.data() + off + 4);
        off += 6;
        if (off + subSize > headerDataSize) break;
        if (sub == "HEDR" && subSize >= 12) {
            stats.header.version = ReadF32(hdr.data() + off);
            stats.header.numRecords = ReadU32(hdr.data() + off + 4);
            stats.header.nextObjectId = ReadU32(hdr.data() + off + 8);
        }
        off += subSize;
    }

    // Walk GRUPs with a depth stack (playbook: proper group semantics, not
    // seek-past). Flat census of every record + top-level group distinction.
    std::vector<uint64_t> groupEnds;  // stack: end offset of each open GRUP
    std::vector<unsigned char> rh(kRecordHeaderSize);
    std::set<std::string> types;
    uint64_t digest = 0;
    while (true) {
        const std::streampos pos = f.tellg();
        if (pos < 0) break;
        const uint64_t upos = static_cast<uint64_t>(pos);
        while (!groupEnds.empty() && upos >= groupEnds.back()) groupEnds.pop_back();
        if (upos >= fileSize) break;

        f.read(reinterpret_cast<char*>(rh.data()), kRecordHeaderSize);
        if (f.gcount() != static_cast<std::streamsize>(kRecordHeaderSize)) break;
        const std::string type = FourCC(rh.data());
        const uint32_t size = ReadU32(rh.data() + 4);
        const uint32_t formId = ReadU32(rh.data() + 12);

        if (type == "GRUP") {
            ++stats.groupsSeen;
            if (groupEnds.empty()) ++stats.topLevelGroups;
            groupEnds.push_back(upos + size); // group size includes its 24B header
            continue;                         // descend: next read is first child
        }
        ++stats.walkedRecords;
        types.insert(type);
        ++stats.typeCounts[type];
        digest = Fnv1a(digest, reinterpret_cast<const unsigned char*>(type.data()), 4);
        digest = Fnv1a(digest, reinterpret_cast<const unsigned char*>(&formId), 4);

        // FormDB seed: sample the EDID (editor ID) subrecord when cheap to read.
        // Skip compressed records (flags bit 0x200) — their body needs zlib.
        const uint32_t flags = ReadU32(rh.data() + 8);
        if (!(flags & 0x200) && size >= 6 && stats.typeSamples[type].size() < 4) {
            std::vector<unsigned char> sub(6);
            f.read(reinterpret_cast<char*>(sub.data()), 6);
            if (f.gcount() == 6 && FourCC(sub.data()) == "EDID") {
                const uint16_t subSize = ReadU16(sub.data() + 4);
                if (subSize > 0 && subSize < 256) {
                    std::vector<char> name(subSize);
                    f.read(name.data(), subSize);
                    if (f.gcount() == subSize) {
                        std::string s(name.data(), subSize);
                        while (!s.empty() && (s.back() == '\0' || s.back() == '\r' ||
                                              s.back() == '\n'))
                            s.pop_back();
                        if (!s.empty()) stats.typeSamples[type].push_back(s);
                    }
                }
            }
        }
        f.seekg(pos + std::streamoff(kRecordHeaderSize + size));
    }

    stats.recordTypes.assign(types.begin(), types.end());
    stats.digest = digest;
    return stats;
}

namespace {

// Read the EDID subrecord (if it is the first subrecord) from record data.
std::string ReadLeadingEdid(std::ifstream& f, uint32_t dataSize, uint32_t flags) {
    if ((flags & 0x200) || dataSize < 6) return {};
    std::vector<unsigned char> sub(6);
    f.read(reinterpret_cast<char*>(sub.data()), 6);
    if (f.gcount() != 6 || FourCC(sub.data()) != "EDID") return {};
    const uint16_t subSize = ReadU16(sub.data() + 4);
    if (subSize == 0 || subSize >= 256) return {};
    std::vector<char> name(subSize);
    f.read(name.data(), subSize);
    if (f.gcount() != subSize) return {};
    std::string s(name.data(), subSize);
    while (!s.empty() && (s.back() == '\0' || s.back() == '\r' || s.back() == '\n'))
        s.pop_back();
    return s;
}

} // namespace

std::vector<NamedRecord> EsmParser::FindByEdid(const fs::path& esmPath,
                                               const std::string& substr) {
    std::vector<NamedRecord> out;
    std::ifstream f(esmPath, std::ios::binary);
    if (!f) throw std::runtime_error("EsmParser: cannot open " + esmPath.string());

    f.seekg(0, std::ios::end);
    const uint64_t fileSize = static_cast<uint64_t>(f.tellg());
    const std::vector<unsigned char> hdr = [&] {
        std::vector<unsigned char> h(kRecordHeaderSize);
        f.seekg(0);
        f.read(reinterpret_cast<char*>(h.data()), kRecordHeaderSize);
        return h;
    }();
    (void)hdr;
    // skip header record data
    f.seekg(0);
    {
        std::vector<unsigned char> h(kRecordHeaderSize);
        f.read(reinterpret_cast<char*>(h.data()), kRecordHeaderSize);
        const uint32_t ds = ReadU32(h.data() + 4);
        f.seekg(std::streamoff(kRecordHeaderSize + ds));
    }

    std::vector<unsigned char> rh(kRecordHeaderSize);
    while (true) {
        const std::streampos pos = f.tellg();
        if (pos < 0 || static_cast<uint64_t>(pos) >= fileSize) break;
        f.read(reinterpret_cast<char*>(rh.data()), kRecordHeaderSize);
        if (f.gcount() != static_cast<std::streamsize>(kRecordHeaderSize)) break;
        const std::string type = FourCC(rh.data());
        const uint32_t size = ReadU32(rh.data() + 4);
        const uint32_t flags = ReadU32(rh.data() + 8);
        const uint32_t formId = ReadU32(rh.data() + 12);
        if (type == "GRUP") continue;
        const std::string edid = ReadLeadingEdid(f, size, flags);
        if (!edid.empty() && edid.find(substr) != std::string::npos)
            out.push_back({type, formId, edid});
        f.seekg(pos + std::streamoff(kRecordHeaderSize + size));
    }
    return out;
}

std::vector<CellRef> EsmParser::CellReferences(const fs::path& esmPath,
                                               uint32_t cellFormId) {
    std::vector<CellRef> out;
    std::ifstream f(esmPath, std::ios::binary);
    if (!f) throw std::runtime_error("EsmParser: cannot open " + esmPath.string());
    f.seekg(0, std::ios::end);
    const uint64_t fileSize = static_cast<uint64_t>(f.tellg());
    f.seekg(0);
    {
        std::vector<unsigned char> h(kRecordHeaderSize);
        f.read(reinterpret_cast<char*>(h.data()), kRecordHeaderSize);
        const uint32_t ds = ReadU32(h.data() + 4);
        f.seekg(std::streamoff(kRecordHeaderSize + ds));
    }

    std::vector<uint64_t> groupEnds;   // open GRUP ends
    std::vector<bool> targetGroups;   // parallel: is this the target cell's group?
    std::vector<unsigned char> rh(kRecordHeaderSize);
    while (true) {
        const std::streampos pos = f.tellg();
        if (pos < 0) break;
        const uint64_t upos = static_cast<uint64_t>(pos);
        while (!groupEnds.empty() && upos >= groupEnds.back()) {
            groupEnds.pop_back();
            targetGroups.pop_back();
        }
        if (upos >= fileSize) break;
        f.read(reinterpret_cast<char*>(rh.data()), kRecordHeaderSize);
        if (f.gcount() != static_cast<std::streamsize>(kRecordHeaderSize)) break;
        const std::string type = FourCC(rh.data());
        const uint32_t size = ReadU32(rh.data() + 4);
        const uint32_t flags = ReadU32(rh.data() + 8);
        const uint32_t formId = ReadU32(rh.data() + 12);

        if (type == "GRUP") {
            // GRUP layout: type(4) size(4) label(4) groupType(4) ...
            const uint32_t label = ReadU32(rh.data() + 8);
            const uint32_t gtype = ReadU32(rh.data() + 12);
            const bool isTarget =
                (label == cellFormId) || (gtype == 8 || gtype == 6) &&
                !groupEnds.empty() && targetGroups.back();
            groupEnds.push_back(upos + size);
            targetGroups.push_back(isTarget);
            continue;
        }

        const bool inTarget = !targetGroups.empty() && targetGroups.back();
        if (inTarget && (type == "REFR" || type == "ACHR") && !(flags & 0x200) &&
            size >= 6) {
            // DATA subrecord: x,y,z (float32) + rotation, 24 bytes in FNV.
            std::vector<unsigned char> d(size);
            f.read(reinterpret_cast<char*>(d.data()), size);
            if (f.gcount() == static_cast<std::streamsize>(size)) {
                for (size_t off = 0; off + 6 <= size;) {
                    const std::string sub = FourCC(d.data() + off);
                    const uint16_t ss = ReadU16(d.data() + off + 4);
                    if (off + 6 + ss > size) break;
                    if (sub == "DATA" && ss >= 12) {
                        CellRef r;
                        r.formId = formId;
                        r.x = ReadF32(d.data() + off + 6);
                        r.y = ReadF32(d.data() + off + 10);
                        r.z = ReadF32(d.data() + off + 14);
                        out.push_back(r);
                        break;
                    }
                    off += 6 + ss;
                }
            }
        }
        f.seekg(pos + std::streamoff(kRecordHeaderSize + size));
    }
    return out;
}

bool EsmParser::WritePpm(const std::vector<CellRef>& refs, const fs::path& out,
                         int width, int height) {
    if (refs.empty() || width <= 0 || height <= 0) return false;
    float minX = refs[0].x, maxX = refs[0].x, minY = refs[0].y, maxY = refs[0].y;
    for (const auto& r : refs) {
        minX = std::min(minX, r.x); maxX = std::max(maxX, r.x);
        minY = std::min(minY, r.y); maxY = std::max(maxY, r.y);
    }
    const float spanX = std::max(maxX - minX, 1.0f);
    const float spanY = std::max(maxY - minY, 1.0f);
    const int pad = 20;
    const int w = width, h = height;
    std::vector<unsigned char> img(size_t(w) * h * 3, 0);
    for (size_t i = 0; i < img.size(); i += 3) { img[i] = 11; img[i+1] = 11; img[i+2] = 11; }
    for (const auto& r : refs) {
        const int px = pad + int((r.x - minX) / spanX * (w - 2 * pad));
        const int py = h - 1 - (pad + int((r.y - minY) / spanY * (h - 2 * pad)));
        for (int dy = -1; dy <= 1; ++dy)
            for (int dx = -1; dx <= 1; ++dx) {
                const int x = px + dx, y = py + dy;
                if (x < 0 || y < 0 || x >= w || y >= h) continue;
                const size_t idx = (size_t(y) * w + x) * 3;
                img[idx] = 142; img[idx + 1] = 245; img[idx + 2] = 142;
            }
    }
    std::ofstream o(out, std::ios::binary);
    if (!o) return false;
    o << "P6\n" << w << " " << h << "\n255\n";
    o.write(reinterpret_cast<const char*>(img.data()),
            static_cast<std::streamsize>(img.size()));
    return static_cast<bool>(o);
}

} // namespace mojave::asset
