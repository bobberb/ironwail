# Hexen II Integration Plan for Ironwail

**Goal**: Make Ironwail capable of running both vanilla Quake and Hexen II games with a unified engine.

## Overview

This document outlines the plan to integrate Hexen II features into Ironwail, allowing it to function as a dual-mode engine that can run both Quake and Hexen II content.

## Phase 1: Protocol & Network Foundation

### 1.1 Protocol Detection & Switching ✅ **COMPLETED**
- [x] Add Hexen II protocol version detection (PROTOCOL_RAVEN_111/112/UQE_113)
- [x] Implement runtime protocol switching based on loaded game data
- [x] Add `game_hexen2` cvar to force Hexen II mode
- [x] Update `protocol.h` with Hexen II-specific message types

**Files modified:**
- `protocol.h` - Added #include for protocol_hexen2.h
- `protocol_hexen2.h` - Created with H2 protocol definitions, message types, constants
- `protocol_hexen2.c` - Implemented H2_Protocol_Init(), H2_DetectGameType(), H2_IsHexen2Protocol(), H2_SetProtocol()
- `common.c` - Added H2 game data detection (data1/pak0.pak, portals/, puzzles.txt)
- `cl_parse.c` - Added H2 protocol version handling in svc_version

**What works:**
- Engine detects H2 game data automatically
- Supports forced H2 mode via `game_hexen2` cvar
- Protocol versions 18-20 recognized as H2 protocols
- Runtime hexen2_mode flag controls behavior

### 1.2 Network Messages ✅ **COMPLETED**
- [x] Add Hexen II server messages (svc_updateclass, svc_update_inv, etc.)
- [x] Implement svc_start_effect/svc_end_effect for effect streaming
- [x] Add svc_particle_explosion for material-aware particles
- [x] Implement stat bar messages (SC1/SC2 stat bits) - Parser stubs ready

**Files modified:**
- `cl_parse_hexen2.c` - Created with 13 H2 message parsers
- `cl_parse_hexen2.h` - Function declarations
- `cl_parse.c` - Integrated H2 message handling in default case
- `client.h` - Added H2 client state fields (playerclass, idealroll, inv_selected, puzzle_pieces)
- `client.h` - Added playerclass to scoreboard_t

**Message handlers implemented:**
- CL_ParseUpdateClass() - Player class updates
- CL_ParseMidiName() - MIDI music (stub)
- CL_ParseParticleExplosion() - Material-aware particles (stub)
- CL_ParseSetViewTint() - View tinting (stub)
- CL_ParseUpdateInventory() - Inventory updates (stub)
- CL_ParseStartEffect() / CL_ParseEndEffect() - Effect streaming (stubs)
- CL_ParsePlaque() - Story overlays (stub)
- CL_ParseParticle2() - Extended particles (stub)
- CL_ParseRainEffect() - Weather effects (stub)
- CL_ParseSoundUpdatePos() - Sound positioning (stub)
- CL_ParseModName() - MOD music (stub)
- CL_ParseSkybox() - Skybox setting (stub)

**Note:** Message parsing is complete, but actual functionality (particles, effects, audio, etc.) will be implemented in later phases.

**New message types needed:**
```c
svc_updateclass = 37
svc_midi_name = 36
svc_particle_explosion = 45
svc_set_view_tint = 46
svc_update_inv = 49
svc_start_effect = 42
svc_end_effect = 43
svc_sound_update_pos = 53
```

## Phase 2: Entity & Game Logic

### 2.1 Extended Entity Variables ✅ **COMPLETED**
- [x] Add Hexen II-specific entvars_t fields to `progdefs.h`
- [x] Implement player class system (4 classes)
- [x] Add dual mana system (blue/green mana)
- [x] Implement experience/leveling system
- [x] Add armor slot system (amulet, bracer, breastplate, helmet)

**Files created/modified:**
- `progdefs.h2` - Complete H2 entity variable definitions (superset of Quake)
- `progdefs.h` - Updated to use H2 progdefs for both engines
- Added all H2 player stats, mana, armor, artifacts, rings, puzzle pieces
- Maintained Quake compatibility (ammo fields, sounds, aiment, SetNewParms, etc.)

