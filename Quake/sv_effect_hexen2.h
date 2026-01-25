/*
Copyright (C) 1996-1997 Id Software, Inc.
Copyright (C) 1997-1998 Raven Software Corp.
Copyright (C) 2024 Ironwail developers

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

// sv_effect_hexen2.h -- Server-side Hexen II effect system

#ifndef SV_EFFECT_HEXEN2_H
#define SV_EFFECT_HEXEN2_H

// Clear all effects (call on level change)
void SV_ClearEffects(void);

// Send all active effects to a client (for reconnection/spawn)
void SV_UpdateEffects(sizebuf_t *sb);

// Parse a starteffect() builtin call and send to clients
// Returns the effect index, or -1 on error
int SV_ParseEffect(void);

// End an effect by index
void SV_EndEffect(int idx);

// Save/load effects for save games
void SV_SaveEffects(FILE *f);
void SV_LoadEffects(FILE *f);

#endif // SV_EFFECT_HEXEN2_H
