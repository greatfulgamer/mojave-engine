#include "asset/TerrainMap.h"
#include "asset/RecordData.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>

namespace mojave::asset {

namespace {

uint32_t U32(const unsigned char* p) {
    return uint32_t(p[0]) | (uint32_t(p[1]) << 8) | (uint32_t(p[2]) << 16) |
           (uint32_t(p[3]) << 24);
}
uint16_t U16(const unsigned char* p) {
    return uint16_t(p[0]) | (uint16_t(p[1]) << 8);
}
int32_t I32(const unsigned char* p) { return static_cast<int32_t>(U32(p)); }
float F32(const unsigned char* p) {
    uint32_t b = U32(p);
    float f;
    std::memcpy(&f, &b, 4);
    return f;
}
std::string CC(const unsigned char* p) {
    return std::string(reinterpret_cast<const char*>(p), 4);
}

constexpr size_t kRecHdr = 24;

// One exterior cell's terrain: 33x33 vertex heights (world units).
struct CellTerrain {
    int gx = 0, gy = 0;
    std::vector<float> h;  // 33*33
    bool valid = false;
};

// LAND VHGT: float32 base height (world units) then 33*33 int8 deltas,
// each delta in units of 8 world units. Every delta (including the first)
// advances the running height; h[i] = base + sum(deltas[0..i]) * 8.
// Citation: docs/formats/land-records.md (UESP Tes4Mod:Land; OpenMW loader).
bool DecodeVhgt(const std::vector<unsigned char>& d, std::vector<float>& out) {
    if (d.size() < 4 + 33 * 33) return false;
    const float base = F32(d.data());
    out.assign(33 * 33, 0.0f);
    float cur = base;
    const int8_t* deltas = reinterpret_cast<const int8_t*>(d.data() + 4);
    for (int i = 0; i < 33 * 33; ++i) {
        cur += static_cast<float>(deltas[i]) * 8.0f;
        out[i] = cur;
    }
    return true;
}

} // namespace

TerrainResult TerrainMap::Render(const fs::path& esmPath, int gx0, int gy0,
                                 int gW, int gH, const fs::path& outPpm,
                                 int ppv) {
    TerrainResult res;
    std::ifstream f(esmPath, std::ios::binary);
    if (!f) throw std::runtime_error("TerrainMap: cannot open " + esmPath.string());
    f.seekg(0, std::ios::end);
    const uint64_t fileSize = static_cast<uint64_t>(f.tellg());
    f.seekg(0);
    {
        std::vector<unsigned char> h(kRecHdr);
        f.read(reinterpret_cast<char*>(h.data()), kRecHdr);
        const uint32_t ds = U32(h.data() + 4);
        f.seekg(std::streamoff(kRecHdr + ds));
    }

    std::map<std::pair<int,int>, CellTerrain> cells;
    int lastGx = INT32_MIN, lastGy = INT32_MIN;
    bool haveGrid = false;
    uint64_t cellsWithXclc = 0, landsSeen = 0, landsCompressed = 0, landsInRange = 0,
             cellsSeen = 0, cellsCompressed = 0;
    std::vector<unsigned char> rh(kRecHdr);
    while (true) {
        const std::streampos pos = f.tellg();
        if (pos < 0 || static_cast<uint64_t>(pos) >= fileSize) break;
        f.read(reinterpret_cast<char*>(rh.data()), kRecHdr);
        if (f.gcount() != static_cast<std::streamsize>(kRecHdr)) break;
        const std::string type = CC(rh.data());
        const uint32_t size = U32(rh.data() + 4);
        const uint32_t flags = U32(rh.data() + 8);
        if (type == "GRUP") continue;
        if (type == "CELL") { ++cellsSeen; if (RecordIsCompressed(flags)) ++cellsCompressed; }
        if (type == "LAND") {
            ++landsSeen;
            if (RecordIsCompressed(flags)) ++landsCompressed;
        }

        const bool wantedType = (type == "CELL" || type == "LAND");
        if (wantedType && size >= 6) {
            std::vector<unsigned char> raw(size);
            f.read(reinterpret_cast<char*>(raw.data()), size);
            std::vector<unsigned char> d;
            if (f.gcount() == static_cast<std::streamsize>(size) &&
                RecordData(raw, flags, d)) {
                if (type == "CELL") {
                    // XCLC carries the exterior grid: int32 x, int32 y.
                    for (size_t off = 0; off + 6 <= d.size();) {
                        const std::string sub = CC(d.data() + off);
                        const uint16_t ss = U16(d.data() + off + 4);
                        if (off + 6 + ss > d.size()) break;
                        if (sub == "XCLC" && ss >= 8) {
                            lastGx = I32(d.data() + off + 6);
                            lastGy = I32(d.data() + off + 10);
                            haveGrid = true;
                            break;
                        }
                        off += 6 + ss;
                    }
                } else { // LAND
                    if (haveGrid && lastGx >= gx0 && lastGx < gx0 + gW &&
                        lastGy >= gy0 && lastGy < gy0 + gH) {
                        ++landsInRange;
                        for (size_t off = 0; off + 6 <= d.size();) {
                            const std::string sub = CC(d.data() + off);
                            const uint16_t ss = U16(d.data() + off + 4);
                            if (off + 6 + ss > d.size()) break;
                            if (sub == "VHGT") {
                                CellTerrain ct;
                                ct.gx = lastGx; ct.gy = lastGy;
                                ct.valid = DecodeVhgt(
                                    std::vector<unsigned char>(d.begin() + off + 6,
                                                               d.begin() + off + 6 + ss),
                                    ct.h);
                                if (ct.valid) {
                                    if (cells.size() < 6) {
                                        float cmn=ct.h[0], cmx=ct.h[0];
                                        for (float z : ct.h) { cmn=std::min(cmn,z); cmx=std::max(cmx,z); }
                                        std::cout << "  [cell " << ct.gx << "," << ct.gy
                                                  << "] base=" << ct.h[0]
                                                  << " min=" << cmn << " max=" << cmx << "\n";
                                    }
                                    cells[{ct.gx, ct.gy}] = std::move(ct);
                                }
                                break;
                            }
                            off += 6 + ss;
                        }
                    }
                }
            }
        }
        f.seekg(pos + std::streamoff(kRecHdr + size));
    }

    std::cout << "  [debug] cellsSeen=" << cellsSeen
              << " cellsWithXclc=" << cellsWithXclc
              << " landsSeen=" << landsSeen
              << " landsCompressed=" << landsCompressed
              << " landsInRange=" << landsInRange
              << " cellsCompressed=" << cellsCompressed << "\n";
    if (cells.empty()) return res;

    const int vW = gW * 32 + 1;   // shared edges between adjacent cells
    const int vH = gH * 32 + 1;
    std::vector<float> grid(vW * vH, 0.0f);
    std::vector<char> have(vW * vH, 0);
    float mn = 1e30f, mx = -1e30f;
    for (const auto& [key, ct] : cells) {
        const int ox = (ct.gx - gx0) * 32, oy = (ct.gy - gy0) * 32;
        for (int y = 0; y < 33; ++y)
            for (int x = 0; x < 33; ++x) {
                const int X = ox + x, Y = oy + y;
                if (X < 0 || Y < 0 || X >= vW || Y >= vH) continue;
                const float z = ct.h[y * 33 + x];
                grid[Y * vW + X] = z;
                have[Y * vW + X] = 1;
                mn = std::min(mn, z);
                mx = std::max(mx, z);
            }
        ++res.cellsComposite;
    }

    const int W = vW * ppv, H = vH * ppv;
    res.width = W; res.height = H; res.minZ = mn; res.maxZ = mx;
    // Robust height range for shading: clamp to the 2nd..98th percentile so
    // void/degenerate edge cells do not flatten the relief contrast.
    std::vector<float> sorted;
    sorted.reserve(grid.size());
    for (size_t i = 0; i < grid.size(); ++i)
        if (have[i]) sorted.push_back(grid[i]);
    std::sort(sorted.begin(), sorted.end());
    float lo = mn, hi = mx;
    if (!sorted.empty()) {
        lo = sorted[size_t(sorted.size() * 0.02)];
        hi = sorted[std::min(sorted.size() - 1, size_t(sorted.size() * 0.98))];
    }
    const float span = std::max(hi - lo, 1.0f);
    std::vector<unsigned char> img(size_t(W) * H * 3);
    // Hillshade: light from NW, slope from central differences.
    for (int py = 0; py < H; ++py) {
        for (int px = 0; px < W; ++px) {
            const int vx = std::min(px / ppv, vW - 1);
            const int vy = std::min(py / ppv, vH - 1);
            const int xm = std::max(vx - 1, 0), xp = std::min(vx + 1, vW - 1);
            const int ym = std::max(vy - 1, 0), yp = std::min(vy + 1, vH - 1);
            const float dzdx = (grid[vy * vW + xp] - grid[vy * vW + xm]);
            const float dzdy = (grid[yp * vW + vx] - grid[ym * vW + vx]);
            // Vertex spacing: one exterior cell = 4096 world units / 32 = 128.
            // Slopes are dz per 2*128 units (central difference).
            const float nx = -dzdx / 256.0f, ny = -dzdy / 256.0f, nz = 1.0f;
            const float len = std::sqrt(nx * nx + ny * ny + nz * nz);
            const float lx = -0.57f, ly = 0.57f, lz = 0.59f; // NW light
            float shade = (nx * lx + ny * ly + nz * lz) / len;
            shade = std::clamp(shade, 0.0f, 1.0f);
            const float t = (grid[vy * vW + vx] - lo) / span;
            const unsigned char base = static_cast<unsigned char>(40 + 120 * t);
            const unsigned char v =
                static_cast<unsigned char>(std::clamp(base * (0.35f + 0.9f * shade), 0.0f, 255.0f));
            const size_t idx = (size_t(py) * W + px) * 3;
            // Desert palette: sand/rock (not green — Commander's note).
            img[idx] = static_cast<unsigned char>(std::clamp(v * 1.00f, 0.0f, 255.0f));
            img[idx + 1] = static_cast<unsigned char>(std::clamp(v * 0.86f, 0.0f, 255.0f));
            img[idx + 2] = static_cast<unsigned char>(std::clamp(v * 0.60f, 0.0f, 255.0f));
        }
    }
    std::ofstream o(outPpm, std::ios::binary);
    if (!o) return res;
    o << "P6\n" << W << " " << H << "\n255\n";
    o.write(reinterpret_cast<const char*>(img.data()),
            static_cast<std::streamsize>(img.size()));
    res.wrote = static_cast<bool>(o);
    return res;
}

} // namespace mojave::asset
