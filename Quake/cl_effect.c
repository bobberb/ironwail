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

// cl_effect.c -- Hexen II client-side effect system

#include "quakedef.h"
#include "cl_effect.h"
#include "protocol_hexen2.h"

#define HX_FRAME_TIME	0.05f

// Effect entity pool
static entity_t		EffectEntities[MAX_EFFECT_ENTITIES];
static qboolean		EntityUsed[MAX_EFFECT_ENTITIES];
static int			EffectEntityCount;

// Effect storage (in client state)
static EffectT		Effects[MAX_EFFECTS];

// Forward declarations
static int NewEffectEntity(void);
static void FreeEffectEntity(int idx);
static void CL_FreeEffect(int idx);
static void CL_LinkEffectEntity(entity_t *ent);

/*
===============
CL_InitEffects
===============
*/
void CL_InitEffects(void)
{
	// Nothing to initialize currently
}

/*
===============
CL_ClearEffects
===============
*/
void CL_ClearEffects(void)
{
	memset(Effects, 0, sizeof(Effects));
	memset(EntityUsed, 0, sizeof(EntityUsed));
	EffectEntityCount = 0;
}

/*
===============
CL_FreeEffect
===============
*/
static void CL_FreeEffect(int idx)
{
	int i;

	switch (Effects[idx].type)
	{
	case CE_RAIN:
	case CE_SNOW:
	case CE_FOUNTAIN:
	case CE_QUAKE:
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
		FreeEffectEntity(Effects[idx].ef.Smoke.entity_index);
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
	case CE_FBOOM:
	case CE_BOMB:
	case CE_FIREWALL_SMALL:
	case CE_FIREWALL_MEDIUM:
	case CE_FIREWALL_LARGE:
		FreeEffectEntity(Effects[idx].ef.Smoke.entity_index);
		break;

	case CE_WHITE_FLASH:
	case CE_BLUE_FLASH:
	case CE_SM_BLUE_FLASH:
	case CE_RED_FLASH:
		FreeEffectEntity(Effects[idx].ef.Flash.entity_index);
		break;

	case CE_RIDER_DEATH:
	case CE_GRAVITYWELL:
		break;

	case CE_TELEPORTERPUFFS:
		for (i = 0; i < 8; ++i)
			FreeEffectEntity(Effects[idx].ef.Teleporter.entity_index[i]);
		break;

	case CE_TELEPORTERBODY:
		FreeEffectEntity(Effects[idx].ef.Teleporter.entity_index[0]);
		break;

	case CE_BONESHARD:
	case CE_BONESHRAPNEL:
		FreeEffectEntity(Effects[idx].ef.Missile.entity_index);
		break;

	case CE_CHUNK:
		for (i = 0; i < Effects[idx].ef.Chunk.numChunks; i++)
			FreeEffectEntity(Effects[idx].ef.Chunk.entity_index[i]);
		break;
	}

	memset(&Effects[idx], 0, sizeof(EffectT));
}

