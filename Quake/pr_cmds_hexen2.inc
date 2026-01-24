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

// pr_cmds_hexen2.c -- Hexen II-specific builtin functions

#include "quakedef.h"

/*
=================
PF_h2_lightstylestatic

Static lightstyle (doesn't animate)
void lightstylestatic(float style, string value) = #5
=================
*/
static void PF_h2_lightstylestatic (void)
{
	// For now, just use regular lightstyle
	// TODO: Implement static lightstyle optimization
	PF_lightstyle();
}

/*
=================
PF_h2_tracearea

Trace with bounding box instead of just a line
float tracearea(vector v1, vector v2, vector mins, vector maxs, float nomonsters, entity ent) = #33
=================
*/
static void PF_h2_tracearea (void)
{
	float	*v1, *v2, *mins, *maxs;
	trace_t	trace;
	int	nomonsters;
	edict_t	*ent;

	v1 = G_VECTOR(OFS_PARM0);
	v2 = G_VECTOR(OFS_PARM1);
	mins = G_VECTOR(OFS_PARM2);
	maxs = G_VECTOR(OFS_PARM3);
	nomonsters = G_FLOAT(OFS_PARM4);
	ent = G_EDICT(OFS_PARM5);

	trace = SV_Move (v1, mins, maxs, v2, nomonsters, ent);

	pr_global_struct->trace_allsolid = trace.allsolid;
	pr_global_struct->trace_startsolid = trace.startsolid;
	pr_global_struct->trace_fraction = trace.fraction;
	pr_global_struct->trace_inwater = trace.inwater;
	pr_global_struct->trace_inopen = trace.inopen;
	VectorCopy (trace.endpos, pr_global_struct->trace_endpos);
	VectorCopy (trace.plane.normal, pr_global_struct->trace_plane_normal);
	pr_global_struct->trace_plane_dist =  trace.plane.dist;
	if (trace.ent)
		pr_global_struct->trace_ent = EDICT_TO_PROG(trace.ent);
	else
		pr_global_struct->trace_ent = EDICT_TO_PROG(qcvm->edicts);
}

/*
=================
PF_h2_particle2

Extended particle with color
void particle2(vector org, vector dmin, vector dmax, float color, float effect, float count) = #42
=================
*/
static void PF_h2_particle2 (void)
{
	// Stub: TODO - implement extended particle system in Phase 3
	Con_DPrintf("PF_h2_particle2: stub\n");
}

/*
=================
PF_h2_vhlen

Horizontal vector length (ignores Z component)
float vhlen(vector v) = #50
=================
*/
static void PF_h2_vhlen (void)
{
	float	*value1;
	float	newv;

	value1 = G_VECTOR(OFS_PARM0);

	newv = value1[0] * value1[0] + value1[1] * value1[1];
	newv = sqrt(newv);

	G_FLOAT(OFS_RETURN) = newv;
}

/*
=================
PF_h2_AdvanceFrame

Advance animation frame
void AdvanceFrame(float start, float end) = #63
=================
*/
static void PF_h2_AdvanceFrame (void)
{
	// Stub: TODO - implement proper animation frame advance
	Con_DPrintf("PF_h2_AdvanceFrame: stub\n");
}

/*
=================
PF_h2_RewindFrame

Rewind animation frame
void RewindFrame(float start, float end) = #65
=================
*/
static void PF_h2_RewindFrame (void)
{
	// Stub: TODO - implement proper animation frame rewind
	Con_DPrintf("PF_h2_RewindFrame: stub\n");
}

/*
=================
PF_h2_setclass

Set player class
void setclass(entity player, float newclass) = #66
=================
*/
static void PF_h2_setclass (void)
{
	int		entnum;
	edict_t	*e;
	float	newclass;

	e = G_EDICT(OFS_PARM0);
	entnum = NUM_FOR_EDICT(e);
	newclass = G_FLOAT(OFS_PARM1);

	if (entnum < 1 || entnum > svs.maxclients)
	{
		Con_Printf ("PF_h2_setclass: not a client\n");
		return;
	}

	e->v.playerclass = newclass;
	Con_DPrintf("PF_h2_setclass: entity %d set to class %d\n", entnum, (int)newclass);
}

