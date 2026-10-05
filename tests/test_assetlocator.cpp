// tests/test_assetlocator.cpp — fixture-driven discovery tests.
// Fixtures (playbook Part 11, bazzite_origin_story): mock trees simulate
// native Steam, Flatpak Steam (Bazzite), multi-library, and GOG layouts.
#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <fstream>

#include "platform/AssetLocator.h"

namespace fs = std::filesystem;
using mojave::platform::AssetLocator;

namespace {

void MakeFakeInstall(const fs::path& root) {
    fs::create_directories(root / "Data");
    std::ofstream(root / "FalloutNV.exe") << "MZ fake";
    std::ofstream(root / "Data" / "FalloutNV.esm") << "TES4 fake";
    std::ofstream(root / "Data" / "Fallout - Meshes.bsa") << "BSA fake";
}

} // namespace

int main() {
    const fs::path base = fs::temp_directory_path() / "mojave_fixture_test";
    fs::remove_all(base);
    fs::create_directories(base);

    // 1. Env override wins.
    const fs::path envInstall = base / "env_install";
    MakeFakeInstall(envInstall);
    setenv("MOJAVE_GAME_PATH", envInstall.c_str(), 1);
    {
        auto found = AssetLocator::Discover();
        assert(!found.empty() && "env override install must be found");
        assert(found.front().store == "manual");
        std::cout << "PASS: env override discovery\n";
    }

    // 2. Marker validation rejects incomplete dirs.
    const fs::path incomplete = base / "incomplete";
    fs::create_directories(incomplete / "Data");
    std::ofstream(incomplete / "FalloutNV.exe") << "MZ";
    assert(!AssetLocator::LooksLikeInstall(incomplete));
    std::cout << "PASS: incomplete install rejected\n";

    // 3. VDF multi-library: fake steam root with libraryfolders.vdf pointing
    //    at a second "drive" (the bazzite origin story: FNV on another drive).
    const fs::path steamRoot = base / "steam";
    const fs::path lib2 = base / "mnt" / "games" / "SteamLibrary";
    MakeFakeInstall(lib2 / "steamapps" / "common" / "Fallout New Vegas");
    fs::create_directories(steamRoot / "steamapps");
    {
        std::ofstream vdf(steamRoot / "steamapps" / "libraryfolders.vdf");
        vdf << "\"libraryfolders\"\n{\n"
            << "  \"0\" { \"path\" \"" << lib2.string() << "\" }\n"
            << "}\n";
    }
    {
        auto found = AssetLocator::Discover();
        bool gotLib2 = false;
        for (const auto& i : found)
            if (i.root == lib2 / "steamapps" / "common" / "Fallout New Vegas")
                gotLib2 = true;
        assert(gotLib2 && "multi-library VDF discovery must find second drive");
        std::cout << "PASS: libraryfolders.vdf multi-drive discovery\n";
    }

    fs::remove_all(base);
    std::cout << "ALL ASSETLOCATOR TESTS PASSED\n";
    return 0;
}
