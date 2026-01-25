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

// sv_effect_hexen2.c -- Server-side Hexen II effect system
// Adapted from uhexen2 sv_effect.c

#include "quakedef.h"
#include "cl_effect.h"
#include "protocol_hexen2.h"

// Server-side effect storage
static EffectT	sv_Effects[MAX_EFFECTS];

// Forward declarations
static void SV_SendEffect(sizebuf_t *sb, int idx);

/*
===============
SV_ClearEffects

Clear all effects (called on level change)
===============
*/
void SV_ClearEffects(void)
{
	memset(sv_Effects, 0, sizeof(sv_Effects));
}

/*
===============
SV_SendEffect

Send effect data to clients
===============
*/
static void SV_SendEffect(sizebuf_t *sb, int idx)
{
	if (!hexen2_mode)
		return;

	MSG_WriteByte(sb, svc_h2_start_effect);
	MSG_WriteByte(sb, idx);
	MSG_WriteByte(sb, sv_Effects[idx].type);

	switch (sv_Effects[idx].type)
	{
	case CE_RAIN:
		MSG_WriteCoord(sb, sv_Effects[idx].ef.Rain.min_org[0], sv.protocolflags);
		MSG_WriteCoord(sb, sv_Effects[idx].ef.Rain.min_org[1], sv.protocolflags);
		MSG_WriteCoord(sb, sv_Effects[idx].ef.Rain.min_org[2], sv.protocolflags);
		MSG_WriteCoord(sb, sv_Effects[idx].ef.Rain.max_org[0], sv.protocolflags);
		MSG_WriteCoord(sb, sv_Effects[idx].ef.Rain.max_org[1], sv.protocolflags);
		MSG_WriteCoord(sb, sv_Effects[idx].ef.Rain.max_org[2], sv.protocolflags);
		MSG_WriteCoord(sb, sv_Effects[idx].ef.Rain.e_size[0], sv.protocolflags);
		MSG_WriteCoord(sb, sv_Effects[idx].ef.Rain.e_size[1], sv.protocolflags);
		MSG_WriteCoord(sb, sv_Effects[idx].ef.Rain.e_size[2], sv.protocolflags);
		MSG_WriteCoord(sb, sv_Effects[idx].ef.Rain.dir[0], sv.protocolflags);
		MSG_WriteCoord(sb, sv_Effects[idx].ef.Rain.dir[1], sv.protocolflags);
		MSG_WriteCoord(sb, sv_Effects[idx].ef.Rain.dir[2], sv.protocolflags);
		MSG_WriteShort(sb, sv_Effects[idx].ef.Rain.color);
		MSG_WriteShort(sb, sv_Effects[idx].ef.Rain.count);
		MSG_WriteFloat(sb, sv_Effects[idx].ef.Rain.wait);
		break;

	case CE_SNOW:
		MSG_WriteCoord(sb, sv_Effects[idx].ef.Rain.min_org[0], sv.protocolflags);
		MSG_WriteCoord(sb, sv_Effects[idx].ef.Rain.min_org[1], sv.protocolflags);
		MSG_WriteCoord(sb, sv_Effects[idx].ef.Rain.min_org[2], sv.protocolflags);
		MSG_WriteCoord(sb, sv_Effects[idx].ef.Rain.max_org[0], sv.protocolflags);
		MSG_WriteCoord(sb, sv_Effects[idx].ef.Rain.max_org[1], sv.protocolflags);
		MSG_WriteCoord(sb, sv_Effects[idx].ef.Rain.max_org[2], sv.protocolflags);
		MSG_WriteByte(sb, sv_Effects[idx].ef.Rain.flags);
		MSG_WriteCoord(sb, sv_Effects[idx].ef.Rain.dir[0], sv.protocolflags);
		MSG_WriteCoord(sb, sv_Effects[idx].ef.Rain.dir[1], sv.protocolflags);
		MSG_WriteCoord(sb, sv_Effects[idx].ef.Rain.dir[2], sv.protocolflags);
		MSG_WriteByte(sb, sv_Effects[idx].ef.Rain.count);
		break;

	case CE_FOUNTAIN:
		MSG_WriteCoord(sb, sv_Effects[idx].ef.Fountain.pos[0], sv.protocolflags);
		MSG_WriteCoord(sb, sv_Effects[idx].ef.Fountain.pos[1], sv.protocolflags);
		MSG_WriteCoord(sb, sv_Effects[idx].ef.Fountain.pos[2], sv.protocolflags);
		MSG_WriteAngle(sb, sv_Effects[idx].ef.Fountain.angle[0], sv.protocolflags);
		MSG_WriteAngle(sb, sv_Effects[idx].ef.Fountain.angle[1], sv.protocolflags);
		MSG_WriteAngle(sb, sv_Effects[idx].ef.Fountain.angle[2], sv.protocolflags);
		MSG_WriteCoord(sb, sv_Effects[idx].ef.Fountain.movedir[0], sv.protocolflags);
		MSG_WriteCoord(sb, sv_Effects[idx].ef.Fountain.movedir[1], sv.protocolflags);
		MSG_WriteCoord(sb, sv_Effects[idx].ef.Fountain.movedir[2], sv.protocolflags);
		MSG_WriteShort(sb, sv_Effects[idx].ef.Fountain.color);
		MSG_WriteByte(sb, sv_Effects[idx].ef.Fountain.cnt);
		break;

	case CE_QUAKE:
		MSG_WriteCoord(sb, sv_Effects[idx].ef.Quake.origin[0], sv.protocolflags);
		MSG_WriteCoord(sb, sv_Effects[idx].ef.Quake.origin[1], sv.protocolflags);
		MSG_WriteCoord(sb, sv_Effects[idx].ef.Quake.origin[2], sv.protocolflags);
		MSG_WriteFloat(sb, sv_Effects[idx].ef.Quake.radius);
		break;

	case CE_WHITE_SMOKE:
	case CE_GREEN_SMOKE:
	case CE_GREY_SMOKE:
	case CE_RED_SMOKE:
	case CE_SLOW_WHITE_SMOKE:
	case CE_TELESMK1:
	case CE_TELESMK2:
	case CE_GHOST:
	case CE_REDCLOUD:
	case CE_ACID_MUZZFL:
	case CE_FLAMESTREAM:
	case CE_FLAMEWALL:
	case CE_FLAMEWALL2:
	case CE_ONFIRE:
		MSG_WriteCoord(sb, sv_Effects[idx].ef.Smoke.origin[0], sv.protocolflags);
		MSG_WriteCoord(sb, sv_Effects[idx].ef.Smoke.origin[1], sv.protocolflags);
		MSG_WriteCoord(sb, sv_Effects[idx].ef.Smoke.origin[2], sv.protocolflags);
		MSG_WriteFloat(sb, sv_Effects[idx].ef.Smoke.velocity[0]);
		MSG_WriteFloat(sb, sv_Effects[idx].ef.Smoke.velocity[1]);
		MSG_WriteFloat(sb, sv_Effects[idx].ef.Smoke.velocity[2]);
		MSG_WriteFloat(sb, sv_Effects[idx].ef.Smoke.framelength);
		MSG_WriteFloat(sb, sv_Effects[idx].ef.Smoke.frame);
		break;

	case CE_SM_WHITE_FLASH:
	case CE_YELLOWRED_FLASH:
	case CE_BLUESPARK:
	case CE_YELLOWSPARK:
	case CE_SM_CIRCLE_EXP:
	case CE_BG_CIRCLE_EXP:
	case CE_SM_EXPLOSION:
	case CE_LG_EXPLOSION:
	case CE_FLOOR_EXPLOSION:
	case CE_FLOOR_EXPLOSION3:
	case CE_BLUE_EXPLOSION:
	case CE_REDSPARK:
	case CE_GREENSPARK:
	case CE_ICEHIT:
	case CE_MEDUSA_HIT:
	case CE_MEZZO_REFLECT:
	case CE_FLOOR_EXPLOSION2:
	case CE_XBOW_EXPLOSION:
	case CE_NEW_EXPLOSION:
	case CE_MAGIC_MISSILE_EXPLOSION:
	case CE_BONE_EXPLOSION:
	case CE_BLDRN_EXPL:
	case CE_ACID_HIT:
	case CE_ACID_SPLAT:
	case CE_ACID_EXPL:
	case CE_LBALL_EXPL:
	case CE_FIREWALL_SMALL:
	case CE_FIREWALL_MEDIUM:
	case CE_FIREWALL_LARGE:
	case CE_FBOOM:
	case CE_BOMB:
	case CE_BRN_BOUNCE:
	case CE_LSHOCK:
		MSG_WriteCoord(sb, sv_Effects[idx].ef.Smoke.origin[0], sv.protocolflags);
		MSG_WriteCoord(sb, sv_Effects[idx].ef.Smoke.origin[1], sv.protocolflags);
		MSG_WriteCoord(sb, sv_Effects[idx].ef.Smoke.origin[2], sv.protocolflags);
		break;

	case CE_WHITE_FLASH:
	case CE_BLUE_FLASH:
	case CE_SM_BLUE_FLASH:
	case CE_RED_FLASH:
		MSG_WriteCoord(sb, sv_Effects[idx].ef.Smoke.origin[0], sv.protocolflags);
		MSG_WriteCoord(sb, sv_Effects[idx].ef.Smoke.origin[1], sv.protocolflags);
		MSG_WriteCoord(sb, sv_Effects[idx].ef.Smoke.origin[2], sv.protocolflags);
		break;

	case CE_RIDER_DEATH:
		MSG_WriteCoord(sb, sv_Effects[idx].ef.RD.origin[0], sv.protocolflags);
		MSG_WriteCoord(sb, sv_Effects[idx].ef.RD.origin[1], sv.protocolflags);
		MSG_WriteCoord(sb, sv_Effects[idx].ef.RD.origin[2], sv.protocolflags);
		break;

	case CE_TELEPORTERPUFFS:
		MSG_WriteCoord(sb, sv_Effects[idx].ef.Teleporter.origin[0], sv.protocolflags);
		MSG_WriteCoord(sb, sv_Effects[idx].ef.Teleporter.origin[1], sv.protocolflags);
		MSG_WriteCoord(sb, sv_Effects[idx].ef.Teleporter.origin[2], sv.protocolflags);
		break;

	case CE_TELEPORTERBODY:
		MSG_WriteCoord(sb, sv_Effects[idx].ef.Teleporter.origin[0], sv.protocolflags);
		MSG_WriteCoord(sb, sv_Effects[idx].ef.Teleporter.origin[1], sv.protocolflags);
		MSG_WriteCoord(sb, sv_Effects[idx].ef.Teleporter.origin[2], sv.protocolflags);
		MSG_WriteFloat(sb, sv_Effects[idx].ef.Teleporter.velocity[0][0]);
		MSG_WriteFloat(sb, sv_Effects[idx].ef.Teleporter.velocity[0][1]);
		MSG_WriteFloat(sb, sv_Effects[idx].ef.Teleporter.velocity[0][2]);
		MSG_WriteFloat(sb, sv_Effects[idx].ef.Teleporter.skinnum);
		break;

	case CE_BONESHARD:
	case CE_BONESHRAPNEL:
		MSG_WriteCoord(sb, sv_Effects[idx].ef.Missile.origin[0], sv.protocolflags);
		MSG_WriteCoord(sb, sv_Effects[idx].ef.Missile.origin[1], sv.protocolflags);
		MSG_WriteCoord(sb, sv_Effects[idx].ef.Missile.origin[2], sv.protocolflags);
		MSG_WriteFloat(sb, sv_Effects[idx].ef.Missile.velocity[0]);
		MSG_WriteFloat(sb, sv_Effects[idx].ef.Missile.velocity[1]);
		MSG_WriteFloat(sb, sv_Effects[idx].ef.Missile.velocity[2]);
		MSG_WriteFloat(sb, sv_Effects[idx].ef.Missile.angle[0]);
		MSG_WriteFloat(sb, sv_Effects[idx].ef.Missile.angle[1]);
		MSG_WriteFloat(sb, sv_Effects[idx].ef.Missile.angle[2]);
		MSG_WriteFloat(sb, sv_Effects[idx].ef.Missile.avelocity[0]);
		MSG_WriteFloat(sb, sv_Effects[idx].ef.Missile.avelocity[1]);
		MSG_WriteFloat(sb, sv_Effects[idx].ef.Missile.avelocity[2]);
		break;

	case CE_GRAVITYWELL:
		MSG_WriteCoord(sb, sv_Effects[idx].ef.RD.origin[0], sv.protocolflags);
		MSG_WriteCoord(sb, sv_Effects[idx].ef.RD.origin[1], sv.protocolflags);
		MSG_WriteCoord(sb, sv_Effects[idx].ef.RD.origin[2], sv.protocolflags);
		MSG_WriteShort(sb, sv_Effects[idx].ef.RD.color);
		MSG_WriteFloat(sb, sv_Effects[idx].ef.RD.lifetime);
		break;

	case CE_CHUNK:
		MSG_WriteCoord(sb, sv_Effects[idx].ef.Chunk.origin[0], sv.protocolflags);
		MSG_WriteCoord(sb, sv_Effects[idx].ef.Chunk.origin[1], sv.protocolflags);
		MSG_WriteCoord(sb, sv_Effects[idx].ef.Chunk.origin[2], sv.protocolflags);
		MSG_WriteByte(sb, sv_Effects[idx].ef.Chunk.type);
		MSG_WriteCoord(sb, sv_Effects[idx].ef.Chunk.srcVel[0], sv.protocolflags);
		MSG_WriteCoord(sb, sv_Effects[idx].ef.Chunk.srcVel[1], sv.protocolflags);
		MSG_WriteCoord(sb, sv_Effects[idx].ef.Chunk.srcVel[2], sv.protocolflags);
		MSG_WriteByte(sb, sv_Effects[idx].ef.Chunk.numChunks);
		break;

	default:
		Con_DPrintf("SV_SendEffect: bad type %d\n", sv_Effects[idx].type);
		break;
	}
}

