# Current work

## Loop status

Cadence: playbook Part 8.3 (morning triage → midday implement → evening review →
overnight regression). Live file: update whenever verified state changes.

## Active slice

**Phase 1 step 1 — ESM walk + census: DONE (Round 4)**

- [x] TES4 header, GRUP descent, census, digest, EDID samples
- [x] `mojave --discover` · `--esm-info` · `--esm-census`
- [x] Golden invariants: `walked + groups == HEDR numRecords`; census sums; sample EDIDs
- [x] CI GREEN on Linux + Windows (portable SetEnv, Windows ScanRegistry, MSVC flag split)
- [x] Progress-card ritual delivered to 10-of-spades, raul, randall-clark

## Round log

| Round | Slice | Discovery |
|---|---|---|
| 1 | Skeleton + AssetLocator + VDF | discover finds real installs |
| 2 | ESM header + goldens | FNV master: v1.34, 542,016 records |
| 3 | GRUP descent census | `walked + groups == HEDR` exactly |
| 4 | Per-type census + EDID + CI hardening | 107 types; named cells; CI green both platforms |
| 5 | `--find` + `--cell-map` (REFR/ACHR positions → PPM) | Goodsprings 156 objects plotted (dot plot — off-path, superseded) |
| 6 | `--terrain-map` (LAND/VHGT → hillshade) | **Real terrain: single cell continuous; FNV compression flag 0x40000; VHGT 1096B; zlib** |
| 7 | Terrain research + live display | **Worldspace mixing discovered (4 worldspaces share grid coords); VHGT column-major; render live on raul** |

## Open research item (do NOT patch further without it)

Seamless multi-cell terrain: exact cell placement/orientation. Next RESEARCHER task:
study OpenMW's *terrain rendering* placement code (not just loadland) and copy the
proven convention. Then implement once, with an edge-continuity golden test.

## Next slice candidates (morning triage pick)

1. **Cell REFR dump → top-down PNG map** of a real cell (e.g. Goodsprings):
   parse one CELL's REFR positions → plot → the first VISIBLE world output
   before the renderer exists. Strong progress-card material.
2. **Subrecord → FormDB skeleton** (generalize beyond EDID).
3. **BSA mounting → VFS** (needed before NIF/DDS).

## Reporting ritual (Commander directive, 2026-10-05)

Every round ends with a **progress card** (PNG + TXT) delivered to the
Commander Inbox on all three machines:
- 10 of Spades: `~/Documents/Commanders-Inbox/` (+ lowercase variant)
- raul-tejada: `~/Documents/Commander Inbox/`
- randall-clark: `~/Documents/Commander Inbox/`

Card = gates + what this round did + the game-read facts. Files produced in
`/tmp/opencode/progress/`. The card generator is `scripts/progress-card.sh`.

## Verified state

- `mojave --discover` finds real installs (general-lee-oliver, Bazzite)
- `mojave --esm-info` parses the real master file (see golden in test output)
- Tests: assetlocator, vdf, esmparser — all green on the build farm

## Build farm

- general-lee-oliver: gcc 16.2, 56 cores, static cmake at /tmp/cmake-3.30.5-linux-x86_64/bin
- Clone: /tmp/mojave-build (git pull → cmake --build build -j16 → ctest)
