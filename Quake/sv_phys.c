/*
Copyright (C) 1996-2001 Id Software, Inc.
Copyright (C) 2002-2009 John Fitzgibbons and others
Copyright (C) 2010-2014 QuakeSpasm developers

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
// sv_phys.c

#include "quakedef.h"

/*


pushmove objects do not obey gravity, and do not interact with each other or trigger fields, but block normal movement and push normal objects when they move.

onground is set for toss objects when they come to a complete rest.  it is set for steping or walking objects

doors, plats, etc are SOLID_BSP, and MOVETYPE_PUSH
bonus items are SOLID_TRIGGER touch, and MOVETYPE_TOSS
corpses are SOLID_NOT and MOVETYPE_TOSS
crates are SOLID_BBOX and MOVETYPE_TOSS
walking monsters are SOLID_SLIDEBOX and MOVETYPE_STEP
flying/floating monsters are SOLID_SLIDEBOX and MOVETYPE_FLY

solid_edge items only clip against bsp models.

*/

cvar_t	sv_friction = {"sv_friction","4",CVAR_NOTIFY|CVAR_SERVERINFO};
cvar_t	sv_stopspeed = {"sv_stopspeed","100",CVAR_NONE};
cvar_t	sv_gravity = {"sv_gravity","800",CVAR_NOTIFY|CVAR_SERVERINFO};
cvar_t	sv_maxvelocity = {"sv_maxvelocity","2000",CVAR_NONE};
cvar_t	sv_nostep = {"sv_nostep","0",CVAR_NONE};
cvar_t	sv_freezenonclients = {"sv_freezenonclients","0",CVAR_NONE};


#define	MOVE_EPSILON	0.01

void SV_Physics_Toss (edict_t *ent);

/*
================
SV_CheckAllEnts
================
*/
void SV_CheckAllEnts (void)
{
	int			e;
	edict_t		*check;

// see if any solid entities are inside the final position
	check = NEXT_EDICT(qcvm->edicts);
	for (e=1 ; e<qcvm->num_edicts ; e++, check = NEXT_EDICT(check))
	{
		if (check->free)
			continue;
		if (ENT_MOVETYPE(check) == MOVETYPE_PUSH
		|| ENT_MOVETYPE(check) == MOVETYPE_NONE
		|| ENT_MOVETYPE(check) == MOVETYPE_NOCLIP)
			continue;

		if (SV_TestEntityPosition (check))
			Con_Printf ("entity in invalid position\n");
	}
}

/*
================
SV_CheckVelocity
================
*/
void SV_CheckVelocity (edict_t *ent)
{
	int		i;

//
// bound velocity
//
	for (i=0 ; i<3 ; i++)
	{
		if (IS_NAN(ENT_VELOCITY(ent)[i]))
		{
			Con_Printf ("Got a NaN velocity on %s\n", ENT_CLASSNAME(ent));
			ENT_VELOCITY(ent)[i] = 0;
		}
		if (IS_NAN(ENT_ORIGIN(ent)[i]))
		{
			Con_Printf ("Got a NaN origin on %s\n", ENT_CLASSNAME(ent));
			ENT_ORIGIN(ent)[i] = 0;
		}
		if (ENT_VELOCITY(ent)[i] > sv_maxvelocity.value)
			ENT_VELOCITY(ent)[i] = sv_maxvelocity.value;
		else if (ENT_VELOCITY(ent)[i] < -sv_maxvelocity.value)
			ENT_VELOCITY(ent)[i] = -sv_maxvelocity.value;
	}
}

/*
=============
SV_RunThink

Runs thinking code if time.  There is some play in the exact time the think
function will be called, because it is called before any movement is done
in a frame.  Not used for pushmove objects, because they must be exact.
Returns false if the entity removed itself.
=============
*/
qboolean SV_RunThink (edict_t *ent)
{
	float	thinktime;

	thinktime = ENT_NEXTTHINK(ent);
	if (thinktime <= 0 || thinktime > qcvm->time + host_frametime)
		return true;

	if (thinktime < qcvm->time)
		thinktime = qcvm->time;	// don't let things stay in the past.
								// it is possible to start that way
								// by a trigger with a local time.

	ent->oldthinktime = thinktime;
	ent->oldframe = ENT_FRAME(ent); //johnfitz

	ENT_NEXTTHINK(ent) = 0;
	pr_global_struct->time = thinktime;
	pr_global_struct->self = EDICT_TO_PROG(ent);
	pr_global_struct->other = EDICT_TO_PROG(qcvm->edicts);
	{
		func_t think_func = ENT_THINK(ent);
		if (!think_func)
		{
			int num = NUM_FOR_EDICT(ent);
			const char *classname = PR_GetString(ENT_INT(ent, classname));
			Con_Printf("SV_RunThink: entity %d (%s) has NULL think but nextthink=%f\n",
				num, classname ? classname : "unknown", thinktime);
			return true;  // Skip execution rather than crash
		}
		PR_ExecuteProgram (think_func);
	}

	return !ent->free;
}

/*
==================
SV_Impact

Two entities have touched, so run their touch functions
==================
*/
void SV_Impact (edict_t *e1, edict_t *e2)
{
	int		old_self, old_other;

	old_self = pr_global_struct->self;
	old_other = pr_global_struct->other;

	pr_global_struct->time = qcvm->time;
	if (ENT_TOUCH(e1) && ENT_SOLID(e1) != SOLID_NOT)
	{
		pr_global_struct->self = EDICT_TO_PROG(e1);
		pr_global_struct->other = EDICT_TO_PROG(e2);
		PR_ExecuteProgram (ENT_TOUCH(e1));
	}

	if (ENT_TOUCH(e2) && ENT_SOLID(e2) != SOLID_NOT)
	{
		pr_global_struct->self = EDICT_TO_PROG(e2);
		pr_global_struct->other = EDICT_TO_PROG(e1);
		PR_ExecuteProgram (ENT_TOUCH(e2));
	}

	pr_global_struct->self = old_self;
	pr_global_struct->other = old_other;
}


/*
==================
ClipVelocity

Slide off of the impacting object
returns the blocked flags (1 = floor, 2 = step / wall)
==================
*/
#define	STOP_EPSILON	0.1

int ClipVelocity (vec3_t in, vec3_t normal, vec3_t out, float overbounce)
{
	float	backoff;
	float	change;
	int		i, blocked;

	blocked = 0;
	if (normal[2] > 0)
		blocked |= 1;		// floor
	if (!normal[2])
		blocked |= 2;		// step

	backoff = DotProduct (in, normal) * overbounce;

	for (i=0 ; i<3 ; i++)
	{
		change = normal[i]*backoff;
		out[i] = in[i] - change;
		if (out[i] > -STOP_EPSILON && out[i] < STOP_EPSILON)
			out[i] = 0;
	}

	return blocked;
}