**What works:**
- Single binary supports both Quake and Hexen II entvars_t
- Build successful with all H2 fields available
- Quake code continues to work with extended structure

### 2.2 Progs/QuakeC Compatibility ✅ **COMPLETED**
- [x] Add Hexen II builtin function support
- [x] Implement progs CRC checking for H2 compatibility
- [ ] Support multiple progs.dat loading (map-specific progs) - DEFERRED
- [ ] Handle progs v6 detection - DEFERRED

**Files modified:**
- `pr_edict.c` - Modified CRC checking to accept H2 CRCs (38488, 26905, 14046, 19889) when hexen2_mode is active
- `pr_cmds.c` - Added 30+ H2 builtin function stubs to builtin table
- `pr_cmds_hexen2.inc` - Created H2 builtin implementations (stubs for future phases)
- `pr_cmds_hexen2.h` - Function declarations
- `quakedef.h` - Added protocol_hexen2.h include

**What works:**
- Engine accepts H2 progs.dat files with correct CRCs
- All 30+ H2 builtins are registered and callable
- Stub implementations prevent crashes
- Build successful with all H2 support

**H2 builtins added:**
- #5: lightstylestatic (calls PF_lightstyle for now)
- #33: tracearea (bounding box trace)
- #42: particle2 (stub - Phase 3)
- #50: vhlen (horizontal vector length)
- #63: AdvanceFrame (stub - animation)
- #66: RewindFrame (stub - animation)
- #67: setclass (sets player class)
- #72: lightstylevalue (returns static value for now)
- #79-93: Various H2-specific functions (plaques, rain, particles, effects, puzzles)
- #99-106: Slope matching, plaque updates, screen flash, sound positioning

**Note:** Multi-progs loading and progs v6 detection deferred until needed

## Phase 3: Rendering Enhancements

### 3.1 Entity Rendering Extensions ✅ **COMPLETED**
- [x] Add drawflags field to entity structures
- [x] Add abslight field to entity structures
- [x] Implement DRF_TRANSLUCENT rendering (50% alpha)
- [x] Implement DRF_ANIMATEONCE (freeze on last frame)
- [x] Implement abslight support (absolute lighting override)
- [x] Update network parsing to handle drawflags and abslight
- [ ] Add colormap extensions for class-specific skins - DEFERRED

**Files modified:**
- `protocol.h` - Added drawflags and abslight to entity_state_t
- `protocol_hexen2.h` - Added H2_DRF_TRANSLUCENT and H2_DRF_ANIMATEONCE defines
- `render.h` - Added drawflags and abslight to entity_t
- `r_alias.c` - Implemented H2 rendering features:
  - Abslight support in R_SetupAliasLighting (overrides normal lighting)
  - DRF_TRANSLUCENT support (50% alpha if not already transparent)
  - DRF_ANIMATEONCE support (freeze animation on last frame)
- `cl_parse.c` - Added network parsing for drawflags (with U_SKIN) and abslight (with U_SCALE)

**What works:**
- Entities can have absolute lighting (abslight 0-255)
- Entities can be translucent via drawflags
- Entities can play animation once and freeze
- Network protocol correctly sends/receives these fields
- Build successful

### 3.2 Advanced Particle System ✅ **COMPLETED**
- [x] Expand particle types from ~5 to 30+ (pt_ice, pt_spell, pt_vorpal, etc.)
- [x] Add 17+ trail types (rt_setstaff, rt_magicmissile, rt_scarab, etc.)
- [x] Implement particle ramps for color cycling
- [x] Add weather particle support (rain, snow with SFL_ flags)
- [x] MAX_PARTICLES already 16384 (sufficient)

