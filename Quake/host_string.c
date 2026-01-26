/*
 * host_string.c --
 * Internationalized string resource for Hexen II
 *
 * Based on uhexen2 implementation
 * Copyright (C) 1997-1998 Raven Software Corp.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 */

#include "quakedef.h"
#include "host_string.h"

static char	*host_strings = NULL;
static int	*host_string_index = NULL;
int		host_string_count = 0;

/*
==================
Host_LoadStrings

Loads strings.txt which contains localized game text.
Each line is a separate string, accessed by index.
The '@' character is translated to newline for multiline text.
==================
*/
void Host_LoadStrings (void)
{
	int		i, count, start;
	signed char	newline_char;

	/* Only load in Hexen II mode */
	if (!hexen2_mode)
		return;

	/* Try lowercase first, then uppercase (DOS/Windows installs use Strings.txt) */
	host_strings = (char *)COM_LoadHunkFile ("strings.txt", NULL);
	if (!host_strings)
		host_strings = (char *)COM_LoadHunkFile ("Strings.txt", NULL);
	if (!host_strings)
	{
		Con_Warning("Host_LoadStrings: couldn't load strings.txt\n");
		host_string_count = 0;
		return;
	}

	newline_char = -1;

	/* First pass: count lines */
	for (i = count = 0; host_strings[i] != 0; i++)
	{
		if (host_strings[i] == '\r' || host_strings[i] == '\n')
		{
			if (newline_char == host_strings[i] || newline_char == -1)
			{
				newline_char = host_strings[i];
				count++;
			}
		}
	}

	if (!count)
	{
		Con_Warning("Host_LoadStrings: no string lines found\n");
		host_string_count = 0;
		return;
	}

	host_string_index = (int *)Hunk_AllocName ((count + 1)*sizeof(int), "string_index");

	/* Second pass: build index and null-terminate each line */
	for (i = count = start = 0; host_strings[i] != 0; i++)
	{
		if (host_strings[i] == '\r' || host_strings[i] == '\n')
		{
			if (newline_char == host_strings[i])
			{
				host_string_index[count] = start;
				start = i + 1;
				count++;
			}
			else
			{
				start++;
			}

			host_strings[i] = 0;
		}
		/* Translate '@' to newline for multiline text display */
		else if (host_strings[i] == '@')
		{
			host_strings[i] = '\n';
		}
	}

	host_string_count = count;
	Sys_Printf("Host_LoadStrings: loaded %d strings\n", count);
}

/*
==================
Host_GetString

Returns the string at the given index, or empty string if invalid.
==================
*/
const char *Host_GetString (int idx)
{
	if (!host_strings || !host_string_index)
		return "";

	if (idx < 0 || idx >= host_string_count)
	{
		Con_DPrintf("Host_GetString: index %d out of range (0-%d)\n", idx, host_string_count - 1);
		return "";
	}

	return &host_strings[host_string_index[idx]];
}