/*
===============
CL_ParseEffect

Parse a new effect from the server
===============
*/
void CL_ParseEffect(void)
{
	int			idx;
	qboolean	ImmediateFree;

	ImmediateFree = false;

	idx = MSG_ReadByte();
	if (Effects[idx].type)
		CL_FreeEffect(idx);

	memset(&Effects[idx], 0, sizeof(EffectT));
	Effects[idx].type = MSG_ReadByte();

	switch (Effects[idx].type)
	{
	case CE_RAIN:
		Effects[idx].ef.Rain.min_org[0] = MSG_ReadCoord(cl.protocolflags);
		Effects[idx].ef.Rain.min_org[1] = MSG_ReadCoord(cl.protocolflags);
		Effects[idx].ef.Rain.min_org[2] = MSG_ReadCoord(cl.protocolflags);
		Effects[idx].ef.Rain.max_org[0] = MSG_ReadCoord(cl.protocolflags);
		Effects[idx].ef.Rain.max_org[1] = MSG_ReadCoord(cl.protocolflags);
		Effects[idx].ef.Rain.max_org[2] = MSG_ReadCoord(cl.protocolflags);
		Effects[idx].ef.Rain.e_size[0] = MSG_ReadCoord(cl.protocolflags);
		Effects[idx].ef.Rain.e_size[1] = MSG_ReadCoord(cl.protocolflags);
		Effects[idx].ef.Rain.e_size[2] = MSG_ReadCoord(cl.protocolflags);
		Effects[idx].ef.Rain.dir[0] = MSG_ReadCoord(cl.protocolflags);
		Effects[idx].ef.Rain.dir[1] = MSG_ReadCoord(cl.protocolflags);
		Effects[idx].ef.Rain.dir[2] = MSG_ReadCoord(cl.protocolflags);
		Effects[idx].ef.Rain.color = MSG_ReadShort();
		Effects[idx].ef.Rain.count = MSG_ReadShort();
		Effects[idx].ef.Rain.wait = MSG_ReadFloat();
		break;

	case CE_SNOW:
		Effects[idx].ef.Rain.min_org[0] = MSG_ReadCoord(cl.protocolflags);
		Effects[idx].ef.Rain.min_org[1] = MSG_ReadCoord(cl.protocolflags);
		Effects[idx].ef.Rain.min_org[2] = MSG_ReadCoord(cl.protocolflags);
		Effects[idx].ef.Rain.max_org[0] = MSG_ReadCoord(cl.protocolflags);
		Effects[idx].ef.Rain.max_org[1] = MSG_ReadCoord(cl.protocolflags);
		Effects[idx].ef.Rain.max_org[2] = MSG_ReadCoord(cl.protocolflags);
		Effects[idx].ef.Rain.flags = MSG_ReadByte();
		Effects[idx].ef.Rain.dir[0] = MSG_ReadCoord(cl.protocolflags);
		Effects[idx].ef.Rain.dir[1] = MSG_ReadCoord(cl.protocolflags);
		Effects[idx].ef.Rain.dir[2] = MSG_ReadCoord(cl.protocolflags);
		Effects[idx].ef.Rain.count = MSG_ReadByte();
		break;

	case CE_FOUNTAIN:
		Effects[idx].ef.Fountain.pos[0] = MSG_ReadCoord(cl.protocolflags);
		Effects[idx].ef.Fountain.pos[1] = MSG_ReadCoord(cl.protocolflags);
		Effects[idx].ef.Fountain.pos[2] = MSG_ReadCoord(cl.protocolflags);
		Effects[idx].ef.Fountain.angle[0] = MSG_ReadAngle(cl.protocolflags);
		Effects[idx].ef.Fountain.angle[1] = MSG_ReadAngle(cl.protocolflags);
		Effects[idx].ef.Fountain.angle[2] = MSG_ReadAngle(cl.protocolflags);
		Effects[idx].ef.Fountain.movedir[0] = MSG_ReadCoord(cl.protocolflags);
		Effects[idx].ef.Fountain.movedir[1] = MSG_ReadCoord(cl.protocolflags);
		Effects[idx].ef.Fountain.movedir[2] = MSG_ReadCoord(cl.protocolflags);
		Effects[idx].ef.Fountain.color = MSG_ReadShort();
		Effects[idx].ef.Fountain.cnt = MSG_ReadByte();
		AngleVectors(Effects[idx].ef.Fountain.angle,
					 Effects[idx].ef.Fountain.vforward,
					 Effects[idx].ef.Fountain.vright,
					 Effects[idx].ef.Fountain.vup);
		break;

	case CE_QUAKE:
		Effects[idx].ef.Quake.origin[0] = MSG_ReadCoord(cl.protocolflags);
		Effects[idx].ef.Quake.origin[1] = MSG_ReadCoord(cl.protocolflags);
		Effects[idx].ef.Quake.origin[2] = MSG_ReadCoord(cl.protocolflags);
		Effects[idx].ef.Quake.radius = MSG_ReadFloat();
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
		Effects[idx].ef.Smoke.origin[0] = MSG_ReadCoord(cl.protocolflags);
		Effects[idx].ef.Smoke.origin[1] = MSG_ReadCoord(cl.protocolflags);
		Effects[idx].ef.Smoke.origin[2] = MSG_ReadCoord(cl.protocolflags);
		Effects[idx].ef.Smoke.velocity[0] = MSG_ReadFloat();
		Effects[idx].ef.Smoke.velocity[1] = MSG_ReadFloat();
		Effects[idx].ef.Smoke.velocity[2] = MSG_ReadFloat();
		Effects[idx].ef.Smoke.framelength = MSG_ReadFloat();
		Effects[idx].ef.Smoke.frame = MSG_ReadFloat();

		Effects[idx].ef.Smoke.entity_index = NewEffectEntity();
		if (Effects[idx].ef.Smoke.entity_index == -1)
			ImmediateFree = true;
		else
		{
			entity_t *ent = &EffectEntities[Effects[idx].ef.Smoke.entity_index];
			VectorCopy(Effects[idx].ef.Smoke.origin, ent->origin);

			// Load appropriate sprite model based on effect type
			switch (Effects[idx].type)
			{
			case CE_WHITE_SMOKE:
			case CE_SLOW_WHITE_SMOKE:
				ent->model = Mod_ForName("models/whtsmk1.spr", true);
				ent->drawflags = H2_DRF_TRANSLUCENT;
				break;
			case CE_GREEN_SMOKE:
				ent->model = Mod_ForName("models/grnsmk1.spr", true);
				ent->drawflags = H2_DRF_TRANSLUCENT;
				break;
			case CE_GREY_SMOKE:
				ent->model = Mod_ForName("models/grysmk1.spr", true);
				ent->drawflags = H2_DRF_TRANSLUCENT;
				break;
			case CE_RED_SMOKE:
				ent->model = Mod_ForName("models/redsmk1.spr", true);
				ent->drawflags = H2_DRF_TRANSLUCENT;
				break;
			case CE_TELESMK1:
				ent->model = Mod_ForName("models/telesmk1.spr", true);
				ent->drawflags = H2_DRF_TRANSLUCENT;
				break;
			case CE_TELESMK2:
				ent->model = Mod_ForName("models/telesmk2.spr", true);
				ent->drawflags = H2_DRF_TRANSLUCENT;
				break;
			case CE_REDCLOUD:
				ent->model = Mod_ForName("models/rcloud.spr", true);
				break;
			case CE_FLAMESTREAM:
				ent->model = Mod_ForName("models/flamestr.spr", true);
				ent->drawflags = H2_DRF_TRANSLUCENT | H2_MLS_ABSLIGHT;
				ent->abslight = 255;
				ent->frame = Effects[idx].ef.Smoke.frame;
				break;
			case CE_ACID_MUZZFL:
				ent->model = Mod_ForName("models/muzzle1.spr", true);
				ent->drawflags = H2_DRF_TRANSLUCENT | H2_MLS_ABSLIGHT;
				ent->abslight = 51;
				break;
			case CE_FLAMEWALL:
				ent->model = Mod_ForName("models/firewal1.spr", true);
				break;
			case CE_FLAMEWALL2:
				ent->model = Mod_ForName("models/firewal2.spr", true);
				ent->drawflags = H2_DRF_TRANSLUCENT;
				break;
			case CE_ONFIRE:
				{
					int rdm = rand() & 3;
					if (rdm < 1)
						ent->model = Mod_ForName("models/firewal1.spr", true);
					else if (rdm < 2)
						ent->model = Mod_ForName("models/firewal2.spr", true);
					else
						ent->model = Mod_ForName("models/firewal3.spr", true);
					ent->drawflags = H2_DRF_TRANSLUCENT;
					ent->abslight = 255;
					ent->frame = Effects[idx].ef.Smoke.frame;
				}
				break;
			case CE_GHOST:
				ent->model = Mod_ForName("models/ghost.spr", true);
				ent->drawflags = H2_DRF_TRANSLUCENT | H2_MLS_ABSLIGHT;
				ent->abslight = 127;
				break;
			default:
				break;
			}
		}
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
	case CE_FBOOM:
	case CE_BOMB:
	case CE_FIREWALL_SMALL:
	case CE_FIREWALL_MEDIUM:
	case CE_FIREWALL_LARGE:
		Effects[idx].ef.Smoke.origin[0] = MSG_ReadCoord(cl.protocolflags);
		Effects[idx].ef.Smoke.origin[1] = MSG_ReadCoord(cl.protocolflags);
		Effects[idx].ef.Smoke.origin[2] = MSG_ReadCoord(cl.protocolflags);

		Effects[idx].ef.Smoke.entity_index = NewEffectEntity();
		if (Effects[idx].ef.Smoke.entity_index == -1)
			ImmediateFree = true;
		else
		{
			entity_t *ent = &EffectEntities[Effects[idx].ef.Smoke.entity_index];
			VectorCopy(Effects[idx].ef.Smoke.origin, ent->origin);

			// Load appropriate sprite model based on effect type
			switch (Effects[idx].type)
			{
			case CE_SM_WHITE_FLASH:
				ent->model = Mod_ForName("models/sm_white.spr", true);
				break;
			case CE_YELLOWRED_FLASH:
				ent->model = Mod_ForName("models/yr_flsh.spr", true);
				ent->drawflags = H2_DRF_TRANSLUCENT;
				break;
			case CE_BLUESPARK:
				ent->model = Mod_ForName("models/bspark.spr", true);
				break;
			case CE_YELLOWSPARK:
				ent->model = Mod_ForName("models/spark.spr", true);
				break;
			case CE_SM_CIRCLE_EXP:
				ent->model = Mod_ForName("models/fcircle.spr", true);
				break;
			case CE_BG_CIRCLE_EXP:
				ent->model = Mod_ForName("models/xplod29.spr", true);
				break;
			case CE_SM_EXPLOSION:
				ent->model = Mod_ForName("models/sm_expld.spr", true);
				break;
			case CE_LG_EXPLOSION:
				ent->model = Mod_ForName("models/bg_expld.spr", true);
				break;
			case CE_FLOOR_EXPLOSION:
				ent->model = Mod_ForName("models/fl_expld.spr", true);
				break;
			case CE_FLOOR_EXPLOSION3:
				ent->model = Mod_ForName("models/biggy.spr", true);
				break;
			case CE_BLUE_EXPLOSION:
				ent->model = Mod_ForName("models/xpspblue.spr", true);
				break;
			case CE_REDSPARK:
				ent->model = Mod_ForName("models/rspark.spr", true);
				break;
			case CE_GREENSPARK:
				ent->model = Mod_ForName("models/gspark.spr", true);
				break;
			case CE_ICEHIT:
				ent->model = Mod_ForName("models/icehit.spr", true);
				break;
			case CE_MEDUSA_HIT:
				ent->model = Mod_ForName("models/medhit.spr", true);
				break;
			case CE_MEZZO_REFLECT:
				ent->model = Mod_ForName("models/mezzoref.spr", true);
				break;
			case CE_FLOOR_EXPLOSION2:
				ent->model = Mod_ForName("models/flrexpl2.spr", true);
				break;
			case CE_XBOW_EXPLOSION:
				ent->model = Mod_ForName("models/xbowexpl.spr", true);
				break;
			case CE_NEW_EXPLOSION:
				ent->model = Mod_ForName("models/gen_expl.spr", true);
				break;
			case CE_MAGIC_MISSILE_EXPLOSION:
				ent->model = Mod_ForName("models/mm_expld.spr", true);
				break;
			case CE_BONE_EXPLOSION:
				ent->model = Mod_ForName("models/bonexpld.spr", true);
				break;
			case CE_BLDRN_EXPL:
				ent->model = Mod_ForName("models/xplsn_1.spr", true);
				break;
			case CE_ACID_HIT:
				ent->model = Mod_ForName("models/axplsn_2.spr", true);
				break;
			case CE_ACID_SPLAT:
				ent->model = Mod_ForName("models/axplsn_1.spr", true);
				break;
			case CE_ACID_EXPL:
				ent->model = Mod_ForName("models/axplsn_5.spr", true);
				ent->drawflags = H2_MLS_ABSLIGHT;
				ent->abslight = 255;
				break;
			case CE_FBOOM:
				ent->model = Mod_ForName("models/fboom.spr", true);
				break;
			case CE_BOMB:
				ent->model = Mod_ForName("models/pow.spr", true);
				break;
			case CE_LBALL_EXPL:
				ent->model = Mod_ForName("models/Bluexp3.spr", true);
				break;
			case CE_FIREWALL_SMALL:
				ent->model = Mod_ForName("models/firewal1.spr", true);
				break;
			case CE_FIREWALL_MEDIUM:
				ent->model = Mod_ForName("models/firewal5.spr", true);
				break;
			case CE_FIREWALL_LARGE:
				ent->model = Mod_ForName("models/firewal4.spr", true);
				break;
			case CE_BRN_BOUNCE:
				ent->model = Mod_ForName("models/spark.spr", true);
				break;
			case CE_LSHOCK:
				ent->model = Mod_ForName("models/vorpshok.mdl", true);
				ent->drawflags = H2_MLS_TORCH;
				ent->angles[2] = 90;
				ent->scale = 255;
				break;
			default:
				break;
			}
		}
		break;

	case CE_WHITE_FLASH:
	case CE_BLUE_FLASH:
	case CE_SM_BLUE_FLASH:
	case CE_RED_FLASH:
		Effects[idx].ef.Flash.origin[0] = MSG_ReadCoord(cl.protocolflags);
		Effects[idx].ef.Flash.origin[1] = MSG_ReadCoord(cl.protocolflags);
		Effects[idx].ef.Flash.origin[2] = MSG_ReadCoord(cl.protocolflags);
		Effects[idx].ef.Flash.reverse = 0;

		Effects[idx].ef.Flash.entity_index = NewEffectEntity();
		if (Effects[idx].ef.Flash.entity_index == -1)
			ImmediateFree = true;
		else
		{
			entity_t *ent = &EffectEntities[Effects[idx].ef.Flash.entity_index];
			VectorCopy(Effects[idx].ef.Flash.origin, ent->origin);

			// Load appropriate sprite model based on effect type
			switch (Effects[idx].type)
			{
			case CE_WHITE_FLASH:
				ent->model = Mod_ForName("models/gryspt.spr", true);
				break;
			case CE_BLUE_FLASH:
				ent->model = Mod_ForName("models/bluflash.spr", true);
				break;
			case CE_SM_BLUE_FLASH:
				ent->model = Mod_ForName("models/sm_blue.spr", true);
				break;
			case CE_RED_FLASH:
				ent->model = Mod_ForName("models/redspt.spr", true);
				break;
			default:
				break;
			}
			ent->drawflags = H2_DRF_TRANSLUCENT;
		}
		break;

	case CE_RIDER_DEATH:
		Effects[idx].ef.RD.origin[0] = MSG_ReadCoord(cl.protocolflags);
		Effects[idx].ef.RD.origin[1] = MSG_ReadCoord(cl.protocolflags);
		Effects[idx].ef.RD.origin[2] = MSG_ReadCoord(cl.protocolflags);
		break;

	case CE_GRAVITYWELL:
		Effects[idx].ef.RD.origin[0] = MSG_ReadCoord(cl.protocolflags);
		Effects[idx].ef.RD.origin[1] = MSG_ReadCoord(cl.protocolflags);
		Effects[idx].ef.RD.origin[2] = MSG_ReadCoord(cl.protocolflags);
		Effects[idx].ef.RD.color = MSG_ReadShort();
		Effects[idx].ef.RD.lifetime = MSG_ReadFloat();
		break;

	case CE_TELEPORTERPUFFS:
		Effects[idx].ef.Teleporter.origin[0] = MSG_ReadCoord(cl.protocolflags);
		Effects[idx].ef.Teleporter.origin[1] = MSG_ReadCoord(cl.protocolflags);
		Effects[idx].ef.Teleporter.origin[2] = MSG_ReadCoord(cl.protocolflags);
		Effects[idx].ef.Teleporter.framelength = 0.05f;
		// Allocate 8 entities for teleporter puffs
		{
			int dir = 0;
			for (int i = 0; i < 8; ++i)
			{
				Effects[idx].ef.Teleporter.entity_index[i] = NewEffectEntity();
				if (Effects[idx].ef.Teleporter.entity_index[i] == -1)
				{
					ImmediateFree = true;
					break;
				}
				else
				{
					entity_t *ent = &EffectEntities[Effects[idx].ef.Teleporter.entity_index[i]];
					VectorCopy(Effects[idx].ef.Teleporter.origin, ent->origin);

					float sinval = sinf(dir * M_PI / 180.0f);
					float cosval = cosf(dir * M_PI / 180.0f);

					Effects[idx].ef.Teleporter.velocity[i][0] = 10 * cosval;
					Effects[idx].ef.Teleporter.velocity[i][1] = 10 * sinval;
					Effects[idx].ef.Teleporter.velocity[i][2] = 0;
					dir += 45;

					ent->model = Mod_ForName("models/telesmk2.spr", true);
					ent->drawflags = H2_DRF_TRANSLUCENT;
				}
			}
		}
		break;

	case CE_TELEPORTERBODY:
		Effects[idx].ef.Teleporter.origin[0] = MSG_ReadCoord(cl.protocolflags);
		Effects[idx].ef.Teleporter.origin[1] = MSG_ReadCoord(cl.protocolflags);
		Effects[idx].ef.Teleporter.origin[2] = MSG_ReadCoord(cl.protocolflags);
		Effects[idx].ef.Teleporter.velocity[0][0] = MSG_ReadFloat();
		Effects[idx].ef.Teleporter.velocity[0][1] = MSG_ReadFloat();
		Effects[idx].ef.Teleporter.velocity[0][2] = MSG_ReadFloat();
		Effects[idx].ef.Teleporter.skinnum = MSG_ReadFloat();
		Effects[idx].ef.Teleporter.framelength = 0.05f;

		Effects[idx].ef.Teleporter.entity_index[0] = NewEffectEntity();
		if (Effects[idx].ef.Teleporter.entity_index[0] == -1)
			ImmediateFree = true;
		else
		{
			entity_t *ent = &EffectEntities[Effects[idx].ef.Teleporter.entity_index[0]];
			VectorCopy(Effects[idx].ef.Teleporter.origin, ent->origin);

			ent->model = Mod_ForName("models/teleport.mdl", true);
			ent->drawflags = H2_SCALE_TYPE_XYONLY | H2_DRF_TRANSLUCENT;
			ent->scale = 100;
			ent->skinnum = (int)Effects[idx].ef.Teleporter.skinnum;
		}
		break;

	case CE_BONESHARD:
	case CE_BONESHRAPNEL:
		Effects[idx].ef.Missile.origin[0] = MSG_ReadCoord(cl.protocolflags);
		Effects[idx].ef.Missile.origin[1] = MSG_ReadCoord(cl.protocolflags);
		Effects[idx].ef.Missile.origin[2] = MSG_ReadCoord(cl.protocolflags);
		Effects[idx].ef.Missile.velocity[0] = MSG_ReadFloat();
		Effects[idx].ef.Missile.velocity[1] = MSG_ReadFloat();
		Effects[idx].ef.Missile.velocity[2] = MSG_ReadFloat();
		Effects[idx].ef.Missile.angle[0] = MSG_ReadFloat();
		Effects[idx].ef.Missile.angle[1] = MSG_ReadFloat();
		Effects[idx].ef.Missile.angle[2] = MSG_ReadFloat();
		Effects[idx].ef.Missile.avelocity[0] = MSG_ReadFloat();
		Effects[idx].ef.Missile.avelocity[1] = MSG_ReadFloat();
		Effects[idx].ef.Missile.avelocity[2] = MSG_ReadFloat();

		Effects[idx].ef.Missile.entity_index = NewEffectEntity();
		if (Effects[idx].ef.Missile.entity_index == -1)
			ImmediateFree = true;
		else
		{
			entity_t *ent = &EffectEntities[Effects[idx].ef.Missile.entity_index];
			VectorCopy(Effects[idx].ef.Missile.origin, ent->origin);
			VectorCopy(Effects[idx].ef.Missile.angle, ent->angles);
			if (Effects[idx].type == CE_BONESHARD)
				ent->model = Mod_ForName("models/boneshot.mdl", true);
			else
				ent->model = Mod_ForName("models/boneshrd.mdl", true);
		}
		break;

	case CE_CHUNK:
		Effects[idx].ef.Chunk.origin[0] = MSG_ReadCoord(cl.protocolflags);
		Effects[idx].ef.Chunk.origin[1] = MSG_ReadCoord(cl.protocolflags);
		Effects[idx].ef.Chunk.origin[2] = MSG_ReadCoord(cl.protocolflags);
		Effects[idx].ef.Chunk.type = MSG_ReadByte();
		Effects[idx].ef.Chunk.srcVel[0] = MSG_ReadCoord(cl.protocolflags);
		Effects[idx].ef.Chunk.srcVel[1] = MSG_ReadCoord(cl.protocolflags);
		Effects[idx].ef.Chunk.srcVel[2] = MSG_ReadCoord(cl.protocolflags);
		Effects[idx].ef.Chunk.numChunks = MSG_ReadByte();
		Effects[idx].ef.Chunk.time_amount = 4.0f;
		Effects[idx].ef.Chunk.aveScale = 30 + 100 * (Effects[idx].ef.Chunk.numChunks / 40.0f);

		if (Effects[idx].ef.Chunk.numChunks > 16)
			Effects[idx].ef.Chunk.numChunks = 16;

		for (int i = 0; i < Effects[idx].ef.Chunk.numChunks; i++)
		{
			Effects[idx].ef.Chunk.entity_index[i] = NewEffectEntity();
			if (Effects[idx].ef.Chunk.entity_index[i] == -1)
			{
				ImmediateFree = true;
				break;
			}
			else
			{
				entity_t *ent = &EffectEntities[Effects[idx].ef.Chunk.entity_index[i]];
				float final;
				VectorCopy(Effects[idx].ef.Chunk.origin, ent->origin);

				// Initialize velocity with randomization
				VectorCopy(Effects[idx].ef.Chunk.srcVel, Effects[idx].ef.Chunk.velocity[i]);
				VectorScale(Effects[idx].ef.Chunk.velocity[i],
							0.80f + ((rand() % 4) / 10.0f),
							Effects[idx].ef.Chunk.velocity[i]);
				Effects[idx].ef.Chunk.velocity[i][0] += (rand() % 140) - 70;
				Effects[idx].ef.Chunk.velocity[i][1] += (rand() % 140) - 70;
				Effects[idx].ef.Chunk.velocity[i][2] += (rand() % 140) - 70;

				// Random angles
				ent->angles[0] = rand() % 360;
				ent->angles[1] = rand() % 360;
				ent->angles[2] = rand() % 360;

				ent->scale = Effects[idx].ef.Chunk.aveScale + (rand() % 40);

				// Select model based on chunk type
				final = (rand() % 100) * 0.01f;
				switch (Effects[idx].ef.Chunk.type)
				{
				case THINGTYPE_GLASS:
				case THINGTYPE_REDGLASS:
				case THINGTYPE_CLEARGLASS:
				case THINGTYPE_WEBS:
					if (final < 0.20f)
						ent->model = Mod_ForName("models/shard1.mdl", true);
					else if (final < 0.40f)
						ent->model = Mod_ForName("models/shard2.mdl", true);
					else if (final < 0.60f)
						ent->model = Mod_ForName("models/shard3.mdl", true);
					else if (final < 0.80f)
						ent->model = Mod_ForName("models/shard4.mdl", true);
					else
						ent->model = Mod_ForName("models/shard5.mdl", true);
					if (Effects[idx].ef.Chunk.type == THINGTYPE_CLEARGLASS)
					{
						ent->skinnum = 1;
						ent->drawflags |= H2_DRF_TRANSLUCENT;
					}
					else if (Effects[idx].ef.Chunk.type == THINGTYPE_REDGLASS)
						ent->skinnum = 2;
					else if (Effects[idx].ef.Chunk.type == THINGTYPE_WEBS)
					{
						ent->skinnum = 3;
						ent->drawflags |= H2_DRF_TRANSLUCENT;
					}
					break;
				case THINGTYPE_WOOD:
					if (final < 0.25f)
						ent->model = Mod_ForName("models/splnter1.mdl", true);
					else if (final < 0.50f)
						ent->model = Mod_ForName("models/splnter2.mdl", true);
					else if (final < 0.75f)
						ent->model = Mod_ForName("models/splnter3.mdl", true);
					else
						ent->model = Mod_ForName("models/splnter4.mdl", true);
					break;
				case THINGTYPE_METAL:
					if (final < 0.25f)
						ent->model = Mod_ForName("models/metlchk1.mdl", true);
					else if (final < 0.50f)
						ent->model = Mod_ForName("models/metlchk2.mdl", true);
					else if (final < 0.75f)
						ent->model = Mod_ForName("models/metlchk3.mdl", true);
					else
						ent->model = Mod_ForName("models/metlchk4.mdl", true);
					break;
				case THINGTYPE_FLESH:
					if (final < 0.33f)
						ent->model = Mod_ForName("models/flesh1.mdl", true);
					else if (final < 0.66f)
						ent->model = Mod_ForName("models/flesh2.mdl", true);
					else
						ent->model = Mod_ForName("models/flesh3.mdl", true);
					break;
				case THINGTYPE_BROWNSTONE:
					if (final < 0.25f)
						ent->model = Mod_ForName("models/schunk1.mdl", true);
					else if (final < 0.50f)
						ent->model = Mod_ForName("models/schunk2.mdl", true);
					else if (final < 0.75f)
						ent->model = Mod_ForName("models/schunk3.mdl", true);
					else
						ent->model = Mod_ForName("models/schunk4.mdl", true);
					ent->skinnum = 1;
					break;
				case THINGTYPE_CLAY:
				case THINGTYPE_BONE:
					if (final < 0.25f)
						ent->model = Mod_ForName("models/clshard1.mdl", true);
					else if (final < 0.50f)
						ent->model = Mod_ForName("models/clshard2.mdl", true);
					else if (final < 0.75f)
						ent->model = Mod_ForName("models/clshard3.mdl", true);
					else
						ent->model = Mod_ForName("models/clshard4.mdl", true);
					if (Effects[idx].ef.Chunk.type == THINGTYPE_BONE)
						ent->skinnum = 1;
					break;
				case THINGTYPE_LEAVES:
					if (final < 0.33f)
						ent->model = Mod_ForName("models/leafchk1.mdl", true);
					else if (final < 0.66f)
						ent->model = Mod_ForName("models/leafchk2.mdl", true);
					else
						ent->model = Mod_ForName("models/leafchk3.mdl", true);
					break;
				case THINGTYPE_HAY:
					if (final < 0.33f)
						ent->model = Mod_ForName("models/hay1.mdl", true);
					else if (final < 0.66f)
						ent->model = Mod_ForName("models/hay2.mdl", true);
					else
						ent->model = Mod_ForName("models/hay3.mdl", true);
					break;
				case THINGTYPE_CLOTH:
					if (final < 0.33f)
						ent->model = Mod_ForName("models/clthchk1.mdl", true);
					else if (final < 0.66f)
						ent->model = Mod_ForName("models/clthchk2.mdl", true);
					else
						ent->model = Mod_ForName("models/clthchk3.mdl", true);
					break;
				case THINGTYPE_WOOD_LEAF:
					if (final < 0.14f)
						ent->model = Mod_ForName("models/splnter1.mdl", true);
					else if (final < 0.28f)
						ent->model = Mod_ForName("models/leafchk1.mdl", true);
					else if (final < 0.42f)
						ent->model = Mod_ForName("models/splnter2.mdl", true);
					else if (final < 0.56f)
						ent->model = Mod_ForName("models/leafchk2.mdl", true);
					else if (final < 0.70f)
						ent->model = Mod_ForName("models/splnter3.mdl", true);
					else if (final < 0.84f)
						ent->model = Mod_ForName("models/leafchk3.mdl", true);
					else
						ent->model = Mod_ForName("models/splnter4.mdl", true);
					break;
				case THINGTYPE_WOOD_METAL:
					if (final < 0.125f)
						ent->model = Mod_ForName("models/splnter1.mdl", true);
					else if (final < 0.25f)
						ent->model = Mod_ForName("models/metlchk1.mdl", true);
					else if (final < 0.375f)
						ent->model = Mod_ForName("models/splnter2.mdl", true);
					else if (final < 0.50f)
						ent->model = Mod_ForName("models/metlchk2.mdl", true);
					else if (final < 0.625f)
						ent->model = Mod_ForName("models/splnter3.mdl", true);
					else if (final < 0.75f)
						ent->model = Mod_ForName("models/metlchk3.mdl", true);
					else if (final < 0.875f)
						ent->model = Mod_ForName("models/splnter4.mdl", true);
					else
						ent->model = Mod_ForName("models/metlchk4.mdl", true);
					break;
				case THINGTYPE_WOOD_STONE:
					if (final < 0.125f)
						ent->model = Mod_ForName("models/splnter1.mdl", true);
					else if (final < 0.25f)
						ent->model = Mod_ForName("models/schunk1.mdl", true);
					else if (final < 0.375f)
						ent->model = Mod_ForName("models/splnter2.mdl", true);
					else if (final < 0.50f)
						ent->model = Mod_ForName("models/schunk2.mdl", true);
					else if (final < 0.625f)
						ent->model = Mod_ForName("models/splnter3.mdl", true);
					else if (final < 0.75f)
						ent->model = Mod_ForName("models/schunk3.mdl", true);
					else if (final < 0.875f)
						ent->model = Mod_ForName("models/splnter4.mdl", true);
					else
						ent->model = Mod_ForName("models/schunk4.mdl", true);
					break;
				case THINGTYPE_METAL_STONE:
					if (final < 0.125f)
						ent->model = Mod_ForName("models/metlchk1.mdl", true);
					else if (final < 0.25f)
						ent->model = Mod_ForName("models/schunk1.mdl", true);
					else if (final < 0.375f)
						ent->model = Mod_ForName("models/metlchk2.mdl", true);
					else if (final < 0.50f)
						ent->model = Mod_ForName("models/schunk2.mdl", true);
					else if (final < 0.625f)
						ent->model = Mod_ForName("models/metlchk3.mdl", true);
					else if (final < 0.75f)
						ent->model = Mod_ForName("models/schunk3.mdl", true);
					else if (final < 0.875f)
						ent->model = Mod_ForName("models/metlchk4.mdl", true);
					else
						ent->model = Mod_ForName("models/schunk4.mdl", true);
					break;
				case THINGTYPE_METAL_CLOTH:
					if (final < 0.14f)
						ent->model = Mod_ForName("models/metlchk1.mdl", true);
					else if (final < 0.28f)
						ent->model = Mod_ForName("models/clthchk1.mdl", true);
					else if (final < 0.42f)
						ent->model = Mod_ForName("models/metlchk2.mdl", true);
					else if (final < 0.56f)
						ent->model = Mod_ForName("models/clthchk2.mdl", true);
					else if (final < 0.70f)
						ent->model = Mod_ForName("models/metlchk3.mdl", true);
					else if (final < 0.84f)
						ent->model = Mod_ForName("models/clthchk3.mdl", true);
					else
						ent->model = Mod_ForName("models/metlchk4.mdl", true);
					break;
				case THINGTYPE_ICE:
					ent->model = Mod_ForName("models/shard.mdl", true);
					ent->skinnum = 0;
					ent->frame = rand() % 2;
					ent->drawflags |= H2_DRF_TRANSLUCENT | H2_MLS_ABSLIGHT;
					ent->abslight = 127;
					break;
				case THINGTYPE_METEOR:
					ent->model = Mod_ForName("models/tempmetr.mdl", true);
					break;
				case THINGTYPE_ACID:
					ent->model = Mod_ForName("models/sucwp2p.mdl", true);
					break;
				case THINGTYPE_GREENFLESH:
					if (final < 0.33f)
						ent->model = Mod_ForName("models/sflesh1.mdl", true);
					else if (final < 0.66f)
						ent->model = Mod_ForName("models/sflesh2.mdl", true);
					else
						ent->model = Mod_ForName("models/sflesh3.mdl", true);
					break;
				default: // THINGTYPE_GREYSTONE or unknown
					if (final < 0.25f)
						ent->model = Mod_ForName("models/schunk1.mdl", true);
					else if (final < 0.50f)
						ent->model = Mod_ForName("models/schunk2.mdl", true);
					else if (final < 0.75f)
						ent->model = Mod_ForName("models/schunk3.mdl", true);
					else
						ent->model = Mod_ForName("models/schunk4.mdl", true);
					break;
				}
			}
		}
		break;

	default:
		Con_DPrintf("CL_ParseEffect: Unknown effect type %d\n", Effects[idx].type);
		break;
	}

	if (ImmediateFree)
		Effects[idx].type = CE_NONE;
}

