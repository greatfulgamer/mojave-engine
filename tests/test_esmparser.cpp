// tests/test_esmparser.cpp
// Two layers (playbook Part 11.1):
//   L1 goldens: synthetic miniature ESM fixture — runs everywhere (CI).
//   Real-file golden: parses the player's FalloutNV.esm when available
//                     (MOJAVE_GAME_PATH set or auto-discovered); prints SKIP otherwise.
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

#include "asset/EsmParser.h"
#include "platform/AssetLocator.h"

namespace fs = std::filesystem;
using mojave::asset::EsmParser;

namespace {

void PutU32(std::vector<unsigned char>& v, uint32_t x) {
    v.push_back(x & 0xff); v.push_back((x >> 8) & 0xff);
    v.push_back((x >> 16) & 0xff); v.push_back((x >> 24) & 0xff);
}
void PutU16(std::vector<unsigned char>& v, uint16_t x) {
    v.push_back(x & 0xff); v.push_back((x >> 8) & 0xff);
}
void PutStr(std::vector<unsigned char>& v, const char* s) {
    for (int i = 0; i < 4; ++i) v.push_back(static_cast<unsigned char>(s[i]));
}
void PutF32(std::vector<unsigned char>& v, float f) {
    uint32_t bits; std::memcpy(&bits, &f, 4); PutU32(v, bits);
}

// Build a minimal valid ESM: TES4 header (HEDR) + one GRUP with two records.
std::vector<unsigned char> BuildMiniEsm() {
    std::vector<unsigned char> hed;
    PutStr(hed, "HEDR");
    PutU16(hed, 12);
    PutF32(hed, 1.34f);
    PutU32(hed, 2);       // numRecords
    PutU32(hed, 0x800);   // nextObjectId

    std::vector<unsigned char> f;
    PutStr(f, "TES4");
    PutU32(f, static_cast<uint32_t>(hed.size()));
    PutU32(f, 0);         // flags
    PutU32(f, 0);         // formID
    PutU16(f, 0); PutU16(f, 0); PutU16(f, 0); PutU16(f, 0);
    f.insert(f.end(), hed.begin(), hed.end());

    // GRUP (type GRUP, label = 'WEAP' as a 4CC u32) containing records.
    std::vector<unsigned char> body;
    for (const char* t : {"WEAP", "WEAP"}) {
        PutStr(body, t);
        PutU32(body, 0);      // dataSize
        PutU32(body, 0);      // flags
        PutU32(body, 0x100);  // formID
        PutU16(body, 0); PutU16(body, 0); PutU16(body, 0); PutU16(body, 0);
    }
    PutStr(f, "GRUP");
    PutU32(f, static_cast<uint32_t>(24 + body.size()));
    PutStr(f, "WEAP");
    PutU32(f, 0);  PutU16(f, 0); PutU16(f, 0); PutU16(f, 0); PutU16(f, 0);
    f.insert(f.end(), body.begin(), body.end());
    return f;
}

} // namespace

int main() {
    // ---- L1 synthetic golden ----
    const fs::path p = fs::temp_directory_path() / "mojave_mini.esm";
    {
        auto bytes = BuildMiniEsm();
        std::ofstream(p, std::ios::binary).write(
            reinterpret_cast<const char*>(bytes.data()),
            static_cast<std::streamsize>(bytes.size()));
    }
    auto s = EsmParser::Parse(p);
    assert(s.header.numRecords == 2);
    assert(s.header.version > 1.33f && s.header.version < 1.35f);
    assert(s.walkedRecords == 2 && "records inside the GRUP must be descended into");
    assert(s.topLevelGroups == 1);
    assert(s.groupsSeen == 1);
    assert(s.digest != 0);
    assert(!s.recordTypes.empty() && s.recordTypes[0] == "WEAP");
    std::cout << "PASS: synthetic ESM golden (version=" << s.header.version
              << ", numRecords=" << s.header.numRecords
              << ", walked=" << s.walkedRecords
              << ", groups=" << s.topLevelGroups << ")\n";
    fs::remove(p);

    // ---- Real-file golden (skip cleanly when no install) ----
    fs::path esm;
    if (const char* env = std::getenv("MOJAVE_GAME_PATH")) {
        esm = fs::path(env) / "Data" / "FalloutNV.esm";
    } else {
        auto found = mojave::platform::AssetLocator::Discover();
        if (!found.empty()) esm = found.front().root / "Data" / "FalloutNV.esm";
    }
    if (!fs::exists(esm)) {
        std::cout << "SKIP: real FalloutNV.esm not available on this machine\n";
        return 0;
    }
    auto real = EsmParser::Parse(esm);
    std::cout << "PASS: real esm — version=" << real.header.version
              << " numRecords=" << real.header.numRecords
              << " walked=" << real.walkedRecords
              << " groups=" << real.groupsSeen
              << " fileSize=" << real.fileSize
              << " digest=" << std::hex << real.digest << std::dec << "\n";
    assert(real.header.version > 1.33f && real.header.version < 1.35f);
    assert(real.header.numRecords > 100000 && "FNV master should have >100k records");
    assert(real.walkedRecords > 450000 && "GRUP descent must reach the bulk of the file");
    // Discovered invariant (documented in docs/formats/esm-records.md):
    // HEDR numRecords counts GRUP headers too: walked + groups == numRecords.
    assert(real.walkedRecords + real.groupsSeen == real.header.numRecords &&
           "HEDR total must equal walked records + group headers");
    assert(real.digest != 0);
    // Census invariants (FormDB seed).
    uint64_t sum = 0;
    for (const auto& [t, c] : real.typeCounts) sum += c;
    assert(sum == real.walkedRecords && "type census must sum to walked records");
    for (const char* t : {"CELL", "REFR", "NPC_", "WEAP", "WRLD", "CONT", "STAT"})
        assert(real.typeCounts.count(t) && "expected record type missing from census");
    assert(real.typeCounts.at("CELL") > 1000 && "FNV has thousands of cells");
    assert(real.typeCounts.at("REFR") > 10000 && "FNV has tens of thousands of refs");
    // EDID samples extracted (FormDB seed).
    assert(!real.typeSamples["CELL"].empty() && "CELL EDID samples expected");
    std::cout << "PASS: census — " << real.typeCounts.size() << " types, top: ";
    {
        std::vector<std::pair<uint64_t, std::string>> v;
        for (const auto& [t, c] : real.typeCounts) v.push_back({c, t});
        std::sort(v.begin(), v.end(), std::greater<>());
        for (size_t i = 0; i < 5 && i < v.size(); ++i)
            std::cout << v[i].second << "=" << v[i].first << " ";
        std::cout << "\n  sample CELL: " << real.typeSamples["CELL"][0] << "\n";
    }
    std::cout << "ALL ESMPARSER TESTS PASSED\n";
    return 0;
}
