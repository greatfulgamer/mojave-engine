# ESM/ESP Record Format — research reference

Per AGENTS.md rule 7: every format constant cites a source. This is the Day-3
research package (ARCHITECT/RESEARCHER deliverable) before any coder touches
byte-level code.

## Sources (cited)

| Source | Used for |
|---|---|
| UESP "Tes4Mod:Mod File Formats" / "Tes4Mod:Record" pages | record header layout, GRUP structure |
| xNVSE `NVSE/NVSE/GameForms.h`, `GameData.cpp` | FNV-specific runtime interpretation (form IDs, masters) |
| OpenNV parsers (locally: `~/opennv-ref`, nikamigaming fork) | cross-check of record iteration behaviour |
| In-file inspection of `FalloutNV.esm` (player-owned, read-only) | goldens |

## File header (first record)

An ESM/ESP begins with a TES4 record:

```
u32  type        = 'TES4'
u32  dataSize
u32  flags
u32  formID      = 0x00000000
u16  timestamp
u16  versionControlInfo
u16  internalVersion
u16  unknown
--- then subrecords totalling dataSize ---
HEDR subrecord (dataSize = 12 for FNV):
    float  version            (1.34 for Fallout: New Vegas)
    u32    numRecords
    u32    nextObjectID
```
Citation: UESP Tes4Mod record header; verified against the real file (golden below).

FNV uses **version 1.34**. FNV's master `FalloutNV.esm` header is followed by
GRUP groups (type `GRUP`), each with its own header, containing records.

## Record header (each record)

```
u32  type        (4CC, e.g. 'WEAP', 'NPC_')
u32  dataSize
u32  flags       (bit 0x20 = deleted, 0x200 = compressed, etc.)
u32  formID
u16  timestamp
u16  versionControlInfo
u16  internalVersion
u16  unknown
--- dataSize bytes of data, or a u32 decompressed-size prefix if compressed ---
```

## GRUP structure

```
u32  type        = 'GRUP'
u32  groupSize    (includes this 24-byte header)
u32  label        (type 0 = top-level, value = record type 4CC)
i32  groupType    (0 = top-level, 1 = world children, ...)
u16  timestamp
u16  versionControlInfo
u16  unknown
u16  unknown
```

## Compression

When flags bit 0x200 (`compressed`) is set, the record data begins with a **u32
decompressed size**, and the remaining bytes are zlib-compressed. FNV uses zlib;
this affects real record bodies, not the top-level record count.

For Phase 1 "The World Is Read", our immediate deliverable is bounded: parse the
TES4 header, walk top-level GRUPs, and count records + compute a stable digest.
Record body interpretation (subrecords → FormDB) comes next with its own format
docs and goldens.

## Determinism / golden notes

- `numRecords` from HEDR is authoritative for the total record count.
- The walk count (records seen via GRUP traversal) is compared against IHEAD/
  header totals as a sanity cross-check (UESP notes these can differ from the
  HEDR count in modded files — document, don't hide).
- Goldens are generated from the player's own copy and committed as counts +
  hashes only (never game bytes — legal rule).
