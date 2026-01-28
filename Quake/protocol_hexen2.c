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

// Global flag indicating if Hexen II mode is active
qboolean hexen2_mode = false;

// Global flag indicating if Portal of Praevus mission pack is detected
qboolean hexen2_missionpack = false;

// Global flag indicating if demo version is detected (only Paladin/Assassin)
qboolean hexen2_demo = false;

// Console variable for forcing Hexen II mode
static cvar_t cv_game_hexen2 = {"game_hexen2", "0", CVAR_NONE};

/*
=================
H2_Protocol_Init

Initialize Hexen II protocol support
=================
*/
void H2_Protocol_Init(void)
{
	Cvar_RegisterVariable(&cv_game_hexen2);
	Con_Printf("Hexen II protocol support initialized\n");
}

/*
=================
H2_IsHexen2Protocol

Returns true if the given protocol version is a Hexen II protocol
=================
*/
qboolean H2_IsHexen2Protocol(int protocol)
{
	switch (protocol)
	{
		case PROTOCOL_RAVEN_107:
		case PROTOCOL_RAVEN_109:
		case PROTOCOL_RAVEN_111:
		case PROTOCOL_RAVEN_112:
		case PROTOCOL_UQE_113:
			return true;
		default:
			return false;
	}
}

/*
=================
H2_GetProtocolName

Returns a human-readable name for the protocol version
=================
*/
const char *H2_GetProtocolName(int protocol)
{
	switch (protocol)
	{
		case PROTOCOL_RAVEN_109:
			return "Hexen II 1.09";
		case PROTOCOL_RAVEN_111:
			return "Hexen II 1.11";
		case PROTOCOL_RAVEN_112:
			return "Hexen II 1.12 (Mission Pack)";
		case PROTOCOL_UQE_113:
			return "Hexen II 1.13 (UQE Patch)";
		case PROTOCOL_NETQUAKE:
			// Note: PROTOCOL_RAVEN_107 also equals 15
			return "NetQuake / Hexen II 1.03";
		case PROTOCOL_FITZQUAKE:
			return "FitzQuake";
		case PROTOCOL_RMQ:
			return "RMQ";
		default:
			return "Unknown";
	}
}

/*
=================
H2_DetectGameType

Detect if we should be running in Hexen II mode based on game data.
Called after COM_InitFilesystem has set up search paths.
=================
*/
void H2_DetectGameType(void)
{
	int handle;
	FILE *dummy;
	unsigned int path_id;

	// Check if user forced Hexen II mode
	if (cv_game_hexen2.value != 0)
	{
		hexen2_mode = true;
		Con_Printf("Hexen II mode: FORCED via cvar\n");
		return;
	}

	// Auto-detect based on game data in search paths
	hexen2_mode = false;
	hexen2_missionpack = false;

	// Check for H2-specific files that would be in the search path
	// puzzles.txt is in data1/pak0.pak and uniquely identifies H2
	handle = COM_FOpenFile("puzzles.txt", &dummy, &path_id);
	if (handle >= 0)
	{
		fclose(dummy);
		hexen2_mode = true;
		Con_DPrintf("Hexen II mode: DETECTED (puzzles.txt found)\n");
	}

	// Also check for H2 menu graphics as backup detection
	if (!hexen2_mode)
	{
		handle = COM_FOpenFile("gfx/menu/title0.lmp", &dummy, &path_id);
		if (handle >= 0)
		{
			fclose(dummy);
			hexen2_mode = true;
			Con_DPrintf("Hexen II mode: DETECTED (H2 menu graphics found)\n");
		}
	}

	// Check for Portal of Praevus mission pack
	// The mission pack has unique files like the demoness portrait
	if (hexen2_mode)
	{
		handle = COM_FOpenFile("gfx/cport5.lmp", &dummy, &path_id);
		if (handle >= 0)
		{
			fclose(dummy);
			hexen2_missionpack = true;
			Con_DPrintf("Portal of Praevus: DETECTED (demoness portrait found)\n");
		}

		// Also check -portals command line flag
		if (!hexen2_missionpack && COM_CheckParm("-portals"))
		{
			hexen2_missionpack = true;
			Con_DPrintf("Portal of Praevus: ENABLED via -portals flag\n");
		}
	}

	// Check for demo version
	// Demo only has Paladin and Assassin, lacks crusader.mdl
	if (hexen2_mode)
	{
		hexen2_demo = true;	// Assume demo until proven otherwise
		handle = COM_FOpenFile("models/crusader.mdl", &dummy, &path_id);
		if (handle >= 0)
		{
			fclose(dummy);
			hexen2_demo = false;
			Con_DPrintf("Hexen II: Full version detected (crusader.mdl found)\n");
		}
		else
		{
			Con_DPrintf("Hexen II: Demo version detected (crusader.mdl missing)\n");
		}
	}

	if (hexen2_mode)
	{
		if (hexen2_missionpack)
			Con_Printf("Hexen II mode: ENABLED (with Portal of Praevus)\n");
		else if (hexen2_demo)
			Con_Printf("Hexen II mode: ENABLED (Demo version)\n");
		else
			Con_Printf("Hexen II mode: ENABLED\n");
	}
	else
	{
		Con_DPrintf("Quake mode: ENABLED\n");
	}
}

/*
=================
H2_SetProtocol

Set the protocol version, potentially switching to Hexen II mode
=================
*/
void H2_SetProtocol(int protocol)
{
	if (H2_IsHexen2Protocol(protocol))
	{
		hexen2_mode = true;
		Con_Printf("Protocol: %s (%d)\n", H2_GetProtocolName(protocol), protocol);
	}
}
