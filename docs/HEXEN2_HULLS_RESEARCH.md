# Hexen II Collision Hulls Research

## Overview

Quake and Hexen II use different numbers of collision hulls in their BSP files.
This affects entity collision detection and requires changes for full H2 support.

## Hull Comparison

### Quake (4 hulls, 3 used)

| Hull | Size | Purpose |
|------|------|---------|
| 0 | Point | Trace rays, projectiles |
| 1 | 32x32x56 | Player |
| 2 | 64x64x88 | Large monsters (Shambler) |
| 3 | Unused | Reserved |

**Selection logic** (world.c):
```c
if (size[0] < 3)
    hull = &model->hulls[0];      // Point
else if (size[0] <= 32)
    hull = &model->hulls[1];      // Player
else
    hull = &model->hulls[2];      // Large
```

### Hexen II (8 hulls, 6 used)

| Hull | Size | Purpose |
|------|------|---------|
| 0 | Point | Trace rays, projectiles |
| 1 | 32x32x? | Full player |
| 2 | ? | Unknown/unused |
| 3 | 32x28 | Crouching player |
| 4 | 8x8 | Pentacles (small items) |
| 5 | Large | Golem (large monsters) |
| 6-7 | ? | Additional sizes |

**Selection logic** (world.c):
```c
if (move_ent->v.hull)              // Entity specifies hull explicitly
    hull = &model->hulls[move_ent->v.hull - 1];
else if (size[0] < 3)
    hull = &model->hulls[0];       // Point
else if (size[0] <= 8 && spawnflags & 1)
    hull = &model->hulls[4];       // Pentacles
else if (size[0] <= 32 && size[2] <= 28)
    hull = &model->hulls[3];       // Crouching
else if (size[0] <= 32)
    hull = &model->hulls[1];       // Player
else
    hull = &model->hulls[5];       // Golem
```

## Key Differences

### 1. Entity Hull Field
H2 entities have a `.hull` field allowing explicit hull selection:
```c
float hull;  // in entvars_t
```
This bypasses automatic size-based selection.

### 2. Crouching Support
H2 hull 3 (32x28) enables crouching mechanics - player height reduced.

### 3. Small Object Hull
H2 hull 4 for "pentacles" (8x8) - precise collision for small items.

### 4. BSP Format
Both use BSPVERSION 29, but:
- Quake: `MAX_MAP_HULLS = 4`
- Hexen II: `MAX_MAP_HULLS = 8`

The BSP `dmodel_t` structure stores `headnode[MAX_MAP_HULLS]`.

## HexenC vs QuakeC Translation

### Problem
H2 progs.dat uses `.hull` field and expects 8 hulls.
Quake maps only have 4 hulls compiled.

### Solutions

1. **Runtime hull mapping**: Map H2 hull requests to available Quake hulls
   ```c
   // H2 hull -> Q1 hull
   // 0 -> 0 (point)
   // 1 -> 1 (player)
   // 3 -> 1 (crouch -> player, lose precision)
   // 4 -> 0 (small -> point)
   // 5 -> 2 (golem -> large)
   ```

2. **Ignore .hull field**: Use size-based selection only (breaks some H2 entities)

3. **Require H2 BSPs**: Only support maps compiled with H2 tools

## Required Ironwail Changes

### bspfile.h
```c
#ifdef HEXEN2_SUPPORT
#define MAX_MAP_HULLS 8
#else
#define MAX_MAP_HULLS 4
#endif
```

Or use runtime detection based on BSP content.

### world.c
Add H2 hull selection logic when `hexen2_mode`:
- Check `ent->v.hull` field
- Use extended size checks
- Support crouching hull

### gl_model.c / sv_model.c
Handle loading BSPs with 4 or 8 hulls based on detected format.

### progdefs.h
Already includes `.hull` field in H2 progdefs.

## Recommendations

1. **For now**: Use Quake hull selection, H2 maps need H2-compiled BSPs
2. **Future**: Add runtime MAX_MAP_HULLS based on loaded BSP
3. **Testing**: Verify with original H2 maps which use all hull types

## References

- uhexen2/source/engine/hexen2/world.c:125 - SV_HullForEntity
- uhexen2/source/common/bspfile.h:25 - MAX_MAP_HULLS definition
- ironwail/Quake/world.c:132 - Quake SV_HullForEntity
