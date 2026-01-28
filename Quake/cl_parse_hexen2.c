/*
Copyright (C) 1996-1997 Id Software, Inc.
Copyright (C) 1997-1998 Raven Software Corp.
Copyright (C) 2010-2014 QuakeSpasm developers
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

#include "quakedef.h"
#include "protocol_hexen2.h"
#include "sbar_hexen2.h"
#include "bgmusic.h"
#include "host_string.h"

// Deathmatch: current king of the hill player (-1 = none)
int h2_kingofhill = -1;

/*
================
CL_ParseUpdateClass

Parse svc_h2_updateclass message
Updates player class for scoreboard/HUD
================
*/
void CL_ParseUpdateClass(void)
{
	int slot, playerclass;

	slot = MSG_ReadByte();
	playerclass = MSG_ReadByte();

	if (slot >= cl.maxclients)
		Host_Error("CL_ParseUpdateClass: slot >= cl.maxclients");

	cl.scores[slot].playerclass = playerclass;

	// If this is our player, update client state
	if (slot == cl.viewentity - 1)
		cl.playerclass = playerclass;
}

/*
================
CL_ParseMidiName

Parse svc_h2_midi_name message
Plays background music, searching for OGG/MP3/FLAC/WAV replacements in music/
================
*/
void CL_ParseMidiName(void)
{
	const char *midi_name;
	const char *fname;
	char basename[MAX_QPATH];

	midi_name = MSG_ReadString();

	if (!midi_name || !*midi_name)
	{
		BGM_Stop();
		return;
	}

	/* Extract just the filename (strip any path like "midi/") */
	fname = COM_SkipPath(midi_name);

	/* Strip extension to get base name */
	COM_StripExtension(fname, basename, sizeof(basename));

	Con_DPrintf("Playing music: %s\n", basename);

	/* BGM_Play searches for music/<basename>.ogg/mp3/flac/wav/etc */
	BGM_Play(basename);
}

/*
================
CL_ParseParticleExplosion

Parse svc_h2_particle_explosion message
Material-aware particle explosion
================
*/
void CL_ParseParticleExplosion(void)
{
	vec3_t org;
	int color, radius, counter;

	org[0] = MSG_ReadCoord(cl.protocolflags);
	org[1] = MSG_ReadCoord(cl.protocolflags);
	org[2] = MSG_ReadCoord(cl.protocolflags);
	color = MSG_ReadByte();
	radius = MSG_ReadByte();
	counter = MSG_ReadByte();

	// H2 material-aware colored particle explosion
	R_ColoredParticleExplosion(org, color, radius, counter);
}

/*
================
CL_ParseSetViewTint

Parse svc_h2_set_view_tint message
Sets view color tint on the view weapon model
================
*/
void CL_ParseSetViewTint(void)
{
	int tint_color;

	tint_color = MSG_ReadByte();

	// Set the colorshade on the view entity (weapon model)
	// This is used for special effects like powerups that tint the weapon
	cl.viewent.colorshade = tint_color;

	Con_DPrintf("View tint set to: %d\n", tint_color);
}

