# ADR-0001 — Bootstrap

- **Status:** Accepted
- **Date:** 2026-10-05
- **Deciders:** Fleet Commander (Paul) + 10 of Spades (recon/build)

## Context

Seven months of fighting NVMP-on-Linux ended at a closed-binary injection wall
(`falloutnv+0x15945a`, client.dll). The Commander's directive: follow the
"Project Mojave" playbook (see research/the-tomb/TOMB-INDEX.md) — ship the
ENGINE, not the game; assets come from the player's own Steam/GOG copy.

## Decision

1. New repo `mojave-engine`: C++20 core, CMake+CPM, layered per playbook Part 4
   (L0 platform → L1 asset pipeline → L2 world runtime → L3 sim → L4 game
   systems → L5 netcode → L6 client shell).
2. Cross-platform from commit #1: CI matrix Linux+Windows (macOS later).
3. Project Rule #1: no hardcoded filesystem paths — `src/platform/AssetLocator`
   owns all discovery (NVMP's tombstone is the cautionary tale cited in-file).
4. Fleet placement: source of truth = GitHub `greatfulgamer/mojave-engine`;
   build farm = general-lee-oliver (56 cores, gcc 16); parity oracle =
   OpenNV repo (nikamigaming fork) already on the general; asset corpus =
   NV-Ultra's 36K-DDS upscale layer + High-and-Dry MO2 profile.
5. Phase gates and kill criteria per playbook Part 7 / 13.2.

## Consequences

(+) Reuses OpenNV/xNVSE/OpenMW knowledge without inheriting their scope traps.
(+) The Phase 0 deliverable (`./mojave --discover`) is testable this week.
(−) A second engine codebase exists alongside OpenNV — mitigated by treating
    OpenNV strictly as a format oracle, never a runtime dependency.
