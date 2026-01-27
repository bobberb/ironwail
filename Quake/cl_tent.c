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
// cl_tent.c -- client side temporary entities

#include "quakedef.h"
#include "protocol_hexen2.h"

// Hexen II stream temp entity types
#define TE_H2_STREAM_LIGHTNING_SMALL	24
#define TE_H2_STREAM_CHAIN				25
#define TE_H2_STREAM_SUNSTAFF1			26
#define TE_H2_STREAM_SUNSTAFF2			27
#define TE_H2_STREAM_LIGHTNING			28
#define TE_H2_STREAM_COLORBEAM			29
#define TE_H2_STREAM_ICECHUNKS			30
#define TE_H2_STREAM_GAZE				31
#define TE_H2_STREAM_FAMINE				32

// Stream flags
#define STREAM_ATTACHED			16
#define STREAM_TRANSLUCENT		32

#define MAX_STREAMS				32
#define MAX_STREAM_ENTITIES		128

typedef struct
{
	int		type;
	int		entity;
	int		tag;
	int		flags;
	int		skin;
	qmodel_t *models[4];
	vec3_t	source;
	vec3_t	dest;
	vec3_t	offset;
	float	endTime;
	float	lastTrailTime;
} stream_t;

static stream_t cl_Streams[MAX_STREAMS];
static entity_t StreamEntities[MAX_STREAM_ENTITIES];
static int StreamEntityCount;

int			num_temp_entities;
entity_t	cl_temp_entities[MAX_TEMP_ENTITIES];
beam_t		cl_beams[MAX_BEAMS];

sfx_t			*cl_sfx_wizhit;
sfx_t			*cl_sfx_knighthit;
sfx_t			*cl_sfx_tink1;
sfx_t			*cl_sfx_ric1;
sfx_t			*cl_sfx_ric2;
sfx_t			*cl_sfx_ric3;
sfx_t			*cl_sfx_r_exp3;

/*
=================
CL_ParseTEnt
=================
*/
void CL_InitTEnts (void)
{
	// Q1-specific monster impact sounds - not present in H2
	if (!hexen2_mode)
	{
		cl_sfx_wizhit = S_PrecacheSound ("wizard/hit.wav");
		cl_sfx_knighthit = S_PrecacheSound ("hknight/hit.wav");
	}
	cl_sfx_tink1 = S_PrecacheSound ("weapons/tink1.wav");
	cl_sfx_ric1 = S_PrecacheSound ("weapons/ric1.wav");
	cl_sfx_ric2 = S_PrecacheSound ("weapons/ric2.wav");
	cl_sfx_ric3 = S_PrecacheSound ("weapons/ric3.wav");
	cl_sfx_r_exp3 = S_PrecacheSound ("weapons/r_exp3.wav");
}

/*
=================
CL_ClearTEnts

Clear all temporary entities including H2 streams
=================
*/
void CL_ClearTEnts (void)
{
	memset(cl_beams, 0, sizeof(cl_beams));
	memset(cl_Streams, 0, sizeof(cl_Streams));
	num_temp_entities = 0;
	StreamEntityCount = 0;
}

/*
=================
CL_ParseBeam
=================
*/
void CL_ParseBeam (qmodel_t *m)
{
	int		ent;
	vec3_t	start, end;
	beam_t	*b;
	int		i;

	ent = MSG_ReadShort ();

	start[0] = MSG_ReadCoord (cl.protocolflags);
	start[1] = MSG_ReadCoord (cl.protocolflags);
	start[2] = MSG_ReadCoord (cl.protocolflags);

	end[0] = MSG_ReadCoord (cl.protocolflags);
	end[1] = MSG_ReadCoord (cl.protocolflags);
	end[2] = MSG_ReadCoord (cl.protocolflags);

// override any beam with the same entity
	for (i=0, b=cl_beams ; i< MAX_BEAMS ; i++, b++)
		if (b->entity == ent)
		{
			b->entity = ent;
			b->model = m;
			b->starttime = cl.time - 0.001;
			b->endtime = cl.time + 0.2;
			VectorCopy (start, b->start);
			VectorCopy (end, b->end);
			return;
		}

// find a free beam
	for (i=0, b=cl_beams ; i< MAX_BEAMS ; i++, b++)
	{
		if (!b->model || b->starttime > cl.time || b->endtime < cl.time)
		{
			b->entity = ent;
			b->model = m;
			b->starttime = cl.time - 0.001;
			b->endtime = cl.time + 0.2;
			VectorCopy (start, b->start);
			VectorCopy (end, b->end);
			return;
		}
	}

	//johnfitz -- less spammy overflow message
	if (!dev_overflows.beams || dev_overflows.beams + CONSOLE_RESPAM_TIME < realtime )
	{
		Con_Printf ("Beam list overflow!\n");
		dev_overflows.beams = realtime;
	}
	//johnfitz
}