/*
================
CL_ParseUpdateInventory

Parse svc_h2_update_inv message
Updates player stats and inventory using SC1/SC2 bitfields
================
*/
void CL_ParseUpdateInventory(void)
{
	int test;
	unsigned int sc1 = 0, sc2 = 0;

	// Read which stat bytes are present
	test = MSG_ReadByte();
	if (test & 1)
		sc1 |= MSG_ReadByte();
	if (test & 2)
		sc1 |= MSG_ReadByte() << 8;
	if (test & 4)
		sc1 |= MSG_ReadByte() << 16;
	if (test & 8)
		sc1 |= MSG_ReadByte() << 24;
	if (test & 16)
		sc2 |= MSG_ReadByte();
	if (test & 32)
		sc2 |= MSG_ReadByte() << 8;
	if (test & 64)
		sc2 |= MSG_ReadByte() << 16;
	if (test & 128)
		sc2 |= MSG_ReadByte() << 24;

	// Parse SC1 stats
	if (sc1 & H2_SC1_HEALTH)
		cl.stats[STAT_HEALTH] = MSG_ReadShort();
	if (sc1 & H2_SC1_LEVEL)
		MSG_ReadByte();  // Player level (not used in stats)
	if (sc1 & H2_SC1_INTELLIGENCE)
		MSG_ReadByte();  // Intelligence stat
	if (sc1 & H2_SC1_WISDOM)
		MSG_ReadByte();  // Wisdom stat
	if (sc1 & H2_SC1_STRENGTH)
		MSG_ReadByte();  // Strength stat
	if (sc1 & H2_SC1_DEXTERITY)
		MSG_ReadByte();  // Dexterity stat
	if (sc1 & H2_SC1_WEAPON)
		cl.stats[STAT_ACTIVEWEAPON] = MSG_ReadByte();
	if (sc1 & H2_SC1_BLUEMANA)
		cl.stats[STAT_SHELLS] = MSG_ReadByte();  // Blue mana -> STAT_SHELLS
	if (sc1 & H2_SC1_GREENMANA)
		cl.stats[STAT_NAILS] = MSG_ReadByte();   // Green mana -> STAT_NAILS
	if (sc1 & H2_SC1_EXPERIENCE)
		MSG_ReadLong();  // Experience points

	// Artifact counts - track acquisition time for blinking effect
#define UPDATE_INV_CNT(flag, idx) \
	if (sc1 & flag) { \
		int newcnt = MSG_ReadByte(); \
		if (newcnt > cl.inv_cnt[idx]) \
			cl.inv_gettime[idx] = cl.time; \
		cl.inv_cnt[idx] = newcnt; \
	}
	UPDATE_INV_CNT(H2_SC1_CNT_TORCH, H2_INV_TORCH);
	UPDATE_INV_CNT(H2_SC1_CNT_H_BOOST, H2_INV_HP_BOOST);
	UPDATE_INV_CNT(H2_SC1_CNT_SH_BOOST, H2_INV_SUPER_HP_BOOST);
	UPDATE_INV_CNT(H2_SC1_CNT_MANA_BOOST, H2_INV_MANA_BOOST);
	UPDATE_INV_CNT(H2_SC1_CNT_TELEPORT, H2_INV_TELEPORT);
	UPDATE_INV_CNT(H2_SC1_CNT_TOME, H2_INV_TOME);
	UPDATE_INV_CNT(H2_SC1_CNT_SUMMON, H2_INV_SUMMON);
	UPDATE_INV_CNT(H2_SC1_CNT_INVISIBILITY, H2_INV_INVISIBILITY);
	UPDATE_INV_CNT(H2_SC1_CNT_GLYPH, H2_INV_GLYPH);
	UPDATE_INV_CNT(H2_SC1_CNT_HASTE, H2_INV_HASTE);
	UPDATE_INV_CNT(H2_SC1_CNT_BLAST, H2_INV_BLAST);
	UPDATE_INV_CNT(H2_SC1_CNT_POLYMORPH, H2_INV_POLYMORPH);
	UPDATE_INV_CNT(H2_SC1_CNT_FLIGHT, H2_INV_FLIGHT);
	UPDATE_INV_CNT(H2_SC1_CNT_CUBEOFFORCE, H2_INV_CUBEOFFORCE);
	UPDATE_INV_CNT(H2_SC1_CNT_INVINCIBILITY, H2_INV_INVINCIBILITY);
#undef UPDATE_INV_CNT

	if (sc1 & H2_SC1_ARTIFACT_ACTIVE)
		cl.artifact_active = (int)MSG_ReadFloat();
	if (sc1 & H2_SC1_ARTIFACT_LOW)
		MSG_ReadFloat();  // Artifact low warning
	if (sc1 & H2_SC1_MOVETYPE)
		cl.movetype = MSG_ReadByte();  // Movement type (affects drift and bob)
	if (sc1 & H2_SC1_CAMERAMODE)
		cl.cameramode = MSG_ReadByte();
	if (sc1 & H2_SC1_HASTED)
		cl.hasted = MSG_ReadFloat();  // Haste movement multiplier
	if (sc1 & H2_SC1_INVENTORY)
		cl.inv_selected = MSG_ReadByte();
	if (sc1 & H2_SC1_RINGS_ACTIVE)
		cl.rings_active = (int)MSG_ReadFloat();

	// Parse SC2 stats
	if (sc2 & H2_SC2_RINGS_LOW)
		MSG_ReadFloat();  // Rings low warning
	if (sc2 & H2_SC2_AMULET)
		cl.armor_amulet = MSG_ReadByte();
	if (sc2 & H2_SC2_BRACER)
		cl.armor_bracer = MSG_ReadByte();
	if (sc2 & H2_SC2_BREASTPLATE)
		cl.armor_breastplate = MSG_ReadByte();
	if (sc2 & H2_SC2_HELMET)
		cl.armor_helmet = MSG_ReadByte();
	if (sc2 & H2_SC2_FLIGHT_T)
		cl.ring_flight = MSG_ReadByte();
	if (sc2 & H2_SC2_WATER_T)
		cl.ring_water = MSG_ReadByte();
	if (sc2 & H2_SC2_TURNING_T)
		cl.ring_turning = MSG_ReadByte();
	if (sc2 & H2_SC2_REGEN_T)
		cl.ring_regeneration = MSG_ReadByte();
	if (sc2 & H2_SC2_HASTE_T)
		MSG_ReadFloat();  // Haste time remaining
	if (sc2 & H2_SC2_TOME_T)
		MSG_ReadFloat();  // Tome time remaining

	// Puzzle pieces (truncate to 9 chars + null terminator)
	if (sc2 & H2_SC2_PUZZLE1)
		q_strlcpy(cl.puzzle_pieces[0], MSG_ReadString(), sizeof(cl.puzzle_pieces[0]));
	if (sc2 & H2_SC2_PUZZLE2)
		q_strlcpy(cl.puzzle_pieces[1], MSG_ReadString(), sizeof(cl.puzzle_pieces[1]));
	if (sc2 & H2_SC2_PUZZLE3)
		q_strlcpy(cl.puzzle_pieces[2], MSG_ReadString(), sizeof(cl.puzzle_pieces[2]));
	if (sc2 & H2_SC2_PUZZLE4)
		q_strlcpy(cl.puzzle_pieces[3], MSG_ReadString(), sizeof(cl.puzzle_pieces[3]));
	if (sc2 & H2_SC2_PUZZLE5)
		q_strlcpy(cl.puzzle_pieces[4], MSG_ReadString(), sizeof(cl.puzzle_pieces[4]));
	if (sc2 & H2_SC2_PUZZLE6)
		q_strlcpy(cl.puzzle_pieces[5], MSG_ReadString(), sizeof(cl.puzzle_pieces[5]));
	if (sc2 & H2_SC2_PUZZLE7)
		q_strlcpy(cl.puzzle_pieces[6], MSG_ReadString(), sizeof(cl.puzzle_pieces[6]));
	if (sc2 & H2_SC2_PUZZLE8)
		q_strlcpy(cl.puzzle_pieces[7], MSG_ReadString(), sizeof(cl.puzzle_pieces[7]));

	if (sc2 & H2_SC2_MAXHEALTH)
		cl.max_health = MSG_ReadShort();
	if (sc2 & H2_SC2_MAXMANA)
		cl.max_mana = MSG_ReadByte();
	if (sc2 & H2_SC2_FLAGS)
		MSG_ReadFloat();  // Player flags

	// Mission pack objectives (protocol 19+)
	if (sc2 & H2_SC2_OBJ)
		cl.info_mask = MSG_ReadLong();
	if (sc2 & H2_SC2_OBJ2)
		cl.info_mask2 = MSG_ReadLong();

	// Rebuild inventory order if any artifact counts changed
	if (sc1 & (H2_SC1_CNT_TORCH | H2_SC1_CNT_H_BOOST | H2_SC1_CNT_SH_BOOST |
			   H2_SC1_CNT_MANA_BOOST | H2_SC1_CNT_TELEPORT | H2_SC1_CNT_TOME |
			   H2_SC1_CNT_SUMMON | H2_SC1_CNT_INVISIBILITY | H2_SC1_CNT_GLYPH |
			   H2_SC1_CNT_HASTE | H2_SC1_CNT_BLAST | H2_SC1_CNT_POLYMORPH |
			   H2_SC1_CNT_FLIGHT | H2_SC1_CNT_CUBEOFFORCE | H2_SC1_CNT_INVINCIBILITY))
	{
		Sbar_H2_InvChanged();
	}
}

