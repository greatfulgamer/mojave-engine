// src/main.cpp — Phase 0/1 CLI.
//   mojave --discover            list discovered FNV installs
//   mojave --esm-info [path]     parse TES4 header + count records
//                                (path optional; auto-discovers otherwise)
#include <algorithm>
#include <functional>
#include <iostream>
#include <vector>

#include "asset/EsmParser.h"
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

    std::cout << "Usage: mojave --discover | --esm-info [path] | --esm-census [path]\n";
    return 0;
}
