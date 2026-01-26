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

#ifndef _MENU_HEXEN2_H
#define _MENU_HEXEN2_H

// menu_hexen2.h -- Hexen II menu system

// Player class constants
#define H2_MAX_PLAYER_CLASS		5	// Paladin, Crusader, Necromancer, Assassin, Demoness
#define H2_NUM_BASE_CLASSES		4	// Without Portals expansion
#define H2_NUM_DIFFLEVELS		4	// Easy, Normal, Hard, Nightmare equivalent

// Player class indices
#define H2_CLASS_PALADIN		1
#define H2_CLASS_CRUSADER		2
#define H2_CLASS_NECROMANCER	3
#define H2_CLASS_ASSASSIN		4
#define H2_CLASS_DEMONESS		5	// Portals expansion only

// Class names (for display)
extern const char *h2_class_names[];		// lowercase names
extern const char *h2_class_names_upper[];	// uppercase for bigfont

// Class-specific difficulty names
extern const char *h2_diff_names[H2_MAX_PLAYER_CLASS][H2_NUM_DIFFLEVELS];

// Bigfont support
void M_H2_BuildBigCharWidth(void);
int M_H2_BigCharWidth(int c1, int c2);
void M_H2_DrawBigString(int x, int y, const char *str);
void M_H2_DrawBigCharacter(int x, int y, int num);

// Animated title support
void M_H2_ScrollTitle(const char *name);

// Menu initialization
void M_H2_Init(void);

// Main menu handlers
void M_H2_Draw(void);
void M_H2_Keydown(int key);

// Hexen II specific menus
void M_Menu_H2_Class_f(void);
void M_Menu_H2_Difficulty_f(void);

// Check if Portals expansion is available
qboolean M_H2_HasPortals(void);

// Selected class (1-5, matches H2_CLASS_* values)
extern int h2_player_class;

#endif /* _MENU_HEXEN2_H */