/*
=================
NewStream

Find or allocate a stream slot (H2)
=================
*/
static stream_t *NewStream(int ent, int tag)
{
	stream_t	*stream;
	int			i;

	// Search for a stream with matching entity and tag
	for (i = 0, stream = cl_Streams; i < MAX_STREAMS; i++, stream++)
	{
		if (stream->entity == ent && stream->tag == tag)
			return stream;
	}
	// Search for a free stream
	for (i = 0, stream = cl_Streams; i < MAX_STREAMS; i++, stream++)
	{
		if (!stream->models[0] || stream->endTime < cl.time)
			return stream;
	}
	return NULL;
}

/*
=================
ParseStream

Parse a stream temp entity message (H2)
=================
*/
static void ParseStream(int type)
{
	int			ent, tag, flags, skin;
	vec3_t		source, dest;
	float		duration;
	stream_t	*stream;
	qmodel_t	*models[4];

	ent = MSG_ReadShort();
	flags = MSG_ReadByte();
	tag = flags & 15;
	duration = (float)MSG_ReadByte() * 0.05f;
	skin = 0;
	if (type == TE_H2_STREAM_COLORBEAM)
		skin = MSG_ReadByte();
	source[0] = MSG_ReadCoord(cl.protocolflags);
	source[1] = MSG_ReadCoord(cl.protocolflags);
	source[2] = MSG_ReadCoord(cl.protocolflags);
	dest[0] = MSG_ReadCoord(cl.protocolflags);
	dest[1] = MSG_ReadCoord(cl.protocolflags);
	dest[2] = MSG_ReadCoord(cl.protocolflags);

	models[1] = models[2] = models[3] = NULL;
	switch (type)
	{
	case TE_H2_STREAM_CHAIN:
		models[0] = Mod_ForName("models/stchain.mdl", true);
		break;
	case TE_H2_STREAM_SUNSTAFF1:
		models[0] = Mod_ForName("models/stsunsf1.mdl", true);
		models[1] = Mod_ForName("models/stsunsf2.mdl", true);
		models[2] = Mod_ForName("models/stsunsf3.mdl", true);
		models[3] = Mod_ForName("models/stsunsf4.mdl", true);
		break;
	case TE_H2_STREAM_SUNSTAFF2:
		models[0] = Mod_ForName("models/stsunsf5.mdl", true);
		models[2] = Mod_ForName("models/stsunsf3.mdl", true);
		models[3] = Mod_ForName("models/stsunsf4.mdl", true);
		break;
	case TE_H2_STREAM_LIGHTNING:
		models[0] = Mod_ForName("models/stlghtng.mdl", true);
		break;
	case TE_H2_STREAM_LIGHTNING_SMALL:
		models[0] = Mod_ForName("models/stltng2.mdl", true);
		break;
	case TE_H2_STREAM_FAMINE:
		models[0] = Mod_ForName("models/fambeam.mdl", true);
		break;
	case TE_H2_STREAM_COLORBEAM:
		models[0] = Mod_ForName("models/stclrbm.mdl", true);
		break;
	case TE_H2_STREAM_ICECHUNKS:
		models[0] = Mod_ForName("models/stice.mdl", true);
		break;
	case TE_H2_STREAM_GAZE:
		models[0] = Mod_ForName("models/stmedgaz.mdl", true);
		break;
	default:
		models[0] = NULL;
		break;
	}

	if (models[0] == NULL)
	{
		Con_DPrintf("ParseStream: unknown stream type %d\n", type);
		return;
	}

	stream = NewStream(ent, tag);
	if (stream == NULL)
	{
		Con_DPrintf("Stream list overflow\n");
		return;
	}

	stream->type = type;
	stream->tag = tag;
	stream->flags = flags;
	stream->entity = ent;
	stream->skin = skin;
	stream->models[0] = models[0];
	stream->models[1] = models[1];
	stream->models[2] = models[2];
	stream->models[3] = models[3];
	stream->endTime = cl.time + duration;
	stream->lastTrailTime = 0;
	VectorCopy(source, stream->source);
	VectorCopy(dest, stream->dest);
	if (flags & STREAM_ATTACHED)
	{
		VectorSubtract(source, cl_entities[ent].origin, stream->offset);
	}
}

