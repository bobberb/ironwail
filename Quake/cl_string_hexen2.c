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

/*
 * Hexen II puzzle and info string loading
 * Adapted from uhexen2 cl_string.c
 */

#include "quakedef.h"

/* puzzle piece strings for Sbar_H2_DrawInfoOverlay() */
static char	*puzzle_strings = NULL;
static int	*puzzle_string_index = NULL;
int		puzzle_string_count = 0;

/*
================
CL_LoadPuzzleStrings

Load puzzle piece names from puzzles.txt
Format:
  Line 1: <count>
  Line 2+: <shortname> <full display name>
================
*/
void CL_LoadPuzzleStrings(void)
{
	int		i, j, count = 0;
	char		*start, *end, *space;

	puzzle_string_index = NULL;
	puzzle_string_count = 0;
	puzzle_strings = (char *)COM_LoadHunkFile("puzzles.txt", NULL);
	if (!puzzle_strings)
	{
		Con_DPrintf("puzzles.txt not found\n");
		return;
	}

	/*
	 * Format of puzzles.txt:
	 * Line #1 : <number of lines excluding this one>
	 * Line #2+: <one-word short name><one space><full name in multiple words>
	 */

	j = atoi(puzzle_strings);	/* the intended number of lines */
	if (j < 1)
		return;
	if (j > 256)
		j = 256;

	start = puzzle_strings;
	while (*start && *start != '\r' && *start != '\n')
	{	/* find first newline, clear the start */
		*start++ = 0;
	}
	while (*start == '\r' || *start == '\n')
		*start++ = 0;

	/* first pass: count valid entries and null-terminate them */
	while (*start && count < j)
	{
		/* skip leading whitespace */
		while (*start == ' ' || *start == '\t')
			start++;
		if (*start == 0)
			break;
		if (*start == '\r' || *start == '\n')
		{
			*start++ = 0;
			continue;
		}

		/* find the end of line */
		end = start;
		while (*end && *end != '\r' && *end != '\n')
			end++;

		/* find the space between shortname and fullname */
		space = start;
		while (*space && *space != ' ' && *space != '\t' && space < end)
			space++;
		if (space >= end || *space == 0)
		{
			/* malformed line, skip */
			start = end;
			while (*start == '\r' || *start == '\n')
				*start++ = 0;
			continue;
		}

		/* null-terminate shortname */
		*space++ = 0;
		/* skip extra whitespace */
		while (*space == ' ' || *space == '\t')
			*space++ = 0;
		/* null-terminate fullname at end of line */
		*end = 0;

		count++;
		start = end + 1;
		while (*start == '\r' || *start == '\n')
			*start++ = 0;
	}

	if (!count)
		return;

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

		i++;
	}

	Con_DPrintf("Read in %d puzzle piece names\n", count);
}

/*
================
CL_FindPuzzleString

Find the full display name for a puzzle piece short name
================
*/
const char *CL_FindPuzzleString(const char *shortname)
{
	int		i;

	for (i = 0; i < puzzle_string_count; i += 2)
	{
		if (q_strcasecmp(shortname, &puzzle_strings[puzzle_string_index[i]]) == 0)
			return &puzzle_strings[puzzle_string_index[i + 1]];
	}

	return NULL;
}

/*
 * Mission pack (Portal of Praevus) objectives strings
 * Loaded from infolist.txt
 */
static char	*info_strings = NULL;
static int	*info_string_index = NULL;
int		info_string_count = 0;

/*
================
CL_LoadInfoStrings

Load mission pack objectives from infolist.txt
Format: one objective string per line
================
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
		Con_DPrintf("infolist.txt not found (mission pack not installed?)\n");
		return;
	}

	newline_char = -1;

	/* first pass: count lines */
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
		Con_Warning("infolist.txt: no objective strings found\n");
		return;
	}

	info_string_index = (int *)Hunk_Alloc((count + 1) * sizeof(int));

	/* second pass: build index and null-terminate strings */
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
	Con_DPrintf("Read in %d objectives\n", count);
}

/*
================
CL_GetInfoString

Get objective string by index
================
*/
const char *CL_GetInfoString(int idx)
{
	if (idx < 0 || idx >= info_string_count)
		return "";
	return &info_strings[info_string_index[idx]];
}