/*
===============
SV_UpdateEffects

Send all active effects to a client (for reconnection/spawn)
===============
*/
void SV_UpdateEffects(sizebuf_t *sb)
{
	int idx;

	if (!hexen2_mode)
		return;

	for (idx = 0; idx < MAX_EFFECTS; idx++)
	{
		if (sv_Effects[idx].type)
			SV_SendEffect(sb, idx);
	}
}

/*
===============
SV_ParseEffect

Parse a starteffect() builtin call and send to clients.
This is called from PF_h2_starteffect().

Returns the effect index, or -1 on error.
===============
*/
int SV_ParseEffect(void)
{
	int		idx;
	byte	effect;
	float	*vec;

	if (!hexen2_mode)
		return -1;

	effect = (byte)G_FLOAT(OFS_PARM0);

	// Find a free effect slot
	for (idx = 0; idx < MAX_EFFECTS; idx++)
	{
		if (!sv_Effects[idx].type ||
			(sv_Effects[idx].expire_time && sv_Effects[idx].expire_time <= qcvm->time))
			break;
	}

	if (idx >= MAX_EFFECTS)
	{
		Con_DPrintf("SV_ParseEffect: MAX_EFFECTS reached\n");
		return -1;
	}

	memset(&sv_Effects[idx], 0, sizeof(EffectT));
	sv_Effects[idx].type = effect;

	switch (effect)
	{
	case CE_RAIN:
		vec = G_VECTOR(OFS_PARM1);
		VectorCopy(vec, sv_Effects[idx].ef.Rain.min_org);
		vec = G_VECTOR(OFS_PARM2);
		VectorCopy(vec, sv_Effects[idx].ef.Rain.max_org);
		vec = G_VECTOR(OFS_PARM3);
		VectorCopy(vec, sv_Effects[idx].ef.Rain.e_size);
		vec = G_VECTOR(OFS_PARM4);
		VectorCopy(vec, sv_Effects[idx].ef.Rain.dir);
		sv_Effects[idx].ef.Rain.color = (int)G_FLOAT(OFS_PARM5);
		sv_Effects[idx].ef.Rain.count = (int)G_FLOAT(OFS_PARM6);
		sv_Effects[idx].ef.Rain.wait = G_FLOAT(OFS_PARM7);
		sv_Effects[idx].ef.Rain.next_time = 0;
		break;

	case CE_SNOW:
		vec = G_VECTOR(OFS_PARM1);
		VectorCopy(vec, sv_Effects[idx].ef.Rain.min_org);
		vec = G_VECTOR(OFS_PARM2);
		VectorCopy(vec, sv_Effects[idx].ef.Rain.max_org);
		sv_Effects[idx].ef.Rain.flags = (int)G_FLOAT(OFS_PARM3);
		vec = G_VECTOR(OFS_PARM4);
		VectorCopy(vec, sv_Effects[idx].ef.Rain.dir);
		sv_Effects[idx].ef.Rain.count = (int)G_FLOAT(OFS_PARM5);
		sv_Effects[idx].ef.Rain.next_time = 0;
		break;

	case CE_FOUNTAIN:
		vec = G_VECTOR(OFS_PARM1);
		VectorCopy(vec, sv_Effects[idx].ef.Fountain.pos);
		vec = G_VECTOR(OFS_PARM2);
		VectorCopy(vec, sv_Effects[idx].ef.Fountain.angle);
		vec = G_VECTOR(OFS_PARM3);
		VectorCopy(vec, sv_Effects[idx].ef.Fountain.movedir);
		sv_Effects[idx].ef.Fountain.color = (int)G_FLOAT(OFS_PARM4);
		sv_Effects[idx].ef.Fountain.cnt = (int)G_FLOAT(OFS_PARM5);
		break;

	case CE_QUAKE:
		vec = G_VECTOR(OFS_PARM1);
		VectorCopy(vec, sv_Effects[idx].ef.Quake.origin);
		sv_Effects[idx].ef.Quake.radius = G_FLOAT(OFS_PARM2);
		break;

	case CE_WHITE_SMOKE:
	case CE_GREEN_SMOKE:
	case CE_GREY_SMOKE:
	case CE_RED_SMOKE:
	case CE_SLOW_WHITE_SMOKE:
	case CE_TELESMK1:
	case CE_TELESMK2:
	case CE_GHOST:
	case CE_REDCLOUD:
		vec = G_VECTOR(OFS_PARM1);
		VectorCopy(vec, sv_Effects[idx].ef.Smoke.origin);
		vec = G_VECTOR(OFS_PARM2);
		VectorCopy(vec, sv_Effects[idx].ef.Smoke.velocity);
		sv_Effects[idx].ef.Smoke.framelength = G_FLOAT(OFS_PARM3);
		sv_Effects[idx].ef.Smoke.frame = 0;
		sv_Effects[idx].expire_time = qcvm->time + 1;
		break;

	case CE_ACID_MUZZFL:
	case CE_FLAMESTREAM:
	case CE_FLAMEWALL:
	case CE_FLAMEWALL2:
	case CE_ONFIRE:
		// Mission pack smoke variants with frame parameter
		vec = G_VECTOR(OFS_PARM1);
		VectorCopy(vec, sv_Effects[idx].ef.Smoke.origin);
		vec = G_VECTOR(OFS_PARM2);
		VectorCopy(vec, sv_Effects[idx].ef.Smoke.velocity);
		sv_Effects[idx].ef.Smoke.framelength = 0.05f;
		sv_Effects[idx].ef.Smoke.frame = G_FLOAT(OFS_PARM3);
		sv_Effects[idx].expire_time = qcvm->time + 1;
		break;

	case CE_SM_WHITE_FLASH:
	case CE_YELLOWRED_FLASH:
	case CE_BLUESPARK:
	case CE_YELLOWSPARK:
	case CE_SM_CIRCLE_EXP:
	case CE_BG_CIRCLE_EXP:
	case CE_SM_EXPLOSION:
	case CE_LG_EXPLOSION:
	case CE_FLOOR_EXPLOSION:
	case CE_FLOOR_EXPLOSION3:
	case CE_BLUE_EXPLOSION:
	case CE_REDSPARK:
	case CE_GREENSPARK:
	case CE_ICEHIT:
	case CE_MEDUSA_HIT:
	case CE_MEZZO_REFLECT:
	case CE_FLOOR_EXPLOSION2:
	case CE_XBOW_EXPLOSION:
	case CE_NEW_EXPLOSION:
	case CE_MAGIC_MISSILE_EXPLOSION:
	case CE_BONE_EXPLOSION:
	case CE_BLDRN_EXPL:
	case CE_ACID_HIT:
	case CE_ACID_SPLAT:
	case CE_ACID_EXPL:
	case CE_LBALL_EXPL:
	case CE_FIREWALL_SMALL:
	case CE_FIREWALL_MEDIUM:
	case CE_FIREWALL_LARGE:
	case CE_FBOOM:
	case CE_BOMB:
	case CE_BRN_BOUNCE:
	case CE_LSHOCK:
		vec = G_VECTOR(OFS_PARM1);
		VectorCopy(vec, sv_Effects[idx].ef.Smoke.origin);
		sv_Effects[idx].expire_time = qcvm->time + 1;
		break;

	case CE_WHITE_FLASH:
	case CE_BLUE_FLASH:
	case CE_SM_BLUE_FLASH:
	case CE_RED_FLASH:
		vec = G_VECTOR(OFS_PARM1);
		VectorCopy(vec, sv_Effects[idx].ef.Flash.origin);
		sv_Effects[idx].expire_time = qcvm->time + 1;
		break;

	case CE_RIDER_DEATH:
		vec = G_VECTOR(OFS_PARM1);
		VectorCopy(vec, sv_Effects[idx].ef.RD.origin);
		break;

	case CE_GRAVITYWELL:
		vec = G_VECTOR(OFS_PARM1);
		VectorCopy(vec, sv_Effects[idx].ef.RD.origin);
		sv_Effects[idx].ef.RD.color = (int)G_FLOAT(OFS_PARM2);
		sv_Effects[idx].ef.RD.lifetime = G_FLOAT(OFS_PARM3);
		break;

	case CE_TELEPORTERPUFFS:
		vec = G_VECTOR(OFS_PARM1);
		VectorCopy(vec, sv_Effects[idx].ef.Teleporter.origin);
		sv_Effects[idx].expire_time = qcvm->time + 1;
		break;

	case CE_TELEPORTERBODY:
		vec = G_VECTOR(OFS_PARM1);
		VectorCopy(vec, sv_Effects[idx].ef.Teleporter.origin);
		vec = G_VECTOR(OFS_PARM2);
		VectorCopy(vec, sv_Effects[idx].ef.Teleporter.velocity[0]);
		sv_Effects[idx].ef.Teleporter.skinnum = G_FLOAT(OFS_PARM3);
		sv_Effects[idx].expire_time = qcvm->time + 1;
		break;

	case CE_BONESHARD:
	case CE_BONESHRAPNEL:
		vec = G_VECTOR(OFS_PARM1);
		VectorCopy(vec, sv_Effects[idx].ef.Missile.origin);
		vec = G_VECTOR(OFS_PARM2);
		VectorCopy(vec, sv_Effects[idx].ef.Missile.velocity);
		vec = G_VECTOR(OFS_PARM3);
		VectorCopy(vec, sv_Effects[idx].ef.Missile.angle);
		// uhexen2 uses velocity for avelocity here
		vec = G_VECTOR(OFS_PARM2);
		VectorCopy(vec, sv_Effects[idx].ef.Missile.avelocity);
		sv_Effects[idx].expire_time = qcvm->time + 10;
		break;

	case CE_CHUNK:
		vec = G_VECTOR(OFS_PARM1);
		VectorCopy(vec, sv_Effects[idx].ef.Chunk.origin);
		sv_Effects[idx].ef.Chunk.type = (unsigned char)G_FLOAT(OFS_PARM2);
		vec = G_VECTOR(OFS_PARM3);
		VectorCopy(vec, sv_Effects[idx].ef.Chunk.srcVel);
		sv_Effects[idx].ef.Chunk.numChunks = (unsigned char)G_FLOAT(OFS_PARM4);
		sv_Effects[idx].expire_time = qcvm->time + 3;
		break;

	default:
		Con_DPrintf("SV_ParseEffect: bad type %d\n", effect);
		memset(&sv_Effects[idx], 0, sizeof(EffectT));
		return -1;
	}

	// Send to all clients
	SV_SendEffect(&sv.reliable_datagram, idx);

	return idx;
}

