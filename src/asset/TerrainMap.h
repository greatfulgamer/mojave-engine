#pragma once
// src/asset/TerrainMap.h — read real terrain from LAND records and render a
// shaded-relief image. This is the playbook's Phase 1 first visual: the actual
// Mojave heightmap, not a scatter plot.
// Format citations: docs/formats/land-records.md (UESP VHGT/VNML/VCLR, XCLC).

#include <cstdint>
#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

namespace mojave::asset {

struct TerrainResult {
    int cellsComposite = 0;
    int width = 0, height = 0;
    float minZ = 0, maxZ = 0;
    bool wrote = false;
};

class TerrainMap {
public:
    // Composite the heightmaps of exterior cells [gx0..gx0+gW) x [gy0..gy0+gH)
    // and write a shaded-relief PPM. Exterior cell grid: 4096 world units per
    // cell; LAND VHGT gives a 33x33 vertex grid per cell (shared edges).
    static TerrainResult Render(const fs::path& esmPath, int gx0, int gy0,
                                int gW, int gH, const fs::path& outPpm,
                                int pixelsPerVertex);
};

} // namespace mojave::asset
