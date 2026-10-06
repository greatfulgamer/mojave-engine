#include "asset/EsmParser.h"

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
        digest = Fnv1a(digest, reinterpret_cast<const unsigned char*>(type.data()), 4);
        digest = Fnv1a(digest, reinterpret_cast<const unsigned char*>(&formId), 4);
        f.seekg(pos + std::streamoff(kRecordHeaderSize + size));
    }

    stats.recordTypes.assign(types.begin(), types.end());
    stats.digest = digest;
    return stats;
}

} // namespace mojave::asset
