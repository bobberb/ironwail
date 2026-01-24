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

Detect if we should be running in Hexen II mode based on game data
=================
*/
void H2_DetectGameType(void)
{
	// Check if user forced Hexen II mode
	if (cv_game_hexen2.value != 0)
	{
		hexen2_mode = true;
		Con_Printf("Hexen II mode: FORCED via cvar\n");
		return;
	}

	// Auto-detect based on game directory
	// Look for common Hexen II identifiers
	hexen2_mode = false;

	// Hexen II uses data1/ and portals/ instead of Quake's id1/
	// Check for these directories/files
	{
		int handle;
		FILE *dummy;
		unsigned int path_id;

		// Check for data1 directory with pak files (base Hexen II)
		handle = COM_FOpenFile("data1/pak0.pak", &dummy, &path_id);
		if (handle >= 0)
		{
			fclose(dummy);
			hexen2_mode = true;
			Con_Printf("Hexen II mode: DETECTED (data1/pak0.pak found)\n");
			return;
		}

		// Check for portals directory (Portal of Praevus mission pack)
		handle = COM_FOpenFile("portals/pak3.pak", &dummy, &path_id);
		if (handle >= 0)
		{
			fclose(dummy);
			hexen2_mode = true;
			Con_Printf("Hexen II mode: DETECTED (portals/pak3.pak found)\n");
			return;
		}

		// Check for puzzles.txt (H2-specific file)
		handle = COM_FOpenFile("puzzles.txt", &dummy, &path_id);
		if (handle >= 0)
		{
			fclose(dummy);
			hexen2_mode = true;
			Con_Printf("Hexen II mode: DETECTED (puzzles.txt found)\n");
			return;
		}
	}

	// Check for Hexen II progs.dat CRC
	// TODO: Add CRC checking when progs loading is integrated

	if (hexen2_mode)
		Con_Printf("Hexen II mode: ENABLED\n");
	else
		Con_Printf("Quake mode: ENABLED\n");
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
