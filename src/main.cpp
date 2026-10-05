// src/main.cpp — Phase 0 demo: `./mojave --discover` prints found installs.
#include <iostream>

#include "platform/AssetLocator.h"

int main(int argc, char** argv) {
    const bool discover = (argc > 1 && std::string(argv[1]) == "--discover");
    std::cout << "Mojave Engine v0.1.0\n";
    if (discover) {
        auto installs = mojave::platform::AssetLocator::Discover();
        std::cout << "Found " << installs.size() << " install(s):\n";
        for (const auto& i : installs) {
            std::cout << "  [" << i.store << "] " << i.root.string() << "\n";
        }
        return 0;
    }
    std::cout << "Usage: mojave --discover\n";
    return 0;
}
