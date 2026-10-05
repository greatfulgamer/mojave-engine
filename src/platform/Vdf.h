#pragma once
// src/platform/Vdf.h
// Minimal Valve VDF parser — enough for steamapps/libraryfolders.vdf.
// Reference: Valve Developer Community "KeyValues" format (cited per AGENTS.md rule 7).

#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace mojave::platform::Vdf {

// Returns the "path" entries of every library folder in the VDF.
std::vector<fs::path> ParseLibraryFolders(const fs::path& vdfPath);

} // namespace mojave::platform::Vdf