/*
===============
SV_EndEffect

End an effect by index. Called from PF_h2_endeffect().
===============
*/
void SV_EndEffect(int idx)
{
	if (!hexen2_mode)
		return;

	if (idx < 0 || idx >= MAX_EFFECTS)
	{
		Con_DPrintf("SV_EndEffect: invalid index %d\n", idx);
		return;
	}

	if (!sv_Effects[idx].type)
		return;

	// Send end effect message to clients
	MSG_WriteByte(&sv.reliable_datagram, svc_h2_end_effect);
	MSG_WriteByte(&sv.reliable_datagram, idx);

	// Clear the effect
	memset(&sv_Effects[idx], 0, sizeof(EffectT));
}

/*
===============
SV_SaveEffects

Save effects to a file for save games
===============
*/
void SV_SaveEffects(FILE *f)
{
	int idx, count;

	if (!hexen2_mode)
	{
		fprintf(f, "Effects: 0\n");
		return;
	}

	// Count active effects
	for (idx = count = 0; idx < MAX_EFFECTS; idx++)
	{
		if (sv_Effects[idx].type)
			count++;
	}

	fprintf(f, "Effects: %d\n", count);

	for (idx = 0; idx < MAX_EFFECTS; idx++)
	{
		if (!sv_Effects[idx].type)
			continue;

		fprintf(f, "Effect: %d %d %f: ", idx, sv_Effects[idx].type, sv_Effects[idx].expire_time);

		switch (sv_Effects[idx].type)
		{
		case CE_RAIN:
			fprintf(f, "%f %f %f ", sv_Effects[idx].ef.Rain.min_org[0],
					sv_Effects[idx].ef.Rain.min_org[1], sv_Effects[idx].ef.Rain.min_org[2]);
			fprintf(f, "%f %f %f ", sv_Effects[idx].ef.Rain.max_org[0],
					sv_Effects[idx].ef.Rain.max_org[1], sv_Effects[idx].ef.Rain.max_org[2]);
			fprintf(f, "%f %f %f ", sv_Effects[idx].ef.Rain.e_size[0],
					sv_Effects[idx].ef.Rain.e_size[1], sv_Effects[idx].ef.Rain.e_size[2]);
			fprintf(f, "%f %f %f ", sv_Effects[idx].ef.Rain.dir[0],
					sv_Effects[idx].ef.Rain.dir[1], sv_Effects[idx].ef.Rain.dir[2]);
			fprintf(f, "%d %d %f\n", sv_Effects[idx].ef.Rain.color,
					sv_Effects[idx].ef.Rain.count, sv_Effects[idx].ef.Rain.wait);
			break;

		case CE_SNOW:
			fprintf(f, "%f %f %f ", sv_Effects[idx].ef.Rain.min_org[0],
					sv_Effects[idx].ef.Rain.min_org[1], sv_Effects[idx].ef.Rain.min_org[2]);
			fprintf(f, "%f %f %f ", sv_Effects[idx].ef.Rain.max_org[0],
					sv_Effects[idx].ef.Rain.max_org[1], sv_Effects[idx].ef.Rain.max_org[2]);
			fprintf(f, "%d ", sv_Effects[idx].ef.Rain.flags);
			fprintf(f, "%f %f %f ", sv_Effects[idx].ef.Rain.dir[0],
					sv_Effects[idx].ef.Rain.dir[1], sv_Effects[idx].ef.Rain.dir[2]);
			fprintf(f, "%d\n", sv_Effects[idx].ef.Rain.count);
			break;

		case CE_FOUNTAIN:
			fprintf(f, "%f %f %f ", sv_Effects[idx].ef.Fountain.pos[0],
					sv_Effects[idx].ef.Fountain.pos[1], sv_Effects[idx].ef.Fountain.pos[2]);
			fprintf(f, "%f %f %f ", sv_Effects[idx].ef.Fountain.angle[0],
					sv_Effects[idx].ef.Fountain.angle[1], sv_Effects[idx].ef.Fountain.angle[2]);
			fprintf(f, "%f %f %f ", sv_Effects[idx].ef.Fountain.movedir[0],
					sv_Effects[idx].ef.Fountain.movedir[1], sv_Effects[idx].ef.Fountain.movedir[2]);
			fprintf(f, "%d %d\n", sv_Effects[idx].ef.Fountain.color, sv_Effects[idx].ef.Fountain.cnt);
			break;

		case CE_QUAKE:
			fprintf(f, "%f %f %f %f\n", sv_Effects[idx].ef.Quake.origin[0],
					sv_Effects[idx].ef.Quake.origin[1], sv_Effects[idx].ef.Quake.origin[2],
					sv_Effects[idx].ef.Quake.radius);
			break;

		case CE_WHITE_SMOKE:
		case CE_GREEN_SMOKE:
		case CE_GREY_SMOKE:
		case CE_RED_SMOKE:
		case CE_SLOW_WHITE_SMOKE:
		case CE_TELESMK1:
		case CE_TELESMK2:
		case CE_GHOST:
		case CE_REDCLOUD:
		case CE_ACID_MUZZFL:
		case CE_FLAMESTREAM:
		case CE_FLAMEWALL:
		case CE_FLAMEWALL2:
		case CE_ONFIRE:
			fprintf(f, "%f %f %f ", sv_Effects[idx].ef.Smoke.origin[0],
					sv_Effects[idx].ef.Smoke.origin[1], sv_Effects[idx].ef.Smoke.origin[2]);
			fprintf(f, "%f %f %f ", sv_Effects[idx].ef.Smoke.velocity[0],
					sv_Effects[idx].ef.Smoke.velocity[1], sv_Effects[idx].ef.Smoke.velocity[2]);
			fprintf(f, "%f %f\n", sv_Effects[idx].ef.Smoke.framelength, sv_Effects[idx].ef.Smoke.frame);
			break;

		case CE_SM_WHITE_FLASH:
		case CE_YELLOWRED_FLASH:
		case CE_BLUESPARK:
		case CE_YELLOWSPARK:
		case CE_SM_CIRCLE_EXP:
		case CE_BG_CIRCLE_EXP:
		case CE_SM_EXPLOSION:
		case CE_LG_EXPLOSION:
		case CE_FLOOR_EXPLOSION:
		case CE_FLOOR_EXPLOSION3:
		case CE_BLUE_EXPLOSION:
		case CE_REDSPARK:
		case CE_GREENSPARK:
		case CE_ICEHIT:
		case CE_MEDUSA_HIT:
		case CE_MEZZO_REFLECT:
		case CE_FLOOR_EXPLOSION2:
		case CE_XBOW_EXPLOSION:
		case CE_NEW_EXPLOSION:
		case CE_MAGIC_MISSILE_EXPLOSION:
		case CE_BONE_EXPLOSION:
		case CE_BLDRN_EXPL:
		case CE_BRN_BOUNCE:
		case CE_LSHOCK:
		case CE_ACID_HIT:
		case CE_ACID_SPLAT:
		case CE_ACID_EXPL:
		case CE_LBALL_EXPL:
		case CE_FIREWALL_SMALL:
		case CE_FIREWALL_MEDIUM:
		case CE_FIREWALL_LARGE:
		case CE_FBOOM:
		case CE_BOMB:
		case CE_WHITE_FLASH:
		case CE_BLUE_FLASH:
		case CE_SM_BLUE_FLASH:
		case CE_RED_FLASH:
			fprintf(f, "%f %f %f\n", sv_Effects[idx].ef.Smoke.origin[0],
					sv_Effects[idx].ef.Smoke.origin[1], sv_Effects[idx].ef.Smoke.origin[2]);
			break;

		case CE_RIDER_DEATH:
			fprintf(f, "%f %f %f\n", sv_Effects[idx].ef.RD.origin[0],
					sv_Effects[idx].ef.RD.origin[1], sv_Effects[idx].ef.RD.origin[2]);
			break;

		case CE_GRAVITYWELL:
			fprintf(f, "%f %f %f %d %f\n", sv_Effects[idx].ef.RD.origin[0],
					sv_Effects[idx].ef.RD.origin[1], sv_Effects[idx].ef.RD.origin[2],
					sv_Effects[idx].ef.RD.color, sv_Effects[idx].ef.RD.lifetime);
			break;

		case CE_TELEPORTERPUFFS:
		case CE_TELEPORTERBODY:
			fprintf(f, "%f %f %f\n", sv_Effects[idx].ef.Teleporter.origin[0],
					sv_Effects[idx].ef.Teleporter.origin[1], sv_Effects[idx].ef.Teleporter.origin[2]);
			break;

		case CE_BONESHARD:
		case CE_BONESHRAPNEL:
			fprintf(f, "%f %f %f ", sv_Effects[idx].ef.Missile.origin[0],
					sv_Effects[idx].ef.Missile.origin[1], sv_Effects[idx].ef.Missile.origin[2]);
			fprintf(f, "%f %f %f ", sv_Effects[idx].ef.Missile.velocity[0],
					sv_Effects[idx].ef.Missile.velocity[1], sv_Effects[idx].ef.Missile.velocity[2]);
			fprintf(f, "%f %f %f ", sv_Effects[idx].ef.Missile.angle[0],
					sv_Effects[idx].ef.Missile.angle[1], sv_Effects[idx].ef.Missile.angle[2]);
			fprintf(f, "%f %f %f\n", sv_Effects[idx].ef.Missile.avelocity[0],
					sv_Effects[idx].ef.Missile.avelocity[1], sv_Effects[idx].ef.Missile.avelocity[2]);
			break;

		case CE_CHUNK:
			fprintf(f, "%f %f %f ", sv_Effects[idx].ef.Chunk.origin[0],
					sv_Effects[idx].ef.Chunk.origin[1], sv_Effects[idx].ef.Chunk.origin[2]);
			fprintf(f, "%u ", (unsigned)sv_Effects[idx].ef.Chunk.type);
			fprintf(f, "%f %f %f ", sv_Effects[idx].ef.Chunk.srcVel[0],
					sv_Effects[idx].ef.Chunk.srcVel[1], sv_Effects[idx].ef.Chunk.srcVel[2]);
			fprintf(f, "%u\n", (unsigned)sv_Effects[idx].ef.Chunk.numChunks);
			break;

		default:
			fprintf(f, "\n");
			break;
		}
	}
}

