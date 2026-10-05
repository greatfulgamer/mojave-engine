#pragma once
// src/platform/AssetLocator.h
// THE file that seven months of NVMP debugging was about. Get it right.
// Discovery order (never hardcoded, always override-able):
//   1. MOJAVE_GAME_PATH env override (tests + power users)
//   2. Linux: native Steam + Flatpak Steam (libraryfolders.vdf aware)
//   3. Linux: GOG conventions
//   4. Windows: registry (Steam + GOG)
//   5. User config file (~/.config/mojave/gamepaths.txt)
// Candidates must pass LooksLikeInstall() marker validation.

#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace mojave::platform {

struct GameInstall {
    fs::path root;         // ".../Fallout New Vegas"
    std::string store;     // "steam" | "gog" | "manual"
    std::string versionHash; // exe sha256 prefix (version pinning)
};

class AssetLocator {
public:
    static std::vector<GameInstall> Discover();

    // Positive markers: a real FNV install has ALL of these.
    static bool LooksLikeInstall(const fs::path& root);

    // Persist the user's ultimate choice so discovery is first-run-only.
    static void SaveChoice(const GameInstall& choice);

private:
    static void ScanSteamRoot(const fs::path& steamRoot,
                              std::vector<GameInstall>& out);
    static void ScanCommonLinuxPaths(std::vector<GameInstall>& out);
    static void ScanConfigFile(std::vector<GameInstall>& out);
#ifdef _WIN32
    static void ScanRegistry(std::vector<GameInstall>& out);
#endif
};

} // namespace mojave::platform