/*
============
SV_FlyMove

The basic solid body movement clip that slides along multiple planes
Returns the clipflags if the velocity was modified (hit something solid)
1 = floor
2 = wall / step
4 = dead stop
If steptrace is not NULL, the trace of any vertical wall hit will be stored
============
*/
#define	MAX_CLIP_PLANES	5
int SV_FlyMove (edict_t *ent, float time, trace_t *steptrace)
{
	int			bumpcount, numbumps;
	vec3_t		dir;
	float		d;
	int			numplanes;
	vec3_t		planes[MAX_CLIP_PLANES];
	vec3_t		primal_velocity, original_velocity, new_velocity;
	int			i, j;
	trace_t		trace;
	vec3_t		end;
	float		time_left;
	int			blocked;

	numbumps = 4;

	blocked = 0;
	VectorCopy (ENT_VELOCITY(ent), original_velocity);
	VectorCopy (ENT_VELOCITY(ent), primal_velocity);
	numplanes = 0;

	time_left = time;

	for (bumpcount=0 ; bumpcount<numbumps ; bumpcount++)
	{
		if (!ENT_VELOCITY(ent)[0] && !ENT_VELOCITY(ent)[1] && !ENT_VELOCITY(ent)[2])
			break;

		for (i=0 ; i<3 ; i++)
			end[i] = ENT_ORIGIN(ent)[i] + time_left * ENT_VELOCITY(ent)[i];

		trace = SV_Move (ENT_ORIGIN(ent), ENT_MINS(ent), ENT_MAXS(ent), end, false, ent);

		if (trace.allsolid)
		{	// entity is trapped in another solid
			VectorCopy (vec3_origin, ENT_VELOCITY(ent));
			return 3;
		}

		if (trace.fraction > 0)
		{	// actually covered some distance
			VectorCopy (trace.endpos, ENT_ORIGIN(ent));
			VectorCopy (ENT_VELOCITY(ent), original_velocity);
			numplanes = 0;
		}

		if (trace.fraction == 1)
			 break;		// moved the entire distance

		if (!trace.ent)
			Sys_Error ("SV_FlyMove: !trace.ent");

		if (trace.plane.normal[2] > 0.7)
		{
			blocked |= 1;		// floor
			if (ENT_SOLID(trace.ent) == SOLID_BSP)
			{
				ENT_FLAGS(ent) = (int)ENT_FLAGS(ent) | FL_ONGROUND;
				ENT_GROUNDENTITY(ent) = EDICT_TO_PROG(trace.ent);
			}
		}
		if (!trace.plane.normal[2])
		{
			blocked |= 2;		// step
			if (steptrace)
				*steptrace = trace;	// save for player extrafriction
		}

//
// run the impact function
//
		SV_Impact (ent, trace.ent);
		if (ent->free)
			break;		// removed by the impact function


		time_left -= time_left * trace.fraction;

	// cliped to another plane
		if (numplanes >= MAX_CLIP_PLANES)
		{	// this shouldn't really happen
			VectorCopy (vec3_origin, ENT_VELOCITY(ent));
			return 3;
		}

		VectorCopy (trace.plane.normal, planes[numplanes]);
		numplanes++;

//
// modify original_velocity so it parallels all of the clip planes
//
		for (i=0 ; i<numplanes ; i++)
		{
			ClipVelocity (original_velocity, planes[i], new_velocity, 1);
			for (j=0 ; j<numplanes ; j++)
				if (j != i)
				{
					if (DotProduct (new_velocity, planes[j]) < 0)
						break;	// not ok
				}
			if (j == numplanes)
				break;
		}

		if (i != numplanes)
		{	// go along this plane
			VectorCopy (new_velocity, ENT_VELOCITY(ent));
		}
		else
		{	// go along the crease
			if (numplanes != 2)
			{
//				Con_Printf ("clip velocity, numplanes == %i\n",numplanes);
				VectorCopy (vec3_origin, ENT_VELOCITY(ent));
				return 7;
			}
			CrossProduct (planes[0], planes[1], dir);
			d = DotProduct (dir, ENT_VELOCITY(ent));
			VectorScale (dir, d, ENT_VELOCITY(ent));
		}

//
// if original velocity is against the original velocity, stop dead
// to avoid tiny occilations in sloping corners
//
		if (DotProduct (ENT_VELOCITY(ent), primal_velocity) <= 0)
		{
			VectorCopy (vec3_origin, ENT_VELOCITY(ent));
			return blocked;
		}
	}

	return blocked;
}


/*
============
SV_AddGravity

============
*/
void SV_AddGravity (edict_t *ent)
{
	float	ent_gravity;
	eval_t	*val;

	val = GetEdictFieldValueByName(ent, "gravity");
	if (val && val->_float)
		ent_gravity = val->_float;
	else
		ent_gravity = 1.0;

	ENT_VELOCITY(ent)[2] -= ent_gravity * sv_gravity.value * host_frametime;
}


/*
===============================================================================

PUSHMOVE

===============================================================================
*/

/*
============
SV_PushEntity

Does not change the entities velocity at all
============
*/
trace_t SV_PushEntity (edict_t *ent, vec3_t push)
{
	trace_t	trace;
	vec3_t	end;

	VectorAdd (ENT_ORIGIN(ent), push, end);

	if (ENT_MOVETYPE(ent) == MOVETYPE_FLYMISSILE)
		trace = SV_Move (ENT_ORIGIN(ent), ENT_MINS(ent), ENT_MAXS(ent), end, MOVE_MISSILE, ent);
	else if (ENT_SOLID(ent) == SOLID_TRIGGER || ENT_SOLID(ent) == SOLID_NOT)
	// only clip against bmodels
		trace = SV_Move (ENT_ORIGIN(ent), ENT_MINS(ent), ENT_MAXS(ent), end, MOVE_NOMONSTERS, ent);
	else
		trace = SV_Move (ENT_ORIGIN(ent), ENT_MINS(ent), ENT_MAXS(ent), end, MOVE_NORMAL, ent);

	VectorCopy (trace.endpos, ENT_ORIGIN(ent));
	SV_LinkEdict (ent, true);

	if (trace.ent)
		SV_Impact (ent, trace.ent);

	return trace;
}


