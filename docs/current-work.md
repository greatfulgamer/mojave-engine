# Current work

## Loop status

Cadence: playbook Part 8.3 (morning triage → midday implement → evening review →
overnight regression). Live file: update whenever verified state changes.

## Active slice

**Phase 1 step 1 — "The World Is Read" (ESM walk: DONE)**

- [x] Data: `docs/formats/esm-records.md` (cited: UESP, xNVSE, OpenNV, in-file)
- [x] Archive: synthetic mini-ESM golden test (runs in CI, no game files)
- [x] Real-file golden: TES4 header + GRUP descent + census + digest
- [x] CLI: `mojave --esm-info`
- [x] **Discovered invariant:** HEDR `numRecords` = walked records + GRUP headers
      (465,016 + 77,000 = 542,016 exactly on the vanilla master)
- [x] 107 unique record types enumerated; digest `71f57e617a2dbfe1` (stable)

## Next slice candidates (morning triage pick)

1. **Per-type census + first subrecord parser** → FormDB skeleton
   (pick one type: CELL/WRLD for the world path, or WEAP as a small warm-up)
2. **BSA mounting** → VFS reads `Fallout - Meshes.bsa` / `Textures.bsa`,
   entry count + extract-one-mesh goldens (needed before NIF/DDS)

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
