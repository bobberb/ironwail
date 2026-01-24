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

#ifndef _SBAR_HEXEN2_H
#define _SBAR_HEXEN2_H

// Hexen II status bar / HUD

// Ring types
#define H2_RING_FLIGHT		1
#define H2_RING_WATER		2
#define H2_RING_REGENERATION	4
#define H2_RING_TURNING		8

// Inventory display
#define H2_INV_MAX_ICON		6	// Max icons visible at once
#define H2_INV_DISPLAY_TIME	4	// Seconds to show inventory

// Artifact indices (relative to cnt_torch in entvars)
#define H2_INV_TORCH		0
#define H2_INV_HP_BOOST		1
#define H2_INV_SUPER_HP_BOOST	2
#define H2_INV_MANA_BOOST	3
#define H2_INV_TELEPORT		4
#define H2_INV_TOME		5
#define H2_INV_SUMMON		6
#define H2_INV_INVISIBILITY	7
#define H2_INV_GLYPH		8
#define H2_INV_HASTE		9
#define H2_INV_BLAST		10
#define H2_INV_POLYMORPH	11
#define H2_INV_FLIGHT		12
#define H2_INV_CUBEOFFORCE	13
#define H2_INV_INVINCIBILITY	14
#define H2_INV_MAX		15

// Function declarations
void Sbar_H2_Init(void);
void Sbar_H2_Draw(void);
void Sbar_H2_DrawMini(void);		// Minimal HUD mode
void Sbar_H2_IntermissionOverlay(void);
void Sbar_H2_FinaleOverlay(void);

// Inventory commands
void Sbar_H2_InvLeft(void);
void Sbar_H2_InvRight(void);
void Sbar_H2_InvUse(void);
void Sbar_H2_InvOff(void);

// Info display
void Sbar_H2_ShowInfo(qboolean show);
void Sbar_H2_ShowDM(qboolean show);

// Inventory state management
void Sbar_H2_InvChanged(void);	// Called when artifact counts change

#endif /* _SBAR_HEXEN2_H */
