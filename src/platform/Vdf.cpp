#include "platform/Vdf.h"

#include <fstream>
#include <sstream>

namespace mojave::platform::Vdf {

namespace {

// Strip VDF comments and quotes; tokenize by whitespace/braces is overkill —
// libraryfolders.vdf "path" lines are simple:  "path"  "/mount/point"
// Format reference: Valve Developer Community, KeyValues (VDF) — flat sections.
std::string StripComment(const std::string& in) {
    auto pos = in.find("//");
    return in.substr(0, pos);
}

} // namespace

std::vector<fs::path> ParseLibraryFolders(const fs::path& vdfPath) {
    std::vector<fs::path> out;
    std::ifstream f(vdfPath);
    if (!f) return out;

    std::string line;
    while (std::getline(f, line)) {
        line = StripComment(line);
        // Match: "path"  "..."
        auto q1 = line.find('"');
        if (q1 == std::string::npos) continue;
        auto q2 = line.find('"', q1 + 1);
        if (q2 == std::string::npos) continue;
        if (line.substr(q1 + 1, q2 - q1 - 1) != "path") continue;
        auto v1 = line.find('"', q2 + 1);
        if (v1 == std::string::npos) continue;
        auto v2 = line.find('"', v1 + 1);
        if (v2 == std::string::npos) continue;
        out.emplace_back(line.substr(v1 + 1, v2 - v1 - 1));
    }
    return out;
}

} // namespace mojave::platform::Vdf
