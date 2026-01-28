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
// sv_move.c -- monster movement

#include "quakedef.h"

/* Defined in pr_cmds.c - sets trace globals from a trace result */
extern void PR_SetTraceGlobals (trace_t *trace);

#define	STEPSIZE	18

/*
=============
SV_CheckBottom

Returns false if any part of the bottom of the entity is off an edge that
is not a staircase.

=============
*/
int c_yes, c_no;

qboolean SV_CheckBottom (edict_t *ent)
{
	vec3_t	mins, maxs, start, stop;
	trace_t	trace;
	int		x, y;
	float	mid, bottom;

	VectorAdd (ENT_ORIGIN(ent), ENT_MINS(ent), mins);
	VectorAdd (ENT_ORIGIN(ent), ENT_MAXS(ent), maxs);

// if all of the points under the corners are solid world, don't bother
// with the tougher checks
// the corners must be within 16 of the midpoint
	start[2] = mins[2] - 1;
	for	(x=0 ; x<=1 ; x++)
		for	(y=0 ; y<=1 ; y++)
		{
			start[0] = x ? maxs[0] : mins[0];
			start[1] = y ? maxs[1] : mins[1];
			if (SV_PointContents (start) != CONTENTS_SOLID)
				goto realcheck;
		}

	c_yes++;
	return true;		// we got out easy

realcheck:
	c_no++;
//
// check it for real...
//
	start[2] = mins[2];

// the midpoint must be within 16 of the bottom
	start[0] = stop[0] = (mins[0] + maxs[0])*0.5;
	start[1] = stop[1] = (mins[1] + maxs[1])*0.5;
	stop[2] = start[2] - 2*STEPSIZE;
	trace = SV_Move (start, vec3_origin, vec3_origin, stop, true, ent);

	if (trace.fraction == 1.0)
		return false;
	mid = bottom = trace.endpos[2];

// the corners must be within 16 of the midpoint
	for	(x=0 ; x<=1 ; x++)
		for	(y=0 ; y<=1 ; y++)
		{
			start[0] = stop[0] = x ? maxs[0] : mins[0];
			start[1] = stop[1] = y ? maxs[1] : mins[1];

			trace = SV_Move (start, vec3_origin, vec3_origin, stop, true, ent);

			if (trace.fraction != 1.0 && trace.endpos[2] > bottom)
				bottom = trace.endpos[2];
			if (trace.fraction == 1.0 || mid - trace.endpos[2] > STEPSIZE)
				return false;
		}

	c_yes++;
	return true;
}