/*
============
SV_PushMove
============
*/
cvar_t sv_gameplayfix_elevators = {"sv_gameplayfix_elevators", "2", CVAR_ARCHIVE}; // 0=off; 1=clients only; 2=all entities
void SV_PushMove (edict_t *pusher, float movetime)
{
	int			i, e;
	edict_t		*check, *block;
	vec4_t		mins, maxs, move;
	vec3_t		entorig, pushorig;
	float		solid_backup;
	int			num_moved;
	edict_t		**moved_edict; //johnfitz -- dynamically allocate
	vec3_t		*moved_from; //johnfitz -- dynamically allocate
	int			mark; //johnfitz

	if (!ENT_VELOCITY(pusher)[0] && !ENT_VELOCITY(pusher)[1] && !ENT_VELOCITY(pusher)[2])
	{
		ENT_LTIME(pusher) += movetime;
		return;
	}

	for (i=0 ; i<3 ; i++)
	{
		move[i] = ENT_VELOCITY(pusher)[i] * movetime;
		mins[i] = ENT_ABSMIN(pusher)[i] + move[i];
		maxs[i] = ENT_ABSMAX(pusher)[i] + move[i];
	}

	VectorCopy (ENT_ORIGIN(pusher), pushorig);

// move the pusher to it's final position

	VectorAdd (ENT_ORIGIN(pusher), move, ENT_ORIGIN(pusher));
	ENT_LTIME(pusher) += movetime;
	SV_LinkEdict (pusher, false);

	//johnfitz -- dynamically allocate
	mark = Hunk_LowMark ();
	moved_edict = (edict_t **) Hunk_AllocNoFill (qcvm->num_edicts*sizeof(edict_t *));
	moved_from = (vec3_t *) Hunk_AllocNoFill (qcvm->num_edicts*sizeof(vec3_t));
	//johnfitz

// see if any solid entities are inside the final position
	num_moved = 0;
	check = NEXT_EDICT(qcvm->edicts);
	for (e=1 ; e<qcvm->num_edicts ; e++, check = NEXT_EDICT(check))
	{
		qboolean riding;
		int movemask;
		if (check->free)
			continue;
		movemask = 1 << (int)ENT_MOVETYPE(check);
		if (movemask & ((1<<MOVETYPE_PUSH) | (1<<MOVETYPE_NONE) | (1<<MOVETYPE_NOCLIP)))
			continue;

	// if the entity is standing on the pusher, it will definately be moved
		if ( ! ( ((int)ENT_FLAGS(check) & FL_ONGROUND)
		&& PROG_TO_EDICT(ENT_GROUNDENTITY(check)) == pusher) )
		{
#ifdef USE_SSE2
			__m128 check_absmin_vec = _mm_loadu_ps (ENT_ABSMIN(check));
			__m128 check_absmax_vec = _mm_loadu_ps (ENT_ABSMAX(check));
			__m128 maxs_vec = _mm_loadu_ps (maxs);
			__m128 mins_vec = _mm_loadu_ps (mins);
			if (_mm_movemask_ps (_mm_cmpnlt_ps (check_absmin_vec, maxs_vec)) & 7)
				continue;
			if (_mm_movemask_ps (_mm_cmpngt_ps (check_absmax_vec, mins_vec)) & 7)
				continue;
#else
			if ( ENT_ABSMIN(check)[0] >= maxs[0]
			|| ENT_ABSMIN(check)[1] >= maxs[1]
			|| ENT_ABSMIN(check)[2] >= maxs[2]
			|| ENT_ABSMAX(check)[0] <= mins[0]
			|| ENT_ABSMAX(check)[1] <= mins[1]
			|| ENT_ABSMAX(check)[2] <= mins[2] )
				continue;
#endif

		// see if the ent's bbox is inside the pusher's final position
			if (!SV_TestEntityPosition (check))
				continue;

			riding = false;
		}
		else
			riding = true;

	// remove the onground flag for non-players
		if (ENT_MOVETYPE(check) != MOVETYPE_WALK)
			ENT_FLAGS(check) = (int)ENT_FLAGS(check) & ~FL_ONGROUND;

		VectorCopy (ENT_ORIGIN(check), entorig);
		VectorCopy (ENT_ORIGIN(check), moved_from[num_moved]);
		moved_edict[num_moved] = check;
		num_moved++;

		// try moving the contacted entity
		// https://www.quake-info-pool.net/q1/qfix.htm#movetype_push
		solid_backup = ENT_SOLID(pusher);
		if (solid_backup == SOLID_BSP ||
			solid_backup == SOLID_BBOX ||
			solid_backup == SOLID_SLIDEBOX)
		{
			ENT_SOLID(pusher) = SOLID_NOT;
			SV_PushEntity (check, move);
			ENT_SOLID(pusher) = solid_backup;
		}

	// if it is still inside the pusher, block
		block = SV_TestEntityPosition (check);
		if (block)
		{	// fail the move
			if (ENT_MINS(check)[0] == ENT_MAXS(check)[0])
				continue;
			if (ENT_SOLID(check) == SOLID_NOT || ENT_SOLID(check) == SOLID_TRIGGER)
			{	// corpse
				ENT_MINS(check)[0] = ENT_MINS(check)[1] = 0;
				VectorCopy (ENT_MINS(check), ENT_MAXS(check));
				continue;
			}

			// try moving the entity up a bit if it's blocked by the pusher while also standing on it
			if (riding && block == pusher &&
				(sv_gameplayfix_elevators.value >= 2.f ||
				(sv_gameplayfix_elevators.value && e <= svs.maxclients)))
			{
				ENT_ORIGIN(check)[2] += DIST_EPSILON;
				if (!SV_TestEntityPosition (check))
				{
					// notify developer about potential issue
					if (map_checks.value || developer.value)
					{
						vec3_t check_center, pusher_center;

						VectorAdd (ENT_ABSMIN(check), ENT_ABSMAX(check), check_center);
						VectorScale (check_center, 0.5f, check_center);
						VectorAdd (ENT_ABSMIN(pusher), ENT_ABSMAX(pusher), pusher_center);
						VectorScale (pusher_center, 0.5f, pusher_center);

						Con_Warning ("sv_gameplayfix_elevators nudged %s #%d at (%.0f %.0f %.0f) above %s #%d at (%.0f %.0f %.0f)\n",
							ENT_CLASSNAME(check), NUM_FOR_EDICT (check), check_center[0], check_center[1], check_center[2],
							ENT_CLASSNAME(pusher), NUM_FOR_EDICT (pusher), pusher_center[0], pusher_center[1], pusher_center[2]
						);
					}

					// move on to next entity
					continue;
				}
			}

			VectorCopy (entorig, ENT_ORIGIN(check));
			SV_LinkEdict (check, true);

			VectorCopy (pushorig, ENT_ORIGIN(pusher));
			SV_LinkEdict (pusher, false);
			ENT_LTIME(pusher) -= movetime;

			// if the pusher has a "blocked" function, call it
			// otherwise, just stay in place until the obstacle is gone
			if (ENT_BLOCKED(pusher))
			{
				pr_global_struct->self = EDICT_TO_PROG(pusher);
				pr_global_struct->other = EDICT_TO_PROG(check);
				PR_ExecuteProgram (ENT_BLOCKED(pusher));
			}

		// move back any entities we already moved
			for (i=0 ; i<num_moved ; i++)
			{
				VectorCopy (moved_from[i], ENT_ORIGIN(moved_edict[i]));
				SV_LinkEdict (moved_edict[i], false);
			}
			Hunk_FreeToLowMark (mark); //johnfitz
			return;
		}
	}

	Hunk_FreeToLowMark (mark); //johnfitz

}

