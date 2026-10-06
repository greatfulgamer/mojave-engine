#pragma once
// src/asset/EsmParser.h — bounded ESM/ESP reader (Phase 1 step 1).
// Format reference: docs/formats/esm-records.md (citations: UESP, xNVSE, OpenNV).
// Scope of THIS iteration: file header (TES4 + HEDR) + top-level GRUP walk +
// record count + stable digest. No record-body interpretation yet (next slice).

#include <cstdint>
#include <filesystem>
#include <map>
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
    std::map<std::string, uint64_t> typeCounts;        // census per 4CC
    std::map<std::string, std::vector<std::string>> typeSamples; // first EDIDs per 4CC
    uint64_t fileSize = 0;
    uint64_t digest = 0;          // FNV-1a over type+formID of every walked record
};

struct CellRef {
    uint32_t formId = 0;
    float x = 0, y = 0, z = 0;
};

struct NamedRecord {
    std::string type;
    uint32_t formId = 0;
    std::string edid;
};

class EsmParser {
public:
    // Throws std::runtime_error on malformed input (fail closed, per AGENTS.md).
    static EsmStats Parse(const fs::path& esmPath);

    // All records whose EDID contains `substr` (case-sensitive substring).
    static std::vector<NamedRecord> FindByEdid(const fs::path& esmPath,
                                               const std::string& substr);

    // References (REFR/ACHR) belonging to the cell with this formID, with the
    // world position from their DATA subrecord. This is the first "read the
    // world" query: raw object placement straight from the master file.
    static std::vector<CellRef> CellReferences(const fs::path& esmPath,
                                               uint32_t cellFormId);

    // Plot references to an 8-bit PPM (P6) — self-contained, no deps.
    // Returns false if the set is empty or the file cannot be written.
    static bool WritePpm(const std::vector<CellRef>& refs, const fs::path& out,
                         int width, int height);
};

} // namespace mojave::asset
