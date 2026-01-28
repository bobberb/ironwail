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
#include "host_string.h"

// Hexen II mission pack objective tracking
// These bitmasks track which objectives have been achieved
// TODO: Full implementation requires save/load support and network sync to clients
static unsigned int h2_info_mask = 0;   // Objectives 0-31
static unsigned int h2_info_mask2 = 0;  // Objectives 32-63

/*
=================
PF_h2_lightstylestatic

Static lightstyle (doesn't animate)
void lightstylestatic(float style, float value) = #5

Unlike regular lightstyle which takes a string pattern for animation,
lightstylestatic takes an integer brightness value (0-25 = 'a'-'z').
=================
*/
static const char *h2_lightstyle_defs[] = {
	"a", "b", "c", "d", "e", "f", "g",
	"h", "i", "j", "k", "l", "m", "n",
	"o", "p", "q", "r", "s", "t", "u",
	"v", "w", "x", "y", "z"
};

static void PF_h2_lightstylestatic (void)
{
	int		style, value;
	const char	*val;
	client_t	*client;
	int		j;

	style = G_FLOAT(OFS_PARM0);
	value = G_FLOAT(OFS_PARM1);

	// Clamp value to valid range (0-25 = 'a'-'z')
	if (value < 0)
		value = 0;
	else if (value > 25)
		value = 25;

	val = h2_lightstyle_defs[value];

	// Bounds check style
	if (style < 0 || style >= MAX_LIGHTSTYLES)
	{
		Con_Warning ("PF_h2_lightstylestatic: style %d out of range\n", style);
		return;
	}

	// Set the lightstyle
	sv.lightstyles[style] = val;

	// Send to clients
	if (sv.state != ss_active)
		return;

	for (j = 0, client = svs.clients; j < svs.maxclients; j++, client++)
	{
		if (client->active || client->spawned)
		{
			MSG_WriteChar (&client->message, svc_lightstyle);
			MSG_WriteChar (&client->message, style);
			MSG_WriteString (&client->message, val);
		}
	}
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

	PR_SetTraceGlobals (&trace);
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

	if (ENT_FRAME(ent) < start || ENT_FRAME(ent) > end)
	{
		// Didn't start in the range
		ENT_FRAME(ent) = start;
		result = 0;
	}
	else if (ENT_FRAME(ent) == end)
	{
		// Wrapping
		ENT_FRAME(ent) = start;
		result = 1;
	}
	else
	{
		// Regular advance
		ENT_FRAME(ent)++;
		if (ENT_FRAME(ent) == end)
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

	if (ENT_FRAME(ent) > start || ENT_FRAME(ent) < end)
	{
		// Didn't start in the range
		ENT_FRAME(ent) = start;
		result = 0;
	}
	else if (ENT_FRAME(ent) == end)
	{
		// Wrapping
		ENT_FRAME(ent) = start;
		result = 1;
	}
	else
	{
		// Regular rewind (decrement)
		ENT_FRAME(ent)--;
		if (ENT_FRAME(ent) == end)
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

	// Set playerclass via runtime offset for H2 mode compatibility
	if (h2_globals.fields.playerclass >= 0)
		((float *)&e->v)[h2_globals.fields.playerclass] = newclass;
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
	extern int d_lightstylevalue[256];

	style = G_FLOAT(OFS_PARM0);
	if (style < 0 || style >= MAX_LIGHTSTYLES)
	{
		Con_Warning("PF_h2_lightstylevalue: invalid style %d\n", style);
		G_FLOAT(OFS_RETURN) = 0;
		return;
	}

	/* Return current value scaled to 'a'-'z' range (0-11)
	 * d_lightstylevalue is 8.8 fraction of base light, 'm'=264
	 * Divide by 22 to convert to letter scale */
	G_FLOAT(OFS_RETURN) = (int)((float)d_lightstylevalue[style] / 22.0f);
}

/*
=================
PF_h2_plaque_draw

Draw story plaque - displays text from strings.txt
void plaque_draw(float to, float index) = #79

The first param is the message destination (MSG_ONE, MSG_ALL, etc.)
The second param is the string index into strings.txt (0 = clear plaque)
=================
*/
static void PF_h2_plaque_draw (void)
{
	int idx;
	sizebuf_t *dest;

	// OFS_PARM0 is the destination, read by WriteDest
	idx = (int)G_FLOAT(OFS_PARM1);

	// 0 means "clear the plaque message"
	if (idx < 0)
	{
		Con_Warning("PF_h2_plaque_draw: invalid index %d\n", idx);
		return;
	}

	dest = WriteDest();
	if (!dest)
		return;

	MSG_WriteByte(dest, svc_h2_plaque);
	MSG_WriteShort(dest, idx);
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

Monster move step - used by monster AI for movement
float movestep(float x, float y, float z, float set_trace) = #82

If set_trace is true, trace globals (trace_ent, etc.) are set from the move.
=================
*/
static void PF_h2_movestep (void)
{
	vec3_t			move;
	edict_t			*ent;
	dfunction_t		*oldf;
	int				oldself;
	qboolean		set_trace;

	ent = PROG_TO_EDICT(pr_global_struct->self);

	move[0] = G_FLOAT(OFS_PARM0);
	move[1] = G_FLOAT(OFS_PARM1);
	move[2] = G_FLOAT(OFS_PARM2);
	set_trace = (G_FLOAT(OFS_PARM3) != 0);

	/* Save program state, because SV_movestep may call other progs */
	oldf = qcvm->xfunction;
	oldself = pr_global_struct->self;

	G_FLOAT(OFS_RETURN) = SV_movestep(ent, move, true, set_trace);

	/* Restore program state */
	qcvm->xfunction = oldf;
	pr_global_struct->self = oldself;
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

Set puzzle piece model - constructs "models/puzzle/<name>.mdl" and assigns to entity
void setpuzzlemodel(entity e, string m) = #87
=================
*/
static void PF_h2_setpuzzlemodel (void)
{
	int		i;
	const char	*m, **check;
	const char	*fullpath;
	qmodel_t	*mod;
	edict_t		*e;

	e = G_EDICT(OFS_PARM0);
	m = G_STRING(OFS_PARM1);

	// Construct full model path
	fullpath = va("models/puzzle/%s.mdl", m);

	// Check if model was properly precached
	for (i = 0, check = sv.model_precache; i < MAX_MODELS && *check; i++, check++)
	{
		if (!strcmp(*check, fullpath))
			break;
	}

	if (i >= MAX_MODELS)
	{
		PR_RunError("PF_h2_setpuzzlemodel: model_precache overflow");
		return;
	}

	if (!*check)
	{
		// Model wasn't precached - add it now with a warning
		// (uhexen2 behavior: auto-precache with warning)
		Con_Warning("NO PRECACHE FOR PUZZLE PIECE: %s\n", fullpath);
		fullpath = Hunk_Strdup(fullpath, "puzzlemodel");
		sv.model_precache[i] = fullpath;
		sv.models[i] = Mod_ForName(fullpath, true);
		e->v.model = PR_SetEngineString(fullpath);
	}
	else
	{
		e->v.model = PR_SetEngineString(*check);
	}

	e->v.modelindex = i;

	// Set bounding box from model
	mod = sv.models[(int)e->v.modelindex];
	if (mod)
	{
		if (mod->type == mod_brush)
			SetMinMaxSize(e, mod->clipmins, mod->clipmaxs, true);
		else
			SetMinMaxSize(e, mod->mins, mod->maxs, true);
	}
	else
	{
		SetMinMaxSize(e, vec3_origin, vec3_origin, true);
	}
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

Precache puzzle piece model - constructs "models/puzzle/<name>.mdl" and precaches
string precache_puzzle_model(string s) = #90
=================
*/
static void PF_h2_precache_puzzle_model (void)
{
	int		i;
	const char	*s;
	const char	*fullpath;
	const char	*stored;

	if (sv.state != ss_loading)
		PR_RunError("PF_h2_precache_puzzle_model: Precache can only be done in spawn functions");

	s = G_STRING(OFS_PARM0);
	G_INT(OFS_RETURN) = G_INT(OFS_PARM0);  // Return the original string

	PR_CheckEmptyString(s);

	// Construct full model path
	fullpath = va("models/puzzle/%s.mdl", s);

	// Check if already precached or find empty slot
	for (i = 0; i < MAX_MODELS; i++)
	{
		if (!sv.model_precache[i])
		{
			// Found empty slot - add the model
			stored = Hunk_Strdup(fullpath, "puzzlemodel");
			sv.model_precache[i] = stored;
			sv.models[i] = Mod_ForName(stored, true);
			return;
		}
		if (!strcmp(sv.model_precache[i], fullpath))
		{
			// Already precached
			return;
		}
	}

	PR_RunError("PF_h2_precache_puzzle_model: overflow");
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
	const char *str;

	str = Host_GetString(idx);
	G_INT(OFS_RETURN) = PR_SetEngineString(str);
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
	vec3_t v_forward, v_right, v_up, result;

	factors = G_VECTOR(OFS_PARM0);

	/* Get direction vectors using H2 runtime offsets */
	H2_GLOBAL_VEC(h2_globals.ofs_v_forward, v_forward);
	H2_GLOBAL_VEC(h2_globals.ofs_v_right, v_right);
	H2_GLOBAL_VEC(h2_globals.ofs_v_up, v_up);

	result[0] = (v_right[0] * factors[0]) +
				(v_forward[0] * factors[1]) +
				(v_up[0] * factors[2]);

	result[1] = (v_right[1] * factors[0]) +
				(v_forward[1] * factors[1]) +
				(v_up[1] * factors[2]);

	result[2] = (v_right[2] * factors[0]) +
				(v_forward[2] * factors[1]) +
				(v_up[2] * factors[2]);

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
	vec3_t v_forward, v_right, v_up, factors, result;

	minv = G_VECTOR(OFS_PARM0);
	maxv = G_VECTOR(OFS_PARM1);

	// Pick random factors in the range
	num = (float)rand() / (float)RAND_MAX;
	factors[0] = ((maxv[0] - minv[0]) * num) + minv[0];
	num = (float)rand() / (float)RAND_MAX;
	factors[1] = ((maxv[1] - minv[1]) * num) + minv[1];
	num = (float)rand() / (float)RAND_MAX;
	factors[2] = ((maxv[2] - minv[2]) * num) + minv[2];

	/* Get direction vectors using H2 runtime offsets */
	H2_GLOBAL_VEC(h2_globals.ofs_v_forward, v_forward);
	H2_GLOBAL_VEC(h2_globals.ofs_v_right, v_right);
	H2_GLOBAL_VEC(h2_globals.ofs_v_up, v_up);

	// Apply to direction vectors
	result[0] = (v_right[0] * factors[0]) +
				(v_forward[0] * factors[1]) +
				(v_up[0] * factors[2]);

	result[1] = (v_right[1] * factors[0]) +
				(v_forward[1] * factors[1]) +
				(v_up[1] * factors[2]);

	result[2] = (v_right[2] * factors[0]) +
				(v_forward[2] * factors[1]) +
				(v_up[2] * factors[2]);

	VectorCopy(result, G_VECTOR(OFS_RETURN));
}

/*
=================
PF_h2_matchAngleToSlope

Match entity angle to slope - used for creatures walking on sloped terrain
void matchAngleToSlope(vector slope, entity who) = #99
=================
*/
static void PF_h2_matchAngleToSlope (void)
{
	edict_t	*actor;
	vec3_t	slope;
	vec3_t	old_forward, old_right, old_up;
	vec3_t	new_forward, new_right, new_up;
	vec3_t	new_angles2;
	float	forward_len;
	float	pitch, yaw;
	float	mod, dot;

	// Get slope normal from OFS_PARM0
	VectorCopy(G_VECTOR(OFS_PARM0), slope);

	// Get entity from OFS_PARM1
	actor = G_EDICT(OFS_PARM1);

	// Get old direction vectors from entity's current angles
	AngleVectors(actor->v.angles, old_forward, old_right, old_up);

	// Convert slope normal to angles (inline vectoangles)
	if (slope[1] == 0 && slope[0] == 0)
	{
		yaw = 0;
		if (slope[2] > 0)
			pitch = 90;
		else
			pitch = 270;
	}
	else
	{
		yaw = atan2(slope[1], slope[0]) * 180 / M_PI;
		if (yaw < 0)
			yaw += 360;

		forward_len = sqrt(slope[0]*slope[0] + slope[1]*slope[1]);
		pitch = atan2(slope[2], forward_len) * 180 / M_PI;
		if (pitch < 0)
			pitch += 360;
	}

	// Adjust pitch relative to flat ground (90 degrees = straight up)
	pitch = pitch - 90;

	// Get direction vector for the slope's yaw
	VectorSet(new_angles2, 0, yaw, 0);
	AngleVectors(new_angles2, new_forward, new_right, new_up);

	// Calculate how much the slope aligns with old movement direction
	mod = DotProduct(new_forward, old_right);
	if (mod < 0)
		mod = 1;
	else
		mod = -1;

	dot = DotProduct(new_forward, old_forward);

	// Apply pitch and roll to entity based on slope alignment
	actor->v.angles[0] = dot * pitch;
	actor->v.angles[2] = (1 - fabs(dot)) * pitch * mod;
}

/*
=================
PF_h2_updateInfoPlaque

Update mission objective tracking (Portal of Praevus mission pack)
void updateInfoPlaque(float index, float mode) = #100

Index is 0-63, mode is:
  1 (FORCE_ON): Force objective to be set
  2 (FORCE_OFF): Force objective to be cleared
  Otherwise: Toggle the objective state
=================
*/
static void PF_h2_updateInfoPlaque (void)
{
	unsigned int idx, mode;
	unsigned int check;
	unsigned int *use;

	idx = (unsigned int)G_FLOAT(OFS_PARM0);
	mode = (unsigned int)G_FLOAT(OFS_PARM1);

	// Index must be in range 0-63
	if (idx < 32)
	{
		use = &h2_info_mask;
		check = 1U << idx;
	}
	else if (idx < 64)
	{
		use = &h2_info_mask2;
		check = 1U << (idx - 32);
	}
	else
	{
		Con_Warning("PF_h2_updateInfoPlaque: bad objective index %u\n", idx);
		return;
	}

	// Apply mode
	if ((mode & 1 /* FORCE_ON */) && (*use & check))
		; // Already set, do nothing
	else if ((mode & 2 /* FORCE_OFF */) && !(*use & check))
		; // Already clear, do nothing
	else
		*use ^= check; // Toggle

	// TODO: Send update to clients when network sync is implemented
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
	// Send "wf" (white flash) command to all clients via stufftext
	// Used by buddha.hc::buddha_die() and other dramatic moments
	MSG_WriteByte (&sv.reliable_datagram, svc_stufftext);
	MSG_WriteString (&sv.reliable_datagram, "wf\n");
}

/*
=================
PF_h2_UpdateSoundPos

Update positional sound - sends current entity position for a playing sound
void UpdateSoundPos(entity e, float channel) = #105
=================
*/
static void PF_h2_UpdateSoundPos (void)
{
	edict_t	*entity;
	int		channel;
	int		ent;
	vec3_t	pos;

	entity = G_EDICT(OFS_PARM0);
	channel = (int)G_FLOAT(OFS_PARM1);

	if (channel < 0 || channel > 7)
	{
		Con_Warning("PF_h2_UpdateSoundPos: channel = %i\n", channel);
		return;
	}

	// Check if there's room in the datagram (1 cmd + 2 short + 12 coords = ~15 bytes)
	if (sv.datagram.cursize > MAX_DATAGRAM - 16)
		return;

	ent = NUM_FOR_EDICT(entity);

	// Compute entity center (origin + half of bounding box)
	pos[0] = entity->v.origin[0] + 0.5f * (entity->v.mins[0] + entity->v.maxs[0]);
	pos[1] = entity->v.origin[1] + 0.5f * (entity->v.mins[1] + entity->v.maxs[1]);
	pos[2] = entity->v.origin[2] + 0.5f * (entity->v.mins[2] + entity->v.maxs[2]);

	// Pack entity and channel into a short (entity in upper bits, channel in lower 3)
	MSG_WriteByte(&sv.datagram, svc_h2_sound_update_pos);
	MSG_WriteShort(&sv.datagram, (ent << 3) | channel);
	MSG_WriteCoord(&sv.datagram, pos[0], sv.protocolflags);
	MSG_WriteCoord(&sv.datagram, pos[1], sv.protocolflags);
	MSG_WriteCoord(&sv.datagram, pos[2], sv.protocolflags);
}

/*
=================
PF_h2_StopSound

Stop a playing sound on an entity/channel
void StopSound(entity e, float channel) = #106
=================
*/
static void PF_h2_StopSound (void)
{
	edict_t	*entity;
	int		channel;
	int		ent;

	entity = G_EDICT(OFS_PARM0);
	channel = (int)G_FLOAT(OFS_PARM1);

	if (channel < 0 || channel > 7)
	{
		Con_Warning("PF_h2_StopSound: channel = %i\n", channel);
		return;
	}

	// Check if there's room in the datagram (1 cmd + 2 short = 3 bytes)
	if (sv.datagram.cursize > MAX_DATAGRAM - 4)
		return;

	ent = NUM_FOR_EDICT(entity);

	// Pack entity and channel into a short (entity in upper bits, channel in lower 3)
	// Uses standard svc_stopsound which is protocol-independent
	MSG_WriteByte(&sv.datagram, svc_stopsound);
	MSG_WriteShort(&sv.datagram, (ent << 3) | channel);
}

/*
=================
PF_h2_pimpmodel

Modding extension (not native H2) by Inky
Allows dynamic customization of model effects (spin, float, glow, light, trails)

float pimpmodel(entity reference, vector glow_color) = #111

The entity's fields are used as follows:
  .model      - model path to customize
  .spawnflags - 1=spin, 2=float, 4=glow orb, 8=cast light
  .flags      - EF_* model flags to apply (trails, rotate, etc)
  .abslight   - glow alpha (0-1)
  .view_ofs   - glow offset from origin
  .health     - glow radius
  .max_health - cast light radius
  .style      - light style

Returns 1.0 on success, 0.0 on failure
=================
*/
static void PF_h2_pimpmodel (void)
{
	edict_t		*ent;
	float		*glow_color;
	const char	*modelname;
	qmodel_t	*mod;
	int			spawnflags, modelflags;
	float		abslight, glow_radius, light_radius;
	float		*view_ofs;
	client_t	*client;
	int			i;

	ent = G_EDICT(OFS_PARM0);
	glow_color = G_VECTOR(OFS_PARM1);

	// Get model name from entity
	modelname = PR_GetString(ENT_STRING_T(ent, model));
	if (!modelname || !modelname[0])
	{
		Con_DPrintf("PF_h2_pimpmodel: no model set on entity\n");
		G_FLOAT(OFS_RETURN) = 0.0f;
		return;
	}

	// Find the model
	mod = Mod_ForName(modelname, false);
	if (!mod)
	{
		Con_DPrintf("PF_h2_pimpmodel: model '%s' not found\n", modelname);
		G_FLOAT(OFS_RETURN) = 0.0f;
		return;
	}

	// Read entity fields
	spawnflags = (int)ENT_FLOAT(ent, spawnflags);
	modelflags = (int)ENT_FLOAT(ent, flags);
	abslight = ENT_FLOAT(ent, abslight);
	view_ofs = ENT_VEC(ent, view_ofs);
	glow_radius = ENT_FLOAT(ent, health);
	light_radius = ENT_FLOAT(ent, max_health);

	// Apply model flags directly to the model
	// This affects all entities using this model
	if (modelflags)
		mod->flags = modelflags;

	// For spin/float without the full EF_ROTATE, set it if requested
	if ((spawnflags & 1) || (spawnflags & 2))  // PIMP_SPIN or PIMP_FLOAT
		mod->flags |= EF_ROTATE;

	// Send pimp settings to all connected clients via stufftext
	// Format: "modelpimp <model> <spawnflags> <flags> <r> <g> <b> <abslight> <ox> <oy> <oz> <glow_r> <light_r>"
	// Clients parse this and store in their cl_modelpimp table
	for (i = 0, client = svs.clients; i < svs.maxclients; i++, client++)
	{
		if (!client->active && !client->spawned)
			continue;

		MSG_WriteByte(&client->message, svc_stufftext);
		MSG_WriteString(&client->message, va("modelpimp \"%s\" %d %d %g %g %g %g %g %g %g %g %g\n",
			modelname,
			spawnflags,
			modelflags,
			glow_color[0], glow_color[1], glow_color[2],
			abslight,
			view_ofs[0], view_ofs[1], view_ofs[2],
			glow_radius,
			light_radius));
	}

	G_FLOAT(OFS_RETURN) = 1.0f;
}
