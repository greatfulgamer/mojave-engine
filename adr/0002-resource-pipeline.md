# ADR-0002 — Modern Resource Pipeline for Upscale Mod Compatibility

- **Status:** Accepted (from TOMB playbook draft)
- **Date:** 2026-10-05

## Context

Target users run Wabbajack lists (e.g., "High and Dry"): 4x AI-upscaled
textures (16–40GB), lighting overhauls, MO2 profiles. Vanilla Gamebryo's
pipeline is fixed-function, gamma-incorrect, and streams eagerly into a 4GB
address space. The modlists prove demand exceeds what the original engine can
deliver.

## Decision

1. Renderer targets OpenGL 3.3 Core minimum with a gamma-correct (sRGB
   framebuffer) pipeline from commit #1. No "retro-accurate" legacy gamma mode —
   mods assume modern output.
2. Textures stream asynchronously by demand with a configurable VRAM budget.
   BC1–BC7 block-compressed DDS supported (BC7 = modern re-compress target).
3. Mipmap policy: prefer mod-shipped mips; generate if absent. Trilinear
   default; anisotropy = user setting.
4. VFS resolves mods at load; dedup identical textures by content hash to
   reclaim VRAM (upscale packs overlap vanilla ~15%).
5. **WorldDNA bifurcation:**
   - `gameplayDNA` = ordered ESM/ESP set + collision meshes → JOIN GATE
   - `visualDNA` = textures, LOD, shaders → advisory only, logged, NEVER blocks
     join (cosmetic divergence tolerated)

## Consequences

(+) High-and-Dry-class lists load unmodified on day one.
(+) 64-bit + streaming = no 4GB ceiling.
(−) Must honor sRGB flags per-DDS; misflagged mod assets get a diagnostic,
    not silent brightness bugs.
(−) Two DNA hashes to maintain in the join handshake.

Alternatives considered: replicate Gamebryo fixed-function lighting (rejected —
contradicts goal #4, breaks upscale packs visually).
