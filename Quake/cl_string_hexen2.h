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

// cl_string_hexen2.h -- Hexen II internationalized strings

#ifndef CL_STRING_HEXEN2_H
#define CL_STRING_HEXEN2_H

// Load puzzle piece names from puzzles.txt
void CL_LoadPuzzleStrings(void);

// Look up puzzle piece full name by short name (returns NULL if not found)
const char *CL_FindPuzzleString(const char *shortname);

// Load mission pack objective strings from infolist.txt
void CL_LoadInfoStrings(void);

// Get objective string by index (returns "" if out of range)
const char *CL_GetInfoString(int idx);

// Get total number of info strings
int CL_GetInfoStringCount(void);

// Clear all loaded strings (called on disconnect)
void CL_ClearStrings(void);

#endif // CL_STRING_HEXEN2_H
