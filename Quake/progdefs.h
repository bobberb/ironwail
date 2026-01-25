/*
Copyright (C) 1996-2001 Id Software, Inc.
Copyright (C) 2002-2009 John Fitzgibbons and others
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

#ifndef __PROGDEFS_H
#define __PROGDEFS_H

/*
 * Use Quake's progdefs as base for compatibility.
 * H2-specific fields are accessed via runtime offset lookup.
 */
#include "progdefs.q1"

/* Hexen II progs CRC values for runtime detection */
#define PROGHEADER_CRC_H2_V103		14046	// Hexen II 1.03/demo
#define PROGHEADER_CRC_H2_V111		38488	// Hexen II 1.11 (also 1.09)
#define PROGHEADER_CRC_H2_V112		26905	// Portal of Praevus (Mission Pack)
#define PROGHEADER_CRC_H2_UQE		19889	// UQE patch

#endif	/* __PROGDEFS_H */

