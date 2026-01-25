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
#include "sv_effect_hexen2.h"

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

Extended particle with min/max bounds
void particle2(vector org, vector dmin, vector dmax, float color, float effect, float count) = #42
=================
*/
static void PF_h2_particle2 (void)
{
	float	*org, *dmin, *dmax;
	int		color, effect, count;

	org = G_VECTOR(OFS_PARM0);
	dmin = G_VECTOR(OFS_PARM1);
	dmax = G_VECTOR(OFS_PARM2);
	color = (int)G_FLOAT(OFS_PARM3);
	effect = (int)G_FLOAT(OFS_PARM4);
	count = (int)G_FLOAT(OFS_PARM5);

	if (sv.datagram.cursize > MAX_DATAGRAM - 36)
		return;

	MSG_WriteByte(&sv.datagram, svc_h2_particle2);
	MSG_WriteCoord(&sv.datagram, org[0], sv.protocolflags);
	MSG_WriteCoord(&sv.datagram, org[1], sv.protocolflags);
	MSG_WriteCoord(&sv.datagram, org[2], sv.protocolflags);
	MSG_WriteFloat(&sv.datagram, dmin[0]);
	MSG_WriteFloat(&sv.datagram, dmin[1]);
	MSG_WriteFloat(&sv.datagram, dmin[2]);
	MSG_WriteFloat(&sv.datagram, dmax[0]);
	MSG_WriteFloat(&sv.datagram, dmax[1]);
	MSG_WriteFloat(&sv.datagram, dmax[2]);
	MSG_WriteShort(&sv.datagram, color);
	MSG_WriteByte(&sv.datagram, count);
	MSG_WriteByte(&sv.datagram, effect);
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

Advance animation frame. Returns:
  0 = frame advanced normally (or started fresh)
  1 = frame wrapped back to start
  2 = frame reached end

void AdvanceFrame(float start, float end) = #63
=================
*/
static void PF_h2_AdvanceFrame (void)
{
	edict_t	*ent;
	float	start, end, result;

	ent = PROG_TO_EDICT(pr_global_struct->self);
	start = G_FLOAT(OFS_PARM0);
	end = G_FLOAT(OFS_PARM1);

	if (ent->v.frame < start || ent->v.frame > end)
	{
		// Didn't start in the range
		ent->v.frame = start;
		result = 0;
	}
	else if (ent->v.frame == end)
	{
		// Wrapping
		ent->v.frame = start;
		result = 1;
	}
	else
	{
		// Regular advance
		ent->v.frame++;
		if (ent->v.frame == end)
			result = 2;
		else
			result = 0;
	}

	G_FLOAT(OFS_RETURN) = result;
}

/*
=================
PF_h2_RewindFrame

Rewind animation frame (plays backwards from start to end).
Note: start > end for rewind. Returns:
  0 = frame rewound normally (or started fresh)
  1 = frame wrapped back to start
  2 = frame reached end

void RewindFrame(float start, float end) = #65
=================
*/
static void PF_h2_RewindFrame (void)
{
	edict_t	*ent;
	float	start, end, result;

	ent = PROG_TO_EDICT(pr_global_struct->self);
	start = G_FLOAT(OFS_PARM0);
	end = G_FLOAT(OFS_PARM1);

	if (ent->v.frame > start || ent->v.frame < end)
	{
		// Didn't start in the range
		ent->v.frame = start;
		result = 0;
	}
	else if (ent->v.frame == end)
	{
		// Wrapping
		ent->v.frame = start;
		result = 1;
	}
	else
	{
		// Regular rewind (decrement)
		ent->v.frame--;
		if (ent->v.frame == end)
			result = 2;
		else
			result = 0;
	}

	G_FLOAT(OFS_RETURN) = result;
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

Start immediate rain effect (non-persistent, one-shot)
void rain_go(vector min_org, vector max_org, vector e_size, vector dir, float color, float count) = #80

Note: This is different from starteffect(CE_RAIN, ...) which creates a persistent effect.
This sends svc_h2_raineffect directly to clients.
=================
*/
static void PF_h2_rain_go (void)
{
	float	*min_org, *max_org, *e_size, *dir;
	vec3_t	org;
	int		color, count, x_dir, y_dir;

	min_org = G_VECTOR(OFS_PARM0);
	max_org = G_VECTOR(OFS_PARM1);
	e_size = G_VECTOR(OFS_PARM2);
	dir = G_VECTOR(OFS_PARM3);
	color = (int)G_FLOAT(OFS_PARM4);
	count = (int)G_FLOAT(OFS_PARM5);

	org[0] = min_org[0];
	org[1] = min_org[1];
	org[2] = max_org[2];

	x_dir = (int)dir[0];
	y_dir = (int)dir[1];

	if (sv.datagram.cursize > MAX_DATAGRAM - 20)
		return;

	MSG_WriteByte(&sv.datagram, svc_h2_raineffect);
	MSG_WriteCoord(&sv.datagram, org[0], sv.protocolflags);
	MSG_WriteCoord(&sv.datagram, org[1], sv.protocolflags);
	MSG_WriteCoord(&sv.datagram, org[2], sv.protocolflags);
	MSG_WriteCoord(&sv.datagram, e_size[0], sv.protocolflags);
	MSG_WriteCoord(&sv.datagram, e_size[1], sv.protocolflags);
	MSG_WriteCoord(&sv.datagram, e_size[2], sv.protocolflags);
	MSG_WriteAngle(&sv.datagram, x_dir, sv.protocolflags);
	MSG_WriteAngle(&sv.datagram, y_dir, sv.protocolflags);
	MSG_WriteShort(&sv.datagram, color);
	MSG_WriteShort(&sv.datagram, count);
}

/*
=================
PF_h2_particleexplosion

Material-aware particle explosion
void particleexplosion(vector org, float color, float radius, float counter) = #81

Sends svc_h2_particle_explosion to clients.
=================
*/
static void PF_h2_particleexplosion (void)
{
	float	*org;
	int		color, radius, counter;

	org = G_VECTOR(OFS_PARM0);
	color = (int)G_FLOAT(OFS_PARM1);
	radius = (int)G_FLOAT(OFS_PARM2);
	counter = (int)G_FLOAT(OFS_PARM3);

	if (sv.datagram.cursize > MAX_DATAGRAM - 12)
		return;

	MSG_WriteByte(&sv.datagram, svc_h2_particle_explosion);
	MSG_WriteCoord(&sv.datagram, org[0], sv.protocolflags);
	MSG_WriteCoord(&sv.datagram, org[1], sv.protocolflags);
	MSG_WriteCoord(&sv.datagram, org[2], sv.protocolflags);
	MSG_WriteShort(&sv.datagram, color);
	MSG_WriteShort(&sv.datagram, radius);
	MSG_WriteShort(&sv.datagram, counter);
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

Advance weapon animation frame. Handles bidirectional animation.
Returns:
  0 = WF_NORMAL_ADVANCE - frame advanced normally
  1 = WF_CYCLE_STARTED - animation cycle started fresh
  2 = WF_CYCLE_WRAPPED - animation wrapped back to start
  3 = WF_LAST_FRAME - reached the end frame

void advanceweaponframe(float start, float end) = #83
=================
*/
#define WF_NORMAL_ADVANCE	0
#define WF_CYCLE_STARTED	1
#define WF_CYCLE_WRAPPED	2
#define WF_LAST_FRAME		3

static void PF_h2_advanceweaponframe (void)
{
	edict_t	*ent;
	float	startframe, endframe;
	float	state;

	ent = PROG_TO_EDICT(pr_global_struct->self);
	startframe = G_FLOAT(OFS_PARM0);
	endframe = G_FLOAT(OFS_PARM1);

	if ((endframe > startframe && (ent->v.weaponframe > endframe || ent->v.weaponframe < startframe)) ||
	    (endframe < startframe && (ent->v.weaponframe < endframe || ent->v.weaponframe > startframe)))
	{
		// Start fresh
		ent->v.weaponframe = startframe;
		state = WF_CYCLE_STARTED;
	}
	else if (ent->v.weaponframe == endframe)
	{
		// Wrap back to start
		ent->v.weaponframe = startframe;
		state = WF_CYCLE_WRAPPED;
	}
	else
	{
		// Advance in the appropriate direction
		if (startframe > endframe)
			ent->v.weaponframe = ent->v.weaponframe - 1;
		else if (startframe < endframe)
			ent->v.weaponframe = ent->v.weaponframe + 1;

		if (ent->v.weaponframe == endframe)
			state = WF_LAST_FRAME;
		else
			state = WF_NORMAL_ADVANCE;
	}

	G_FLOAT(OFS_RETURN) = state;
}

/*
=================
PF_h2_particle3

Extended particle with box bounds
void particle3(vector org, vector box, float color, float effect, float count) = #85
=================
*/
static void PF_h2_particle3 (void)
{
	float	*org, *box;
	int		color, effect, count;

	org = G_VECTOR(OFS_PARM0);
	box = G_VECTOR(OFS_PARM1);
	color = (int)G_FLOAT(OFS_PARM2);
	effect = (int)G_FLOAT(OFS_PARM3);
	count = (int)G_FLOAT(OFS_PARM4);

	if (sv.datagram.cursize > MAX_DATAGRAM - 15)
		return;

	MSG_WriteByte(&sv.datagram, svc_h2_particle3);
	MSG_WriteCoord(&sv.datagram, org[0], sv.protocolflags);
	MSG_WriteCoord(&sv.datagram, org[1], sv.protocolflags);
	MSG_WriteCoord(&sv.datagram, org[2], sv.protocolflags);
	MSG_WriteByte(&sv.datagram, (int)box[0]);
	MSG_WriteByte(&sv.datagram, (int)box[1]);
	MSG_WriteByte(&sv.datagram, (int)box[2]);
	MSG_WriteShort(&sv.datagram, color);
	MSG_WriteByte(&sv.datagram, count);
	MSG_WriteByte(&sv.datagram, effect);
}

/*
=================
PF_h2_particle4

Extended particle with radius
void particle4(vector org, float radius, float color, float effect, float count) = #86
=================
*/
static void PF_h2_particle4 (void)
{
	float	*org;
	int		radius, color, effect, count;

	org = G_VECTOR(OFS_PARM0);
	radius = (int)G_FLOAT(OFS_PARM1);
	color = (int)G_FLOAT(OFS_PARM2);
	effect = (int)G_FLOAT(OFS_PARM3);
	count = (int)G_FLOAT(OFS_PARM4);

	if (sv.datagram.cursize > MAX_DATAGRAM - 13)
		return;

	MSG_WriteByte(&sv.datagram, svc_h2_particle4);
	MSG_WriteCoord(&sv.datagram, org[0], sv.protocolflags);
	MSG_WriteCoord(&sv.datagram, org[1], sv.protocolflags);
	MSG_WriteCoord(&sv.datagram, org[2], sv.protocolflags);
	MSG_WriteByte(&sv.datagram, radius);
	MSG_WriteShort(&sv.datagram, color);
	MSG_WriteByte(&sv.datagram, count);
	MSG_WriteByte(&sv.datagram, effect);
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

Start visual effect. Effect type and parameters are read from QC parameters.
This is a complex function that handles 60+ effect types with varying
numbers of parameters (varargs).

float starteffect(float type, ...) = #88

Returns the effect index for persistent effects, -1 on error.
=================
*/
static void PF_h2_starteffect (void)
{
	int result;

	result = SV_ParseEffect();
	G_FLOAT(OFS_RETURN) = (float)result;
}

/*
=================
PF_h2_endeffect

End visual effect by index
void endeffect(float index1, float index2) = #89

Note: uhexen2 reads both parm0 and parm1 but only uses parm1.
The first parameter appears to be unused in the original implementation.
=================
*/
static void PF_h2_endeffect (void)
{
	int idx;

	/* uhexen2 reads both but only uses parm1 */
	idx = (int)G_FLOAT(OFS_PARM1);

	SV_EndEffect(idx);
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

Clamp input vector components to range
vector concatv(vector in, vector range) = #91

Despite the name, this doesn't concatenate strings.
It clamps each component of 'in' to [-range, +range].
=================
*/
static void PF_h2_concatv (void)
{
	float *in, *range;
	vec3_t result;

	in = G_VECTOR(OFS_PARM0);
	range = G_VECTOR(OFS_PARM1);

	VectorCopy(in, result);

	if (result[0] < -range[0])
		result[0] = -range[0];
	if (result[0] > range[0])
		result[0] = range[0];
	if (result[1] < -range[1])
		result[1] = -range[1];
	if (result[1] > range[1])
		result[1] = range[1];
	if (result[2] < -range[2])
		result[2] = -range[2];
	if (result[2] > range[2])
		result[2] = range[2];

	VectorCopy(result, G_VECTOR(OFS_RETURN));
}

/*
=================
PF_h2_GetString

Get localized string from index
string GetString(float index) = #92

Returns the localized string from strings.txt by index.
Note: This requires the host string system (strings.txt loading)
which provides internationalized game text.
=================
*/
static void PF_h2_GetString (void)
{
	int idx = (int)G_FLOAT(OFS_PARM0);

	// Note: Full implementation requires loading strings.txt
	// For now, return empty string to avoid crashes
	// The game will still work but text will be missing
	Con_DPrintf("PF_h2_GetString: index %d (strings.txt not loaded)\n", idx);
	G_INT(OFS_RETURN) = PR_SetEngineString("");
}

/*
=================
PF_h2_SpawnTemp

Spawn temporary entity
entity SpawnTemp() = #93

In H2, temporary entities are managed in a separate pool for
faster allocation/deallocation. Here we use regular ED_Alloc()
which works identically, just without the optimization.
=================
*/
static void PF_h2_SpawnTemp (void)
{
	edict_t	*ed;

	ed = ED_Alloc();

	RETURN_EDICT(ed);
}

/*
=================
PF_h2_v_factor

Calculate vector from factors applied to direction vectors
vector v_factor(vector factors) = #94

Returns: (v_right * factors.x) + (v_forward * factors.y) + (v_up * factors.z)
=================
*/
static void PF_h2_v_factor (void)
{
	float *factors;
	vec3_t result;

	factors = G_VECTOR(OFS_PARM0);

	result[0] = (pr_global_struct->v_right[0] * factors[0]) +
				(pr_global_struct->v_forward[0] * factors[1]) +
				(pr_global_struct->v_up[0] * factors[2]);

	result[1] = (pr_global_struct->v_right[1] * factors[0]) +
				(pr_global_struct->v_forward[1] * factors[1]) +
				(pr_global_struct->v_up[1] * factors[2]);

	result[2] = (pr_global_struct->v_right[2] * factors[0]) +
				(pr_global_struct->v_forward[2] * factors[1]) +
				(pr_global_struct->v_up[2] * factors[2]);

	VectorCopy(result, G_VECTOR(OFS_RETURN));
}

/*
=================
PF_h2_v_factorrange

Calculate vector from random factors in range applied to direction vectors
vector v_factorrange(vector minv, vector maxv) = #95

Picks random factors between minv and maxv, then:
Returns: (v_right * factor.x) + (v_forward * factor.y) + (v_up * factor.z)
=================
*/
static void PF_h2_v_factorrange (void)
{
	float *minv, *maxv;
	float num;
	vec3_t factors, result;

	minv = G_VECTOR(OFS_PARM0);
	maxv = G_VECTOR(OFS_PARM1);

	// Pick random factors in the range
	num = (float)rand() / (float)RAND_MAX;
	factors[0] = ((maxv[0] - minv[0]) * num) + minv[0];
	num = (float)rand() / (float)RAND_MAX;
	factors[1] = ((maxv[1] - minv[1]) * num) + minv[1];
	num = (float)rand() / (float)RAND_MAX;
	factors[2] = ((maxv[2] - minv[2]) * num) + minv[2];

	// Apply to direction vectors
	result[0] = (pr_global_struct->v_right[0] * factors[0]) +
				(pr_global_struct->v_forward[0] * factors[1]) +
				(pr_global_struct->v_up[0] * factors[2]);

	result[1] = (pr_global_struct->v_right[1] * factors[0]) +
				(pr_global_struct->v_forward[1] * factors[1]) +
				(pr_global_struct->v_up[1] * factors[2]);

	result[2] = (pr_global_struct->v_right[2] * factors[0]) +
				(pr_global_struct->v_forward[2] * factors[1]) +
				(pr_global_struct->v_up[2] * factors[2]);

	VectorCopy(result, G_VECTOR(OFS_RETURN));
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