/*
================
SV_PushRotate

Hexen II: Push/rotate entities when a SOLID_BSP has angular velocity.
Handles rotating platforms, doors, etc.
Ported from uhexen2 sv_phys.c
================
*/
static void SV_PushRotate (edict_t *pusher, float movetime)
{
	int		i, e, t;
	edict_t		*check, *block;
	vec3_t		move, a, amove, mins, maxs, move2, move3, testmove;
	vec3_t		entorig, pushorig, pushorigangles;
	int		num_moved;
	int		mark;
	edict_t		**moved_edict;
	vec3_t		*moved_from;
	vec3_t		org, org2, check_center;
	vec3_t		forward, right, up;
	edict_t		*ground;
	qboolean	moveit;

	// Calculate angular and linear movement
	for (i = 0; i < 3; i++)
	{
		amove[i] = ENT_AVELOCITY(pusher)[i] * movetime;
		move[i] = ENT_VELOCITY(pusher)[i] * movetime;
		mins[i] = ENT_ABSMIN(pusher)[i] + move[i];
		maxs[i] = ENT_ABSMAX(pusher)[i] + move[i];
	}

	// Get rotation vectors (negated for transforming world->entity space)
	VectorSubtract(vec3_origin, amove, a);
	AngleVectors(a, forward, right, up);

	// Save original position
	VectorCopy(ENT_ORIGIN(pusher), pushorig);
	VectorCopy(ENT_ANGLES(pusher), pushorigangles);

	// Move the pusher to its final position
	VectorAdd(ENT_ORIGIN(pusher), move, ENT_ORIGIN(pusher));
	VectorAdd(ENT_ANGLES(pusher), amove, ENT_ANGLES(pusher));
	ENT_LTIME(pusher) += movetime;
	SV_LinkEdict(pusher, false);

	// Allocate arrays for moved entities
	mark = Hunk_LowMark();
	moved_edict = (edict_t **)Hunk_Alloc(qcvm->num_edicts * sizeof(edict_t *));
	moved_from = (vec3_t *)Hunk_Alloc(qcvm->num_edicts * sizeof(vec3_t));

	// See if any solid entities are inside the final position
	num_moved = 0;
	check = NEXT_EDICT(qcvm->edicts);
	VectorSet(testmove, 0, 0, 0);

	for (e = 1; e < qcvm->num_edicts; e++, check = NEXT_EDICT(check))
	{
		if (check->free)
			continue;
		if (ENT_MOVETYPE(check) == MOVETYPE_PUSH ||
		    ENT_MOVETYPE(check) == MOVETYPE_NONE ||
		    ENT_MOVETYPE(check) == MOVETYPE_NOCLIP)
			continue;

		// Check MOVETYPE_FOLLOW for H2 compatibility
		eval_t *val = GetEdictFieldValueByName(check, "movetype");
		if (val && (int)val->_float == 12) // MOVETYPE_FOLLOW = 12 in H2
			continue;

		// If the entity is standing on the pusher, it will definitely be moved
		moveit = false;
		ground = PROG_TO_EDICT(ENT_GROUNDENTITY(check));
		if ((int)ENT_FLAGS(check) & FL_ONGROUND)
		{
			if (ground == pusher)
				moveit = true;
		}

		if (!moveit)
		{
			// Quick bounding box rejection
			if (ENT_ABSMIN(check)[0] >= maxs[0] ||
			    ENT_ABSMIN(check)[1] >= maxs[1] ||
			    ENT_ABSMIN(check)[2] >= maxs[2] ||
			    ENT_ABSMAX(check)[0] <= mins[0] ||
			    ENT_ABSMAX(check)[1] <= mins[1] ||
			    ENT_ABSMAX(check)[2] <= mins[2])
				continue;

			// See if the ent's bbox is inside the pusher's final position
			if (!SV_TestEntityPosition(check))
				continue;
		}

		// Remove onground flag for non-players
		if (ENT_MOVETYPE(check) != MOVETYPE_WALK)
			ENT_FLAGS(check) = (int)ENT_FLAGS(check) & ~FL_ONGROUND;

		VectorCopy(ENT_ORIGIN(check), entorig);
		VectorCopy(ENT_ORIGIN(check), moved_from[num_moved]);
		moved_edict[num_moved] = check;
		num_moved++;

		// Put check in first move spot
		VectorAdd(ENT_ORIGIN(check), move, ENT_ORIGIN(check));

		// Use center of model (H2 origins are on the bottom)
		for (i = 0; i < 3; i++)
			check_center[i] = (ENT_ABSMIN(check)[i] + ENT_ABSMAX(check)[i]) / 2;

		// Calculate destination position
		VectorSubtract(check_center, ENT_ORIGIN(pusher), org);
		// Put check back
		VectorSubtract(ENT_ORIGIN(check), move, ENT_ORIGIN(check));

		org2[0] = DotProduct(org, forward);
		org2[1] = -DotProduct(org, right);
		org2[2] = DotProduct(org, up);
		VectorSubtract(org2, org, move2);

		// Add all moves together
		VectorAdd(move, move2, move3);

		// Try moving the contacted entity with multiple fallback strategies
		for (t = 0; t < 13; t++)
		{
			switch (t)
			{
			case 0: // Try x, y and z
				VectorCopy(move3, testmove);
				break;
			case 1: // Try xy only
				VectorSubtract(ENT_ORIGIN(check), testmove, ENT_ORIGIN(check));
				testmove[0] = move3[0];
				testmove[1] = move3[1];
				testmove[2] = 0;
				break;
			case 2: // Try z only
				VectorSubtract(ENT_ORIGIN(check), testmove, ENT_ORIGIN(check));
				testmove[0] = 0;
				testmove[1] = 0;
				testmove[2] = move3[2];
				break;
			case 3: // Try none
				VectorSubtract(ENT_ORIGIN(check), testmove, ENT_ORIGIN(check));
				testmove[0] = 0;
				testmove[1] = 0;
				testmove[2] = 0;
				break;
			case 4: // Try xy in opposite dir
				testmove[0] = move3[0] * -1;
				testmove[1] = move3[1] * -1;
				testmove[2] = move3[2];
				break;
			case 5: // Try z in opposite dir
				VectorSubtract(ENT_ORIGIN(check), testmove, ENT_ORIGIN(check));
				testmove[0] = move3[0];
				testmove[1] = move3[1];
				testmove[2] = move3[2] * -1;
				break;
			case 6: // Try xyz in opposite dir
				VectorSubtract(ENT_ORIGIN(check), testmove, ENT_ORIGIN(check));
				testmove[0] = move3[0] * -1;
				testmove[1] = move3[1] * -1;
				testmove[2] = move3[2] * -1;
				break;
			case 7: // Try move3 times 2
				VectorSubtract(ENT_ORIGIN(check), testmove, ENT_ORIGIN(check));
				VectorScale(move3, 2, testmove);
				break;
			case 8: // Try normalized org
				VectorSubtract(ENT_ORIGIN(check), testmove, ENT_ORIGIN(check));
				VectorScale(org, movetime, org);
				VectorCopy(org, testmove);
				break;
			case 9: // Try normalized org z * 3 only
				VectorSubtract(ENT_ORIGIN(check), testmove, ENT_ORIGIN(check));
				testmove[0] = 0;
				testmove[1] = 0;
				testmove[2] = org[2] * 3;
				break;
			case 10: // Try normalized org xy * 2 only
				VectorSubtract(ENT_ORIGIN(check), testmove, ENT_ORIGIN(check));
				testmove[0] = org[0] * 2;
				testmove[1] = org[1] * 2;
				testmove[2] = 0;
				break;
			case 11: // Try xy in opposite org dir
				VectorSubtract(ENT_ORIGIN(check), testmove, ENT_ORIGIN(check));
				testmove[0] = org[0] * -2;
				testmove[1] = org[1] * -2;
				testmove[2] = org[2];
				break;
			case 12: // Try z in opposite dir
				VectorSubtract(ENT_ORIGIN(check), testmove, ENT_ORIGIN(check));
				testmove[0] = org[0];
				testmove[1] = org[1];
				testmove[2] = org[2] * -3;
				break;
			}

			if (t != 3)
			{
				// Temporarily make pusher non-solid
				ENT_SOLID(pusher) = SOLID_NOT;
				SV_PushEntity(check, move3);
				// Rotate the entity's yaw
				ENT_ANGLES(check)[YAW] += amove[YAW];
				ENT_SOLID(pusher) = SOLID_BSP;
			}

			// If it is still inside the pusher, try next strategy
			block = SV_TestEntityPosition(check);
			if (!block)
				break;
		}

		if (block)
		{
			// Fail the move
			if (ENT_MINS(check)[0] == ENT_MAXS(check)[0])
				continue;
			if (ENT_SOLID(check) == SOLID_NOT || ENT_SOLID(check) == SOLID_TRIGGER)
			{
				// Corpse - shrink to point
				ENT_MINS(check)[0] = ENT_MINS(check)[1] = 0;
				VectorCopy(ENT_MINS(check), ENT_MAXS(check));
				continue;
			}

			VectorCopy(entorig, ENT_ORIGIN(check));
			SV_LinkEdict(check, true);

			VectorCopy(pushorig, ENT_ORIGIN(pusher));
			VectorCopy(pushorigangles, ENT_ANGLES(pusher));
			SV_LinkEdict(pusher, false);
			ENT_LTIME(pusher) -= movetime;

			// If the pusher has a "blocked" function, call it
			if (ENT_BLOCKED(pusher))
			{
				pr_global_struct->self = EDICT_TO_PROG(pusher);
				pr_global_struct->other = EDICT_TO_PROG(check);
				PR_ExecuteProgram(ENT_BLOCKED(pusher));
			}

			// Move back any entities we already moved
			for (i = 0; i < num_moved; i++)
			{
				VectorCopy(moved_from[i], ENT_ORIGIN(moved_edict[i]));
				ENT_ANGLES(moved_edict[i])[YAW] -= amove[YAW];
				SV_LinkEdict(moved_edict[i], false);
			}

			Hunk_FreeToLowMark(mark);
			return;
		}
	}

	Hunk_FreeToLowMark(mark);
}