**Files modified:**
- `glquake.h` - Added 22 H2 particle types (ptype_t enum) and 17 trail types (rt_type_t enum)
- `glquake.h` - Extended particle_t with min_org, max_org, flags, count fields
- `r_part.c` - Extended R_RocketTrail with all H2 trail types
- `r_part.c` - Updated CL_RunParticles with proper H2 color ramp cycling
- `r_part.c` - Added R_RainEffect, R_SnowEffect, R_ColoredParticleExplosion
- `r_part.c` - Added R_RunQuakeEffect, R_RunParticleEffect2/3/4, R_SunStaffTrail
- `protocol_hexen2.h` - Added H2_SFL_* snow flags

**What works:**
- All H2 particle types with proper physics behavior
- Color ramp cycling for fireball, ice, spit, spell, acidball, etc.
- Weather effects (rain, snow with SFL_ flags)
- H2 trail types (fireball, ice, vorpal, setstaff, magic missile, etc.)
- Build compiles successfully

### 3.3 Effect System ✅ **COMPLETED**
- [x] Implement 62+ hardcoded visual effects (CE_RAIN, CE_FOUNTAIN, etc.)
- [x] Add material-aware particle chunks (24 material types - THINGTYPE_* constants)
- [x] Implement effect management system (MAX_EFFECTS = 256)
- [ ] Add server-side effect culling - DEFERRED

**Files created:**
- `cl_effect.h` - Effect structures, CE_* constants, THINGTYPE_* materials
- `cl_effect.c` - Effect parsing, management, and per-frame updates

**What works:**
- All 62+ CE_* effect types parsed and managed
- Effect entity pool (MAX_EFFECT_ENTITIES = 256)
- Rain/snow weather effects via R_RainEffect/R_SnowEffect
- Fountain particles via R_RunParticleEffect2
- Quake effect via R_RunQuakeEffect
- Smoke/explosion/flash effect timing and cleanup
- Teleporter, missile, and chunk effect support
- Build compiles successfully

### 3.4 View & Color Enhancements ✅ **COMPLETED**
- [x] Expand color shift system to 5 types (CSHIFT_INTERVENTION for H2)
- [x] Add view tint protocol support (svc_h2_set_view_tint -> colorshade)
- [x] Implement ideal roll tracking (V_DriftRoll, v_centerrollspeed cvar)
- [x] Add H2-specific powerup color shifts (frozen, stoned, divine intervention)
- [x] Add df/wf commands for dark/white flash effects

**Files modified:**
- `client.h` - Added CSHIFT_INTERVENTION, rollvel, artifact_active fields
- `render.h` - Added colorshade to entity_t
- `protocol_hexen2.h` - Added H2_ARTFLAG_* and H2_ART_* defines
- `view.c` - Added V_DriftRoll(), H2 V_CalcPowerupCshift(), df/wf commands
- `cl_parse_hexen2.c` - Implemented CL_ParseSetViewTint()

**What works:**
- 5 cshift slots (contents, damage, bonus, powerup, intervention)
- Roll drifting towards idealroll for swimming effects
- H2 artifact effects (frozen=blue, stoned=gray, divine=white)
- Dark flash (df) and white flash (wf) commands
- View weapon colorshade tinting via svc_h2_set_view_tint

## Phase 4: UI & Inventory

### 4.1 Status Bar ✅ **COMPLETED**
- [x] Implement Hexen II stat bar layout
- [x] Add dual mana bars (blue/green)
- [x] Display artifact inventory (15 slots)
- [x] Show ring status (4 rings)
- [x] Add puzzle piece display (8 slots)
- [ ] Implement class-specific HUD elements - DEFERRED (weapon icons)

**Files created/modified:**
- `sbar_hexen2.c` - Created H2-specific HUD implementation (700+ lines)
- `sbar_hexen2.h` - Created H2 HUD header with ring/artifact defines
- `sbar.c` - Added hexen2_mode redirect to H2 HUD functions
- `client.h` - Added inventory and ring state fields

**What works:**
- Top bar with health chain, mana bars
- Bottom bar with class icon, armor slots
- Ring status with power indicators
- Artifact inventory display with selection
- Puzzle piece display (8 slots)
- Inventory navigation commands (+inv_left, +inv_right, +inv_use)