/*
=============
SV_movestep

Called by monster program code.
The move will be adjusted for slopes and stairs, but if the move isn't
possible, no move is done, false is returned, and
pr_global_struct->trace_normal is set to the normal of the blocking wall

set_trace: If true, sets trace globals (trace_ent, etc.) from the movement trace.
           Used by H2's walkmove() to allow monsters to detect what they touched.
=============
*/
qboolean SV_movestep (edict_t *ent, vec3_t move, qboolean relink, qboolean set_trace)
{
	float		dz;
	vec3_t		oldorg, neworg, end;
	trace_t		trace;
	int			i;
	edict_t		*enemy;

// try the move
	VectorCopy (ENT_ORIGIN(ent), oldorg);
	VectorAdd (ENT_ORIGIN(ent), move, neworg);

// flying monsters don't step up
// H2: unless FL_HUNTFACE or FL_NOZ is set
	if ( ((int)ENT_FLAGS(ent) & (FL_SWIM | FL_FLY))
	  && !((int)ENT_FLAGS(ent) & FL_HUNTFACE)
	  && !((int)ENT_FLAGS(ent) & FL_NOZ) )
	{
	// try one move with vertical motion, then one without
		for (i=0 ; i<2 ; i++)
		{
			VectorAdd (ENT_ORIGIN(ent), move, neworg);
			enemy = PROG_TO_EDICT(ENT_ENEMY(ent));
			if (i == 0 && enemy != qcvm->edicts)
			{
				dz = ENT_ORIGIN(ent)[2] - ENT_ORIGIN(PROG_TO_EDICT(ENT_ENEMY(ent)))[2];
				// H2: FL_HUNTFACE makes monster go for enemy's face
				if ((int)ENT_FLAGS(ent) & FL_HUNTFACE)
					dz += ENT_VIEW_OFS(PROG_TO_EDICT(ENT_ENEMY(ent)))[2];
				if (dz > 40)
					neworg[2] -= 8;
				if (dz < 30)
					neworg[2] += 8;
			}

			// H2: Check water exit before move for swim monsters
			if ( ((int)ENT_FLAGS(ent) & FL_SWIM) && SV_PointContents(neworg) == CONTENTS_EMPTY )
			{
				// Would end up out of water, don't do z move
				neworg[2] = ENT_ORIGIN(ent)[2];
				trace = SV_Move (ENT_ORIGIN(ent), ENT_MINS(ent), ENT_MAXS(ent), neworg, false, ent);
				if (set_trace)
					PR_SetTraceGlobals (&trace);
				if (trace.fraction < 1 || SV_PointContents(trace.endpos) == CONTENTS_EMPTY)
					return false;	// swim monster left water
			}
			else
			{
				trace = SV_Move (ENT_ORIGIN(ent), ENT_MINS(ent), ENT_MAXS(ent), neworg, false, ent);
				if (set_trace)
					PR_SetTraceGlobals (&trace);
			}

			if (trace.fraction == 1)
			{
				VectorCopy (trace.endpos, ENT_ORIGIN(ent));
				if (relink)
					SV_LinkEdict (ent, true);
				return true;
			}

			if (enemy == qcvm->edicts)
				break;
		}

		return false;
	}

// push down from a step height above the wished position
	neworg[2] += STEPSIZE;
	VectorCopy (neworg, end);
	end[2] -= STEPSIZE*2;

	trace = SV_Move (neworg, ENT_MINS(ent), ENT_MAXS(ent), end, false, ent);
	if (set_trace)
		PR_SetTraceGlobals (&trace);

	if (trace.allsolid)
		return false;

	if (trace.startsolid)
	{
		neworg[2] -= STEPSIZE;
		trace = SV_Move (neworg, ENT_MINS(ent), ENT_MAXS(ent), end, false, ent);
		if (set_trace)
			PR_SetTraceGlobals (&trace);
		if (trace.allsolid || trace.startsolid)
			return false;
	}
	if (trace.fraction == 1)
	{
	// if monster had the ground pulled out, go ahead and fall
		if ( (int)ENT_FLAGS(ent) & FL_PARTIALGROUND )
		{
			VectorAdd (ENT_ORIGIN(ent), move, ENT_ORIGIN(ent));
			if (relink)
				SV_LinkEdict (ent, true);
			ENT_FLAGS(ent) = (int)ENT_FLAGS(ent) & ~FL_ONGROUND;
		//	Con_Printf ("fall down\n");
			return true;
		}

		return false;		// walked off an edge
	}

// check point traces down for dangling corners
	VectorCopy (trace.endpos, ENT_ORIGIN(ent));

	if (!SV_CheckBottom (ent))
	{
		if ( (int)ENT_FLAGS(ent) & FL_PARTIALGROUND )
		{	// entity had floor mostly pulled out from underneath it
			// and is trying to correct
			if (relink)
				SV_LinkEdict (ent, true);
			return true;
		}
		VectorCopy (oldorg, ENT_ORIGIN(ent));
		return false;
	}

	if ( (int)ENT_FLAGS(ent) & FL_PARTIALGROUND )
	{
	//	Con_Printf ("back on ground\n");
		ENT_FLAGS(ent) = (int)ENT_FLAGS(ent) & ~FL_PARTIALGROUND;
	}
	ENT_GROUNDENTITY(ent) = EDICT_TO_PROG(trace.ent);

// the move is ok
	if (relink)
		SV_LinkEdict (ent, true);
	return true;
}


//============================================================================

/*
======================
SV_StepDirection

Turns to the movement direction, and walks the current distance if
facing it.

======================
*/
void PF_changeyaw (void);
qboolean SV_StepDirection (edict_t *ent, float yaw, float dist)
{
	vec3_t		move, oldorigin;
	float		delta;
	qboolean	set_trace;

	ENT_IDEAL_YAW(ent) = yaw;
	PF_changeyaw();

	yaw = yaw*M_PI*2 / 360;
	move[0] = cos(yaw)*dist;
	move[1] = sin(yaw)*dist;
	move[2] = 0;

	// H2: FL_SET_TRACE makes trace globals always set (used by pentacles)
	set_trace = ((int)ENT_FLAGS(ent) & FL_SET_TRACE) ? true : false;

	VectorCopy (ENT_ORIGIN(ent), oldorigin);
	if (SV_movestep (ent, move, false, set_trace))
	{
		delta = ENT_ANGLES(ent)[YAW] - ENT_IDEAL_YAW(ent);
		if (delta > 45 && delta < 315)
		{		// not turned far enough, so don't take the step
			VectorCopy (oldorigin, ENT_ORIGIN(ent));
		}
		SV_LinkEdict (ent, true);
		return true;
	}
	SV_LinkEdict (ent, true);

	return false;
}

