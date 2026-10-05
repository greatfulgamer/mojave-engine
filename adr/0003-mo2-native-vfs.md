# ADR-0003 — MO2-Native VFS

- **Status:** Accepted (from TOMB playbook draft)
- **Date:** 2026-10-05

## Context

Wabbajack lists install as Mod Organizer 2 profiles. If the engine reads
`modlist.txt` / `plugins.txt` and MO2 priority ordering directly, users get
their entire existing mod setup for free on day one — the killer feature over
every predecessor (NVMP included).

## Decision

1. The VFS speaks MO2 natively: `Mo2Profile` parses `modlist.txt` (priority
   order) and `plugins.txt` (plugin order) into layered mounts.
2. Layered mounts shadow earlier ones; `Data/` vanilla loose files sit at the
   bottom, MO2 mod folders in priority order above, engine overlay highest.
3. Case-insensitive normalization at the VFS boundary (Bazzite filesystems are
   case-sensitive; Bethesda shipped mixed-case and mods assume it isn't).
4. Never overwrite original files — mods live in the overlay and shadow
   originals (save-file safety, moddability, clean uninstalls).
5. WorldDNA computed from the resolved VFS state (see ADR-0002 point 5).

## Consequences

(+) Existing MO2 setups (incl. the fleet's High-and-Dry profile) load directly.
(+) Deterministic mod resolution = stable gameplayDNA.
(−) MO2's virtual-filesystem quirks (root-builder layouts) need compatibility
    tests — fixtures under tests/fixtures/mo2_profile_minimal.
