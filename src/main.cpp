// src/main.cpp — Phase 0/1 CLI.
//   mojave --discover            list discovered FNV installs
//   mojave --esm-info [path]     parse TES4 header + count records
//                                (path optional; auto-discovers otherwise)
#include <iostream>

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
                      << "  groups:      " << s.topLevelGroups << "\n"
                      << "  recordTypes: " << s.recordTypes.size() << "\n"
                      << "  fileSize:    " << s.fileSize << "\n"
                      << "  digest:      " << std::hex << s.digest << std::dec << "\n";
        } catch (const std::exception& e) {
            std::cerr << "ERROR: " << e.what() << "\n";
            return 1;
        }
        return 0;
    }

    std::cout << "Usage: mojave --discover | --esm-info [path]\n";
    return 0;
}