/*
======================
SV_FixCheckBottom

======================
*/
void SV_FixCheckBottom (edict_t *ent)
{
//	Con_Printf ("SV_FixCheckBottom\n");
	ENT_FLAGS(ent) = (int)ENT_FLAGS(ent) | FL_PARTIALGROUND;
}


/*
================
SV_NewChaseDir

================
*/
#define	DI_NODIR	-1
void SV_NewChaseDir (edict_t *actor, edict_t *enemy, float dist)
{
	float		deltax,deltay;
	float			d[3];
	float		tdir, olddir, turnaround;

	olddir = anglemod( (int)(ENT_IDEAL_YAW(actor)/45)*45 );
	turnaround = anglemod(olddir - 180);

	deltax = ENT_ORIGIN(enemy)[0] - ENT_ORIGIN(actor)[0];
	deltay = ENT_ORIGIN(enemy)[1] - ENT_ORIGIN(actor)[1];
	if (deltax>10)
		d[1]= 0;
	else if (deltax<-10)
		d[1]= 180;
	else
		d[1]= DI_NODIR;
	if (deltay<-10)
		d[2]= 270;
	else if (deltay>10)
		d[2]= 90;
	else
		d[2]= DI_NODIR;

// try direct route
	if (d[1] != DI_NODIR && d[2] != DI_NODIR)
	{
		if (d[1] == 0)
			tdir = d[2] == 90 ? 45 : 315;
		else
			tdir = d[2] == 90 ? 135 : 215;

		if (tdir != turnaround && SV_StepDirection(actor, tdir, dist))
			return;
	}

// try other directions
	if ( ((rand()&3) & 1) ||  abs((int)deltay)>abs((int)deltax)) // ericw -- explicit int cast to suppress clang suggestion to use fabsf
	{
		tdir=d[1];
		d[1]=d[2];
		d[2]=tdir;
	}

	if (d[1]!=DI_NODIR && d[1]!=turnaround
	&& SV_StepDirection(actor, d[1], dist))
			return;

	if (d[2]!=DI_NODIR && d[2]!=turnaround
	&& SV_StepDirection(actor, d[2], dist))
			return;

/* there is no direct path to the player, so pick another direction */

	if (olddir!=DI_NODIR && SV_StepDirection(actor, olddir, dist))
			return;

	if (rand()&1) 	/*randomly determine direction of search*/
	{
		for (tdir=0 ; tdir<=315 ; tdir += 45)
			if (tdir!=turnaround && SV_StepDirection(actor, tdir, dist) )
					return;
	}
	else
	{
		for (tdir=315 ; tdir >=0 ; tdir -= 45)
			if (tdir!=turnaround && SV_StepDirection(actor, tdir, dist) )
					return;
	}

	if (turnaround != DI_NODIR && SV_StepDirection(actor, turnaround, dist) )
			return;

	ENT_IDEAL_YAW(actor) = olddir;		// can't move

// if a bridge was pulled out from underneath a monster, it may not have
// a valid standing position at all

	if (!SV_CheckBottom (actor))
		SV_FixCheckBottom (actor);

}

/*
======================
SV_CloseEnough

======================
*/
qboolean SV_CloseEnough (edict_t *ent, edict_t *goal, float dist)
{
	int		i;

	for (i=0 ; i<3 ; i++)
	{
		if (ENT_ABSMIN(goal)[i] > ENT_ABSMAX(ent)[i] + dist)
			return false;
		if (ENT_ABSMAX(goal)[i] < ENT_ABSMIN(ent)[i] - dist)
			return false;
	}
	return true;
}

/*
======================
SV_MoveToGoal

======================
*/
void SV_MoveToGoal (void)
{
	edict_t		*ent, *goal;
	float		dist;

	ent = PROG_TO_EDICT(pr_global_struct->self);
	goal = PROG_TO_EDICT(ENT_GOALENTITY(ent));
	dist = G_FLOAT(OFS_PARM0);

	if ( !( (int)ENT_FLAGS(ent) & (FL_ONGROUND|FL_FLY|FL_SWIM) ) )
	{
		G_FLOAT(OFS_RETURN) = 0;
		return;
	}

// if the next step hits the enemy, return immediately
	if ( PROG_TO_EDICT(ENT_ENEMY(ent)) != qcvm->edicts &&  SV_CloseEnough (ent, goal, dist) )
		return;

// bump around...
	if ( (rand()&3)==1 ||
	!SV_StepDirection (ent, ENT_IDEAL_YAW(ent), dist))
	{
		SV_NewChaseDir (ent, goal, dist);
	}
}

