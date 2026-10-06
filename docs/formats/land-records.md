# LAND / VHGT terrain format — research reference

Research gate for terrain (AGENTS.md rule 7). Written AFTER the fact to correct
the process: the first terrain implementation was coded from memory and took
four patches. This is the cited reference.

## Sources

| Source | Used for |
|---|---|
| OpenMW `components/esm4/loadland.hpp` (cc9cii) — fetched 2026-10-05 | VHGT struct layout, `sHeightScale = 8`, `sVertsPerSide = 33`, `sRealSize = 4096` |
| OpenMW `components/esm4/loadland.cpp` | subrecord inventory (DATA/VNML/VHGT/VCLR/BTXT/ATXT/VTXT) |
| UESP Tes4Mod:Mod_File_Format (via OpenMW's citations) | record flag semantics |
| In-file verification on the real `FalloutNV.esm` | compression flag, values |

## Facts established (verified, not assumed)

1. **Record compression flag for FO3/FNV is `0x00040000`** (TES4/Oblivion used
   `0x200`). Compressed data = `u32 decompressedSize` + zlib stream.
   Evidence: all 29,363 LAND records carry 0x40000; decompressing yields the
   expected subrecord structure.
2. **LAND subrecords**: `DATA`(4B flags), `VNML`(33×33×3 normals),
   `VHGT`(4 + 33×33 int8 + 3 trailing bytes = **1096**), `VCLR`(33×33×3 colors),
   plus BTXT/ATXT/VTXT texture layers.
3. **VHGT decode** (matches OpenMW exactly):
   ```
   float height = base;                    // first 4 bytes, world units
   for (i = 0; i < 1089; ++i) {
       height += (int8)gradient[i] * 8;    // sHeightScale = 8
       h[i] = height;
   }
   ```
   Row-major 33×33; row 0 = south, row 32 = north (overlap edge).
4. **Cell grid**: exterior `CELL` records carry `XCLC` (int32 x, int32 y) — the
   cell grid coordinate. Verified: Goodsprings = CELL 0x000daebb, XCLC (-18, 0),
   consistent with its reference extent x −73686..−69641.
5. **Cell size** = 4096 world units; 32 intervals of 128 units per cell, with a
   shared overlap row/column (33rd vertex) to the north and east.
6. Some edge/void cells carry monotonic gradient runs (deep negative heights) —
   world-border data, not decode errors (independent Python decoder agrees with
   the C++ reader byte-for-byte).

## Known-open item (next slice)

The multi-cell composite still shows boundary steps → the cell-grid orientation
(which axis/direction the XCLC grid maps onto the composite) is not yet correct.
Single-cell renders are correct, so the defect is bounded to `TerrainMap`'s
compositor, not the reader.
