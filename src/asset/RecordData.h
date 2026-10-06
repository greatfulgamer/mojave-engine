#pragma once
// src/asset/RecordData.h — record-flag + decompression helper.
// Format citation: docs/formats/esm-records.md — Fallout 3/New Vegas use
// record flag 0x00040000 for compression (TES4/Oblivion used 0x200). When set,
// the record data begins with a u32 decompressed size followed by a zlib stream.
#include <cstdint>
#include <vector>

#ifdef MOJAVE_HAVE_ZLIB
#include <zlib.h>
#endif

namespace mojave::asset {

inline bool RecordIsCompressed(uint32_t flags) {
    return (flags & 0x00040000u) != 0 || (flags & 0x00000200u) != 0;
}

// Returns true and fills `out` (decompressed or verbatim). Returns false only
// when the record is compressed and this build has no zlib.
inline bool RecordData(const std::vector<unsigned char>& raw, uint32_t flags,
                       std::vector<unsigned char>& out) {
    if (!RecordIsCompressed(flags)) {
        out = raw;
        return true;
    }
#ifdef MOJAVE_HAVE_ZLIB
    if (raw.size() < 4) return false;
    const uint32_t usize = uint32_t(raw[0]) | (uint32_t(raw[1]) << 8) |
                           (uint32_t(raw[2]) << 16) | (uint32_t(raw[3]) << 24);
    out.assign(usize, 0);
    uLongf destLen = usize;
    const int rc = uncompress(out.data(), &destLen, raw.data() + 4,
                              static_cast<uLong>(raw.size() - 4));
    if (rc != Z_OK) {
        out.clear();
        return false;
    }
    out.resize(destLen);
    return true;
#else
    out.clear();
    return false;
#endif
}

} // namespace mojave::asset