/*
================
CL_ParsePlaque

Parse svc_h2_plaque message
Display a plaque (story text overlay)
================
*/
void CL_ParsePlaque(void)
{
	int plaque_id;
	const char *plaque_text;

	plaque_id = MSG_ReadShort();

	// Look up plaque text from strings.txt (1-indexed in protocol)
	if (plaque_id > 0)
	{
		plaque_text = Host_GetString(plaque_id - 1);
		if (plaque_text && plaque_text[0])
		{
			// Display as center print for now
			// TODO: Could add dedicated plaque rendering with H2 styling
			SCR_CenterPrint(plaque_text);
		}
	}
	else
	{
		// Clear plaque display
		SCR_CenterPrint("");
	}
}

/*
================
CL_ParseParticle2

Parse svc_h2_particle2 message
Extended particle with min/max bounds
================
*/
void CL_ParseParticle2(void)
{
	vec3_t org, dmin, dmax;
	int i, color, count, effect;

	for (i = 0; i < 3; i++)
		org[i] = MSG_ReadCoord(cl.protocolflags);
	for (i = 0; i < 3; i++)
		dmin[i] = MSG_ReadFloat();
	for (i = 0; i < 3; i++)
		dmax[i] = MSG_ReadFloat();
	color = MSG_ReadShort();
	count = MSG_ReadByte();
	effect = MSG_ReadByte();

	R_RunParticleEffect2(org, dmin, dmax, color, (ptype_t)effect, count);
}

