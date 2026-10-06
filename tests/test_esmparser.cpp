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
    assert(s.walkedRecords == 0 && "records inside GRUP are not top-level walked yet");
    assert(s.topLevelGroups == 1);
    std::cout << "PASS: synthetic ESM golden (version=" << s.header.version
              << ", numRecords=" << s.header.numRecords
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
              << " groups=" << real.topLevelGroups
              << " fileSize=" << real.fileSize
              << " digest=" << std::hex << real.digest << std::dec << "\n";
    assert(real.header.version > 1.33f && real.header.version < 1.35f);
    assert(real.header.numRecords > 100000 && "FNV master should have >100k records");
    std::cout << "ALL ESMPARSER TESTS PASSED\n";
    return 0;
}
