#include "platform/Vdf.h"

#include <fstream>
#include <string>
#include <vector>

namespace mojave::platform::Vdf {

namespace {

// Strip VDF comments. Format reference: Valve Developer Community, KeyValues
// (VDF) — flat sections; "path" values are the only tokens we consume.
std::string StripComment(const std::string& in) {
    auto pos = in.find("//");
    return in.substr(0, pos);
}

// Collect all double-quoted tokens on the line, in order.
std::vector<std::string> Tokens(const std::string& line) {
    std::vector<std::string> out;
    size_t i = 0;
    while (true) {
        auto q1 = line.find('"', i);
        if (q1 == std::string::npos) break;
        auto q2 = line.find('"', q1 + 1);
        if (q2 == std::string::npos) break;
        out.push_back(line.substr(q1 + 1, q2 - q1 - 1));
        i = q2 + 1;
    }
    return out;
}

} // namespace

std::vector<fs::path> ParseLibraryFolders(const fs::path& vdfPath) {
    std::vector<fs::path> out;
    std::ifstream f(vdfPath);
    if (!f) return out;

    std::string line;
    while (std::getline(f, line)) {
        auto tokens = Tokens(StripComment(line));
        // "path" key followed by its value — anywhere on the line
        // (real files use both one-line and multi-line layouts).
        for (size_t i = 0; i + 1 < tokens.size(); ++i) {
            if (tokens[i] == "path") {
                out.emplace_back(tokens[i + 1]);
                break;
            }
        }
    }
    return out;
}

} // namespace mojave::platform::Vdf