/*
=================
CL_ParseTEnt
=================
*/
void CL_ParseTEnt (void)
{
	int		type;
	vec3_t	pos;
	dlight_t	*dl;
	int		rnd;
	int		colorStart, colorLength;

	type = MSG_ReadByte ();
	switch (type)
	{
	case TE_WIZSPIKE:			// spike hitting wall
		pos[0] = MSG_ReadCoord (cl.protocolflags);
		pos[1] = MSG_ReadCoord (cl.protocolflags);
		pos[2] = MSG_ReadCoord (cl.protocolflags);
		R_RunParticleEffect (pos, vec3_origin, 20, 30);
		S_StartSound (-1, 0, cl_sfx_wizhit, pos, 1, 1);
		break;

	case TE_KNIGHTSPIKE:			// spike hitting wall
		pos[0] = MSG_ReadCoord (cl.protocolflags);
		pos[1] = MSG_ReadCoord (cl.protocolflags);
		pos[2] = MSG_ReadCoord (cl.protocolflags);
		R_RunParticleEffect (pos, vec3_origin, 226, 20);
		S_StartSound (-1, 0, cl_sfx_knighthit, pos, 1, 1);
		break;

	case TE_SPIKE:			// spike hitting wall
		pos[0] = MSG_ReadCoord (cl.protocolflags);
		pos[1] = MSG_ReadCoord (cl.protocolflags);
		pos[2] = MSG_ReadCoord (cl.protocolflags);
		R_RunParticleEffect (pos, vec3_origin, 0, 10);
		if ( rand() % 5 )
			S_StartSound (-1, 0, cl_sfx_tink1, pos, 1, 1);
		else
		{
			rnd = rand() & 3;
			if (rnd == 1)
				S_StartSound (-1, 0, cl_sfx_ric1, pos, 1, 1);
			else if (rnd == 2)
				S_StartSound (-1, 0, cl_sfx_ric2, pos, 1, 1);
			else
				S_StartSound (-1, 0, cl_sfx_ric3, pos, 1, 1);
		}
		break;
	case TE_SUPERSPIKE:			// super spike hitting wall
		pos[0] = MSG_ReadCoord (cl.protocolflags);
		pos[1] = MSG_ReadCoord (cl.protocolflags);
		pos[2] = MSG_ReadCoord (cl.protocolflags);
		R_RunParticleEffect (pos, vec3_origin, 0, 20);

		if ( rand() % 5 )
			S_StartSound (-1, 0, cl_sfx_tink1, pos, 1, 1);
		else
		{
			rnd = rand() & 3;
			if (rnd == 1)
				S_StartSound (-1, 0, cl_sfx_ric1, pos, 1, 1);
			else if (rnd == 2)
				S_StartSound (-1, 0, cl_sfx_ric2, pos, 1, 1);
			else
				S_StartSound (-1, 0, cl_sfx_ric3, pos, 1, 1);
		}
		break;

	case TE_GUNSHOT:			// bullet hitting wall
		pos[0] = MSG_ReadCoord (cl.protocolflags);
		pos[1] = MSG_ReadCoord (cl.protocolflags);
		pos[2] = MSG_ReadCoord (cl.protocolflags);
		R_RunParticleEffect (pos, vec3_origin, 0, 20);
		break;

	case TE_EXPLOSION:			// rocket explosion
		pos[0] = MSG_ReadCoord (cl.protocolflags);
		pos[1] = MSG_ReadCoord (cl.protocolflags);
		pos[2] = MSG_ReadCoord (cl.protocolflags);
		R_ParticleExplosion (pos);
		dl = CL_AllocDlight (0);
		VectorCopy (pos, dl->origin);
		dl->radius = 350;
		dl->die = cl.time + 0.5;
		dl->decay = 300;
		S_StartSound (-1, 0, cl_sfx_r_exp3, pos, 1, 1);
		break;

	case TE_TAREXPLOSION:			// tarbaby explosion
		pos[0] = MSG_ReadCoord (cl.protocolflags);
		pos[1] = MSG_ReadCoord (cl.protocolflags);
		pos[2] = MSG_ReadCoord (cl.protocolflags);
		R_BlobExplosion (pos);

		S_StartSound (-1, 0, cl_sfx_r_exp3, pos, 1, 1);
		break;

	case TE_LIGHTNING1:				// lightning bolts
		CL_ParseBeam (Mod_ForName("progs/bolt.mdl", true));
		break;

	case TE_LIGHTNING2:				// lightning bolts
		CL_ParseBeam (Mod_ForName("progs/bolt2.mdl", true));
		break;

	case TE_LIGHTNING3:				// lightning bolts
		CL_ParseBeam (Mod_ForName("progs/bolt3.mdl", true));
		break;

// PGM 01/21/97
	case TE_BEAM:				// grappling hook beam
		CL_ParseBeam (Mod_ForName("progs/beam.mdl", true));
		break;
// PGM 01/21/97

	case TE_LAVASPLASH:
		pos[0] = MSG_ReadCoord (cl.protocolflags);
		pos[1] = MSG_ReadCoord (cl.protocolflags);
		pos[2] = MSG_ReadCoord (cl.protocolflags);
		R_LavaSplash (pos);
		break;

	case TE_TELEPORT:
		pos[0] = MSG_ReadCoord (cl.protocolflags);
		pos[1] = MSG_ReadCoord (cl.protocolflags);
		pos[2] = MSG_ReadCoord (cl.protocolflags);
		R_TeleportSplash (pos);
		break;

	case TE_EXPLOSION2:				// color mapped explosion
		pos[0] = MSG_ReadCoord (cl.protocolflags);
		pos[1] = MSG_ReadCoord (cl.protocolflags);
		pos[2] = MSG_ReadCoord (cl.protocolflags);
		colorStart = MSG_ReadByte ();
		colorLength = MSG_ReadByte ();
		R_ParticleExplosion2 (pos, colorStart, colorLength);
		dl = CL_AllocDlight (0);
		VectorCopy (pos, dl->origin);
		dl->radius = 350;
		dl->die = cl.time + 0.5;
		dl->decay = 300;
		S_StartSound (-1, 0, cl_sfx_r_exp3, pos, 1, 1);
		break;

	// Hexen II stream temp entities
	case TE_H2_STREAM_CHAIN:
	case TE_H2_STREAM_SUNSTAFF1:
	case TE_H2_STREAM_SUNSTAFF2:
	case TE_H2_STREAM_LIGHTNING:
	case TE_H2_STREAM_LIGHTNING_SMALL:
	case TE_H2_STREAM_COLORBEAM:
	case TE_H2_STREAM_ICECHUNKS:
	case TE_H2_STREAM_GAZE:
	case TE_H2_STREAM_FAMINE:
		ParseStream(type);
		break;

	default:
		Sys_Error ("CL_ParseTEnt: bad type");
	}
}


