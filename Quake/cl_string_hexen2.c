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

// cl_string_hexen2.c -- Hexen II internationalized strings
// Puzzle piece names and mission pack objectives

#include "quakedef.h"

// Puzzle piece strings for status bar overlay
static char	*puzzle_strings = NULL;
static int	*puzzle_string_index = NULL;
static int	puzzle_string_count = 0;

// Mission pack objective/info strings
static char	*info_strings = NULL;
static int	*info_string_index = NULL;
static int	info_string_count = 0;

/*
===============
CL_LoadPuzzleStrings

Load puzzle piece names from puzzles.txt
Format:
  Line #1: <number of entries>
  Line #2+: <short_name> <full display name>

Example:
  10
  yourkey1 Key to the Tower
  yourkey2 Key to the Dungeon
===============
*/
void CL_LoadPuzzleStrings(void)
{
	int		i, j, count = 0;
	char	*start, *end, *space;

	puzzle_string_index = NULL;
	puzzle_string_count = 0;
	puzzle_strings = (char *)COM_LoadHunkFile("puzzles.txt", NULL);
	if (!puzzle_strings)
	{
		Con_DPrintf("puzzles.txt not found\n");
		return;
	}

	// Get intended number of lines from first line
	j = atoi(puzzle_strings);
	if (j < 1)
		return;
	if (j > 256)
		j = 256;

	// Find and clear first line
	start = puzzle_strings;
	while (*start && *start != '\r' && *start != '\n')
		*start++ = 0;
	if (!*start)
		return;

	// Skip leading whitespace/newlines
	while (*start && (*start == '\n' || *start == '\r' || *start == ' ' || *start == '\t'))
		*start++ = 0;
	if (!*start)
		return;

	// Parse each line
	while (count <= j)
	{
		i = 0;
		end = start;
		while (*end && *end != '\r' && *end != '\n')
			end++;
		if (!*end)
			end = NULL;
		else
			*end = 0;

		// Find space between short name and full name
		space = start;
		while (*space && *space != ' ' && *space != '\t')
			space++;

		if (*space)
		{
			// Clear whitespace between names
			while (space[i] == ' ' || space[i] == '\t')
			{
				space[i] = 0;
				++i;
			}
			if (space[i])
			{
				// Valid entry found
				count++;
				// Clear trailing whitespace
				while (space[i])
					++i;
				--i;
				while (space[i] == ' ' || space[i] == '\t')
				{
					space[i] = 0;
					--i;
				}
				if (!end)
					break;
				goto forward;
			}
			else
			{
				// No full name
				if (!end)
					break;
				memset(start, 0, end - start);
				goto forward;
			}
		}
		else
		{
			// No space in line
			if (!end)
				break;
			memset(start, 0, end - start);
forward:
			start = ++end;
			while (*start == '\r' || *start == '\n' || *start == ' ' || *start == '\t')
				*start++ = 0;
			if (*start == 0)
				break;
		}
	}

	if (!count)
		return;

	// Build string index (2 entries per puzzle: short name, full name)
	puzzle_string_count = count * 2;
	puzzle_string_index = (int *)Hunk_Alloc(puzzle_string_count * sizeof(int));

	i = 0;
	start = puzzle_strings;
	while (i < puzzle_string_count)
	{
		while (*start == 0)
			start++;

		puzzle_string_index[i] = start - puzzle_strings;

		while (*start != 0)
			start++;

		++i;
	}

	Con_DPrintf("Read %d puzzle piece names\n", count);
}

/*
===============
CL_FindPuzzleString

Look up the full display name for a puzzle piece by its short name
Returns NULL if not found
===============
*/
const char *CL_FindPuzzleString(const char *shortname)
{
	int i;

	if (!puzzle_strings || !puzzle_string_index)
		return NULL;

	for (i = 0; i < puzzle_string_count; i += 2)
	{
		if (q_strcasecmp(shortname, &puzzle_strings[puzzle_string_index[i]]) == 0)
			return &puzzle_strings[puzzle_string_index[i + 1]];
	}

	return NULL;
}

/*
===============
CL_LoadInfoStrings

Load mission pack objective strings from infolist.txt
Each line is a separate objective string.
Used by Portal of Praevus for objective display.
===============
*/
void CL_LoadInfoStrings(void)
{
	int		i, count, start;
	signed char	newline_char;

	info_string_index = NULL;
	info_string_count = 0;
	info_strings = (char *)COM_LoadHunkFile("infolist.txt", NULL);
	if (!info_strings)
	{
		// Not an error for base Hexen II (only mission pack uses this)
		Con_DPrintf("infolist.txt not found\n");
		return;
	}

	newline_char = -1;

	// Count lines
	for (i = count = 0; info_strings[i] != 0; i++)
	{
		if (info_strings[i] == '\r' || info_strings[i] == '\n')
		{
			if (newline_char == info_strings[i] || newline_char == -1)
			{
				newline_char = info_strings[i];
				count++;
			}
		}
	}

	if (!count)
	{
		Con_Warning("infolist.txt: no string lines found\n");
		return;
	}

	// Build string index
	info_string_index = (int *)Hunk_Alloc((count + 1) * sizeof(int));

	for (i = count = start = 0; info_strings[i] != 0; i++)
	{
		if (info_strings[i] == '\r' || info_strings[i] == '\n')
		{
			if (newline_char == info_strings[i])
			{
				info_string_index[count] = start;
				start = i + 1;
				count++;
			}
			else
			{
				start++;
			}

			info_strings[i] = 0;
		}
	}

	info_string_count = count;
	Con_DPrintf("Read %d objectives\n", count);
}

/*
===============
CL_GetInfoString

Get an objective string by index
Returns empty string if index is out of range
===============
*/
const char *CL_GetInfoString(int idx)
{
	if (!info_strings || !info_string_index)
		return "";
	if (idx < 0 || idx >= info_string_count)
		return "";
	return &info_strings[info_string_index[idx]];
}

/*
===============
CL_GetInfoStringCount

Get total number of info strings loaded
===============
*/
int CL_GetInfoStringCount(void)
{
	return info_string_count;
}

/*
===============
CL_ClearStrings

Clear all loaded strings (called on disconnect)
===============
*/
void CL_ClearStrings(void)
{
	// Note: Memory is on hunk, will be freed with hunk reset
	puzzle_strings = NULL;
	puzzle_string_index = NULL;
	puzzle_string_count = 0;
	info_strings = NULL;
	info_string_index = NULL;
	info_string_count = 0;
}