/*
================
SV_Physics_Pusher

================
*/
void SV_Physics_Pusher (edict_t *ent)
{
	float	thinktime;
	float	oldltime;
	float	movetime;

	oldltime = ENT_LTIME(ent);

	thinktime = ENT_NEXTTHINK(ent);
	if (thinktime < ENT_LTIME(ent) + host_frametime)
	{
		movetime = thinktime - ENT_LTIME(ent);
		if (movetime < 0)
			movetime = 0;
	}
	else
		movetime = host_frametime;

	if (movetime)
	{
		// Hexen II: Check for angular velocity on SOLID_BSP entities
		if (hexen2_mode && (ENT_AVELOCITY(ent)[0] || ENT_AVELOCITY(ent)[1] || ENT_AVELOCITY(ent)[2]))
		{
			SV_PushRotate(ent, movetime);
		}
		else
		{
			SV_PushMove (ent, movetime);	// advances ENT_LTIME(ent) if not blocked
		}
	}

	if (thinktime > oldltime && thinktime <= ENT_LTIME(ent))
	{
		ENT_NEXTTHINK(ent) = 0;
		if (!ENT_THINK(ent))
		{
			Con_DPrintf("SV_Physics_Pusher: entity %d has NULL think\n", NUM_FOR_EDICT(ent));
			return;
		}
		pr_global_struct->time = qcvm->time;
		pr_global_struct->self = EDICT_TO_PROG(ent);
		pr_global_struct->other = EDICT_TO_PROG(qcvm->edicts);
		PR_ExecuteProgram (ENT_THINK(ent));
		if (ent->free)
			return;
	}

}


/*
===============================================================================

CLIENT MOVEMENT

===============================================================================
*/

/*
=============
SV_CheckStuck

This is a big hack to try and fix the rare case of getting stuck in the world
clipping hull.
=============
*/
void SV_CheckStuck (edict_t *ent)
{
	int		i, j;
	int		z;
	vec3_t	org;

	if (!SV_TestEntityPosition(ent))
	{
		VectorCopy (ENT_ORIGIN(ent), ENT_OLDORIGIN(ent));
		return;
	}

	VectorCopy (ENT_ORIGIN(ent), org);
	VectorCopy (ENT_OLDORIGIN(ent), ENT_ORIGIN(ent));
	if (!SV_TestEntityPosition(ent))
	{
		Con_DPrintf ("Unstuck.\n");
		SV_LinkEdict (ent, true);
		return;
	}

	for (z=0 ; z< 18 ; z++)
		for (i=-1 ; i <= 1 ; i++)
			for (j=-1 ; j <= 1 ; j++)
			{
				ENT_ORIGIN(ent)[0] = org[0] + i;
				ENT_ORIGIN(ent)[1] = org[1] + j;
				ENT_ORIGIN(ent)[2] = org[2] + z;
				if (!SV_TestEntityPosition(ent))
				{
					Con_DPrintf ("Unstuck.\n");
					SV_LinkEdict (ent, true);
					return;
				}
			}

	VectorCopy (org, ENT_ORIGIN(ent));
	Con_DPrintf ("player is stuck.\n");
}


/*
=============
SV_CheckWater
=============
*/
qboolean SV_CheckWater (edict_t *ent)
{
	vec3_t	point;
	int		cont;

	point[0] = ENT_ORIGIN(ent)[0];
	point[1] = ENT_ORIGIN(ent)[1];
	point[2] = ENT_ORIGIN(ent)[2] + ENT_MINS(ent)[2] + 1;

	ENT_WATERLEVEL(ent) = 0;
	ENT_WATERTYPE(ent) = CONTENTS_EMPTY;
	cont = SV_PointContents (point);
	if (cont <= CONTENTS_WATER)
	{
		ENT_WATERTYPE(ent) = cont;
		ENT_WATERLEVEL(ent) = 1;
		point[2] = ENT_ORIGIN(ent)[2] + (ENT_MINS(ent)[2] + ENT_MAXS(ent)[2])*0.5;
		cont = SV_PointContents (point);
		if (cont <= CONTENTS_WATER)
		{
			ENT_WATERLEVEL(ent) = 2;
			point[2] = ENT_ORIGIN(ent)[2] + ENT_VIEW_OFS(ent)[2];
			cont = SV_PointContents (point);
			if (cont <= CONTENTS_WATER)
				ENT_WATERLEVEL(ent) = 3;
		}
	}

	return ENT_WATERLEVEL(ent) > 1;
}

/*
============
SV_WallFriction

============
*/
void SV_WallFriction (edict_t *ent, trace_t *trace)
{
	vec3_t		forward, right, up;
	float		d, i;
	vec3_t		into, side;

	AngleVectors (ENT_V_ANGLE(ent), forward, right, up);
	d = DotProduct (trace->plane.normal, forward);

	d += 0.5;
	if (d >= 0)
		return;

// cut the tangential velocity
	i = DotProduct (trace->plane.normal, ENT_VELOCITY(ent));
	VectorScale (trace->plane.normal, i, into);
	VectorSubtract (ENT_VELOCITY(ent), into, side);

	ENT_VELOCITY(ent)[0] = side[0] * (1 + d);
	ENT_VELOCITY(ent)[1] = side[1] * (1 + d);
}

