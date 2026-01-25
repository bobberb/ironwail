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

// sv_inventory_hexen2.c -- Server-side Hexen II inventory/stats sync
// Implements svc_h2_update_inv delta compression protocol
// Adapted from uhexen2 sv_main.c

#include "quakedef.h"
#include "protocol_hexen2.h"

/*
================
SV_H2_WriteInventoryUpdate

Send H2 inventory/stats updates using SC1/SC2 delta compression.
Called from SV_WriteClientdataToMessage when in hexen2_mode.

This function compares current entity values against stored old values
and sends only the changed fields using the SC1/SC2 bitmask protocol.
================
*/
void SV_H2_WriteInventoryUpdate(client_t *client, edict_t *ent, sizebuf_t *msg)
{
	unsigned int sc1 = 0, sc2 = 0;
	int test;
	int i;

	// H2 entity field accessors via GetEdictFieldValue
	eval_t *val;

	float health = ent->v.health;
	float bluemana = 0, greenmana = 0;
	float weapon = ent->v.weapon;
	int artifact_active = 0, rings_active = 0;

	// Get H2-specific fields
	val = GetEdictFieldValueByName(ent, "bluemana");
	if (val) bluemana = val->_float;

	val = GetEdictFieldValueByName(ent, "greenmana");
	if (val) greenmana = val->_float;

	val = GetEdictFieldValueByName(ent, "artifact_active");
	if (val) artifact_active = (int)val->_float;

	val = GetEdictFieldValueByName(ent, "rings_active");
	if (val) rings_active = (int)val->_float;

	// Compare to old values and build SC1/SC2 bitmasks
	// We use the oldstats arrays for tracking changes

	// Health (SC1 bit 0)
	if ((int)health != client->oldstats_i[0])
	{
		sc1 |= H2_SC1_HEALTH;
		client->oldstats_i[0] = (int)health;
	}

	// Weapon (SC1 bit 6)
	if ((int)weapon != client->oldstats_i[6])
	{
		sc1 |= H2_SC1_WEAPON;
		client->oldstats_i[6] = (int)weapon;
	}

	// Blue mana -> oldstats_i[7] (SC1 bit 7)
	if ((int)bluemana != client->oldstats_i[7])
	{
		sc1 |= H2_SC1_BLUEMANA;
		client->oldstats_i[7] = (int)bluemana;
	}

	// Green mana -> oldstats_i[8] (SC1 bit 8)
	if ((int)greenmana != client->oldstats_i[8])
	{
		sc1 |= H2_SC1_GREENMANA;
		client->oldstats_i[8] = (int)greenmana;
	}

	// Artifact active -> oldstats_i[26] (SC1 bit 26)
	if (artifact_active != client->oldstats_i[26])
	{
		sc1 |= H2_SC1_ARTIFACT_ACTIVE;
		client->oldstats_i[26] = artifact_active;
	}

	// Rings active -> oldstats_i[31] (SC1 bit 31)
	if (rings_active != client->oldstats_i[31])
	{
		sc1 |= H2_SC1_RINGS_ACTIVE;
		client->oldstats_i[31] = rings_active;
	}

	// Get artifact counts from entity
	static const char *cnt_fields[] = {
		"cnt_torch", "cnt_h_boost", "cnt_sh_boost", "cnt_mana_boost",
		"cnt_teleport", "cnt_tome", "cnt_summon", "cnt_invisibility",
		"cnt_glyph", "cnt_haste", "cnt_blast", "cnt_polymorph",
		"cnt_flight", "cnt_cubeofforce", "cnt_invincibility"
	};
	static const unsigned int cnt_bits[] = {
		H2_SC1_CNT_TORCH, H2_SC1_CNT_H_BOOST, H2_SC1_CNT_SH_BOOST, H2_SC1_CNT_MANA_BOOST,
		H2_SC1_CNT_TELEPORT, H2_SC1_CNT_TOME, H2_SC1_CNT_SUMMON, H2_SC1_CNT_INVISIBILITY,
		H2_SC1_CNT_GLYPH, H2_SC1_CNT_HASTE, H2_SC1_CNT_BLAST, H2_SC1_CNT_POLYMORPH,
		H2_SC1_CNT_FLIGHT, H2_SC1_CNT_CUBEOFFORCE, H2_SC1_CNT_INVINCIBILITY
	};

	int cnt_values[15];
	for (i = 0; i < 15; i++)
	{
		val = GetEdictFieldValueByName(ent, cnt_fields[i]);
		cnt_values[i] = val ? (int)val->_float : 0;

		// Use oldstats_i[40+i] for artifact counts
		if (cnt_values[i] != client->oldstats_i[40 + i])
		{
			sc1 |= cnt_bits[i];
			client->oldstats_i[40 + i] = cnt_values[i];
		}
	}

	// Nothing changed?
	if (!sc1 && !sc2)
		return;

	// Write the update message
	MSG_WriteByte(msg, svc_h2_update_inv);

	// Build test byte indicating which SC1/SC2 bytes are present
	test = 0;
	if (sc1 & 0x000000ff) test |= 1;
	if (sc1 & 0x0000ff00) test |= 2;
	if (sc1 & 0x00ff0000) test |= 4;
	if (sc1 & 0xff000000) test |= 8;
	if (sc2 & 0x000000ff) test |= 16;
	if (sc2 & 0x0000ff00) test |= 32;
	if (sc2 & 0x00ff0000) test |= 64;
	if (sc2 & 0xff000000) test |= 128;

	MSG_WriteByte(msg, test);

	// Write SC1/SC2 bytes
	if (test & 1)   MSG_WriteByte(msg, sc1 & 0xff);
	if (test & 2)   MSG_WriteByte(msg, (sc1 >> 8) & 0xff);
	if (test & 4)   MSG_WriteByte(msg, (sc1 >> 16) & 0xff);
	if (test & 8)   MSG_WriteByte(msg, (sc1 >> 24) & 0xff);
	if (test & 16)  MSG_WriteByte(msg, sc2 & 0xff);
	if (test & 32)  MSG_WriteByte(msg, (sc2 >> 8) & 0xff);
	if (test & 64)  MSG_WriteByte(msg, (sc2 >> 16) & 0xff);
	if (test & 128) MSG_WriteByte(msg, (sc2 >> 24) & 0xff);

	// Write changed SC1 values
	if (sc1 & H2_SC1_HEALTH)
		MSG_WriteShort(msg, (int)health);
	if (sc1 & H2_SC1_WEAPON)
		MSG_WriteByte(msg, (int)weapon);
	if (sc1 & H2_SC1_BLUEMANA)
		MSG_WriteByte(msg, (int)bluemana);
	if (sc1 & H2_SC1_GREENMANA)
		MSG_WriteByte(msg, (int)greenmana);

	// Artifact counts
	if (sc1 & H2_SC1_CNT_TORCH)
		MSG_WriteByte(msg, cnt_values[0]);
	if (sc1 & H2_SC1_CNT_H_BOOST)
		MSG_WriteByte(msg, cnt_values[1]);
	if (sc1 & H2_SC1_CNT_SH_BOOST)
		MSG_WriteByte(msg, cnt_values[2]);
	if (sc1 & H2_SC1_CNT_MANA_BOOST)
		MSG_WriteByte(msg, cnt_values[3]);
	if (sc1 & H2_SC1_CNT_TELEPORT)
		MSG_WriteByte(msg, cnt_values[4]);
	if (sc1 & H2_SC1_CNT_TOME)
		MSG_WriteByte(msg, cnt_values[5]);
	if (sc1 & H2_SC1_CNT_SUMMON)
		MSG_WriteByte(msg, cnt_values[6]);
	if (sc1 & H2_SC1_CNT_INVISIBILITY)
		MSG_WriteByte(msg, cnt_values[7]);
	if (sc1 & H2_SC1_CNT_GLYPH)
		MSG_WriteByte(msg, cnt_values[8]);
	if (sc1 & H2_SC1_CNT_HASTE)
		MSG_WriteByte(msg, cnt_values[9]);
	if (sc1 & H2_SC1_CNT_BLAST)
		MSG_WriteByte(msg, cnt_values[10]);
	if (sc1 & H2_SC1_CNT_POLYMORPH)
		MSG_WriteByte(msg, cnt_values[11]);
	if (sc1 & H2_SC1_CNT_FLIGHT)
		MSG_WriteByte(msg, cnt_values[12]);
	if (sc1 & H2_SC1_CNT_CUBEOFFORCE)
		MSG_WriteByte(msg, cnt_values[13]);
	if (sc1 & H2_SC1_CNT_INVINCIBILITY)
		MSG_WriteByte(msg, cnt_values[14]);

	if (sc1 & H2_SC1_ARTIFACT_ACTIVE)
		MSG_WriteFloat(msg, (float)artifact_active);
	if (sc1 & H2_SC1_RINGS_ACTIVE)
		MSG_WriteFloat(msg, (float)rings_active);
}
