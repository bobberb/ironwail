/*
Copyright (C) 1996-2001 Id Software, Inc.
Copyright (C) 2002-2009 John Fitzgibbons and others
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
 * Note: We always use the H2 extended progdefs to support both
 * Quake and Hexen II in a single binary. The H2 entvars_t is
 * a superset of Quake's entvars_t, so Quake progs work fine.
 * The extra H2 fields are simply ignored when running Quake.
 */
#include "progdefs.h2"

/*
 * For reference, Quake-only progdefs are in progdefs.q1
 * We keep it around for documentation but don't use it.
 */

#endif	/* __PROGDEFS_H */

