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

### 2.1 Extended Entity Variables
- [ ] Add Hexen II-specific entvars_t fields to `progdefs.h`
- [ ] Implement player class system (4 classes)
- [ ] Add dual mana system (blue/green mana)
- [ ] Implement experience/leveling system
- [ ] Add armor slot system (amulet, bracer, breastplate, helmet)

**Key fields to add:**
```c
// Player stats
int level;
float experience;
int strength, dexterity, intelligence, wisdom;

// Mana system
float bluemana, greenmana, max_mana;

// Class
float playerclass;

// Artifacts (15 types)
float cnt_torch, cnt_h_boost, cnt_teleport, etc.

// Rings (4 types)
float ring_flight, ring_water, ring_turning, ring_regeneration;

// Puzzle pieces (8 slots)
string puzzle_inv1 through puzzle_inv8;
```

### 2.2 Progs/QuakeC Compatibility
- [ ] Support multiple progs.dat loading (map-specific progs)
- [ ] Add Hexen II builtin function support
- [ ] Implement progs CRC checking for H2 compatibility
- [ ] Handle progs v6 detection

**Files to modify:**
- `progs.h` - Add H2 builtin functions
- `pr_edict.c` - Extended entity field support
- `pr_cmds.c` - New builtin implementations
- `sv_main.c` - Multi-progs loading

## Phase 3: Rendering Enhancements

### 3.1 Entity Rendering Extensions
- [ ] Implement entity scale support (U_SCALE update bit)
- [ ] Add drawflags for special rendering modes
- [ ] Implement abslight (absolute lighting override)
- [ ] Add colormap extensions for class-specific skins

**Files to modify:**
- `gl_model.h` - Add scale/drawflags to entity_state_t
- `r_alias.c` - Scale and drawflags rendering
- `gl_rmain.c` - Abslight support

### 3.2 Advanced Particle System
- [ ] Expand particle types from ~5 to 30+ (pt_ice, pt_spell, pt_vorpal, etc.)
- [ ] Add 17+ trail types (rt_setstaff, rt_magicmissile, rt_scarab, etc.)
- [ ] Implement particle ramps for color cycling
- [ ] Add weather particle support (rain, snow with SFL_ flags)
- [ ] Increase MAX_PARTICLES from current to 7000

**Files to modify:**
- `r_part.c` - Massive expansion of particle system
- `render.h` - New particle type definitions
- Create new `r_part_h2.c` for H2-specific particle code

### 3.3 Effect System
- [ ] Implement 62+ hardcoded visual effects (CE_RAIN, CE_FOUNTAIN, etc.)
- [ ] Add material-aware particle chunks (24 material types)
- [ ] Implement effect management system (MAX_EFFECTS = 256)
- [ ] Add server-side effect culling

**New file needed:**
- `cl_effect.c` - Effect management system

### 3.4 View & Color Enhancements
- [ ] Expand color shift system to 5 types
- [ ] Add view tint protocol support
- [ ] Implement ideal roll tracking
- [ ] Add pitch drift system for immersion

## Phase 4: UI & Inventory

### 4.1 Status Bar
- [ ] Implement Hexen II stat bar layout
- [ ] Add dual mana bars (blue/green)
- [ ] Display artifact inventory (15 slots)
- [ ] Show ring status (4 rings)
- [ ] Add puzzle piece display (8 slots)
- [ ] Implement class-specific HUD elements

**Files to modify:**
- `sbar.c` - Major rewrite for H2 HUD
- May need new `sbar_h2.c` for H2-specific code

### 4.2 Inventory System
- [ ] Implement artifact inventory (15 items max)
- [ ] Add ring equipping/management
- [ ] Implement puzzle piece collection UI
- [ ] Add inventory blinking on acquisition
- [ ] Handle artifact usage/depletion

### 4.3 Menu Extensions
- [ ] Add class selection menu
- [ ] Implement H2 mission pack detection
- [ ] Add objectives/info string display
- [ ] Support H2 game data detection

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