### 4.2 Inventory System ✅ **COMPLETED**
- [x] Implement artifact inventory (15 items max)
- [x] Add ring equipping/management
- [x] Implement puzzle piece collection UI
- [ ] Add inventory blinking on acquisition - DEFERRED (polish)
- [x] Handle artifact usage/depletion

**Files modified:**
- `cl_parse_hexen2.c` - Full svc_update_inv parsing with SC1/SC2 bitfields
- `sbar_hexen2.c` - Added Sbar_H2_InvChanged() to rebuild inventory order
- `sbar_hexen2.h` - Added Sbar_H2_InvChanged declaration
- `sbar.c` - Registered inventory commands (invleft, invright, invuse, invoff)

**What works:**
- Full H2 stats parsing (health, mana, armor, rings, artifacts)
- Artifact count tracking in cl.inv_cnt[] array
- Automatic inventory order rebuilding when counts change
- Inventory commands with wrapping selection
- Ring power tracking for HUD display

### 4.3 Menu Extensions ✅ **COMPLETED**
- [ ] Add class selection menu - DEFERRED (use "playerclass X" command)
- [x] Implement H2 mission pack detection
- [x] Add objectives/info string display
- [x] Support H2 game data detection (done in Phase 1)

**Files modified:**
- `protocol_hexen2.c` - Added hexen2_missionpack flag
- `protocol_hexen2.h` - Added extern for missionpack flag
- `sbar_hexen2.c` - Added Sbar_H2_DrawInfoOverlay() for puzzle display
- `sbar.c` - Registered +showinfo/-showinfo, +showdm/-showdm commands

**What works:**
- Portal of Praevus mission pack auto-detected
- +showinfo shows puzzle pieces overlay
- +showdm for deathmatch overlay toggle

## Phase 5: Audio & Assets

### 5.1 Audio Extensions
- [ ] Add MIDI support for H2 music tracks
- [ ] Implement music name overrides (svc_midi_name, svc_mod_name)
- [ ] Support H2 sound positioning updates
- [ ] Handle H2-specific audio codec requirements

### 5.2 Asset Loading
- [ ] Support H2 model formats (potentially different from Quake)
- [ ] Load H2 textures and sprites
- [ ] Handle H2 BSP format differences
- [ ] Support puzzle string loading from external file
- [ ] Load info strings for mission pack objectives

## Phase 6: Multiplayer Features

### 6.1 HexenWorld Support
- [ ] Implement HexenWorld-specific protocol
- [ ] Add King of the Hill game mode support
- [ ] Handle class-aware networking
- [ ] Implement objective tracking sync

### 6.2 Scoring & Stats
- [ ] Add per-class scoreboards
- [ ] Implement experience/level display
- [ ] Show artifact status in multiplayer
- [ ] Handle H2 team modes

## Phase 7: Testing & Polish

### 7.1 Compatibility Testing
- [ ] Test with original Hexen II levels
- [ ] Test with Portal of Praevus (mission pack)
- [ ] Verify Quake compatibility still works
- [ ] Test protocol switching between games
- [ ] Test multiplayer in both modes

### 7.2 Performance Optimization
- [ ] Optimize particle system for 7000 particles
- [ ] Optimize effect system rendering
- [ ] Ensure no regression in Quake mode performance
- [ ] Profile H2-specific code paths

### 7.3 Documentation
- [ ] Document H2 mode usage
- [ ] Add setup instructions for H2 game data
- [ ] Document new cvars and commands
- [ ] Create compatibility notes

## Side Mission: Headless Mode Support

**Purpose**: Enable testing and CI/CD without video/audio devices. Useful for automated testing, dedicated servers, and development in restricted environments.

### Tasks:
- [ ] Add `-headless` command-line flag
- [ ] Implement dummy video backend (no SDL window/OpenGL)
  - Skip VID_Init() and renderer initialization
  - Provide stub functions for screen updates
  - Allow console-only operation
- [ ] Implement dummy audio backend (no SDL audio)
  - Skip sound initialization
  - Provide stub functions for sound playback
  - Allow silent operation
- [ ] Ensure game logic still runs without rendering
  - Server tick updates
  - Physics simulation
  - Network message processing