/*
===============
SV_LoadEffects

Load effects from save game data.
Note: Full implementation requires converting from FILE*-based parsing to
string buffer parsing. For now, effects are cleared but not loaded.
TODO: Implement string buffer parsing for full save/load support.
===============
*/
const char *SV_LoadEffects(const char *data)
{
	SV_ClearEffects();

	if (!hexen2_mode || !data)
		return data;

	// TODO: Implement string buffer parsing
	// For now, skip past the effects section if present
	// Effects will be recreated by QuakeC on level restore

	// Try to skip the "Effects: N" line and subsequent effect data
	// This is a simplified skip - effects won't be restored
	if (strncmp(data, "Effects:", 8) == 0)
	{
		// Skip to end of effects section (until we hit EOF or another section)
		const char *p = data;
		while (*p && *p != '\0')
		{
			// Skip lines until we find one that doesn't start with "Effect:" or "Effects:"
			if (strncmp(p, "Effect:", 7) != 0 && strncmp(p, "Effects:", 8) != 0)
			{
				// Check if this line starts with a number or other known pattern
				// For now, just skip all lines that look like effect data
				if (*p >= '0' && *p <= '9')
				{
					// Skip this line
					while (*p && *p != '\n')
						p++;
					if (*p == '\n')
						p++;
					continue;
				}
				break;
			}
			// Skip this line
			while (*p && *p != '\n')
				p++;
			if (*p == '\n')
				p++;
		}
		return p;
	}

	return data;
}