/*
=================
PF_h2_lightstylevalue

Get current lightstyle value
float lightstylevalue(float style) = #71
=================
*/
static void PF_h2_lightstylevalue (void)
{
	int		style;

	style = G_FLOAT(OFS_PARM0);
	if (style < 0 || style >= MAX_LIGHTSTYLES)
	{
		Con_Warning("PF_h2_lightstylevalue: invalid style %d\n", style);
		G_FLOAT(OFS_RETURN) = 0;
		return;
	}

	// TODO: Return actual current value based on animation
	// For now just return a static value
	G_FLOAT(OFS_RETURN) = 264; // 'm' value (normal brightness)
}

/*
=================
PF_h2_plaque_draw

Draw story plaque
void plaque_draw(float) = #79
=================
*/
static void PF_h2_plaque_draw (void)
{
	// Stub: TODO - implement plaque drawing in Phase 4
	Con_DPrintf("PF_h2_plaque_draw: stub\n");
}

/*
=================
PF_h2_rain_go

Start rain effect
void rain_go(vector start, vector end, vector edir, float color, float count) = #80
=================
*/
static void PF_h2_rain_go (void)
{
	// Stub: TODO - implement weather effects in Phase 3
	Con_DPrintf("PF_h2_rain_go: stub\n");
}

/*
=================
PF_h2_particleexplosion

Material-aware particle explosion
void particleexplosion(vector org, float color, float radius, float counter) = #81
=================
*/
static void PF_h2_particleexplosion (void)
{
	// Stub: TODO - implement in Phase 3
	Con_DPrintf("PF_h2_particleexplosion: stub\n");
}

/*
=================
PF_h2_movestep

Monster move step
float movestep(float x, float y, float z, float set_trace) = #82
=================
*/
static void PF_h2_movestep (void)
{
	// Stub: TODO - implement proper monster movement
	Con_DPrintf("PF_h2_movestep: stub\n");
	G_FLOAT(OFS_RETURN) = 0;
}

/*
=================
PF_h2_advanceweaponframe

Advance weapon animation frame
void advanceweaponframe(float start, float end) = #83
=================
*/
static void PF_h2_advanceweaponframe (void)
{
	// Stub: TODO - implement weapon animation
	Con_DPrintf("PF_h2_advanceweaponframe: stub\n");
}

/*
=================
PF_h2_particle3

Extended particle variant 3
void particle3(vector org, vector box, float color, float count) = #85
=================
*/
static void PF_h2_particle3 (void)
{
	// Stub: TODO - implement in Phase 3
	Con_DPrintf("PF_h2_particle3: stub\n");
}

/*
=================
PF_h2_particle4

Extended particle variant 4
void particle4(vector org, float radius, float color, float count) = #86
=================
*/
static void PF_h2_particle4 (void)
{
	// Stub: TODO - implement in Phase 3
	Con_DPrintf("PF_h2_particle4: stub\n");
}

/*
=================
PF_h2_setpuzzlemodel

Set puzzle piece model
void setpuzzlemodel(entity e, string m) = #87
=================
*/
static void PF_h2_setpuzzlemodel (void)
{
	// Stub: TODO - implement in Phase 4
	Con_DPrintf("PF_h2_setpuzzlemodel: stub\n");
}

/*
=================
PF_h2_starteffect

Start visual effect
void starteffect() = #88
=================
*/
static void PF_h2_starteffect (void)
{
	// Stub: TODO - implement effect system in Phase 3
	Con_DPrintf("PF_h2_starteffect: stub\n");
}