/*
=================
CL_NewTempEntity
=================
*/
entity_t *CL_NewTempEntity (void)
{
	entity_t	*ent;

	if (cl_numvisedicts == MAX_VISEDICTS)
		return NULL;
	if (num_temp_entities == MAX_TEMP_ENTITIES)
		return NULL;
	ent = &cl_temp_entities[num_temp_entities];
	memset (ent, 0, sizeof(*ent));
	num_temp_entities++;
	cl_visedicts[cl_numvisedicts] = ent;
	cl_numvisedicts++;
	ent->scale = ENTSCALE_DEFAULT;
	ent->colormap = vid.colormap;
	return ent;
}


/*
=================
NewStreamEntity

Allocate a stream entity directly to visedicts (H2)
=================
*/
static entity_t *NewStreamEntity(void)
{
	entity_t	*ent;

	if (cl_numvisedicts == MAX_VISEDICTS)
		return NULL;
	if (StreamEntityCount == MAX_STREAM_ENTITIES)
		return NULL;

	ent = &StreamEntities[StreamEntityCount++];
	memset(ent, 0, sizeof(*ent));
	cl_visedicts[cl_numvisedicts++] = ent;
	ent->colormap = vid.colormap;
	ent->scale = ENTSCALE_DEFAULT;

	return ent;
}

