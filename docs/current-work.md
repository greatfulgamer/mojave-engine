# Current work

## Loop status

Cadence: playbook Part 8.3 (morning triage → midday implement → evening review →
overnight regression). Live file: update whenever verified state changes.

## Active slice

**Phase 1 step 1 — "The World Is Read" (Day 3: Format Footing)**

- [x] Data: `docs/formats/esm-records.md` (cited: UESP, xNVSE, OpenNV, in-file)
- [x] Archive: synthetic mini-ESM golden test (runs in CI, no game files)
- [x] Real-file golden: TES4 header + GRUP walk + digest on FalloutNV.esm
- [x] CLI: `mojave --esm-info`
- [ ] Next slice: subrecord parser + FormDB skeleton (own format doc + goldens)

## Verified state

- `mojave --discover` finds real installs (general-lee-oliver, Bazzite)
- `mojave --esm-info` parses the real master file (see golden in test output)
- Tests: assetlocator, vdf, esmparser — all green on the build farm

## Build farm

- general-lee-oliver: gcc 16.2, 56 cores, static cmake at /tmp/cmake-3.30.5-linux-x86_64/bin
- Clone: /tmp/mojave-build (git pull → cmake --build build -j16 → ctest)