/*
================
CL_ParseParticle3

Parse svc_h2_particle3 message
Extended particle with box bounds
================
*/
void CL_ParseParticle3(void)
{
	vec3_t org, box;
	int i, color, count, effect;

	for (i = 0; i < 3; i++)
		org[i] = MSG_ReadCoord(cl.protocolflags);
	for (i = 0; i < 3; i++)
		box[i] = MSG_ReadByte();
	color = MSG_ReadShort();
	count = MSG_ReadByte();
	effect = MSG_ReadByte();

	R_RunParticleEffect3(org, box, color, (ptype_t)effect, count);
}

/*
================
CL_ParseParticle4

Parse svc_h2_particle4 message
Extended particle with radius
================
*/
void CL_ParseParticle4(void)
{
	vec3_t org;
	int i, radius, color, count, effect;

	for (i = 0; i < 3; i++)
		org[i] = MSG_ReadCoord(cl.protocolflags);
	radius = MSG_ReadByte();
	color = MSG_ReadShort();
	count = MSG_ReadByte();
	effect = MSG_ReadByte();

	R_RunParticleEffect4(org, (float)radius, color, (ptype_t)effect, count);
}

/*
================
CL_ParseRainEffect

Parse svc_h2_raineffect message
Rain/snow weather effect
================
*/
void CL_ParseRainEffect(void)
{
	vec3_t org, e_size;
	int x_dir, y_dir, color, count;

	// Parse rain effect message (matches PF_h2_rain_go server format)
	org[0] = MSG_ReadCoord(cl.protocolflags);
	org[1] = MSG_ReadCoord(cl.protocolflags);
	org[2] = MSG_ReadCoord(cl.protocolflags);
	e_size[0] = MSG_ReadCoord(cl.protocolflags);
	e_size[1] = MSG_ReadCoord(cl.protocolflags);
	e_size[2] = MSG_ReadCoord(cl.protocolflags);
	x_dir = MSG_ReadAngle(cl.protocolflags);
	y_dir = MSG_ReadAngle(cl.protocolflags);
	color = MSG_ReadShort();
	count = MSG_ReadShort();

	// Create rain particles
	R_RainEffect(org, e_size, x_dir, y_dir, color, count);
}

/*
================
CL_ParseSoundUpdatePos

Parse svc_h2_sound_update_pos message
Update position of a playing sound (for moving sound sources)
================
*/
void CL_ParseSoundUpdatePos(void)
{
	vec3_t pos;
	int channel, ent_num;

	/* Channel short is packed: upper bits = entity, lower 3 bits = channel */
	channel = MSG_ReadShort();
	ent_num = channel >> 3;
	channel &= 7;

	pos[0] = MSG_ReadCoord(cl.protocolflags);
	pos[1] = MSG_ReadCoord(cl.protocolflags);
	pos[2] = MSG_ReadCoord(cl.protocolflags);

	if (ent_num > MAX_EDICTS)
	{
		Con_Warning("svc_sound_update_pos: ent = %i\n", ent_num);
		return;
	}

	S_UpdateSoundPos(ent_num, channel, pos);
}

