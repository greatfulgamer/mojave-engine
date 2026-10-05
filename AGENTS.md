# Mojave Engine — AGENTS.md (the 7 non-negotiables)

Playbook Part 8 contract. Every agent task obeys these; the reviewer enforces them.

1. **No code without an approved ADR** for any cross-layer change.
2. **Tests before merge**, every PR. Coverage gates in CI; skipped tests are flagged.
3. **≤400-line diffs** per agent task — the empirically reviewable chunk size.
4. **Never hardcode a filesystem path** — Platform layer only (`src/platform/`).
   This is Project Rule #1. NVMP's tombstone is the cautionary tale: its hardcoded
   `C:\Program Files (x86)\Steam\...` path detection is why this project exists.
5. **Every file touched → corresponding doc section updated** (ARCHITECTURE.md or docs/).
6. **All netcode changes must pass the determinism suite** (sim runs twice, journal
   hashes must match).
7. **Format constants must cite a reference source** (xNVSE source, OpenNV, UESP,
   nif.xml/nifly, OpenMW loader, BSA docs) — documented in docs/formats/*.md with
   goldens before coders touch byte-level code.

## Cadence (playbook 8.3)

- MORNING (human, ≤30 min): triage CI, pick milestone slice, approve ADRs.
- MIDDAY (agents): ARCHITECT emits task package (ADR + headers + test outline);
  CODER implements against it.
- EVENING (human, ≤45 min): REVIEWER audit read, adjudicate, merge or bounce.
- OVERNIGHT (agents): TESTER regression + storm tests; RESEARCHER reference bundles.
- Sunday: no merges. Sustainability beats velocity (VaultMP's lesson).
