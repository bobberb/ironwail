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
Sets the MIDI music file name to play
================
*/
void CL_ParseMidiName(void)
{
	const char *midi_name;

	midi_name = MSG_ReadString();

	// TODO: Implement MIDI playback when audio system is extended
	Con_DPrintf("MIDI name: %s\n", midi_name);
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

	// TODO: Implement H2 material-aware particles when particle system is extended
	// For now, create a basic explosion effect
	// R_ParticleExplosion(org);
	Con_DPrintf("Particle explosion at (%.1f, %.1f, %.1f), color %d, radius %d\n",
				org[0], org[1], org[2], color, radius);
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
Updates player inventory
================
*/
void CL_ParseUpdateInventory(void)
{
	int slot, amount;

	slot = MSG_ReadByte();
	amount = MSG_ReadByte();

	if (slot < 0 || slot >= H2_MAX_INVENTORY)
		Host_Error("CL_ParseUpdateInventory: slot out of range");

	// TODO: Implement inventory system when HUD is extended
	Con_DPrintf("Inventory update: slot %d = %d\n", slot, amount);
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

	plaque_id = MSG_ReadShort();

	// TODO: Implement plaque display when UI is extended
	Con_DPrintf("Plaque: %d\n", plaque_id);
}

/*
================
CL_ParseParticle2

Parse svc_h2_particle2 message
Extended particle effect
================
*/
void CL_ParseParticle2(void)
{
	vec3_t org, dir;
	int color, count;

	org[0] = MSG_ReadCoord(cl.protocolflags);
	org[1] = MSG_ReadCoord(cl.protocolflags);
	org[2] = MSG_ReadCoord(cl.protocolflags);
	dir[0] = MSG_ReadChar() * 0.0625f;
	dir[1] = MSG_ReadChar() * 0.0625f;
	dir[2] = MSG_ReadChar() * 0.0625f;
	color = MSG_ReadByte();
	count = MSG_ReadByte();

	// TODO: Implement H2 particle system
	Con_DPrintf("Particle2: color %d, count %d\n", color, count);
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
	vec3_t org, dir;
	int color, count;

	org[0] = MSG_ReadCoord(cl.protocolflags);
	org[1] = MSG_ReadCoord(cl.protocolflags);
	org[2] = MSG_ReadCoord(cl.protocolflags);
	dir[0] = MSG_ReadChar();
	dir[1] = MSG_ReadChar();
	dir[2] = MSG_ReadChar();
	color = MSG_ReadByte();
	count = MSG_ReadByte();

	// TODO: Implement rain/snow weather particles
	Con_DPrintf("Rain effect at (%.1f, %.1f, %.1f), count %d\n",
				org[0], org[1], org[2], count);
}

/*
================
CL_ParseSoundUpdatePos

Parse svc_h2_sound_update_pos message
Update position of a playing sound
================
*/
void CL_ParseSoundUpdatePos(void)
{
	vec3_t pos;
	int channel;

	channel = MSG_ReadShort();
	pos[0] = MSG_ReadCoord(cl.protocolflags);
	pos[1] = MSG_ReadCoord(cl.protocolflags);
	pos[2] = MSG_ReadCoord(cl.protocolflags);

	// TODO: Update sound position in audio system
	Con_DPrintf("Sound update pos: channel %d\n", channel);
}

/*
================
CL_ParseModName

Parse svc_h2_mod_name message (UQE 1.13)
Sets the mod music file name
================
*/
void CL_ParseModName(void)
{
	const char *mod_name;

	mod_name = MSG_ReadString();

	// TODO: Implement MOD music playback
	Con_DPrintf("MOD name: %s\n", mod_name);
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

	// TODO: Load skybox when rendering is extended
	Con_DPrintf("Skybox: %s\n", skybox_name);
}