/*
============
SV_FlyExtras

Hexen II: Handle hover bobbing for flying players.
Port from uhexen2/engine/hexen2/sv_phys.c
============
*/
static const float hoverinc = 0.4f;
static void SV_FlyExtras (edict_t *ent)
{
	float hoverz;

	if (!hexen2_mode)
		return;

	if (h2_globals.fields.hoverz < 0)
		return;

	// Jumping makes you lose this flag so reset it
	ENT_FLAGS(ent) = (int)ENT_FLAGS(ent) | FL_ONGROUND;

	hoverz = E_FLOAT(ent, h2_globals.fields.hoverz);

	if ((ENT_VELOCITY(ent)[2] <= 6) && (ENT_VELOCITY(ent)[2] >= -6))
	{
		ENT_VELOCITY(ent)[2] += hoverz;

		if (ENT_VELOCITY(ent)[2] >= 6)
		{
			hoverz = -hoverinc;
			ENT_VELOCITY(ent)[2] += hoverz;
		}
		else if (ENT_VELOCITY(ent)[2] <= -6)
		{
			hoverz = hoverinc;
			ENT_VELOCITY(ent)[2] += hoverz;
		}

		E_FLOAT(ent, h2_globals.fields.hoverz) = hoverz;
	}
	else	// friction for upward or downward progress once key is released
	{
		ENT_VELOCITY(ent)[2] -= ENT_VELOCITY(ent)[2] * 0.1f;
	}
}

/*
=====================
SV_TryUnstick

Player has come to a dead stop, possibly due to the problem with limited
float precision at some angle joins in the BSP hull.

Try fixing by pushing one pixel in each direction.

This is a hack, but in the interest of good gameplay...
======================
*/
int SV_TryUnstick (edict_t *ent, vec3_t oldvel)
{
	int		i;
	vec3_t	oldorg;
	vec3_t	dir;
	int		clip;
	trace_t	steptrace;

	VectorCopy (ENT_ORIGIN(ent), oldorg);
	VectorCopy (vec3_origin, dir);

	for (i=0 ; i<8 ; i++)
	{
// try pushing a little in an axial direction
		switch (i)
		{
			case 0:	dir[0] = 2; dir[1] = 0; break;
			case 1:	dir[0] = 0; dir[1] = 2; break;
			case 2:	dir[0] = -2; dir[1] = 0; break;
			case 3:	dir[0] = 0; dir[1] = -2; break;
			case 4:	dir[0] = 2; dir[1] = 2; break;
			case 5:	dir[0] = -2; dir[1] = 2; break;
			case 6:	dir[0] = 2; dir[1] = -2; break;
			case 7:	dir[0] = -2; dir[1] = -2; break;
		}

		SV_PushEntity (ent, dir);

// retry the original move
		ENT_VELOCITY(ent)[0] = oldvel[0];
		ENT_VELOCITY(ent)[1] = oldvel[1];
		ENT_VELOCITY(ent)[2] = 0;
		clip = SV_FlyMove (ent, 0.1, &steptrace);

		if ( fabs(oldorg[1] - ENT_ORIGIN(ent)[1]) > 4
			|| fabs(oldorg[0] - ENT_ORIGIN(ent)[0]) > 4 )
		{
		//	Con_DPrintf ("unstuck!\n");
			return clip;
		}

// go back to the original pos and try again
		VectorCopy (oldorg, ENT_ORIGIN(ent));
	}

	VectorCopy (vec3_origin, ENT_VELOCITY(ent));
	return 7;		// still not moving
}

/*
=====================
SV_WalkMove

Only used by players
======================
*/
#define	STEPSIZE	18
void SV_WalkMove (edict_t *ent)
{
	vec3_t		upmove, downmove;
	vec3_t		oldorg, oldvel;
	vec3_t		nosteporg, nostepvel;
	int			clip;
	int			oldonground;
	trace_t		steptrace, downtrace;

//
// do a regular slide move unless it looks like you ran into a step
//
	oldonground = (int)ENT_FLAGS(ent) & FL_ONGROUND;
	ENT_FLAGS(ent) = (int)ENT_FLAGS(ent) & ~FL_ONGROUND;

	VectorCopy (ENT_ORIGIN(ent), oldorg);
	VectorCopy (ENT_VELOCITY(ent), oldvel);

	clip = SV_FlyMove (ent, host_frametime, &steptrace);

	if ( !(clip & 2) )
		return;		// move didn't block on a step

	if (!oldonground && ENT_WATERLEVEL(ent) == 0)
		return;		// don't stair up while jumping

	if (ENT_MOVETYPE(ent) != MOVETYPE_WALK)
		return;		// gibbed by a trigger

	if (sv_nostep.value)
		return;

	if ( (int)ENT_FLAGS(sv_player) & FL_WATERJUMP )
		return;

	VectorCopy (ENT_ORIGIN(ent), nosteporg);
	VectorCopy (ENT_VELOCITY(ent), nostepvel);

//
// try moving up and forward to go up a step
//
	VectorCopy (oldorg, ENT_ORIGIN(ent));	// back to start pos

	VectorCopy (vec3_origin, upmove);
	VectorCopy (vec3_origin, downmove);
	upmove[2] = STEPSIZE;
	downmove[2] = -STEPSIZE + oldvel[2]*host_frametime;

// move up
	SV_PushEntity (ent, upmove);	// FIXME: don't link?

// move forward
	ENT_VELOCITY(ent)[0] = oldvel[0];
	ENT_VELOCITY(ent)[1] = oldvel[1];
	ENT_VELOCITY(ent)[2] = 0;
	clip = SV_FlyMove (ent, host_frametime, &steptrace);

// check for stuckness, possibly due to the limited precision of floats
// in the clipping hulls
	if (clip)
	{
		if ( fabs(oldorg[1] - ENT_ORIGIN(ent)[1]) < 0.03125
		&& fabs(oldorg[0] - ENT_ORIGIN(ent)[0]) < 0.03125 )
		{	// stepping up didn't make any progress
			clip = SV_TryUnstick (ent, oldvel);
		}
	}

// extra friction based on view angle
	if ( clip & 2 )
		SV_WallFriction (ent, &steptrace);

// move down
	downtrace = SV_PushEntity (ent, downmove);	// FIXME: don't link?

	if (downtrace.plane.normal[2] > 0.7)
	{
		if (ENT_SOLID(ent) == SOLID_BSP)
		{
			ENT_FLAGS(ent) =	(int)ENT_FLAGS(ent) | FL_ONGROUND;
			ENT_GROUNDENTITY(ent) = EDICT_TO_PROG(downtrace.ent);
		}
	}
	else
	{
// if the push down didn't end up on good ground, use the move without
// the step up.  This happens near wall / slope combinations, and can
// cause the player to hop up higher on a slope too steep to climb
		VectorCopy (nosteporg, ENT_ORIGIN(ent));
		VectorCopy (nostepvel, ENT_VELOCITY(ent));
	}
}


