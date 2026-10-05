# ARCHITECTURE.md — Mojave Engine (living spec)

Layer model per playbook Part 4. ADRs are immutable once accepted; summaries live here.

```
L6  CLIENT SHELL        main menu, save/load UI, mod manager, server browser
L5  NETCODE             replication, prediction, rollback, auth (ENet + protocol)
L4  GAME SYSTEMS        scripts (Lua), quests, dialogue, factions — deterministic tick 30Hz
L3  SIMULATION          physics (Jolt), AI, combat, interaction — fixed 30Hz, fixed-point critical paths
L2  WORLD RUNTIME       cells, actors, refs, inventory — FormDB rebuilt from ESM/ESP
L1  ASSET PIPELINE      ESM/ESP/BSA/NIF/KFM/DDS/BINK loaders; VFS overlay; async IO
L0  PLATFORM LAYER      paths (AssetLocator), FS, input, audio, GL/Vulkan
```

## ADR index

| ADR | Topic | Status |
|---|---|---|
| 0001 | Bootstrap (repo, stack, fleet placement) | Accepted |
| 0002 | Modern resource pipeline (upscale mod compatibility, WorldDNA split) | Accepted |
| 0003 | MO2-native VFS | Accepted |

## Non-negotiables

See AGENTS.md (7 rules). Project Rule #1: never hardcode a filesystem path.