/*
================
CL_ParseModName

Parse svc_h2_mod_name message (UQE 1.13)
Plays MOD/tracker music (IT/S3M/XM/MOD formats)
================
*/
void CL_ParseModName(void)
{
	const char *mod_name;
	const char *fname;
	char basename[MAX_QPATH];

	mod_name = MSG_ReadString();

	if (!mod_name || !*mod_name)
	{
		BGM_Stop();
		return;
	}

	/* Extract just the filename (strip any path) */
	fname = COM_SkipPath(mod_name);

	/* Strip extension to get base name */
	COM_StripExtension(fname, basename, sizeof(basename));

	Con_DPrintf("Playing MOD music: %s\n", basename);

	/* BGM_Play searches for music/<basename>.it/s3m/xm/mod/etc */
	BGM_Play(basename);
}

/*
================
CL_ParseSkybox

Parse svc_h2_skybox message (UQE 1.13)
Sets the skybox
================
*/
void CL_ParseSkybox(void)
{
	const char *skybox_name;

	skybox_name = MSG_ReadString();

	// Load the skybox using Ironwail's existing skybox system
	if (skybox_name[0])
	{
		Sky_LoadSkyBox(skybox_name);
		Con_DPrintf("Skybox loaded: %s\n", skybox_name);
	}
}

/*
================
CL_ParseCutscene

Parse svc_h2_cutscene message
Sets intermission mode 3 and displays center message
================
*/
void CL_ParseCutscene(void)
{
	cl.intermission = 3;
	cl.completed_time = cl.time;
	vid.recalc_refdef = true;
	SCR_CenterPrint(MSG_ReadString());
}

/*
================
CL_ParseSetViewFlags

Parse svc_h2_set_view_flags message
ORs flags into viewmodel's drawflags
================
*/
void CL_ParseSetViewFlags(void)
{
	cl.viewent.drawflags |= MSG_ReadByte();
}

/*
================
CL_ParseClearViewFlags

Parse svc_h2_clear_view_flags message
ANDs out flags from viewmodel's drawflags
================
*/
void CL_ParseClearViewFlags(void)
{
	cl.viewent.drawflags &= ~MSG_ReadByte();
}

/*
================
CL_ParseToggleStatbar

Parse svc_h2_toggle_statbar message
Toggle status bar visibility (does nothing in practice)
================
*/
void CL_ParseToggleStatbar(void)
{
	// This message has no parameters and does nothing in original H2
}

