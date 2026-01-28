# Hexen II Porting Status for Ironwail

This document tracks the progress of porting Hexen II functionality from uhexen2 to Ironwail.

## Status Legend
- [x] Complete
- [~] Partial/In Progress
- [ ] Not Started

## Source References

**uhexen2 engine:** `/home/josh/tmp/claudedir/uhexen2/source/engine/`

**HexenC progs source (hcode_archive):** `/home/josh/tmp/claudedir/uhexen2-hcode_archive/`
- `portals-1.12a/` - Portal of Praevus (mission pack) - our target version
- `portals-1.12a/builtin.hc` - **All 106 builtins with exact signatures**
- `h2-1.11/` - Base Hexen II v1.11 progs source
- GitHub: https://github.com/sezero/uhexen2-hcode_archive

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
| globalvars_t runtime offsets | [x] | `hexen2/progdefs.h` | `progs.h` (h2_globals_t) |

**Runtime offset system (ironwail-8lg - COMPLETE):** H2 has different globalvars_t layouts
depending on progs version. The engine now uses runtime offset lookup via ED_FindGlobalOffset()
for H2-specific globals (v_forward, trace_*, deathmatch, stats, parm1-16, etc.).
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
| PF_particle2 (#42) | [x] | `h2shared/pr_cmds.c:1124` | `pr_cmds_hexen2.inc` |
| PF_vhlen (#50) | [x] | `h2shared/pr_cmds.c:1188` | `pr_cmds.c` |
| PF_AdvanceFrame (#63) | [x] | `h2shared/pr_cmds.c:1279` | `pr_cmds_hexen2.inc` |
| PF_RewindFrame (#65) | [x] | `h2shared/pr_cmds.c:1318` | `pr_cmds_hexen2.inc` |
| PF_setclass (#66) | [x] | `h2shared/pr_cmds.c:1356` | `pr_cmds_hexen2.inc` |
| PF_lightstylevalue (#72) | [x] | `h2shared/pr_cmds.c:1420` | `pr_cmds_hexen2.inc` |
| PF_plaque_draw (#79) | [x] | `h2shared/pr_cmds.c:1504` | `pr_cmds_hexen2.inc` |
| PF_rain_go (#80) | [x] | `h2shared/pr_cmds.c:1542` | `pr_cmds_hexen2.inc` |
| PF_particleexplosion (#81) | [x] | `h2shared/pr_cmds.c:1588` | `pr_cmds_hexen2.inc` |
| PF_movestep (#82) | [x] | `h2shared/pr_cmds.c:1632` | `pr_cmds_hexen2.inc` |
| PF_particle3 (#85) | [x] | `h2shared/pr_cmds.c:1712` | `pr_cmds_hexen2.inc` |
| PF_particle4 (#86) | [x] | `h2shared/pr_cmds.c:1758` | `pr_cmds_hexen2.inc` |
| PF_setpuzzlemodel (#87) | [x] | `h2shared/pr_cmds.c:1802` | `pr_cmds_hexen2.inc` |
| PF_starteffect (#88) | [x] | `h2shared/pr_cmds.c:1846` | `sv_effect_hexen2.c` |
| PF_endeffect (#89) | [x] | `h2shared/pr_cmds.c:1892` | `sv_effect_hexen2.c` |
| PF_precache_puzzle (#90) | [x] | `h2shared/pr_cmds.c:1932` | `pr_cmds_hexen2.inc` |
| PF_concatv (#91) | [x] | `h2shared/pr_cmds.c:1968` | `pr_cmds_hexen2.inc` |
| PF_GetString (#92) | [x] | `h2shared/pr_cmds.c:2002` | `pr_cmds_hexen2.inc`, `host_string.c` |
| PF_SpawnTemp (#93) | [x] | `h2shared/pr_cmds.c:2038` | `pr_cmds_hexen2.inc` |
| PF_v_factor (#94) | [x] | `h2shared/pr_cmds.c:2078` | `pr_cmds_hexen2.inc` |
| PF_v_factorrange (#95) | [x] | `h2shared/pr_cmds.c:2116` | `pr_cmds_hexen2.inc` |
| PF_matchAngleToSlope (#99) | [x] | `h2shared/pr_cmds.c:2208` | `pr_cmds_hexen2.inc` |
| PF_updateInfoPlaque (#100) | [x] | `h2shared/pr_cmds.c:2268` | `pr_cmds_hexen2.inc` |
| PF_stof (#101) | [-] | `h2shared/pr_cmds.c:2308` | Not used by H2 progs (slot 101 = precache_sound4) |
| PF_doWhiteFlash (#104) | [x] | `h2shared/pr_cmds.c:2372` | `pr_cmds_hexen2.inc` |
| PF_UpdateSoundPos (#105) | [x] | `h2shared/pr_cmds.c:2408` | `pr_cmds_hexen2.inc` |
| PF_StopSound (#106) | [x] | `h2shared/pr_cmds.c:2454` | `pr_cmds_hexen2.inc` |

### 2.4 Physics System
| Feature | Status | uhexen2 Source | Ironwail File |
|---------|--------|----------------|---------------|
| SV_PushRotate (rotating BSP) | [x] | `hexen2/sv_phys.c` | `sv_phys.c` |
| Movechain system | [x] | `hexen2/sv_phys.c` | `sv_phys.c` |
| FL_MOVECHAIN_ANGLE | [x] | `hexen2/server.h` | `server.h` |
| Hull field for collision | [x] | `hexen2/world.c` | `world.c` |
| Server inventory sync (SC1/SC2) | [x] | `hexen2/sv_main.c` | `sv_inventory_hexen2.c` |
| Effects save/load | [x] | `hexen2/sv_effect.c` | `sv_effect_hexen2.c` |

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
| Mana bars (actual max_mana) | [x] | `hexen2/sbar.c` | `sbar_hexen2.c` |
| Armor display (per-piece) | [x] | `hexen2/sbar.c` | `sbar_hexen2.c` |
| Artifact inventory | [x] | `hexen2/sbar.c` | `sbar_hexen2.c` |
| Ring status | [x] | `hexen2/sbar.c` | `sbar_hexen2.c` |
| Puzzle display | [x] | `hexen2/sbar.c` | `sbar_hexen2.c` |
| Inventory commands | [x] | `hexen2/cl_input.c` | `sbar.c` |
| H2 menu system | [x] | `hexen2/menu.c` | `menu_hexen2.c` |
| Bigfont rendering | [x] | `hexen2/menu.c` | `menu_hexen2.c` |
| Animated title scroll | [x] | `hexen2/menu.c` | `menu_hexen2.c` |
| Class selection | [x] | `hexen2/menu.c` | `menu_hexen2.c` |
| Difficulty selection | [x] | `hexen2/menu.c` | `menu_hexen2.c` |
| Portals expansion detect | [x] | `hexen2/menu.c` | `menu_hexen2.c` |
| Demoness class (5th) | [x] | `hexen2/menu.c` | `menu_hexen2.c` (auto-detected via portals) |

**HUD Fixes (2026-01-27):**
- Mana bars now use actual `cl.max_mana` from server instead of hardcoded 100
- Armor display now shows individual armor pieces from `cl.armor_*` fields instead of threshold guessing
- Server now sends armor values via SC2 protocol (`sv_inventory_hexen2.c`)
- Added `max_mana`, `max_health`, and `armor_*` fields to client struct

**Server Sync Additions (2026-01-27):**
- Ring times (flight, water, turning, regeneration) now synced via SC2
- Max health and max mana now synced via SC2
- Puzzle pieces (8 slots) now synced via SC2 with string hashing for delta detection
- Server uses oldstats_i[60-77] for SC2 delta tracking

---

## 5. AUDIO & ASSETS (Phase 5) - COMPLETE

| Feature | Status | uhexen2 Source | Ironwail File |
|---------|--------|----------------|---------------|
| MIDI music (OGG/MP3 replacement) | [x] | `hexen2/bgmusic.c` | `bgmusic.c` (uses BGM_Play) |
| svc_midi_name | [x] | `hexen2/cl_parse.c:1842` | `cl_parse_hexen2.c` |
| svc_mod_name | [x] | `hexen2/cl_parse.c:1878` | `cl_parse_hexen2.c` (CL_ParseModName) |
| Sound position updates | [x] | `hexen2/snd_dma.c` | `snd_dma.c`, `pr_cmds_hexen2.inc` |
| 512 sound limit | [-] | `hexen2/snd_dma.c` | Not needed (Ironwail already supports more) |
| H2 model formats | [x] | `hexen2/model.c` | `gl_model.c` (RAPO v50 loader) |
| H2 texture loading | [x] | `hexen2/r_texture.c` | `gl_draw.c`, `wad.c` (gfx.wad + .lmp files) |
| Puzzle strings | [x] | `hexen2/pr_edict.c` | `host_string.c` (strings.txt loading) |

---

## 5b. HUB SYSTEM (Phase 5b) - COMPLETE

| Feature | Status | uhexen2 Source | Ironwail File |
|---------|--------|----------------|---------------|
| sv.startspot field | [x] | `hexen2/server.h` | `server.h` |
| SV_SpawnServer startspot param | [x] | `hexen2/sv_main.c:1934` | `sv_main.c` |
| startspot global (offset 35) | [x] | `hexen2/sv_main.c:2089` | `sv_main.c` |
| SFL_NEW_UNIT/EPISODE flags | [x] | `hexen2/server.h:234-235` | `server.h` |
| PF_changelevel hub routing | [x] | `h2shared/pr_cmds.c:2752` | `pr_cmds.c` |
| changelevel2 command | [x] | `hexen2/host_cmd.c:350` | `host_cmd.c` |
| SaveGamestate | [x] | `hexen2/host_cmd.c:842` | `host_cmd.c` (H2_SaveGamestate) |
| LoadGamestate | [x] | `hexen2/host_cmd.c:986` | `host_cmd.c` (H2_LoadGamestate) |
| RestoreClients | [x] | `hexen2/host_cmd.c:944` | `host_cmd.c` (H2_RestoreClients) |
| SV_SaveEffects/LoadEffects | [x] | `hexen2/sv_effect.c:612,846` | `sv_effect_hexen2.c` |

**Implementation complete:**
- H2_SaveGamestate: saves level state to mapname.gip (entities, lightstyles, effects, globals)
- H2_LoadGamestate: restores full level state including entity data from .gip files
- H2_LoadClientsState: loads client state from clients.gip
- H2_RestoreClients: calls ClientReEnter in progs for returning players
- changelevel2 command: complete hub transition workflow (save state, load new/cached level, restore clients)

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
1. ~~**Global variable layout mismatch** (`ironwail-8lg`) - Different H2 versions have different globalvars_t~~ ✅ COMPLETE (runtime offset lookup)
2. ~~**Graphics loading issues** (`ironwail-hu3.31`) - conchars, menus not loading in H2 mode~~ ✅ Menu system implemented

### High Priority (basic gameplay)
1. ~~**Frame control builtins** (`ironwail-vjj`) - AdvanceFrame, RewindFrame needed for animations~~ ✅ DONE
2. ~~**Effects builtins** (`ironwail-104`) - particle2-4, starteffect, endeffect~~ ✅ DONE

### Medium Priority (full experience)
1. **Rendering features** (`ironwail-80p`) - Verify scale/drawflags working
2. ~~**Protocol extensions** (`ironwail-soo`) - Extended stats SC1/SC2~~ ✅ DONE (server-side inventory sync)
3. ~~**Sound extensions** (`ironwail-deq`) - UpdateSoundPos, StopSound~~ ✅ DONE
4. ~~**Water jump bug** (`ironwail-4m8`) - Some H2 maps require low physics framerates~~ ✅ FIXED (H2 physics capped at 20fps)

### Low Priority (polish)
1. ~~**Effects load from save** - Full string buffer parsing needed for load side~~ ✅ COMPLETE
2. ~~**svc_mod_name** - UQE v1.13 extension for music files~~ ✅ Already implemented

### Known Issues (under investigation)
1. **Spider melee attack not triggering** (`claudedir-re7`) - Spider approaches but doesn't attack. Debug output added to traceline, needs testing with `developer 2`.

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
- `cl_effect.h` / `cl_effect.c` - Client effect system
- `sv_effect_hexen2.h` / `sv_effect_hexen2.c` - Server effect system with save/load
- `sv_inventory_hexen2.c` - Server-side inventory sync (SC1/SC2 protocol)
- `sbar_hexen2.h` / `sbar_hexen2.c` - H2 HUD
- `menu_hexen2.h` / `menu_hexen2.c` - H2 menu system (bigfont, class/difficulty select)
- `pr_cmds_hexen2.h` / `pr_cmds_hexen2.inc` - H2 builtins
- `progdefs.h2` - H2 entity/global structures

### Modified for H2
- `progdefs.h` - Includes H2 defs
- `pr_comp.h` - H2 opcodes enum
- `pr_exec.c` - Opcode implementations
- `pr_edict.c` - H2 globals setup, CRC checking
- `pr_cmds.c` - Builtin registration
- `progs.h` - H2 globals struct
- `sv_main.c` - H2 mode checks, inventory sync hook
- `sv_phys.c` - SV_PushRotate for rotating BSP, movechain system
- `host_cmd.c` - H2 mode checks, effects save/load hook
- `server.h` - FL_MOVECHAIN_ANGLE flag
- `world.c` - Hull field for collision
- `r_alias.c` - H2 rendering features
- `r_part.c` - H2 particles/trails
- `view.c` - H2 view effects
- `sbar.c` - H2 HUD redirect
- `menu.h` - Added m_class, m_difficulty states
- `menu.c` - H2 menu dispatch, M_H2_Init call
- `client.h` - H2 client state (max_mana, max_health, armor_amulet/bracer/breastplate/helmet)
- `render.h` - drawflags/abslight
- `glquake.h` - H2 particle/trail types
- `Makefile` - Added sv_effect_hexen2.o, sv_inventory_hexen2.o, menu_hexen2.o

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
- [x] H2 mode auto-detected (puzzles.txt, data1/)
- [x] Portals auto-detected (pak3.pak, cport5.lmp)
- [x] 5 classes available with Portals (Demoness)
- [x] Map loads in dedicated server (demo1: 306 entities)
- [x] Player spawn point exists (info_player_start)
- [x] H2 progs loads (v1.11 with runtime offset handling)
- [x] sv_sound_distance cvar works
- [ ] Game loads without crash (graphics mode)
- [ ] Player can move
- [ ] Entities render correctly
- [ ] HUD displays properly
- [ ] Sound plays
- [ ] Effects render
- [ ] Can complete demo1 level

---

## Version Reference

### Progs CRC Values (from progs.dat header)
- Hexen II v1.03: CRC 14046
- Hexen II v1.11: CRC 38488
- Hexen II v1.12 (Portal of Praevus): CRC 26905
- Hexen II UQE patch: CRC 19889

### Known Pak File Checksums

#### Hexen II v1.11 (data1/)
- pak0.pak: MD5 c9675191e75dd25a3b9ed81ee7e05eff
- pak1.pak: MD5 c2ac5b0640773eed9ebe1cda2eca2ad0
- progs.dat: MD5 2334c5036d573d6831fc95737142bf73 (CRC 38488)

#### Portal of Praevus v1.12 (portals/)
- pak3.pak: MD5 77ae298dd0dcd16ab12f4a68067ff2c3
- progs.dat: MD5 cb360095b039043dd619cc9cb83144b2 (CRC 26905)

### Progs Version Differences
- v1.11 globalvars_t: has `randomclass`, NO `cl_playerclass`, HAS `modelindex_*`
- v1.12 globalvars_t: has `randomclass`, HAS `cl_playerclass`, NO `modelindex_*`
- Field offsets differ after `randomclass` between versions
- Engine uses runtime offset lookup (ED_FindGlobalOffset) to handle differences
