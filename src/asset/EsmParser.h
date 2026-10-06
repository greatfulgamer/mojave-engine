#pragma once
// src/asset/EsmParser.h — bounded ESM/ESP reader (Phase 1 step 1).
// Format reference: docs/formats/esm-records.md (citations: UESP, xNVSE, OpenNV).
// Scope of THIS iteration: file header (TES4 + HEDR) + top-level GRUP walk +
// record count + stable digest. No record-body interpretation yet (next slice).

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace mojave::asset {

struct EsmHeader {
    float version = 0.0f;   // 1.34 for Fallout: New Vegas
    uint32_t numRecords = 0; // HEDR: authoritative total
    uint32_t nextObjectId = 0;
};

struct EsmStats {
    EsmHeader header;
    uint64_t walkedRecords = 0;   // every record reached via GRUP descent
    uint64_t groupsSeen = 0;      // all GRUPs (any depth)
    uint64_t topLevelGroups = 0;  // GRUPs at depth 0
    std::vector<std::string> recordTypes; // sorted unique 4CCs seen
    uint64_t fileSize = 0;
    uint64_t digest = 0;          // FNV-1a over type+formID of every walked record
};

class EsmParser {
public:
    // Throws std::runtime_error on malformed input (fail closed, per AGENTS.md).
    static EsmStats Parse(const fs::path& esmPath);
};

} // namespace mojave::asset