/*
================
CL_SetupIntermission

Hexen II intermission setup - configures the intermission display based on
the intermission number sent by the server. Each intermission has specific
settings for background picture, message, timing, and display flags.

Intermission meanings:
  1-4: Episode transitions (defeated Horsemen: Famine, Death, Pestilence, War)
  5:   Demo version finale
  6-8: Eidolon finale (3 parts, chained)
  9:   OEM/bundle version finale
  10:  Mission pack finale (defeated Praevus)
  11:  Mission pack episode change (to Tibet)
  12:  Mission pack intro (before first map, menu-triggered only)
================
*/
void CL_SetupIntermission(int num)
{
	// Note: uhexen2 has an "oem" cvar for the Matrox m3D bundle version
	// that redirects intermission 1 to 9. We don't support this rare version.
	cl.intermission = num;

	switch (cl.intermission)
	{
	case 1: // Defeated Famine: episode 1 (village) to 2 (mazaera)
		cl.completed_time = cl.time;
		cl.message_index = 1 + 394;
		cl.intermission_flags = 0;
		cl.intermission_pic = "gfx/meso.lmp";
		cl.lasting_time = 0;
		cl.intermission_next = 0;
		break;

	case 2: // Defeated Death: episode 2 (mazaera) to 3 (egypt)
		cl.completed_time = cl.time;
		cl.message_index = 2 + 394;
		cl.intermission_flags = 0;
		cl.intermission_pic = "gfx/egypt.lmp";
		cl.lasting_time = 0;
		cl.intermission_next = 0;
		break;

	case 3: // Defeated Pestilence: episode 3 (egypt) to 4 (roman)
		cl.completed_time = cl.time;
		cl.message_index = 3 + 394;
		cl.intermission_flags = 0;
		cl.intermission_pic = "gfx/roman.lmp";
		cl.lasting_time = 0;
		cl.intermission_next = 0;
		break;

	case 4: // Defeated War: episode 4 (roman) to finale (castle)
		cl.completed_time = cl.time;
		cl.message_index = 4 + 394;
		cl.intermission_flags = 0;
		cl.intermission_pic = "gfx/castle.lmp";
		cl.lasting_time = 0;
		cl.intermission_next = 0;
		break;

	case 5: // Demo version finale
		cl.completed_time = cl.time;
		// DEMO_MSG_INDEX is 408 for H2, 410 for H2MP strings.txt
		cl.message_index = 408;
		cl.intermission_flags = 0;
		cl.intermission_pic = "gfx/castle.lmp";
		cl.lasting_time = 0;
		cl.intermission_next = 0;
		break;

	case 6: // Defeated Eidolon: finale part 1/3
		cl.completed_time = cl.time;
		cl.message_index = 6 + 386;
		cl.intermission_flags = H2_INTERMISSION_PRINT_DELAY |
		                        H2_INTERMISSION_PRINT_WHITE |
		                        H2_INTERMISSION_PRINT_TOP;
		cl.intermission_pic = "gfx/end-1.lmp";
		cl.lasting_time = 15;
		cl.intermission_next = 7;
		break;

	case 7: // Defeated Eidolon: finale part 2/3
		cl.completed_time = cl.time;
		cl.message_index = 7 + 386;
		cl.intermission_flags = H2_INTERMISSION_PRINT_DELAY |
		                        H2_INTERMISSION_PRINT_WHITE |
		                        H2_INTERMISSION_PRINT_TOP;
		cl.intermission_pic = "gfx/end-2.lmp";
		cl.lasting_time = 15;
		cl.intermission_next = 8;
		break;

	case 8: // Defeated Eidolon: finale part 3/3
		cl.completed_time = cl.time;
		cl.message_index = 8 + 386;
		cl.intermission_flags = H2_INTERMISSION_PRINT_WHITE |
		                        H2_INTERMISSION_PRINT_DELAY |
		                        H2_INTERMISSION_PRINT_TOPMOST;
		cl.intermission_pic = "gfx/end-3.lmp";
		cl.lasting_time = 0;
		cl.intermission_next = 0;
		break;

	case 9: // OEM/bundle version finale
		cl.completed_time = cl.time;
		cl.message_index = 391;
		cl.intermission_flags = H2_INTERMISSION_PRINT_WHITE;
		cl.intermission_pic = "gfx/castle.lmp";
		cl.lasting_time = 0;
		cl.intermission_next = 0;
		break;

	case 10: // Defeated Praevus: mission pack finale
		cl.completed_time = cl.time;
		cl.message_index = 538;
		cl.intermission_flags = 0;
		cl.intermission_pic = "gfx/mpend.lmp";
		cl.lasting_time = 0;
		cl.intermission_next = 0;
		break;

	case 11: // Mission pack episode change to Tibet
		cl.completed_time = cl.time;
		cl.message_index = 545;
		cl.intermission_flags = 0;
		cl.intermission_pic = "gfx/mpmid.lmp";
		cl.lasting_time = 0;
		cl.intermission_next = 0;
		break;

	case 12: // Mission pack intro (menu-triggered only)
		// This intermission is started by the menu system without
		// a server connection, so we use realtime instead of cl.time.
		// When the user presses a key, Key_Event starts the keep1 map.
		cl.completed_time = realtime;
		cl.message_index = 561;
		cl.intermission_flags = H2_INTERMISSION_NOT_CONNECTED |
		                        H2_INTERMISSION_NO_MENUS;
		cl.intermission_pic = "gfx/end-3.lmp";
		cl.lasting_time = 0;
		cl.intermission_next = 0;
		Host_LoadStrings();
		break;

	default: // Unexpected intermission number
		cl.completed_time = cl.time;
		cl.message_index = 0;
		cl.intermission_flags = 0;
		cl.intermission_pic = NULL;
		cl.lasting_time = 0;
		cl.intermission_next = 0;
		Con_Warning("CL_SetupIntermission: bad intermission number %d\n", num);
		break;
	}
}