/*
=================
CL_UpdateStreams

Update H2 stream temp entities
=================
*/
static void CL_UpdateStreams(void)
{
	int			i, j, offset;
	stream_t	*stream;
	vec3_t		dist, org;
	float		d;
	entity_t	*ent;
	float		yaw, pitch, forward;

	StreamEntityCount = 0;

	for (i = 0, stream = cl_Streams; i < MAX_STREAMS; i++, stream++)
	{
		if (!stream->models[0])
			continue;

		if (stream->endTime < cl.time)
		{
			// Allow lightning to fade out with translucency
			if (stream->type != TE_H2_STREAM_LIGHTNING && stream->type != TE_H2_STREAM_LIGHTNING_SMALL)
				continue;
			else if (stream->endTime + 0.25f < cl.time)
				continue;
		}

		// Update attached stream source position
		if ((stream->flags & STREAM_ATTACHED) && stream->endTime >= cl.time)
		{
			VectorAdd(cl_entities[stream->entity].origin, stream->offset, stream->source);
		}

		// Calculate pitch and yaw
		VectorSubtract(stream->dest, stream->source, dist);
		if (dist[1] == 0 && dist[0] == 0)
		{
			yaw = 0;
			if (dist[2] > 0)
				pitch = 90;
			else
				pitch = 270;
		}
		else
		{
			yaw = (int)(atan2(dist[1], dist[0]) * 180 / M_PI);
			if (yaw < 0)
				yaw += 360;
			forward = sqrt(dist[0] * dist[0] + dist[1] * dist[1]);
			pitch = (int)(atan2(dist[2], forward) * 180 / M_PI);
			if (pitch < 0)
				pitch += 360;
		}

		VectorCopy(stream->source, org);
		d = VectorNormalize(dist);

		// Ice chunks have animated offset
		if (stream->type == TE_H2_STREAM_ICECHUNKS)
		{
			offset = (int)(cl.time * 40) % 30;
			for (j = 0; j < 3; j++)
				org[j] += dist[j] * offset;
		}

		// Draw beam segments along the path
		while (d > 0)
		{
			ent = NewStreamEntity();
			if (!ent)
				return;

			VectorCopy(org, ent->origin);
			ent->model = stream->models[0];
			ent->angles[0] = pitch;
			ent->angles[1] = yaw;

			switch (stream->type)
			{
			case TE_H2_STREAM_CHAIN:
				ent->angles[2] = 0;
				ent->drawflags = H2_MLS_ABSLIGHT;
				ent->abslight = 128;
				break;

			case TE_H2_STREAM_SUNSTAFF1:
				ent->angles[2] = (int)(cl.time * 10) % 360;
				ent->drawflags = H2_MLS_ABSLIGHT;
				ent->abslight = 128;

				// Add translucent overlay
				ent = NewStreamEntity();
				if (!ent)
					return;
				VectorCopy(org, ent->origin);
				ent->model = stream->models[1];
				ent->angles[0] = pitch;
				ent->angles[1] = yaw;
				ent->angles[2] = (int)(cl.time * 50) % 360;
				ent->drawflags = H2_MLS_ABSLIGHT | H2_DRF_TRANSLUCENT;
				ent->abslight = 128;
				break;

			case TE_H2_STREAM_SUNSTAFF2:
				ent->angles[2] = (int)(cl.time * 10) % 360;
				ent->drawflags = H2_MLS_ABSLIGHT;
				ent->abslight = 128;
				ent->frame = (int)(cl.time * 10) % 8;
				break;

			case TE_H2_STREAM_LIGHTNING:
				if (stream->endTime < cl.time)
				{
					// Fade out after duration
					ent->drawflags = H2_MLS_ABSLIGHT | H2_DRF_TRANSLUCENT;
					ent->abslight = (int)(128 + (stream->endTime - cl.time) * 192);
				}
				else
				{
					ent->angles[2] = rand() % 360;
					ent->drawflags = H2_MLS_ABSLIGHT;
					ent->abslight = 128;
					ent->frame = rand() % 6;
				}
				break;

			case TE_H2_STREAM_LIGHTNING_SMALL:
				if (stream->endTime < cl.time)
				{
					ent->drawflags = H2_MLS_ABSLIGHT | H2_DRF_TRANSLUCENT;
					ent->abslight = (int)(128 + (stream->endTime - cl.time) * 192);
				}
				else
				{
					ent->angles[2] = rand() % 360;
					ent->frame = rand() % 6;
					ent->drawflags = H2_MLS_ABSLIGHT;
					ent->abslight = 128;
				}
				break;

			case TE_H2_STREAM_FAMINE:
				ent->angles[2] = rand() % 360;
				ent->drawflags = H2_MLS_ABSLIGHT;
				ent->abslight = 128;
				ent->frame = 0;
				break;

			case TE_H2_STREAM_COLORBEAM:
				ent->angles[2] = 0;
				ent->drawflags = H2_MLS_ABSLIGHT;
				ent->abslight = 128;
				ent->skinnum = stream->skin;
				break;

			case TE_H2_STREAM_GAZE:
				ent->angles[2] = 0;
				ent->drawflags = H2_MLS_ABSLIGHT;
				ent->abslight = 128;
				ent->frame = (int)(cl.time * 40) % 36;
				break;

			case TE_H2_STREAM_ICECHUNKS:
				ent->angles[2] = rand() % 360;
				ent->drawflags = H2_MLS_ABSLIGHT;
				ent->abslight = 128;
				ent->frame = rand() % 5;
				break;

			default:
				ent->angles[2] = 0;
				break;
			}

			for (j = 0; j < 3; j++)
				org[j] += dist[j] * 30;
			d -= 30;
		}

		// Add sunstaff endpoint effects
		if (stream->type == TE_H2_STREAM_SUNSTAFF1 || stream->type == TE_H2_STREAM_SUNSTAFF2)
		{
			if (stream->lastTrailTime + 0.2f < cl.time)
			{
				stream->lastTrailTime = cl.time;
				R_SunStaffTrail(stream->source, stream->dest);
			}

			ent = NewStreamEntity();
			if (ent == NULL)
				return;

			VectorCopy(stream->dest, ent->origin);
			ent->model = stream->models[2];
			ent->drawflags = H2_MLS_ABSLIGHT;
			ent->abslight = 128;
			ent->scale = 80 + (rand() & 15);

			ent = NewStreamEntity();
			if (ent == NULL)
				return;

			VectorCopy(stream->dest, ent->origin);
			ent->model = stream->models[3];
			ent->drawflags = H2_MLS_ABSLIGHT | H2_DRF_TRANSLUCENT;
			ent->abslight = 128;
			ent->scale = 150 + (rand() & 15);
		}
	}
}

