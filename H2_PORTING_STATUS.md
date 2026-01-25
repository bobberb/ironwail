# Hexen II Porting Status for Ironwail

This document tracks the progress of porting Hexen II functionality from uhexen2 to Ironwail.

## Status Legend
- [x] Complete
- [~] Partial/In Progress
- [ ] Not Started

## Source Reference
uhexen2 location: `/home/josh/tmp/claudedir/uhexen2/source/engine/`

---

## 1. PROTOCOL & NETWORK (Phase 1) - COMPLETE

| Feature | Status | uhexen2 Source | Ironwail File |
|---------|--------|----------------|---------------|
| Protocol detection | [x] | `hexen2/protocol.h` | `protocol_hexen2.c` |
| H2 message types | [x] | `hexen2/protocol.h` | `protocol_hexen2.h` |
| svc_updateclass | [x] | `hexen2/cl_parse.c` | `cl_parse_hexen2.c` |
| svc_midi_name | [x] | `hexen2/cl_parse.c` | `cl_parse_hexen2.c` (stub) |
| svc_particle_explosion | [x] | `hexen2/cl_parse.c` | `cl_parse_hexen2.c` (stub) |
| svc_set_view_tint | [x] | `hexen2/cl_parse.c` | `cl_parse_hexen2.c` |
| svc_update_inv | [x] | `hexen2/cl_parse.c` | `cl_parse_hexen2.c` |
| svc_start/end_effect | [x] | `hexen2/cl_parse.c` | `cl_parse_hexen2.c` (stub) |
| svc_plaque | [x] | `hexen2/cl_parse.c` | `cl_parse_hexen2.c` (stub) |

---

## 2. ENTITY & PROGS (Phase 2) - COMPLETE

### 2.1 Entity Structure
| Feature | Status | uhexen2 Source | Ironwail File |
|---------|--------|----------------|---------------|
| entvars_t (H2 fields) | [x] | `hexen2/progdefs.h` | `progdefs.h2` |
| globalvars_t | [x] | `hexen2/progdefs.h` | `progdefs.h2` |
| Player class fields | [x] | `hexen2/progdefs.h` | `progdefs.h2` |
| Mana system fields | [x] | `hexen2/progdefs.h` | `progdefs.h2` |
| Armor slot fields | [x] | `hexen2/progdefs.h` | `progdefs.h2` |
| Ring/artifact fields | [x] | `hexen2/progdefs.h` | `progdefs.h2` |
| Puzzle piece fields | [x] | `hexen2/progdefs.h` | `progdefs.h2` |

### 2.2 Opcodes
| Opcode | Status | uhexen2 Source | Ironwail File |
|--------|--------|----------------|---------------|
| OP_MULSTORE_* | [x] | `h2shared/pr_exec.c:356-380` | `pr_exec.c` |
| OP_DIVSTORE_* | [x] | `h2shared/pr_exec.c:382-394` | `pr_exec.c` |
| OP_ADDSTORE_* | [x] | `h2shared/pr_exec.c:396-420` | `pr_exec.c` |
| OP_SUBSTORE_* | [x] | `h2shared/pr_exec.c:422-446` | `pr_exec.c` |
| OP_FETCH_GBL_* | [x] | `h2shared/pr_exec.c:448-476` | `pr_exec.c` |
| OP_CSTATE | [x] | `h2shared/pr_exec.c:570-600` | `pr_exec.c` |
| OP_CWSTATE | [x] | `h2shared/pr_exec.c:602-640` | `pr_exec.c` |
| OP_THINKTIME | [x] | `h2shared/pr_exec.c:642-650` | `pr_exec.c` |
| OP_BITSET/CLR | [x] | `h2shared/pr_exec.c:652-688` | `pr_exec.c` |
| OP_RAND* | [x] | `h2shared/pr_exec.c:690-742` | `pr_exec.c` |
| OP_SWITCH_* | [x] | `h2shared/pr_exec.c:744-780` | `pr_exec.c` |
| OP_CASE* | [x] | `h2shared/pr_exec.c:782-810` | `pr_exec.c` |