/*
=================
PF_h2_endeffect

End visual effect
void endeffect(float index) = #89
=================
*/
static void PF_h2_endeffect (void)
{
	// Stub: TODO - implement effect system in Phase 3
	Con_DPrintf("PF_h2_endeffect: stub\n");
}

/*
=================
PF_h2_precache_puzzle_model

Precache puzzle piece model
void precache_puzzle_model(string m) = #90
=================
*/
static void PF_h2_precache_puzzle_model (void)
{
	// Stub: For now just precache as regular model
	// Will implement properly in Phase 4
	Con_DPrintf("PF_h2_precache_puzzle_model: stub\n");
}

/*
=================
PF_h2_concatv

Concatenate vectors (string concat of vector values)
string concatv(vector v1, vector v2, ...) = #91
=================
*/
static void PF_h2_concatv (void)
{
	// Stub: TODO - implement vector string concatenation
	Con_DPrintf("PF_h2_concatv: stub\n");
	G_INT(OFS_RETURN) = 0;
}

/*
=================
PF_h2_GetString

Get string from index
string GetString(float index) = #92
=================
*/
static void PF_h2_GetString (void)
{
	// Stub: TODO - implement string table lookup
	Con_DPrintf("PF_h2_GetString: stub\n");
	G_INT(OFS_RETURN) = 0;
}

/*
=================
PF_h2_SpawnTemp

Spawn temporary entity
entity SpawnTemp() = #93
=================
*/
static void PF_h2_SpawnTemp (void)
{
	// Stub: TODO - implement temp entity spawning
	edict_t	*ed;
	ed = ED_Alloc();
	RETURN_EDICT(ed);
}

/*
=================
PF_h2_v_factor

Vector factor calculation
float v_factor(vector v1, vector v2) = #94
=================
*/
static void PF_h2_v_factor (void)
{
	// Stub: TODO - implement vector math
	Con_DPrintf("PF_h2_v_factor: stub\n");
	G_FLOAT(OFS_RETURN) = 0;
}

/*
=================
PF_h2_v_factorrange

Vector factor range calculation
float v_factorrange(vector v1, vector v2, float min, float max) = #95
=================
*/
static void PF_h2_v_factorrange (void)
{
	// Stub: TODO - implement vector math
	Con_DPrintf("PF_h2_v_factorrange: stub\n");
	G_FLOAT(OFS_RETURN) = 0;
}

/*
=================
PF_h2_matchAngleToSlope

Match entity angle to slope
void matchAngleToSlope(entity e, vector slope) = #99
=================
*/
static void PF_h2_matchAngleToSlope (void)
{
	// Stub: TODO - implement slope angle matching
	Con_DPrintf("PF_h2_matchAngleToSlope: stub\n");
}

/*
=================
PF_h2_updateInfoPlaque

Update mission objective plaque
void updateInfoPlaque(float index, float show) = #100
=================
*/
static void PF_h2_updateInfoPlaque (void)
{
	// Stub: TODO - implement in Phase 4
	Con_DPrintf("PF_h2_updateInfoPlaque: stub\n");
}

/*
=================
PF_h2_doWhiteFlash

Screen white flash effect
void doWhiteFlash() = #104
=================
*/
static void PF_h2_doWhiteFlash (void)
{
	// Stub: TODO - implement screen flash in Phase 3
	Con_DPrintf("PF_h2_doWhiteFlash: stub\n");
}

/*
=================
PF_h2_UpdateSoundPos

Update positional sound
void UpdateSoundPos(entity e, float channel) = #105
=================
*/
static void PF_h2_UpdateSoundPos (void)
{
	// Stub: TODO - implement sound position updates
	Con_DPrintf("PF_h2_UpdateSoundPos: stub\n");
}

/*
=================
PF_h2_StopSound

Stop a playing sound
void StopSound(entity e, float channel) = #106
=================
*/
static void PF_h2_StopSound (void)
{
	// Stub: TODO - implement sound stopping
	Con_DPrintf("PF_h2_StopSound: stub\n");
}