/*
================
SV_Physics_Client

Player character actions
================
*/
void SV_Physics_Client (edict_t	*ent, int num)
{
	qboolean wasunderwater, forceunderwater;

	if ( ! svs.clients[num-1].active )
		return;		// unconnected slot

//
// call standard client pre-think
//
	pr_global_struct->time = qcvm->time;
	pr_global_struct->self = EDICT_TO_PROG(ent);
	if (GLOBAL_FUNC(PlayerPreThink))
		PR_ExecuteProgram (GLOBAL_FUNC(PlayerPreThink));

//
// do a move
//
	SV_CheckVelocity (ent);

//
// decide which move function to call
//
	switch ((int)ENT_MOVETYPE(ent))
	{
	case MOVETYPE_NONE:
		if (!SV_RunThink (ent))
			return;
		break;

	case MOVETYPE_WALK:
		if (!SV_RunThink (ent))
			return;
		if (!SV_CheckWater (ent) && ! ((int)ENT_FLAGS(ent) & FL_WATERJUMP) )
			SV_AddGravity (ent);
		SV_CheckStuck (ent);
		SV_WalkMove (ent);
		break;

	case MOVETYPE_TOSS:
	case MOVETYPE_BOUNCE:
	case MOVETYPE_GIB:
		SV_Physics_Toss (ent);
		break;

	case MOVETYPE_FLY:
	case MOVETYPE_SWIM:	// H2: SWIM is like FLY but stays in water
		if (!SV_RunThink (ent))
			return;
		SV_CheckWater (ent);	// H2: Check water for swimming
		SV_FlyMove (ent, host_frametime, NULL);
		SV_FlyExtras (ent);	// H2: Hover bobbing & friction
		break;

	case MOVETYPE_NOCLIP:
		if (!SV_RunThink (ent))
			return;
		VectorMA (ENT_ORIGIN(ent), host_frametime, ENT_VELOCITY(ent), ENT_ORIGIN(ent));
		break;

	default:
		Sys_Error ("SV_Physics_client: bad movetype %i", (int)ENT_MOVETYPE(ent));
	}

//
// call standard player post-think
//
	SV_LinkEdict (ent, true);

	wasunderwater = ENT_WATERLEVEL(ent) >= 3;

	pr_global_struct->time = qcvm->time;
	pr_global_struct->self = EDICT_TO_PROG(ent);
	if (GLOBAL_FUNC(PlayerPostThink))
		PR_ExecuteProgram (GLOBAL_FUNC(PlayerPostThink));

	forceunderwater = !wasunderwater && ENT_WATERLEVEL(ent) >= 3;
	if (forceunderwater != ent->forcewater)
	{
		ent->forcewater = forceunderwater;
		ent->sendforcewater = true;
	}
}

//============================================================================

/*
=============
SV_Physics_None

Non moving objects can only think
=============
*/
void SV_Physics_None (edict_t *ent)
{
// regular thinking
	SV_RunThink (ent);
}

/*
=============
SV_Physics_Noclip

A moving object that doesn't obey physics
=============
*/
void SV_Physics_Noclip (edict_t *ent)
{
// regular thinking
	if (!SV_RunThink (ent))
		return;

	VectorMA (ENT_ANGLES(ent), host_frametime, ENT_AVELOCITY(ent), ENT_ANGLES(ent));
	VectorMA (ENT_ORIGIN(ent), host_frametime, ENT_VELOCITY(ent), ENT_ORIGIN(ent));

	SV_LinkEdict (ent, false);
}

/*
=============
SV_Physics_Follow

Entities that are "stuck" to another entity (H2 MOVETYPE_FOLLOW)
The entity's origin is set to aiment's origin + v_angle offset
=============
*/
void SV_Physics_Follow (edict_t *ent)
{
	edict_t *aiment;

	// regular thinking
	if (!SV_RunThink (ent))
		return;

	// Get the entity we're following (aiment field)
	aiment = PROG_TO_EDICT(ENT_AIMENT(ent));
	if (aiment == qcvm->edicts)
	{
		// No valid aiment, just link and return
		SV_LinkEdict (ent, true);
		return;
	}

	// Set origin to aiment's origin plus our v_angle offset
	VectorAdd (ENT_ORIGIN(aiment), ENT_V_ANGLE(ent), ENT_ORIGIN(ent));
	SV_LinkEdict (ent, true);
}

/*
==============================================================================

TOSS / BOUNCE

==============================================================================
*/

/*
=============
SV_CheckWaterTransition

=============
*/
void SV_CheckWaterTransition (edict_t *ent)
{
	int		cont;

	cont = SV_PointContents (ENT_ORIGIN(ent));

	if (!ENT_WATERTYPE(ent))
	{	// just spawned here
		ENT_WATERTYPE(ent) = cont;
		ENT_WATERLEVEL(ent) = 1;
		return;
	}

	if (cont <= CONTENTS_WATER)
	{
		if (ENT_WATERTYPE(ent) == CONTENTS_EMPTY)
		{	// just crossed into water
			SV_StartSound (ent, 0, "misc/h2ohit1.wav", 255, 1);
		}
		ENT_WATERTYPE(ent) = cont;
		ENT_WATERLEVEL(ent) = 1;
	}
	else
	{
		if (ENT_WATERTYPE(ent) != CONTENTS_EMPTY)
		{	// just crossed into water
			SV_StartSound (ent, 0, "misc/h2ohit1.wav", 255, 1);
		}
		ENT_WATERTYPE(ent) = CONTENTS_EMPTY;
		ENT_WATERLEVEL(ent) = cont;
	}
}

/*
=============
SV_Physics_Toss

Toss, bounce, and fly movement.  When onground, do nothing.
=============
*/
void SV_Physics_Toss (edict_t *ent)
{
	trace_t	trace;
	vec3_t	move;
	float	backoff;

	// regular thinking
	if (!SV_RunThink (ent))
		return;

// if onground, return without moving
	if ( ((int)ENT_FLAGS(ent) & FL_ONGROUND) )
		return;

	SV_CheckVelocity (ent);

// add gravity (not for FLY, FLYMISSILE, or H2's BOUNCEMISSILE)
	if (ENT_MOVETYPE(ent) != MOVETYPE_FLY
	&& ENT_MOVETYPE(ent) != MOVETYPE_FLYMISSILE
	&& !(hexen2_mode && ENT_MOVETYPE(ent) == MOVETYPE_BOUNCEMISSILE))
		SV_AddGravity (ent);

// move angles
	VectorMA (ENT_ANGLES(ent), host_frametime, ENT_AVELOCITY(ent), ENT_ANGLES(ent));

// move origin
	VectorScale (ENT_VELOCITY(ent), host_frametime, move);
	trace = SV_PushEntity (ent, move);
	if (trace.fraction == 1)
		return;
	if (ent->free)
		return;

	// BOUNCE and H2's BOUNCEMISSILE use 1.5 backoff for bouncier reflections
	if (ENT_MOVETYPE(ent) == MOVETYPE_BOUNCE ||
	    (hexen2_mode && ENT_MOVETYPE(ent) == MOVETYPE_BOUNCEMISSILE))
		backoff = 1.5;
	else
		backoff = 1;

	ClipVelocity (ENT_VELOCITY(ent), trace.plane.normal, ENT_VELOCITY(ent), backoff);

// stop if on ground
	if (trace.plane.normal[2] > 0.7)
	{
		// BOUNCE and BOUNCEMISSILE keep bouncing if velocity is high enough
		qboolean is_bouncy = (ENT_MOVETYPE(ent) == MOVETYPE_BOUNCE ||
		                      (hexen2_mode && ENT_MOVETYPE(ent) == MOVETYPE_BOUNCEMISSILE));
		if (ENT_VELOCITY(ent)[2] < 60 || !is_bouncy)
		{
			ENT_FLAGS(ent) = (int)ENT_FLAGS(ent) | FL_ONGROUND;
			ENT_GROUNDENTITY(ent) = EDICT_TO_PROG(trace.ent);
			VectorCopy (vec3_origin, ENT_VELOCITY(ent));
			VectorCopy (vec3_origin, ENT_AVELOCITY(ent));
		}
	}

// check for in water
	SV_CheckWaterTransition (ent);
}

/*
===============================================================================

STEPPING MOVEMENT

===============================================================================
*/