/*
===============
CL_EndEffect

Handle svc_h2_end_effect message
===============
*/
void CL_EndEffect(void)
{
	int idx = MSG_ReadByte();
	CL_FreeEffect(idx);
}

/*
===============
CL_UpdateEffects

Update all active effects each frame
===============
*/
void CL_UpdateEffects(void)
{
	int			idx;
	vec3_t		org, org2, alldir;
	int			x_dir, y_dir;
	float		frametime;

	if (cls.state == ca_disconnected)
		return;

	frametime = cl.time - cl.oldtime;
	if (!frametime)
		return;

	for (idx = 0; idx < MAX_EFFECTS; idx++)
	{
		if (!Effects[idx].type)
			continue;

		switch (Effects[idx].type)
		{
		case CE_RAIN:
			org[0] = Effects[idx].ef.Rain.min_org[0];
			org[1] = Effects[idx].ef.Rain.min_org[1];
			org[2] = Effects[idx].ef.Rain.max_org[2];
			org2[0] = Effects[idx].ef.Rain.e_size[0];
			org2[1] = Effects[idx].ef.Rain.e_size[1];
			org2[2] = Effects[idx].ef.Rain.e_size[2];
			x_dir = (int)Effects[idx].ef.Rain.dir[0];
			y_dir = (int)Effects[idx].ef.Rain.dir[1];

			Effects[idx].ef.Rain.next_time += frametime;
			if (Effects[idx].ef.Rain.next_time >= Effects[idx].ef.Rain.wait)
			{
				R_RainEffect(org, org2, x_dir, y_dir,
							 Effects[idx].ef.Rain.color,
							 Effects[idx].ef.Rain.count);
				Effects[idx].ef.Rain.next_time = 0;
			}
			break;

		case CE_SNOW:
			VectorCopy(Effects[idx].ef.Rain.min_org, org);
			VectorCopy(Effects[idx].ef.Rain.max_org, org2);
			VectorCopy(Effects[idx].ef.Rain.dir, alldir);

			Effects[idx].ef.Rain.next_time += frametime;
			if (Effects[idx].ef.Rain.next_time >= 0.10f)
			{
				R_SnowEffect(org, org2, Effects[idx].ef.Rain.flags,
							 alldir, Effects[idx].ef.Rain.count);
				Effects[idx].ef.Rain.next_time = 0;
			}
			break;

		case CE_FOUNTAIN:
			{
				vec3_t mymin, mymax;

				mymin[0] = (-3 * Effects[idx].ef.Fountain.vright[0] * Effects[idx].ef.Fountain.movedir[0]) +
						   (-3 * Effects[idx].ef.Fountain.vforward[0] * Effects[idx].ef.Fountain.movedir[1]) +
						   (2 * Effects[idx].ef.Fountain.vup[0] * Effects[idx].ef.Fountain.movedir[2]);
				mymin[1] = (-3 * Effects[idx].ef.Fountain.vright[1] * Effects[idx].ef.Fountain.movedir[0]) +
						   (-3 * Effects[idx].ef.Fountain.vforward[1] * Effects[idx].ef.Fountain.movedir[1]) +
						   (2 * Effects[idx].ef.Fountain.vup[1] * Effects[idx].ef.Fountain.movedir[2]);
				mymin[2] = (-3 * Effects[idx].ef.Fountain.vright[2] * Effects[idx].ef.Fountain.movedir[0]) +
						   (-3 * Effects[idx].ef.Fountain.vforward[2] * Effects[idx].ef.Fountain.movedir[1]) +
						   (2 * Effects[idx].ef.Fountain.vup[2] * Effects[idx].ef.Fountain.movedir[2]);
				VectorScale(mymin, 15, mymin);

				mymax[0] = (3 * Effects[idx].ef.Fountain.vright[0] * Effects[idx].ef.Fountain.movedir[0]) +
						   (3 * Effects[idx].ef.Fountain.vforward[0] * Effects[idx].ef.Fountain.movedir[1]) +
						   (10 * Effects[idx].ef.Fountain.vup[0] * Effects[idx].ef.Fountain.movedir[2]);
				mymax[1] = (3 * Effects[idx].ef.Fountain.vright[1] * Effects[idx].ef.Fountain.movedir[0]) +
						   (3 * Effects[idx].ef.Fountain.vforward[1] * Effects[idx].ef.Fountain.movedir[1]) +
						   (10 * Effects[idx].ef.Fountain.vup[1] * Effects[idx].ef.Fountain.movedir[2]);
				mymax[2] = (3 * Effects[idx].ef.Fountain.vright[2] * Effects[idx].ef.Fountain.movedir[0]) +
						   (3 * Effects[idx].ef.Fountain.vforward[2] * Effects[idx].ef.Fountain.movedir[1]) +
						   (10 * Effects[idx].ef.Fountain.vup[2] * Effects[idx].ef.Fountain.movedir[2]);
				VectorScale(mymax, 15, mymax);

				R_RunParticleEffect2(Effects[idx].ef.Fountain.pos, mymin, mymax,
									 Effects[idx].ef.Fountain.color, pt_fastgrav,
									 Effects[idx].ef.Fountain.cnt);
			}
			break;

		case CE_QUAKE:
			R_RunQuakeEffect(Effects[idx].ef.Quake.origin, Effects[idx].ef.Quake.radius);
			break;

		case CE_RIDER_DEATH:
			{
				float sinval, cosval;
				Effects[idx].ef.RD.time_amount += frametime;
				if (Effects[idx].ef.RD.time_amount >= 1)
				{
					Effects[idx].ef.RD.stage++;
					Effects[idx].ef.RD.time_amount -= 1;
				}

				VectorCopy(Effects[idx].ef.RD.origin, org);
				sinval = sinf(Effects[idx].ef.RD.time_amount * 2 * M_PI);
				cosval = cosf(Effects[idx].ef.RD.time_amount * 2 * M_PI);
				org[0] += sinval * 30;
				org[1] += cosval * 30;

				if (Effects[idx].ef.RD.stage <= 6)
				{
					R_RiderParticle(Effects[idx].ef.RD.stage + 1, org);
				}
				else
				{
					// Set the rider's origin point for the particles
					R_RiderParticle(0, org);
					if (Effects[idx].ef.RD.stage == 7)
					{
						// White flash at stage 7
						cl.cshifts[CSHIFT_BONUS].destcolor[0] = 255;
						cl.cshifts[CSHIFT_BONUS].destcolor[1] = 255;
						cl.cshifts[CSHIFT_BONUS].destcolor[2] = 255;
						cl.cshifts[CSHIFT_BONUS].percent = 256;
					}
					else if (Effects[idx].ef.RD.stage > 13)
					{
						CL_FreeEffect(idx);
					}
				}
			}
			break;

		case CE_GRAVITYWELL:
			{
				float sinval, cosval;
				Effects[idx].ef.RD.time_amount += frametime * 2;
				if (Effects[idx].ef.RD.time_amount >= 1)
					Effects[idx].ef.RD.time_amount -= 1;

				VectorCopy(Effects[idx].ef.RD.origin, org);
				sinval = sinf(Effects[idx].ef.RD.time_amount * 2 * M_PI);
				cosval = cosf(Effects[idx].ef.RD.time_amount * 2 * M_PI);
				org[0] += sinval * 30;
				org[1] += cosval * 30;

				if (Effects[idx].ef.RD.lifetime < cl.time)
				{
					CL_FreeEffect(idx);
				}
				else
				{
					R_GravityWellParticle(rand() % 8, org, Effects[idx].ef.RD.color);
				}
			}
			break;

		// Smoke/explosion effects - animate sprites
		case CE_WHITE_SMOKE:
		case CE_GREEN_SMOKE:
		case CE_GREY_SMOKE:
		case CE_RED_SMOKE:
		case CE_SLOW_WHITE_SMOKE:
		case CE_TELESMK1:
		case CE_TELESMK2:
		case CE_GHOST:
		case CE_REDCLOUD:
		case CE_FLAMESTREAM:
		case CE_ACID_MUZZFL:
		case CE_FLAMEWALL:
		case CE_FLAMEWALL2:
		case CE_ONFIRE:
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
		case CE_FBOOM:
		case CE_BOMB:
		case CE_FIREWALL_SMALL:
		case CE_FIREWALL_MEDIUM:
		case CE_FIREWALL_LARGE:
			{
				entity_t *ent;
				if (Effects[idx].ef.Smoke.entity_index < 0)
					break;

				ent = &EffectEntities[Effects[idx].ef.Smoke.entity_index];

				Effects[idx].ef.Smoke.time_amount += frametime;

				// Animate sprite frames
				while (Effects[idx].ef.Smoke.time_amount >= HX_FRAME_TIME)
				{
					ent->frame++;
					Effects[idx].ef.Smoke.time_amount -= HX_FRAME_TIME;
				}

				// Check if animation is done
				if (ent->model && ent->frame >= ent->model->numframes)
				{
					CL_FreeEffect(idx);
					break;
				}

				// Update position for moving smoke
				ent->origin[0] += frametime * Effects[idx].ef.Smoke.velocity[0];
				ent->origin[1] += frametime * Effects[idx].ef.Smoke.velocity[1];
				ent->origin[2] += frametime * Effects[idx].ef.Smoke.velocity[2];

				CL_LinkEffectEntity(ent);
			}
			break;

		case CE_WHITE_FLASH:
		case CE_BLUE_FLASH:
		case CE_SM_BLUE_FLASH:
		case CE_RED_FLASH:
			{
				entity_t *ent;
				if (Effects[idx].ef.Flash.entity_index < 0)
					break;

				ent = &EffectEntities[Effects[idx].ef.Flash.entity_index];

				Effects[idx].ef.Flash.time_amount += frametime;

				// Animate frames (forward then backward)
				while (Effects[idx].ef.Flash.time_amount >= HX_FRAME_TIME)
				{
					if (!Effects[idx].ef.Flash.reverse)
					{
						ent->frame++;
						if (ent->model && ent->frame >= ent->model->numframes)
						{
							Effects[idx].ef.Flash.reverse = 1;
							ent->frame = ent->model->numframes - 1;
						}
					}
					else
					{
						ent->frame--;
						if (ent->frame < 0)
						{
							CL_FreeEffect(idx);
							break;
						}
					}
					Effects[idx].ef.Flash.time_amount -= HX_FRAME_TIME;
				}

				if (Effects[idx].type)
					CL_LinkEffectEntity(ent);
			}
			break;

		case CE_TELEPORTERPUFFS:
			{
				entity_t *ent;
				int cur_frame;
				float smoketime;

				Effects[idx].ef.Teleporter.time_amount += frametime;
				smoketime = Effects[idx].ef.Teleporter.framelength;

				ent = &EffectEntities[Effects[idx].ef.Teleporter.entity_index[0]];
				while (Effects[idx].ef.Teleporter.time_amount >= HX_FRAME_TIME)
				{
					ent->frame++;
					Effects[idx].ef.Teleporter.time_amount -= HX_FRAME_TIME;
				}
				cur_frame = ent->frame;

				if (ent->model && cur_frame >= ent->model->numframes)
				{
					CL_FreeEffect(idx);
					break;
				}

				for (int i = 0; i < 8; ++i)
				{
					ent = &EffectEntities[Effects[idx].ef.Teleporter.entity_index[i]];

					ent->origin[0] += (frametime / smoketime) * Effects[idx].ef.Teleporter.velocity[i][0];
					ent->origin[1] += (frametime / smoketime) * Effects[idx].ef.Teleporter.velocity[i][1];
					ent->origin[2] += (frametime / smoketime) * Effects[idx].ef.Teleporter.velocity[i][2];
					ent->frame = cur_frame;

					CL_LinkEffectEntity(ent);
				}
			}
			break;

		case CE_TELEPORTERBODY:
			{
				entity_t *ent;

				Effects[idx].ef.Teleporter.time_amount += frametime;

				ent = &EffectEntities[Effects[idx].ef.Teleporter.entity_index[0]];
				while (Effects[idx].ef.Teleporter.time_amount >= HX_FRAME_TIME)
				{
					ent->scale -= 15;
					Effects[idx].ef.Teleporter.time_amount -= HX_FRAME_TIME;
				}

				ent->angles[1] += 45;

				if (ent->scale <= 10)
				{
					CL_FreeEffect(idx);
				}
				else
				{
					CL_LinkEffectEntity(ent);
				}
			}
			break;

		case CE_BONESHARD:
		case CE_BONESHRAPNEL:
			Effects[idx].ef.Missile.time_amount += frametime;
			if (Effects[idx].ef.Missile.entity_index >= 0)
			{
				entity_t *ent = &EffectEntities[Effects[idx].ef.Missile.entity_index];
				ent->angles[0] += frametime * Effects[idx].ef.Missile.avelocity[0];
				ent->angles[1] += frametime * Effects[idx].ef.Missile.avelocity[1];
				ent->angles[2] += frametime * Effects[idx].ef.Missile.avelocity[2];
				ent->origin[0] += frametime * Effects[idx].ef.Missile.velocity[0];
				ent->origin[1] += frametime * Effects[idx].ef.Missile.velocity[1];
				ent->origin[2] += frametime * Effects[idx].ef.Missile.velocity[2];

				CL_LinkEffectEntity(ent);
			}
			if (Effects[idx].ef.Missile.time_amount > 5.0f)
				CL_FreeEffect(idx);
			break;

		case CE_CHUNK:
			Effects[idx].ef.Chunk.time_amount -= frametime;
			if (Effects[idx].ef.Chunk.time_amount < 0)
				CL_FreeEffect(idx);
			else
			{
				// Update chunk positions with gravity
				for (int i = 0; i < Effects[idx].ef.Chunk.numChunks; i++)
				{
					if (Effects[idx].ef.Chunk.entity_index[i] >= 0)
					{
						entity_t *ent = &EffectEntities[Effects[idx].ef.Chunk.entity_index[i]];
						ent->origin[0] += frametime * Effects[idx].ef.Chunk.velocity[i][0];
						ent->origin[1] += frametime * Effects[idx].ef.Chunk.velocity[i][1];
						ent->origin[2] += frametime * Effects[idx].ef.Chunk.velocity[i][2];
						Effects[idx].ef.Chunk.velocity[i][2] -= frametime * 500; // gravity

						// Scale down near end of lifetime
						if (Effects[idx].ef.Chunk.time_amount < frametime * 3)
							ent->scale = (int)(ent->scale * 0.7f);

						CL_LinkEffectEntity(ent);
					}
				}
			}
			break;
		}
	}
}

/*
===============
NewEffectEntity

Allocate an effect entity from the pool
===============
*/
static int NewEffectEntity(void)
{
	int counter;

	if (EffectEntityCount >= MAX_EFFECT_ENTITIES)
		return -1;

	for (counter = 0; counter < MAX_EFFECT_ENTITIES; counter++)
	{
		if (!EntityUsed[counter])
			break;
	}

	if (counter >= MAX_EFFECT_ENTITIES)
		return -1;

	EntityUsed[counter] = true;
	EffectEntityCount++;
	memset(&EffectEntities[counter], 0, sizeof(entity_t));

	return counter;
}

/*
===============
FreeEffectEntity

Return an effect entity to the pool
===============
*/
static void FreeEffectEntity(int idx)
{
	if (idx >= 0 && idx < MAX_EFFECT_ENTITIES && EntityUsed[idx])
	{
		EntityUsed[idx] = false;
		EffectEntityCount--;
	}
}

/*
===============
CL_LinkEffectEntity

Add an effect entity to the visible entity list
===============
*/
static void CL_LinkEffectEntity(entity_t *ent)
{
	if (cl_numvisedicts < MAX_VISEDICTS)
	{
		cl_visedicts[cl_numvisedicts++] = ent;
	}
}
