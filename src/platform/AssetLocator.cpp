#include "platform/AssetLocator.h"
#include "platform/Vdf.h"

#include <cstdlib>
#include <fstream>

#ifdef _WIN32
#include <windows.h>
#endif

namespace mojave::platform {

namespace {
constexpr const char* kGameDirName = "Fallout New Vegas";
} // namespace

bool AssetLocator::LooksLikeInstall(const fs::path& root) {
    // Markers per playbook Part 2: exe + master + at least one core BSA set
    // (GOG layouts vary — either archive set counts).
    if (!fs::exists(root / "FalloutNV.exe")) return false;
    if (!fs::exists(root / "Data" / "FalloutNV.esm")) return false;
    if (!fs::exists(root / "Data" / "Fallout - Meshes.bsa") &&
        !fs::exists(root / "Data" / "Fallout - Misc.bsa"))
        return false;
    return true;
}

void AssetLocator::ScanSteamRoot(const fs::path& steamRoot,
                                 std::vector<GameInstall>& out) {
    // Parse libraryfolders.vdf: libraries can live on any mounted drive.
    const fs::path vdfPath = steamRoot / "steamapps" / "libraryfolders.vdf";
    if (fs::exists(vdfPath)) {
        auto folders = Vdf::ParseLibraryFolders(vdfPath);
        for (const auto& lib : folders) {
            fs::path candidate = lib / "steamapps" / "common" / kGameDirName;
            if (LooksLikeInstall(candidate))
                out.push_back({candidate, "steam", ""});
        }
    }
    // Always check the root library itself.
    fs::path rootCandidate = steamRoot / "steamapps" / "common" / kGameDirName;
    if (LooksLikeInstall(rootCandidate))
        out.push_back({rootCandidate, "steam", ""});
}

#ifdef _WIN32
// Windows registry discovery — playbook Part 2 matrix:
//   Steam: HKLM\SOFTWARE\WOW6432Node\Valve\Steam @ InstallPath
//   GOG:   HKLM\SOFTWARE\WOW6432Node\GOG.com\Games\1454587515 @ path
void AssetLocator::ScanRegistry(std::vector<GameInstall>& out) {
    auto readReg = [](const wchar_t* subkey, const wchar_t* value) -> std::wstring {
        HKEY h = nullptr;
        std::wstring result;
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, subkey, 0,
                          KEY_READ | KEY_WOW64_32KEY, &h) == ERROR_SUCCESS) {
            wchar_t buf[MAX_PATH];
            DWORD size = sizeof(buf);
            if (RegQueryValueExW(h, value, nullptr, nullptr,
                                 reinterpret_cast<LPBYTE>(buf), &size) == ERROR_SUCCESS)
                result = buf;
            RegCloseKey(h);
        }
        return result;
    };

    const std::wstring steam = readReg(L"SOFTWARE\\Valve\\Steam", L"InstallPath");
    if (!steam.empty())
        ScanSteamRoot(fs::path(steam), out);

    const std::wstring gog = readReg(L"SOFTWARE\\GOG.com\\Games\\1454587515", L"path");
    if (!gog.empty()) {
        fs::path p(gog);
        if (LooksLikeInstall(p)) out.push_back({p, "gog", ""});
    }
}
#endif // _WIN32

void AssetLocator::ScanCommonLinuxPaths(std::vector<GameInstall>& out) {
    const char* home = std::getenv("HOME");
    if (!home) return;
    const fs::path h(home);

    // Native Steam (playbook 2.1 matrix).
    ScanSteamRoot(h / ".local/share/Steam", out);

    // Flatpak Steam — Bazzite / Steam Deck desktop mode. The bazzite_origin_story case.
    ScanSteamRoot(h / ".var/app/com.valvesoftware.Steam/data/Steam", out);
    ScanSteamRoot(h / ".var/app/com.valvesoftware.Steam/.local/share/Steam", out);

    // GOG conventions.
    const fs::path gog = h / "GOG Games" / kGameDirName;
    if (LooksLikeInstall(gog)) out.push_back({gog, "gog", ""});
}

void AssetLocator::ScanConfigFile(std::vector<GameInstall>& out) {
    const char* xdg = std::getenv("XDG_CONFIG_HOME");
    const char* home = std::getenv("HOME");
    if (!home) return;
    fs::path cfgDir = xdg ? fs::path(xdg) : fs::path(home) / ".config";
    std::ifstream f(cfgDir / "mojave" / "gamepaths.txt");
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        fs::path p(line);
        if (LooksLikeInstall(p)) out.push_back({p, "manual", ""});
    }
}

std::vector<GameInstall> AssetLocator::Discover() {
    std::vector<GameInstall> out;

    // 1. Environment overrides — always honored first.
    if (const char* env = std::getenv("MOJAVE_GAME_PATH")) {
        fs::path p(env);
        if (LooksLikeInstall(p)) out.push_back({p, "manual", ""});
    }
    if (const char* env = std::getenv("MOJAVE_STEAM_ROOT")) {
        ScanSteamRoot(fs::path(env), out);
    }

#ifdef _WIN32
    ScanRegistry(out);
#else
    ScanCommonLinuxPaths(out);
#endif
    ScanConfigFile(out);
    return out;
}

void AssetLocator::SaveChoice(const GameInstall& choice) {
    // Persistence stub — Playbook Part 2: config lands in ~/.config/mojave/.
    const char* xdg = std::getenv("XDG_CONFIG_HOME");
    const char* home = std::getenv("HOME");
    if (!home) return;
    fs::path cfgDir = xdg ? fs::path(xdg) : fs::path(home) / ".config";
    fs::create_directories(cfgDir / "mojave");
    std::ofstream f(cfgDir / "mojave" / "gamepaths.txt",
                    std::ios::app);
    if (f) f << choice.root.string() << "\n";
}

} // namespace mojave::platform
