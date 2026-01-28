/*
Copyright (C) 1996-1997 Id Software, Inc.
Copyright (C) 1997-1998 Raven Software Corp.
Copyright (C) 2010-2014 QuakeSpasm developers
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

#ifndef _CL_PARSE_HEXEN2_H
#define _CL_PARSE_HEXEN2_H

// Hexen II message parsing functions
void CL_ParseUpdateClass(void);
void CL_ParseMidiName(void);
void CL_ParseParticleExplosion(void);
void CL_ParseSetViewTint(void);
void CL_ParseUpdateInventory(void);
void CL_ParsePlaque(void);
void CL_ParseParticle2(void);
void CL_ParseParticle3(void);
void CL_ParseParticle4(void);
void CL_ParseRainEffect(void);
void CL_ParseSoundUpdatePos(void);
void CL_ParseModName(void);
void CL_ParseSkybox(void);
void CL_ParseCutscene(void);
void CL_ParseSetViewFlags(void);
void CL_ParseClearViewFlags(void);
void CL_ParseToggleStatbar(void);

// H2 intermission system
void CL_SetupIntermission(int num);

#endif /* _CL_PARSE_HEXEN2_H */
