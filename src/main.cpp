// src/main.cpp — Phase 0/1 CLI.
//   mojave --discover            list discovered FNV installs
//   mojave --esm-info [path]     parse TES4 header + count records
//                                (path optional; auto-discovers otherwise)
#include <algorithm>
#include <functional>
#include <iostream>
#include <vector>

#include "asset/EsmParser.h"
#include "asset/TerrainMap.h"
#include "platform/AssetLocator.h"

namespace fs = std::filesystem;

int main(int argc, char** argv) {
    std::cout << "Mojave Engine v0.1.0\n";
    const std::string cmd = (argc > 1) ? argv[1] : "";

    if (cmd == "--discover") {
        auto installs = mojave::platform::AssetLocator::Discover();
        std::cout << "Found " << installs.size() << " install(s):\n";
        for (const auto& i : installs)
            std::cout << "  [" << i.store << "] " << i.root.string() << "\n";
        return 0;
    }

    if (cmd == "--esm-info") {
        fs::path esm;
        if (argc > 2) {
            esm = fs::path(argv[2]);
        } else {
            auto found = mojave::platform::AssetLocator::Discover();
            if (found.empty()) {
                std::cerr << "No install discovered; pass an explicit esm path.\n";
                return 1;
            }
            esm = found.front().root / "Data" / "FalloutNV.esm";
        }
        try {
            auto s = mojave::asset::EsmParser::Parse(esm);
            std::cout << "esm: " << esm.string() << "\n"
                      << "  version:     " << s.header.version << "\n"
                      << "  numRecords:  " << s.header.numRecords << "\n"
                      << "  nextObjectId:" << s.header.nextObjectId << "\n"
                      << "  walked:      " << s.walkedRecords << "\n"
                      << "  groups:      " << s.groupsSeen
                      << " (top-level " << s.topLevelGroups << ")\n"
                      << "  recordTypes: " << s.recordTypes.size() << "\n"
                      << "  fileSize:    " << s.fileSize << "\n"
                      << "  digest:      " << std::hex << s.digest << std::dec << "\n";
        } catch (const std::exception& e) {
            std::cerr << "ERROR: " << e.what() << "\n";
            return 1;
        }
        return 0;
    }

    if (cmd == "--esm-census") {
        fs::path esm;
        if (argc > 2) {
            esm = fs::path(argv[2]);
        } else {
            auto found = mojave::platform::AssetLocator::Discover();
            if (found.empty()) {
                std::cerr << "No install discovered; pass an explicit esm path.\n";
                return 1;
            }
            esm = found.front().root / "Data" / "FalloutNV.esm";
        }
        try {
            auto s = mojave::asset::EsmParser::Parse(esm);
            std::cout << "record census: " << esm.filename().string() << "\n";
            // top types by count
            std::vector<std::pair<uint64_t, std::string>> v;
            for (const auto& [t, c] : s.typeCounts) v.push_back({c, t});
            std::sort(v.begin(), v.end(), std::greater<>());
            for (size_t i = 0; i < v.size() && i < 25; ++i) {
                std::cout << "  " << v[i].second << "  " << v[i].first << "\n";
            }
            std::cout << "  ... " << (v.size() > 25 ? v.size() - 25 : 0)
                      << " more types, total records " << s.walkedRecords << "\n";
            for (const char* t : {"CELL", "WRLD", "NPC_", "WEAP"}) {
                auto it = s.typeSamples.find(t);
                if (it == s.typeSamples.end() || it->second.empty()) continue;
                std::cout << "  samples " << t << ":";
                for (const auto& n : it->second) std::cout << " " << n;
                std::cout << "\n";
            }
        } catch (const std::exception& e) {
            std::cerr << "ERROR: " << e.what() << "\n";
            return 1;
        }
        return 0;
    }

    if (cmd == "--find" && argc > 2) {
        fs::path esm;
        if (argc > 3) esm = fs::path(argv[3]);
        else {
            auto found = mojave::platform::AssetLocator::Discover();
            if (found.empty()) { std::cerr << "No install discovered.\n"; return 1; }
            esm = found.front().root / "Data" / "FalloutNV.esm";
        }
        try {
            auto hits = mojave::asset::EsmParser::FindByEdid(esm, argv[2]);
            std::cout << "matches for '" << argv[2] << "': " << hits.size() << "\n";
            for (size_t i = 0; i < hits.size(); ++i)
                std::cout << "  " << hits[i].type << " 0x" << std::hex
                          << hits[i].formId << std::dec << "  " << hits[i].edid << "\n";
        } catch (const std::exception& e) { std::cerr << "ERROR: " << e.what() << "\n"; return 1; }
        return 0;
    }

    if (cmd == "--cell-map" && argc > 2) {
        fs::path esm;
        fs::path outPpm = "/tmp/opencode/progress/cell-map.ppm";
        if (argc > 3) esm = fs::path(argv[3]);
        else {
            auto found = mojave::platform::AssetLocator::Discover();
            if (found.empty()) { std::cerr << "No install discovered.\n"; return 1; }
            esm = found.front().root / "Data" / "FalloutNV.esm";
        }
        try {
            auto cells = mojave::asset::EsmParser::FindByEdid(esm, argv[2]);
            size_t pick = cells.size();
            // Prefer an exact EDID match, then any CELL containing the substring.
            for (size_t i = 0; i < cells.size(); ++i)
                if (cells[i].type == "CELL" && cells[i].edid == argv[2]) { pick = i; break; }
            if (pick == cells.size())
                for (size_t i = 0; i < cells.size(); ++i)
                    if (cells[i].type == "CELL") { pick = i; break; }
            if (pick == cells.size()) { std::cerr << "No CELL matching '" << argv[2] << "'\n"; return 1; }
            const auto& cell = cells[pick];
            auto refs = mojave::asset::EsmParser::CellReferences(esm, cell.formId);
            std::cout << "cell: " << cell.edid << " (0x" << std::hex << cell.formId
                      << std::dec << ")\n  references: " << refs.size() << "\n";
            if (!refs.empty()) {
                float minX = refs[0].x, maxX = refs[0].x, minY = refs[0].y, maxY = refs[0].y;
                for (const auto& r : refs) {
                    minX = std::min(minX, r.x); maxX = std::max(maxX, r.x);
                    minY = std::min(minY, r.y); maxY = std::max(maxY, r.y);
                }
                std::cout << "  extent: x " << int(minX) << ".." << int(maxX)
                          << "  y " << int(minY) << ".." << int(maxY) << "\n";
                std::cout << "  sample: " << refs[0].formId << " at ("
                          << int(refs[0].x) << "," << int(refs[0].y) << ","
                          << int(refs[0].z) << ")\n";
            }
            fs::create_directories(outPpm.parent_path());
            if (mojave::asset::EsmParser::WritePpm(refs, outPpm, 1200, 900))
                std::cout << "  map written: " << outPpm.string() << "\n";
            else
                std::cerr << "  map not written (no refs?)\n";
        } catch (const std::exception& e) { std::cerr << "ERROR: " << e.what() << "\n"; return 1; }
        return 0;
    }

    if (cmd == "--terrain-map") {
        // Usage: --terrain-map <gx> <gy> [gW] [gH] [outPpm]
        if (argc < 4) { std::cerr << "usage: --terrain-map <gx> <gy> [gW] [gH] [out]\n"; return 1; }
        const int gx = std::atoi(argv[2]), gy = std::atoi(argv[3]);
        const int gW = (argc > 4) ? std::atoi(argv[4]) : 4;
        const int gH = (argc > 5) ? std::atoi(argv[5]) : 4;
        fs::path outPpm = (argc > 6) ? fs::path(argv[6])
                                     : fs::path("/tmp/opencode/progress/terrain.ppm");
        auto found = mojave::platform::AssetLocator::Discover();
        if (found.empty()) { std::cerr << "No install discovered.\n"; return 1; }
        const fs::path esm = found.front().root / "Data" / "FalloutNV.esm";
        try {
            fs::create_directories(outPpm.parent_path());
            auto t = mojave::asset::TerrainMap::Render(esm, gx, gy, gW, gH, outPpm, 8);
            std::cout << "terrain: cells " << gx << "," << gy << " .. "
                      << (gx + gW - 1) << "," << (gy + gH - 1)
                      << "  (" << t.cellsComposite << "/" << (gW * gH) << " cells found)\n"
                      << "  height range (world units): " << int(t.minZ) << " .. " << int(t.maxZ)
                      << "  -> " << int(t.maxZ - t.minZ) << " units vertical\n"
                      << "  image: " << t.width << "x" << t.height << "\n";
            if (t.wrote) std::cout << "  written: " << outPpm.string() << "\n";
            else std::cerr << "  FAILED to write image\n";
        } catch (const std::exception& e) { std::cerr << "ERROR: " << e.what() << "\n"; return 1; }
        return 0;
    }

    std::cout << "Usage: mojave --discover | --esm-info [path] | --esm-census [path]"
              << " | --find <substr> [esm] | --cell-map <edid> [esm]"
              << " | --terrain-map <gx> <gy> [gW] [gH] [out]\n";
    return 0;
}
