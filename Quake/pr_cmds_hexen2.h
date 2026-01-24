/*
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

// pr_cmds_hexen2.h -- Hexen II builtin function declarations

#ifndef PR_CMDS_HEXEN2_H
#define PR_CMDS_HEXEN2_H

// H2-specific builtin functions
static void PF_h2_lightstylestatic (void);   // #5
static void PF_h2_tracearea (void);          // #33
static void PF_h2_particle2 (void);          // #42
static void PF_h2_vhlen (void);              // #50
static void PF_h2_AdvanceFrame (void);       // #63
static void PF_h2_RewindFrame (void);        // #65
static void PF_h2_setclass (void);           // #66
static void PF_h2_lightstylevalue (void);    // #71
static void PF_h2_plaque_draw (void);        // #79
static void PF_h2_rain_go (void);            // #80
static void PF_h2_particleexplosion (void);  // #81
static void PF_h2_movestep (void);           // #82
static void PF_h2_advanceweaponframe (void); // #83
static void PF_h2_particle3 (void);          // #85
static void PF_h2_particle4 (void);          // #86
static void PF_h2_setpuzzlemodel (void);     // #87
static void PF_h2_starteffect (void);        // #88
static void PF_h2_endeffect (void);          // #89
static void PF_h2_precache_puzzle_model (void); // #90
static void PF_h2_concatv (void);            // #91
static void PF_h2_GetString (void);          // #92
static void PF_h2_SpawnTemp (void);          // #93
static void PF_h2_v_factor (void);           // #94
static void PF_h2_v_factorrange (void);      // #95
static void PF_h2_matchAngleToSlope (void);  // #99
static void PF_h2_updateInfoPlaque (void);   // #100
static void PF_h2_doWhiteFlash (void);       // #104
static void PF_h2_UpdateSoundPos (void);     // #105
static void PF_h2_StopSound (void);          // #106

#endif // PR_CMDS_HEXEN2_H