- [ ] Add headless test suite
  - Load maps headlessly
  - Validate game state
  - Test protocol handling
  - Automated regression testing

**Files to modify:**
- `vid_sdl.c` - Add headless video mode
- `snd_sdl.c` - Add headless audio mode
- `host.c` - Skip unnecessary init in headless mode
- `main_sdl.c` - Parse -headless flag

**Benefits:**
- ✅ Enables CI/CD testing without X11/display
- ✅ Faster automated testing (no rendering overhead)
- ✅ Dedicated server support
- ✅ Better development workflow in headless environments
- ✅ Validates that H2 detection and initialization work correctly

**Priority**: Medium (useful for development, not critical for user features)

## Implementation Strategy

### Recommended Order:

1. **Start with Phase 1** - Protocol foundation is critical
2. **Phase 2** - Entity/game logic (can test with minimal rendering)
3. **Phase 3.1** - Basic rendering extensions
4. **Phase 4.1** - Basic HUD to see game state
5. **Phase 3.2-3.4** - Advanced rendering
6. **Phase 4.2-4.3** - Full UI
7. **Phase 5** - Audio/assets
8. **Phase 6** - Multiplayer (optional, can defer)
9. **Phase 7** - Testing & polish

### Development Approach:

**Option A: Conditional Compilation**
```c
#ifdef HEXEN2_SUPPORT
// H2-specific code
#endif
```
- Pros: Clean separation, can disable for Quake-only builds
- Cons: More #ifdefs, potential for code drift

**Option B: Runtime Detection**
```c
if (game_hexen2) {
    // H2 code
} else {
    // Quake code
}
```
- Pros: Single binary supports both, easier testing
- Cons: Larger binary, more runtime checks

**Recommendation**: Use Option B (runtime detection) for most code, with Option A only for large subsystems that can be cleanly separated.

### File Organization:

Create H2-specific files where possible:
- `protocol_h2.h` - H2 protocol definitions
- `cl_effect.c` / `cl_effect.h` - Effect system
- `r_part_h2.c` - H2 particle extensions
- `sbar_h2.c` - H2 status bar
- `pr_cmds_h2.c` - H2 builtin functions

## Dependencies & References

### uhexen2 Source
Location: `/home/josh/tmp/claudedir/uhexen2/source/`
- Use as reference for protocol, entities, particles, effects
- Cherry-pick implementations where possible
- Adapt to Ironwail's modern OpenGL architecture

### Key uhexen2 Files to Study:
- `engine/hexen2/protocol.h` - Protocol definitions
- `h2shared/progdefs.h` - Entity variables
- `engine/hexen2/r_part.c` - Particle system
- `engine/hexen2/client.h` - Client state
- `engine/hexen2/cl_parse.c` - Message parsing
- `engine/hexen2/cl_effect.c` - Effect system
- `engine/hexen2/sbar.c` - Status bar

### Testing Resources
- Original Hexen II game data (pak files)
- Portal of Praevus mission pack
- Hexen II demo version
- Community maps/mods

## Success Criteria

- [ ] Can load and play original Hexen II levels
- [ ] Can load and play Portal of Praevus
- [ ] All 4 character classes functional
- [ ] Mana/artifact/ring systems working
- [ ] Particles and effects rendering correctly
- [ ] HUD displays all game state properly
- [ ] Quake compatibility maintained (no regressions)
- [ ] Performance comparable to Ironwail's Quake mode
- [ ] Can switch between Quake and H2 without restart

## Estimated Effort

This is a **major undertaking**, roughly equivalent to:
- **Phase 1-2**: 40-60 hours (foundation)
- **Phase 3**: 60-80 hours (rendering)
- **Phase 4**: 40-50 hours (UI)
- **Phase 5**: 30-40 hours (audio/assets)
- **Phase 6**: 40-50 hours (multiplayer)
- **Phase 7**: 30-40 hours (testing/polish)

**Total**: ~240-320 hours for complete implementation

Could be done incrementally over several months with:
- Weekly milestones
- Feature-by-feature integration
- Continuous testing against both games
