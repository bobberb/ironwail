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

	float health = ENT_HEALTH(ent);
	float bluemana = 0, greenmana = 0;
	float weapon = ENT_FLOAT(ent, weapon);
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

	// Get individual armor values
	int armor_amulet = 0, armor_bracer = 0, armor_breastplate = 0, armor_helmet = 0;
	val = GetEdictFieldValueByName(ent, "armor_amulet");
	if (val) armor_amulet = (int)val->_float;
	val = GetEdictFieldValueByName(ent, "armor_bracer");
	if (val) armor_bracer = (int)val->_float;
	val = GetEdictFieldValueByName(ent, "armor_breastplate");
	if (val) armor_breastplate = (int)val->_float;
	val = GetEdictFieldValueByName(ent, "armor_helmet");
	if (val) armor_helmet = (int)val->_float;

	// Get ring time values
	int ring_flight = 0, ring_water = 0, ring_turning = 0, ring_regen = 0;
	val = GetEdictFieldValueByName(ent, "ring_flight");
	if (val) ring_flight = (int)val->_float;
	val = GetEdictFieldValueByName(ent, "ring_water");
	if (val) ring_water = (int)val->_float;
	val = GetEdictFieldValueByName(ent, "ring_turning");
	if (val) ring_turning = (int)val->_float;
	val = GetEdictFieldValueByName(ent, "ring_regeneration");
	if (val) ring_regen = (int)val->_float;

	// Get max health and max mana
	int max_health = 0, max_mana = 0;
	val = GetEdictFieldValueByName(ent, "max_health");
	if (val) max_health = (int)val->_float;
	val = GetEdictFieldValueByName(ent, "max_mana");
	if (val) max_mana = (int)val->_float;

	// Get puzzle piece names
	const char *puzzle[8] = {NULL};
	val = GetEdictFieldValueByName(ent, "puzzle_inv1");
	if (val) puzzle[0] = PR_GetString(val->string);
	val = GetEdictFieldValueByName(ent, "puzzle_inv2");
	if (val) puzzle[1] = PR_GetString(val->string);
	val = GetEdictFieldValueByName(ent, "puzzle_inv3");
	if (val) puzzle[2] = PR_GetString(val->string);
	val = GetEdictFieldValueByName(ent, "puzzle_inv4");
	if (val) puzzle[3] = PR_GetString(val->string);
	val = GetEdictFieldValueByName(ent, "puzzle_inv5");
	if (val) puzzle[4] = PR_GetString(val->string);
	val = GetEdictFieldValueByName(ent, "puzzle_inv6");
	if (val) puzzle[5] = PR_GetString(val->string);
	val = GetEdictFieldValueByName(ent, "puzzle_inv7");
	if (val) puzzle[6] = PR_GetString(val->string);
	val = GetEdictFieldValueByName(ent, "puzzle_inv8");
	if (val) puzzle[7] = PR_GetString(val->string);

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

	// Armor pieces -> SC2 bits 1-4, oldstats_i[60-63]
	if (armor_amulet != client->oldstats_i[60])
	{
		sc2 |= H2_SC2_AMULET;
		client->oldstats_i[60] = armor_amulet;
	}
	if (armor_bracer != client->oldstats_i[61])
	{
		sc2 |= H2_SC2_BRACER;
		client->oldstats_i[61] = armor_bracer;
	}
	if (armor_breastplate != client->oldstats_i[62])
	{
		sc2 |= H2_SC2_BREASTPLATE;
		client->oldstats_i[62] = armor_breastplate;
	}
	if (armor_helmet != client->oldstats_i[63])
	{
		sc2 |= H2_SC2_HELMET;
		client->oldstats_i[63] = armor_helmet;
	}

	// Ring times -> SC2 bits 5-8, oldstats_i[64-67]
	if (ring_flight != client->oldstats_i[64])
	{
		sc2 |= H2_SC2_FLIGHT_T;
		client->oldstats_i[64] = ring_flight;
	}
	if (ring_water != client->oldstats_i[65])
	{
		sc2 |= H2_SC2_WATER_T;
		client->oldstats_i[65] = ring_water;
	}
	if (ring_turning != client->oldstats_i[66])
	{
		sc2 |= H2_SC2_TURNING_T;
		client->oldstats_i[66] = ring_turning;
	}
	if (ring_regen != client->oldstats_i[67])
	{
		sc2 |= H2_SC2_REGEN_T;
		client->oldstats_i[67] = ring_regen;
	}

	// Max health/mana -> SC2 bits 19-20, oldstats_i[68-69]
	if (max_health != client->oldstats_i[68])
	{
		sc2 |= H2_SC2_MAXHEALTH;
		client->oldstats_i[68] = max_health;
	}
	if (max_mana != client->oldstats_i[69])
	{
		sc2 |= H2_SC2_MAXMANA;
		client->oldstats_i[69] = max_mana;
	}

	// Puzzle pieces -> SC2 bits 11-18, oldstats_i[70-77]
	// Use simple hash for string comparison
	for (i = 0; i < 8; i++)
	{
		const char *cur = puzzle[i] ? puzzle[i] : "";
		// Simple string hash: sum of chars
		int hash = 0;
		const char *p = cur;
		while (*p) hash += (unsigned char)*p++;
		hash = hash * 31 + (int)strlen(cur);  // Include length for uniqueness

		if (hash != client->oldstats_i[70 + i])
		{
			sc2 |= (H2_SC2_PUZZLE1 << i);
			client->oldstats_i[70 + i] = hash;
		}
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

	// Write SC2 values in protocol order
	// Armor pieces (bits 1-4)
	if (sc2 & H2_SC2_AMULET)
		MSG_WriteByte(msg, armor_amulet);
	if (sc2 & H2_SC2_BRACER)
		MSG_WriteByte(msg, armor_bracer);
	if (sc2 & H2_SC2_BREASTPLATE)
		MSG_WriteByte(msg, armor_breastplate);
	if (sc2 & H2_SC2_HELMET)
		MSG_WriteByte(msg, armor_helmet);

	// Ring times (bits 5-8)
	if (sc2 & H2_SC2_FLIGHT_T)
		MSG_WriteByte(msg, ring_flight);
	if (sc2 & H2_SC2_WATER_T)
		MSG_WriteByte(msg, ring_water);
	if (sc2 & H2_SC2_TURNING_T)
		MSG_WriteByte(msg, ring_turning);
	if (sc2 & H2_SC2_REGEN_T)
		MSG_WriteByte(msg, ring_regen);

	// Puzzle pieces (bits 11-18)
	if (sc2 & H2_SC2_PUZZLE1)
		MSG_WriteString(msg, puzzle[0] ? puzzle[0] : "");
	if (sc2 & H2_SC2_PUZZLE2)
		MSG_WriteString(msg, puzzle[1] ? puzzle[1] : "");
	if (sc2 & H2_SC2_PUZZLE3)
		MSG_WriteString(msg, puzzle[2] ? puzzle[2] : "");
	if (sc2 & H2_SC2_PUZZLE4)
		MSG_WriteString(msg, puzzle[3] ? puzzle[3] : "");
	if (sc2 & H2_SC2_PUZZLE5)
		MSG_WriteString(msg, puzzle[4] ? puzzle[4] : "");
	if (sc2 & H2_SC2_PUZZLE6)
		MSG_WriteString(msg, puzzle[5] ? puzzle[5] : "");
	if (sc2 & H2_SC2_PUZZLE7)
		MSG_WriteString(msg, puzzle[6] ? puzzle[6] : "");
	if (sc2 & H2_SC2_PUZZLE8)
		MSG_WriteString(msg, puzzle[7] ? puzzle[7] : "");

	// Max health/mana (bits 19-20)
	if (sc2 & H2_SC2_MAXHEALTH)
		MSG_WriteShort(msg, max_health);
	if (sc2 & H2_SC2_MAXMANA)
		MSG_WriteByte(msg, max_mana);
}