/*
=============
SV_Physics_Step

Monsters freefall when they don't have a ground entity, otherwise
all movement is done with discrete steps.

This is also used for objects that have become still on the ground, but
will fall if the floor is pulled out from under them.
=============
*/
void SV_Physics_Step (edict_t *ent)
{
	qboolean	hitsound;

// freefall if not onground
	if ( ! ((int)ENT_FLAGS(ent) & (FL_ONGROUND | FL_FLY | FL_SWIM) ) )
	{
		if (ENT_VELOCITY(ent)[2] < sv_gravity.value*-0.1)
			hitsound = true;
		else
			hitsound = false;

		SV_AddGravity (ent);
		SV_CheckVelocity (ent);
		SV_FlyMove (ent, host_frametime, NULL);
		SV_LinkEdict (ent, true);

		if ( (int)ENT_FLAGS(ent) & FL_ONGROUND )	// just hit ground
		{
			if (hitsound)
				SV_StartSound (ent, 0, "demon/dland2.wav", 255, 1);
		}
	}

// regular thinking
	SV_RunThink (ent);

	SV_CheckWaterTransition (ent);
}


//============================================================================

/*
================
SV_Physics

================
*/
void SV_Physics (void)
{
	int	i;
	int	entity_cap; // For sv_freezenonclients
	edict_t	*ent;
	// Hexen II movechain support
	vec3_t	oldOrigin, oldAngle;
	edict_t	*movechain_ent;
	eval_t	*val;

// let the progs know that a new frame has started
	pr_global_struct->self = EDICT_TO_PROG(qcvm->edicts);
	pr_global_struct->other = EDICT_TO_PROG(qcvm->edicts);
	pr_global_struct->time = qcvm->time;
	{
		func_t func = GLOBAL_FUNC(StartFrame);
		if (func)
			PR_ExecuteProgram (func);
	}

//SV_CheckAllEnts ();

//
// treat each object in turn
//
	ent = qcvm->edicts;

	if (sv_freezenonclients.value)
	  entity_cap = svs.maxclients + 1; // Only run physics on clients and the world
	else
	  entity_cap = qcvm->num_edicts;

	//for (i=0 ; i<sv.num_edicts ; i++, ent = NEXT_EDICT(ent))
	for (i=0 ; i<entity_cap ; i++, ent = NEXT_EDICT(ent))
	{
		if (ent->free)
			continue;

		// Hexen II: Save old origin/angles for movechain propagation
		movechain_ent = NULL;
		if (hexen2_mode)
		{
			val = GetEdictFieldValueByName(ent, "movechain");
			if (val && val->edict != 0)
			{
				movechain_ent = PROG_TO_EDICT(val->edict);
				if (movechain_ent == qcvm->edicts)
					movechain_ent = NULL;
				else
				{
					VectorCopy(ENT_ORIGIN(ent), oldOrigin);
					VectorCopy(ENT_ANGLES(ent), oldAngle);
				}
			}
		}

		if (pr_global_struct->force_retouch)
		{
			SV_LinkEdict (ent, true);	// force retouch even for stationary
		}

		if (i > 0 && i <= svs.maxclients)
			SV_Physics_Client (ent, i);
		else if (ENT_MOVETYPE(ent) == MOVETYPE_PUSH)
			SV_Physics_Pusher (ent);
		else if (ENT_MOVETYPE(ent) == MOVETYPE_NONE)
			SV_Physics_None (ent);
		else if (ENT_MOVETYPE(ent) == MOVETYPE_NOCLIP)
			SV_Physics_Noclip (ent);
		else if (ENT_MOVETYPE(ent) == MOVETYPE_STEP
		|| ENT_MOVETYPE(ent) == MOVETYPE_PUSHPULL)  // H2: pushpull uses step physics
			SV_Physics_Step (ent);
		else if (ENT_MOVETYPE(ent) == MOVETYPE_FOLLOW)
			SV_Physics_Follow (ent);
		else if (ENT_MOVETYPE(ent) == MOVETYPE_TOSS
		|| ENT_MOVETYPE(ent) == MOVETYPE_GIB  // Q1: gib, H2: bouncemissile (same value 11)
		|| ENT_MOVETYPE(ent) == MOVETYPE_BOUNCE
		|| ENT_MOVETYPE(ent) == MOVETYPE_FLY
		|| ENT_MOVETYPE(ent) == MOVETYPE_FLYMISSILE
		|| ENT_MOVETYPE(ent) == MOVETYPE_SWIM)	// H2: swim (like fly but stays in water)
			SV_Physics_Toss (ent);
		else
			Sys_Error ("SV_Physics: bad movetype %i", (int)ENT_MOVETYPE(ent));

		// Hexen II: Propagate movement to chained entities
		if (movechain_ent != NULL)
		{
			vec3_t origin_delta, angle_delta;
			qboolean origin_moved;
			int chain_count;
			edict_t *chain_ent;

			origin_moved = !VectorCompare(ENT_ORIGIN(ent), oldOrigin);
			if (origin_moved || !VectorCompare(ENT_ANGLES(ent), oldAngle))
			{
				VectorSubtract(ENT_ORIGIN(ent), oldOrigin, origin_delta);
				VectorSubtract(ENT_ANGLES(ent), oldAngle, angle_delta);

				chain_ent = movechain_ent;
				for (chain_count = 0; chain_count < 10; chain_count++)
				{
					if (chain_ent->free)
						break;

					// Apply movement delta to chained entity
					VectorAdd(origin_delta, ENT_ORIGIN(chain_ent), ENT_ORIGIN(chain_ent));

					// Apply angle delta if FL_MOVECHAIN_ANGLE is set
					if ((int)ENT_FLAGS(chain_ent) & FL_MOVECHAIN_ANGLE)
					{
						VectorAdd(angle_delta, ENT_ANGLES(chain_ent), ENT_ANGLES(chain_ent));
					}

					// Call chainmoved callback if entity moved and callback exists
					if (origin_moved)
					{
						val = GetEdictFieldValueByName(chain_ent, "chainmoved");
						if (val && val->function)
						{
							pr_global_struct->self = EDICT_TO_PROG(chain_ent);
							pr_global_struct->other = EDICT_TO_PROG(ent);
							PR_ExecuteProgram(val->function);
						}
					}

					// Follow the chain
					val = GetEdictFieldValueByName(chain_ent, "movechain");
					if (!val || val->edict == 0)
						break;
					chain_ent = PROG_TO_EDICT(val->edict);
					if (chain_ent == qcvm->edicts)
						break;
				}
			}
		}

	//johnfitz -- PROTOCOL_FITZQUAKE
	//capture interval to nextthink here and send it to client for better
	//lerp timing, but only if interval is not 0.1 (which client assumes)
		ent->sendinterval = false;
		if (!ent->free && ENT_NEXTTHINK(ent) > qcvm->time && (ENT_MOVETYPE(ent) == MOVETYPE_STEP || ENT_MOVETYPE(ent) == MOVETYPE_WALK || ENT_FRAME(ent) != ent->oldframe))
		{
			int j = Q_rint((ENT_NEXTTHINK(ent)-ent->oldthinktime)*255);
			if (j >= 0 && j < 256 && j != 25 && j != 26) //25 and 26 are close enough to 0.1 to not send
				ent->sendinterval = true;
		}
	//johnfitz
	}

	if (pr_global_struct->force_retouch)
		pr_global_struct->force_retouch--;

	if (!sv_freezenonclients.value) 
	  qcvm->time += host_frametime;
}
