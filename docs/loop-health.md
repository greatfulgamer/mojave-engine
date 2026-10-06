# Loop Health — how we know this is building WELL

The old work failed invisibly. This is the instrument panel. Run
`scripts/loop-status.sh` on the build farm for the live reading.

## The gates (all objective — no vibes)

| Gate | Instrument | Passing means |
|---|---|---|
| Compiles | `cmake --build` | No build ambiguity; farm output captured |
| Tests | `ctest` | 3/3 suites (assetlocator, vdf, esmparser) |
| Demos per phase | `mojave --discover`, `--esm-info` | The phase's runnable artifact exists (playbook rule: "if a phase can't demo, cut its scope") |
| Goldens | synthetic (CI) + real-file (farm) | Byte-level claims verified against the real master, not memory |
| Citations | `docs/formats/*` | Every format constant traceable (xNVSE/UESP/OpenNV/in-file) |
| ADR gate | `adr/*.md` | No code without an approved decision record |
| Diff size | `git log --numstat` | ≤400 lines per commit (reviewable chunks) |
| Determinism suite | (arrives with the sim) | Same seed twice → identical journal hash |
| Storm tests | (arrives with netcode) | Packet loss/jitter → no desync |

## The failure modes we are actively preventing

| "Badly, like before" | Our countermeasure |
|---|---|
| Research lost | Everything in git (`Atomic-Wrangler` research, `mojave-engine` code) + memory palace wings |
| Rehashing one issue >3 times | 3-attempt rule; then re-research or pivot (documented) |
| Vibes-based progress ("60% for 2 months") | Gate table above; every phase ends with a demo |
| Blind eyes (Xvfb black frames) | QA on **real** frames: real display + general's vision-relay |
| RDP failure / input lockup | GNOME RDP (raul, proven) — Sunshine banned |
| Scope explosion (OpenNV's trap) | Vertical slices; kill criteria per phase (playbook 13.2) |
| Solo burnout (VaultMP's fate) | ≤2 hr/day human cadence; agents type |

## The display plan (when the first visual gate lands — Phase 1 Goodsprings)

- Head: raul RX 6700XT + physical monitor over GNOME RDP (the session live now).
- Fallback: 10-of-spades (680M) local display.
- QA: capture from the real display → general vision-relay (qwen3-vl) frames →
  verdicts logged with the frame, so the loop's eyes never black out.