/*
=================
CL_UpdateTEnts
=================
*/
void CL_UpdateTEnts (void)
{
	int			i, j; //johnfitz -- use j instead of using i twice, so we don't corrupt memory
	beam_t		*b;
	vec3_t		dist, org;
	float		d;
	entity_t	*ent;
	float		yaw, pitch;
	float		forward;

	num_temp_entities = 0;

	srand ((int) (cl.time * 1000)); //johnfitz -- freeze beams when paused

	// Update H2 streams first
	if (hexen2_mode)
		CL_UpdateStreams();

// update lightning
	for (i=0, b=cl_beams ; i< MAX_BEAMS ; i++, b++)
	{
		if (!b->model || b->starttime > cl.time || b->endtime < cl.time)
			continue;

	// if coming from the player, update the start position
		if (b->entity == cl.viewentity)
		{
			VectorCopy (cl_entities[cl.viewentity].origin, b->start);
		}

	// calculate pitch and yaw
		VectorSubtract (b->end, b->start, dist);

		if (dist[1] == 0 && dist[0] == 0)
		{
			yaw = 0;
			if (dist[2] > 0)
				pitch = 90;
			else
				pitch = 270;
		}
		else
		{
			yaw = (int) (atan2(dist[1], dist[0]) * 180 / M_PI);
			if (yaw < 0)
				yaw += 360;

			forward = sqrt (dist[0]*dist[0] + dist[1]*dist[1]);
			pitch = (int) (atan2(dist[2], forward) * 180 / M_PI);
			if (pitch < 0)
				pitch += 360;
		}

	// add new entities for the lightning
		VectorCopy (b->start, org);
		d = VectorNormalize(dist);
		while (d > 0)
		{
			ent = CL_NewTempEntity ();
			if (!ent)
				return;
			VectorCopy (org, ent->origin);
			ent->model = b->model;
			ent->angles[0] = pitch;
			ent->angles[1] = yaw;
			ent->angles[2] = rand()%360;

			//johnfitz -- use j instead of using i twice, so we don't corrupt memory
			for (j=0 ; j<3 ; j++)
				org[j] += dist[j]*30;
			d -= 30;
		}
	}
}