### 2.3 Builtin Functions
| Builtin | Status | uhexen2 Source | Ironwail File |
|---------|--------|----------------|---------------|
| PF_lightstylestatic (#5) | [x] | `h2shared/pr_cmds.c:262` | `pr_cmds.c` |
| PF_tracearea (#33) | [x] | `h2shared/pr_cmds.c:1045` | `pr_cmds.c` (stub) |
| PF_particle2 (#42) | [ ] | `h2shared/pr_cmds.c:1124` | `pr_cmds_hexen2.inc` |
| PF_vhlen (#50) | [x] | `h2shared/pr_cmds.c:1188` | `pr_cmds.c` |
| PF_AdvanceFrame (#63) | [ ] | `h2shared/pr_cmds.c:1279` | `pr_cmds_hexen2.inc` |
| PF_RewindFrame (#65) | [ ] | `h2shared/pr_cmds.c:1318` | `pr_cmds_hexen2.inc` |
| PF_setclass (#66) | [x] | `h2shared/pr_cmds.c:1356` | `pr_cmds_hexen2.inc` |
| PF_lightstylevalue (#72) | [ ] | `h2shared/pr_cmds.c:1420` | `pr_cmds_hexen2.inc` |
| PF_plaque_draw (#79) | [ ] | `h2shared/pr_cmds.c:1504` | `pr_cmds_hexen2.inc` |
| PF_rain_go (#80) | [ ] | `h2shared/pr_cmds.c:1542` | `pr_cmds_hexen2.inc` |
| PF_particleexplosion (#81) | [ ] | `h2shared/pr_cmds.c:1588` | `pr_cmds_hexen2.inc` |
| PF_advanceweaponframe (#82) | [ ] | `h2shared/pr_cmds.c:1632` | `pr_cmds_hexen2.inc` |
| PF_particle3 (#85) | [ ] | `h2shared/pr_cmds.c:1712` | `pr_cmds_hexen2.inc` |
| PF_particle4 (#86) | [ ] | `h2shared/pr_cmds.c:1758` | `pr_cmds_hexen2.inc` |
| PF_setpuzzlemodel (#87) | [ ] | `h2shared/pr_cmds.c:1802` | `pr_cmds_hexen2.inc` |
| PF_starteffect (#88) | [ ] | `h2shared/pr_cmds.c:1846` | `pr_cmds_hexen2.inc` |
| PF_endeffect (#89) | [ ] | `h2shared/pr_cmds.c:1892` | `pr_cmds_hexen2.inc` |
| PF_precache_puzzle (#90) | [ ] | `h2shared/pr_cmds.c:1932` | `pr_cmds_hexen2.inc` |
| PF_concatv (#91) | [ ] | `h2shared/pr_cmds.c:1968` | `pr_cmds_hexen2.inc` |
| PF_GetString (#92) | [ ] | `h2shared/pr_cmds.c:2002` | `pr_cmds_hexen2.inc` |
| PF_SpawnTemp (#93) | [ ] | `h2shared/pr_cmds.c:2038` | `pr_cmds_hexen2.inc` |
| PF_v_factor (#94) | [ ] | `h2shared/pr_cmds.c:2078` | `pr_cmds_hexen2.inc` |
| PF_v_factorrange (#95) | [ ] | `h2shared/pr_cmds.c:2116` | `pr_cmds_hexen2.inc` |
| PF_matchAngleToSlope (#99) | [ ] | `h2shared/pr_cmds.c:2208` | `pr_cmds_hexen2.inc` |
| PF_updateInfoPlaque (#100) | [ ] | `h2shared/pr_cmds.c:2268` | `pr_cmds_hexen2.inc` |
| PF_stof (#101) | [ ] | `h2shared/pr_cmds.c:2308` | `pr_cmds_hexen2.inc` |
| PF_doWhiteFlash (#104) | [ ] | `h2shared/pr_cmds.c:2372` | `pr_cmds_hexen2.inc` |
| PF_UpdateSoundPos (#105) | [ ] | `h2shared/pr_cmds.c:2408` | `pr_cmds_hexen2.inc` |
| PF_StopSound (#106) | [ ] | `h2shared/pr_cmds.c:2454` | `pr_cmds_hexen2.inc` |

---

## 3. RENDERING (Phase 3) - COMPLETE (per plan)

### 3.1 Entity Rendering
| Feature | Status | uhexen2 Source | Ironwail File |
|---------|--------|----------------|---------------|
| Entity scale | [x] | `hexen2/r_alias.c` | `r_alias.c` |
| drawflags | [x] | `hexen2/r_alias.c` | `r_alias.c` |
| abslight | [x] | `hexen2/r_alias.c` | `r_alias.c` |
| DRF_TRANSLUCENT | [x] | `hexen2/r_alias.c` | `r_alias.c` |
| DRF_ANIMATEONCE | [x] | `hexen2/r_alias.c` | `r_alias.c` |

### 3.2 Particle System
| Feature | Status | uhexen2 Source | Ironwail File |
|---------|--------|----------------|---------------|
| H2 particle types | [x] | `hexen2/r_part.c` | `r_part.c` |
| H2 trail types | [x] | `hexen2/r_part.c` | `r_part.c` |
| Color ramps | [x] | `hexen2/r_part.c` | `r_part.c` |
| Rain/snow effects | [x] | `hexen2/r_part.c` | `r_part.c` |

### 3.3 Effect System
| Feature | Status | uhexen2 Source | Ironwail File |
|---------|--------|----------------|---------------|
| CE_* effects (62+) | [x] | `hexen2/cl_effect.c` | `cl_effect.c` |
| Effect management | [x] | `hexen2/cl_effect.c` | `cl_effect.c` |
| Material chunks | [x] | `hexen2/cl_effect.c` | `cl_effect.h` |

### 3.4 View Effects
| Feature | Status | uhexen2 Source | Ironwail File |
|---------|--------|----------------|---------------|
| View tint | [x] | `hexen2/view.c` | `view.c` |
| Roll drifting | [x] | `hexen2/view.c` | `view.c` |
| H2 cshifts | [x] | `hexen2/view.c` | `view.c` |

---

## 4. UI & INVENTORY (Phase 4) - COMPLETE (per plan)

| Feature | Status | uhexen2 Source | Ironwail File |
|---------|--------|----------------|---------------|
| H2 status bar | [x] | `hexen2/sbar.c` | `sbar_hexen2.c` |
| Mana bars | [x] | `hexen2/sbar.c` | `sbar_hexen2.c` |
| Artifact inventory | [x] | `hexen2/sbar.c` | `sbar_hexen2.c` |
| Ring status | [x] | `hexen2/sbar.c` | `sbar_hexen2.c` |
| Puzzle display | [x] | `hexen2/sbar.c` | `sbar_hexen2.c` |
| Inventory commands | [x] | `hexen2/cl_input.c` | `sbar.c` |

---

## 5. AUDIO & ASSETS (Phase 5) - NOT STARTED

| Feature | Status | uhexen2 Source | Ironwail File |
|---------|--------|----------------|---------------|
| MIDI music | [ ] | `hexen2/bgmusic.c` | - |
| svc_midi_name | [ ] | `hexen2/cl_parse.c:1842` | `cl_parse_hexen2.c` |
| svc_mod_name | [ ] | `hexen2/cl_parse.c:1878` | `cl_parse_hexen2.c` |
| Sound position updates | [ ] | `hexen2/snd_dma.c` | - |
| 512 sound limit | [ ] | `hexen2/snd_dma.c` | - |
| H2 model formats | [ ] | `hexen2/model.c` | - |
| H2 texture loading | [ ] | `hexen2/r_texture.c` | - |
| Puzzle strings | [ ] | `hexen2/pr_edict.c` | - |

---

## 6. MULTIPLAYER (Phase 6) - NOT STARTED

| Feature | Status | uhexen2 Source | Ironwail File |
|---------|--------|----------------|---------------|
| HexenWorld protocol | [ ] | `hexenworld/server/` | - |
| King of the Hill | [ ] | `hexenworld/server/sv_user.c` | - |
| Class networking | [ ] | `hexenworld/server/sv_main.c` | - |
| Per-class scoreboards | [ ] | `hexenworld/client/sbar.c` | - |
| Team modes | [ ] | `hexenworld/server/sv_user.c` | - |

---

## Current Blockers & Issues

### Critical (game won't run properly)
1. **Global variable layout mismatch** (`ironwail-hu3.34`) - Different H2 versions have different globalvars_t
2. **Graphics loading issues** (`ironwail-hu3.31`) - conchars, menus not loading in H2 mode

### High Priority (basic gameplay)
1. **Frame control builtins** (`ironwail-vjj`) - AdvanceFrame, RewindFrame needed for animations
2. **Effects builtins** (`ironwail-104`) - particle2-4, starteffect, endeffect

### Medium Priority (full experience)
1. **Rendering features** (`ironwail-80p`) - Verify scale/drawflags working
2. **Protocol extensions** (`ironwail-soo`) - Extended stats SC1/SC2
3. **Sound extensions** (`ironwail-deq`) - UpdateSoundPos, StopSound

---

## Key uhexen2 Files Reference

### Engine Core
```
h2shared/pr_exec.c      - Opcode execution (CRITICAL)
h2shared/pr_cmds.c      - Builtin functions (CRITICAL)
h2shared/pr_edict.c     - Entity management
hexen2/progdefs.h       - Entity/global structures (CRITICAL)
```

### Client
```
hexen2/cl_parse.c       - Network message parsing
hexen2/cl_effect.c      - Effect system
hexen2/client.h         - Client state structures
hexen2/sbar.c           - Status bar/HUD
hexen2/view.c           - View/camera handling
```

### Rendering
```
hexen2/r_part.c         - Particle system
hexen2/r_alias.c        - Model rendering
hexen2/r_texture.c      - Texture handling
```

### Protocol
```
hexen2/protocol.h       - Protocol definitions
hexen2/sv_main.c        - Server main loop
hexen2/sv_send.c        - Server message sending
```

### Audio
```
hexen2/bgmusic.c        - Background music
hexen2/snd_dma.c        - Sound system
```

---

## Files Modified in Ironwail

### Created for H2
- `protocol_hexen2.h` / `protocol_hexen2.c` - H2 protocol
- `cl_parse_hexen2.h` / `cl_parse_hexen2.c` - H2 message parsing
- `cl_effect.h` / `cl_effect.c` - Effect system
- `sbar_hexen2.h` / `sbar_hexen2.c` - H2 HUD
- `pr_cmds_hexen2.h` / `pr_cmds_hexen2.inc` - H2 builtins
- `progdefs.h2` - H2 entity/global structures

### Modified for H2
- `progdefs.h` - Includes H2 defs
- `pr_comp.h` - H2 opcodes enum
- `pr_exec.c` - Opcode implementations
- `pr_edict.c` - H2 globals setup, CRC checking
- `pr_cmds.c` - Builtin registration
- `progs.h` - H2 globals struct
- `sv_main.c` - H2 mode checks
- `host_cmd.c` - H2 mode checks
- `r_alias.c` - H2 rendering features
- `r_part.c` - H2 particles/trails
- `view.c` - H2 view effects
- `sbar.c` - H2 HUD redirect
- `client.h` - H2 client state
- `render.h` - drawflags/abslight
- `glquake.h` - H2 particle/trail types

---

## Testing

### Test Script
```bash
/tmp/test_h2.sh
# Or manually:
cd "/tank/josh/Documents/Games/PcGames/HeXen II"
/home/josh/tmp/claudedir/ironwail/result/bin/ironwail -game data1 +map demo1
```

### Test Checklist
- [ ] Game loads without crash
- [ ] Player can move
- [ ] Entities render correctly
- [ ] HUD displays
- [ ] Sound plays
- [ ] Effects render
- [ ] Can complete demo1 level
