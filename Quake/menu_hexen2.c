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

// menu_hexen2.c -- Hexen II menu system

#include "quakedef.h"
#include "menu_hexen2.h"

// Menu cursor animation
#define H2_CURSOR_FRAMES	8

// Title scrolling animation (matches uhexen2 behavior)
static float TitlePercent = 0;
static float TitleTargetPercent = 0;
static float LogoPercent = 0;
static float LogoTargetPercent = 0;
static const char *LastTitleName = "";
static qboolean CanSwitchTitle = true;

// Bigfont kerning table: 27 chars (A-Z plus /) x 27 chars
// Value = number of pixels to move left for the second character
static char BigCharWidth[27][27];
static qboolean bigfont_loaded = false;
static qpic_t *bigfont_pic = NULL;

// Player class for new game (uses _cl_playerclass cvar)
extern cvar_t cl_playerclass;
#define h2_player_class ((int)cl_playerclass.value)

// Help pages
#define H2_NUM_HELP_PAGES	5
static int h2_help_page = 0;

// Class names
const char *h2_class_names[] = {
	"paladin",
	"crusader",
	"necromancer",
	"assassin",
	"demoness"
};

const char *h2_class_names_upper[] = {
	"PALADIN",
	"CRUSADER",
	"NECROMANCER",
	"ASSASSIN",
	"DEMONESS"
};

// Class-specific difficulty names (from original H2)
const char *h2_diff_names[H2_MAX_PLAYER_CLASS][H2_NUM_DIFFLEVELS] = {
	// Paladin
	{ "APPRENTICE", "SQUIRE", "ADEPT", "LORD" },
	// Crusader
	{ "GALLANT", "HOLY AVENGER", "DIVINE HERO", "LEGEND" },
	// Necromancer
	{ "SORCERER", "DARK SERVANT", "WARLOCK", "LICH KING" },
	// Assassin
	{ "ROGUE", "CUTTHROAT", "EXECUTIONER", "WIDOW MAKER" },
	// Demoness (Portals)
	{ "LARVA", "SPAWN", "FIEND", "SHE BITCH" }
};

// Menu cursors
static int h2_main_cursor;
static int h2_class_cursor;
static int h2_diff_cursor;

// Cached graphics
static qpic_t *h2_plaque;
static qpic_t *h2_titles[9];	// title0-8.lmp
static qpic_t *h2_menudot[8];	// menudot1-8.lmp
static qpic_t *h2_portraits[5];	// cport1-5.lmp
static qpic_t *h2_frame;		// frame.lmp
static qpic_t *h2_help[5];		// help01-05.lmp

// Forward declarations
static void M_H2_Main_Draw(void);
static void M_H2_Main_Key(int key);
static void M_H2_SinglePlayer_Draw(void);
static void M_H2_SinglePlayer_Key(int key);
static void M_H2_Class_Draw(void);
static void M_H2_Class_Key(int key);
static void M_H2_Difficulty_Draw(void);
static void M_H2_Difficulty_Key(int key);
static void M_H2_Help_Draw(void);
static void M_H2_Help_Key(int key);
static void M_H2_DrawCursor(int x, int y);
static int M_H2_GetNumClasses(void);
void M_Menu_H2_Setup_f(void);

/*
================
M_H2_HasPortals

Check if the Portals of Praevus expansion is available
================
*/
qboolean M_H2_HasPortals(void)
{
	// Use the flag that was set during H2 detection in protocol_hexen2.c
	if (hexen2_missionpack)
		return true;

	// Also check command line override
	if (COM_CheckParm("-portals"))
		return true;

	return false;
}

/*
================
M_H2_GetNumClasses

Returns the number of available classes (4 or 5 based on expansion)
================
*/
static int M_H2_GetNumClasses(void)
{
	return M_H2_HasPortals() ? H2_MAX_PLAYER_CLASS : H2_NUM_BASE_CLASSES;
}

/*
================
M_H2_BuildBigCharWidth

Build the bigfont kerning table from the bigfont2.lmp texture
================
*/
void M_H2_BuildBigCharWidth(void)
{
	int i, j;

	if (bigfont_loaded)
		return;

	// Initialize with default spacing
	for (i = 0; i < 27; i++)
		for (j = 0; j < 27; j++)
			BigCharWidth[i][j] = 24;  // Default char spacing

	// Try to load bigfont2.lmp to build actual kerning table
	bigfont_pic = Draw_TryCachePic("gfx/menu/bigfont2.lmp",
		TEXPREF_ALPHA | TEXPREF_PAD | TEXPREF_NOPICMIP);

	if (!bigfont_pic)
	{
		Con_DPrintf("M_H2_BuildBigCharWidth: couldn't load gfx/menu/bigfont2.lmp\n");
		bigfont_loaded = true;
		return;
	}

	// Bigfont2.lmp is 160x80 pixels, 8 chars per row (4 rows), each char 20x20 pixels
	// Characters are arranged: A-Z then / (27 chars total across 4 rows)
	// For detailed kerning, we'd need to analyze the raw pixel data
	// For simplicity, use fixed spacing based on char widths

	// Basic character widths for bigfont (approximate)
	static const int char_widths[27] = {
		20, 15, 16, 17, 14, 14, 17, 17, 8, 14,  // A-J
		16, 14, 22, 18, 18, 15, 18, 16, 15, 16, // K-T
		18, 18, 24, 17, 18, 16, 10              // U-Z, /
	};

	// Build simple spacing table (first char width + 2 pixel gap)
	for (i = 0; i < 27; i++)
	{
		for (j = 0; j < 27; j++)
		{
			BigCharWidth[i][j] = char_widths[i];
		}
	}

	bigfont_loaded = true;
}

/*
================
M_H2_BigCharWidth

Get the width when drawing character c2 after c1
================
*/
int M_H2_BigCharWidth(int c1, int c2)
{
	if (c1 == '/')
		c1 = 26;
	else if (c1 >= 'A' && c1 <= 'Z')
		c1 = c1 - 'A';
	else if (c1 >= 'a' && c1 <= 'z')
		c1 = c1 - 'a';
	else
		return 18;  // Default width for unknown chars

	if (c2 == '/')
		c2 = 26;
	else if (c2 >= 'A' && c2 <= 'Z')
		c2 = c2 - 'A';
	else if (c2 >= 'a' && c2 <= 'z')
		c2 = c2 - 'a';
	else
		c2 = 0;

	if (c1 < 0 || c1 >= 27 || c2 < 0 || c2 >= 27)
		return 18;

	return BigCharWidth[c1][c2];
}

/*
================
M_H2_DrawBigCharacter

Draw a single bigfont character at x,y
================
*/
void M_H2_DrawBigCharacter(int x, int y, int num)
{
	int row, col;
	int srcx, srcy;

	if (!bigfont_pic)
		return;

	// Convert character to index
	if (num == '/')
		num = 26;
	else if (num >= 'A' && num <= 'Z')
		num = num - 'A';
	else if (num >= 'a' && num <= 'z')
		num = num - 'a';
	else
		return;  // Unsupported character

	if (num < 0 || num >= 27)
		return;

	// Bigfont layout: 8 chars per row, 4 rows total (A-Z plus /)
	// Texture is 160x80 pixels, so each char is 20x20 pixels
	row = num / 8;
	col = num % 8;

	srcx = col * 20;
	srcy = row * 20;

	M_DrawSubpic(x, y, bigfont_pic, srcx, srcy, 20, 20);
}

/*
================
M_H2_DrawBigString

Draw a string using the bigfont
================
*/
void M_H2_DrawBigString(int x, int y, const char *str)
{
	int startx = x;
	int lastchar = 0;

	if (!str)
		return;

	if (!bigfont_pic)
	{
		// Fallback to regular font if bigfont not available
		while (*str)
		{
			M_DrawCharacter(x, y, *str);
			x += 8;
			str++;
		}
		return;
	}

	while (*str)
	{
		if (*str == ' ')
		{
			x += 10;  // Space width
			lastchar = 0;
		}
		else if (*str == '\n')
		{
			x = startx;
			y += 26;
			lastchar = 0;
		}
		else
		{
			M_H2_DrawBigCharacter(x, y, *str);
			if (lastchar)
				x += M_H2_BigCharWidth(lastchar, *str);
			else
				x += 18;  // First char default width
			lastchar = *str;
		}
		str++;
	}
}

/*
================
M_H2_ScrollTitle

Animated title that scrolls down from top of screen
Based on uhexen2's ScrollTitle implementation
================
*/
void M_H2_ScrollTitle(const char *name)
{
	qpic_t *p;
	float delta;
	int finaly;

	// Animate title percentage toward target
	if (TitlePercent < TitleTargetPercent)
	{
		delta = ((TitleTargetPercent - TitlePercent) / 0.5f) * host_frametime;
		if (delta < 0.004f)
			delta = 0.004f;
		TitlePercent += delta;
		if (TitlePercent > TitleTargetPercent)
			TitlePercent = TitleTargetPercent;
	}
	else if (TitlePercent > TitleTargetPercent)
	{
		delta = ((TitlePercent - TitleTargetPercent) / 0.15f) * host_frametime;
		if (delta < 0.02f)
			delta = 0.02f;
		TitlePercent -= delta;
		if (TitlePercent <= TitleTargetPercent)
		{
			TitlePercent = TitleTargetPercent;
			CanSwitchTitle = true;
		}
	}

	// Animate logo percentage toward target
	if (LogoPercent < LogoTargetPercent)
	{
		delta = ((LogoTargetPercent - LogoPercent) / 0.15f) * host_frametime;
		if (delta < 0.02f)
			delta = 0.02f;
		LogoPercent += delta;
		if (LogoPercent > LogoTargetPercent)
			LogoPercent = LogoTargetPercent;
	}

	// If title changed, start scrolling old one out
	if (q_strcasecmp(LastTitleName, name) != 0 && TitleTargetPercent != 0)
		TitleTargetPercent = 0;

	// When ready to switch, set new title and start scrolling in
	if (CanSwitchTitle)
	{
		LastTitleName = name;
		CanSwitchTitle = false;
		TitleTargetPercent = 1;
		LogoTargetPercent = 1;
	}

	// Draw the title
	p = Draw_CachePic(LastTitleName);
	if (p)
	{
		finaly = (int)((float)p->height * TitlePercent) - p->height;
		M_DrawTransPicCropped((320 - p->width) / 2, finaly, p);
	}

	// Draw the plaque/logo (not during keys menu)
	if (m_state != m_keys && h2_plaque)
	{
		finaly = (int)((float)h2_plaque->height * LogoPercent) - h2_plaque->height;
		M_DrawTransPicCropped(10, finaly, h2_plaque);
	}
}

/*
================
M_H2_ResetScrollTitle

Reset the scroll animation state (call when entering menus)
================
*/
void M_H2_ResetScrollTitle(void)
{
	TitlePercent = 0;
	TitleTargetPercent = 0;
	LogoPercent = 0;
	LogoTargetPercent = 0;
	LastTitleName = "";
	CanSwitchTitle = true;
}

/*
================
M_H2_DrawCursor

Draw the animated Hexen II menu cursor (8 frames)
================
*/
static void M_H2_DrawCursor(int x, int y)
{
	float time;
	int frame;

	// Animate cursor
	time = realtime * 10;
	frame = ((int)time) % H2_CURSOR_FRAMES;

	if (h2_menudot[frame])
		M_DrawTransPic(x, y, h2_menudot[frame]);
}

/*
================
M_H2_Init

Initialize Hexen II menu system and load graphics
================
*/
static qboolean h2_gfx_loaded = false;

/*
================
M_H2_LoadGraphics

Load H2 menu graphics - called lazily on first menu draw
Must be called after video init
================
*/
static void M_H2_LoadGraphics(void)
{
	int i;
	char name[64];

	if (h2_gfx_loaded)
		return;

	h2_gfx_loaded = true;

	// Build bigfont kerning table (also loads bigfont2.lmp)
	M_H2_BuildBigCharWidth();

	// Load plaque
	h2_plaque = Draw_TryCachePic("gfx/menu/hplaque.lmp",
		TEXPREF_ALPHA | TEXPREF_PAD | TEXPREF_NOPICMIP);

	// Load title graphics
	for (i = 0; i < 9; i++)
	{
		q_snprintf(name, sizeof(name), "gfx/menu/title%d.lmp", i);
		h2_titles[i] = Draw_TryCachePic(name,
			TEXPREF_ALPHA | TEXPREF_PAD | TEXPREF_NOPICMIP);
	}

	// Load menu cursor (menudot1-8.lmp)
	for (i = 0; i < H2_CURSOR_FRAMES; i++)
	{
		q_snprintf(name, sizeof(name), "gfx/menu/menudot%d.lmp", i + 1);
		h2_menudot[i] = Draw_TryCachePic(name,
			TEXPREF_ALPHA | TEXPREF_PAD | TEXPREF_NOPICMIP);
	}

	// Load class portraits (cport1-5.lmp)
	for (i = 0; i < 5; i++)
	{
		q_snprintf(name, sizeof(name), "gfx/cport%d.lmp", i + 1);
		h2_portraits[i] = Draw_TryCachePic(name,
			TEXPREF_ALPHA | TEXPREF_PAD | TEXPREF_NOPICMIP);
	}

	// Load portrait frame
	h2_frame = Draw_TryCachePic("gfx/menu/frame.lmp",
		TEXPREF_ALPHA | TEXPREF_PAD | TEXPREF_NOPICMIP);

	// Load help pages (help01-05.lmp)
	for (i = 0; i < H2_NUM_HELP_PAGES; i++)
	{
		q_snprintf(name, sizeof(name), "gfx/menu/help%02d.lmp", i + 1);
		h2_help[i] = Draw_TryCachePic(name,
			TEXPREF_ALPHA | TEXPREF_PAD | TEXPREF_NOPICMIP);
	}
}

void M_H2_Init(void)
{
	// Nothing to do here - graphics are loaded lazily in M_H2_LoadGraphics()
	// which is called from M_H2_Draw() after video system is initialized
}

/*
================
M_H2_PlaySound

Play a Hexen II menu sound with correct path
================
*/
static void M_H2_PlaySound(const char *name)
{
	char path[MAX_QPATH];

	// H2 menu sounds are in raven/ directory
	q_snprintf(path, sizeof(path), "raven/%s", name);
	S_LocalSound(path);
}

static void M_H2_EnterSound(void)
{
	M_H2_PlaySound("menu2.wav");
}

static void M_H2_NavSound(void)
{
	M_H2_PlaySound("menu1.wav");
}

static void M_H2_BackSound(void)
{
	M_H2_PlaySound("menu3.wav");
}

/*
================================================================================
MAIN MENU
================================================================================
*/

#define H2_MAIN_ITEMS		6
#define H2_MAIN_SINGLEPLAYER	0
#define H2_MAIN_MULTIPLAYER	1
#define H2_MAIN_OPTIONS		2
#define H2_MAIN_MODS		3
#define H2_MAIN_HELP		4
#define H2_MAIN_QUIT		5

static void M_H2_Main_Draw(void)
{
	int y;

	// Draw animated title and plaque
	M_H2_ScrollTitle("gfx/menu/title0.lmp");

	// Menu items start at fixed position (title scrolls in from top)
	y = 60;
	M_H2_DrawBigString(72, y, "SINGLE PLAYER");
	y += 20;
	M_H2_DrawBigString(72, y, "MULTIPLAYER");
	y += 20;
	M_H2_DrawBigString(72, y, "OPTIONS");
	y += 20;
	M_H2_DrawBigString(72, y, "MODS");
	y += 20;
	M_H2_DrawBigString(72, y, "HELP");
	y += 20;
	M_H2_DrawBigString(72, y, "QUIT");

	// Draw cursor (matches uhexen2 positioning)
	M_H2_DrawCursor(43, 54 + h2_main_cursor * 20);
}

static void M_H2_Main_Key(int key)
{
	switch (key)
	{
	case K_ESCAPE:
	case K_BBUTTON:
	case K_MOUSE2:
		IN_Activate();
		key_dest = key_game;
		m_state = m_none;
		break;

	case K_DOWNARROW:
	case K_MWHEELDOWN:
		M_H2_NavSound();
		if (++h2_main_cursor >= H2_MAIN_ITEMS)
			h2_main_cursor = 0;
		break;

	case K_UPARROW:
	case K_MWHEELUP:
		M_H2_NavSound();
		if (--h2_main_cursor < 0)
			h2_main_cursor = H2_MAIN_ITEMS - 1;
		break;

	case K_ENTER:
	case K_KP_ENTER:
	case K_ABUTTON:
	case K_MOUSE1:
		m_entersound = true;

		switch (h2_main_cursor)
		{
		case H2_MAIN_SINGLEPLAYER:
			M_Menu_SinglePlayer_f();
			break;

		case H2_MAIN_MULTIPLAYER:
			M_Menu_MultiPlayer_f();
			break;

		case H2_MAIN_OPTIONS:
			M_Menu_Options_f();
			break;

		case H2_MAIN_MODS:
			M_Menu_Mods_f();
			break;

		case H2_MAIN_HELP:
			h2_help_page = 0;
			m_state = m_help;
			break;

		case H2_MAIN_QUIT:
			M_Menu_Quit_f();
			break;
		}
		break;
	}
}

/*
================================================================================
SINGLE PLAYER MENU
================================================================================
*/

#define H2_SP_BASE_ITEMS	3
#define H2_SP_PORTALS_ITEMS	2	// Extra items when Portals detected
#define H2_SP_NEWGAME		0
#define H2_SP_LOADGAME		1
#define H2_SP_SAVEGAME		2
#define H2_SP_OLDMISSION	3	// Portals only: start base H2
#define H2_SP_VIEWINTRO		4	// Portals only: play intro demo

static int h2_sp_cursor;
static int h2_enter_portals;	// 1 = starting Portals campaign, 0 = base H2

static int M_H2_GetSPItems(void)
{
	return M_H2_HasPortals() ? (H2_SP_BASE_ITEMS + H2_SP_PORTALS_ITEMS) : H2_SP_BASE_ITEMS;
}

static void M_H2_SinglePlayer_Draw(void)
{
	int y;
	qboolean has_portals = M_H2_HasPortals();

	// Draw animated title and plaque
	M_H2_ScrollTitle("gfx/menu/title1.lmp");

	// Menu items
	y = 60;
	// Portals: "NEW MISSION", Base: "NEW GAME"
	M_H2_DrawBigString(72, y, has_portals ? "NEW MISSION" : "NEW GAME");
	y += 20;
	M_H2_DrawBigString(72, y, "LOAD");
	y += 20;
	M_H2_DrawBigString(72, y, "SAVE");

	// Portals-only options
	if (has_portals)
	{
		y += 20;
		M_H2_DrawBigString(72, y, "OLD MISSION");
		y += 20;
		M_H2_DrawBigString(72, y, "VIEW INTRO");
	}

	// Draw cursor
	M_H2_DrawCursor(43, 54 + h2_sp_cursor * 20);
}

static void M_H2_SinglePlayer_Key(int key)
{
	int num_items = M_H2_GetSPItems();

	switch (key)
	{
	case K_ESCAPE:
	case K_BBUTTON:
	case K_MOUSE2:
		M_Menu_Main_f();
		break;

	case K_DOWNARROW:
	case K_MWHEELDOWN:
		M_H2_NavSound();
		if (++h2_sp_cursor >= num_items)
			h2_sp_cursor = 0;
		break;

	case K_UPARROW:
	case K_MWHEELUP:
		M_H2_NavSound();
		if (--h2_sp_cursor < 0)
			h2_sp_cursor = num_items - 1;
		break;

	case K_ENTER:
	case K_KP_ENTER:
	case K_ABUTTON:
	case K_MOUSE1:
		m_entersound = true;
		h2_enter_portals = 0;  // Default to base H2 campaign

		switch (h2_sp_cursor)
		{
		case H2_SP_NEWGAME:
			// If Portals detected, NEW MISSION starts Portals campaign
			if (M_H2_HasPortals())
				h2_enter_portals = 1;
			// Fall through to start new game
		case H2_SP_OLDMISSION:
			// OLD MISSION keeps h2_enter_portals = 0 (base campaign)
			// Confirm if game in progress
			if (sv.active)
			{
				// TODO: Add confirmation dialog like uhexen2
				// For now, just disconnect and start
				Cbuf_AddText("disconnect\n");
			}
			// Clear any saved hub state
			// Host_RemoveGIPFiles(NULL);  // TODO: implement if needed
			Cbuf_AddText("maxplayers 1\n");
			Cbuf_AddText("coop 0\n");
			Cbuf_AddText("deathmatch 0\n");
			M_Menu_H2_Class_f();
			break;

		case H2_SP_LOADGAME:
			M_Menu_Load_f();
			break;

		case H2_SP_SAVEGAME:
			if (sv.active)
				M_Menu_Save_f();
			break;

		case H2_SP_VIEWINTRO:
			// Play the Portals intro demo
			if (M_H2_HasPortals())
			{
				IN_Activate();
				key_dest = key_game;
				m_state = m_none;
				Cbuf_AddText("playdemo t9\n");
			}
			break;
		}
		break;
	}
}

/*
================================================================================
CLASS SELECTION MENU
================================================================================
*/

/*
================
M_H2_IsClassAvailable

Check if a class is available (demo restricts to Paladin/Assassin)
Class indices: 0=Paladin, 1=Crusader, 2=Necromancer, 3=Assassin, 4=Demoness
================
*/
static qboolean M_H2_IsClassAvailable(int class_index)
{
	// Demo only allows Paladin (0) and Assassin (3)
	if (hexen2_demo)
		return (class_index == 0 || class_index == 3);

	// Non-Portals doesn't have Demoness (4)
	if (!M_H2_HasPortals() && class_index == 4)
		return false;

	return true;
}

void M_Menu_H2_Class_f(void)
{
	IN_DeactivateForMenu();
	key_dest = key_menu;
	m_state = m_class;
	m_entersound = true;
	h2_class_cursor = h2_player_class - 1;  // Convert 1-based to 0-based
	if (h2_class_cursor < 0)
		h2_class_cursor = 0;
	// Ensure cursor is on an available class
	if (!M_H2_IsClassAvailable(h2_class_cursor))
		h2_class_cursor = 0;	// Default to Paladin
	M_H2_ResetScrollTitle();
}

static void M_H2_Class_Draw(void)
{
	int y, i;
	int num_classes;

	// Draw animated title and plaque
	M_H2_ScrollTitle("gfx/menu/title2.lmp");

	num_classes = M_H2_GetNumClasses();

	// Draw class list
	y = 60;
	for (i = 0; i < num_classes; i++)
	{
		if (M_H2_IsClassAvailable(i))
			M_H2_DrawBigString(72, y, h2_class_names_upper[i]);
		else
			M_Print(76, y + 4, h2_class_names_upper[i]);	// Grayed out (smallfont)
		y += 20;
	}

	// Draw cursor
	M_H2_DrawCursor(43, 54 + h2_class_cursor * 20);

	// Draw class portrait frame and portrait on the right side
	if (h2_frame)
		M_DrawTransPic(242, 54, h2_frame);

	if (h2_class_cursor >= 0 && h2_class_cursor < 5 && h2_portraits[h2_class_cursor])
	{
		// Portrait goes inside the frame
		M_DrawTransPic(246, 58, h2_portraits[h2_class_cursor]);
	}
}

static void M_H2_Class_Key(int key)
{
	int num_classes = M_H2_GetNumClasses();
	int i;

	switch (key)
	{
	case K_ESCAPE:
	case K_BBUTTON:
	case K_MOUSE2:
		M_Menu_SinglePlayer_f();
		break;

	case K_DOWNARROW:
	case K_MWHEELDOWN:
		M_H2_NavSound();
		// Find next available class
		for (i = 0; i < num_classes; i++)
		{
			if (++h2_class_cursor >= num_classes)
				h2_class_cursor = 0;
			if (M_H2_IsClassAvailable(h2_class_cursor))
				break;
		}
		break;

	case K_UPARROW:
	case K_MWHEELUP:
		M_H2_NavSound();
		// Find previous available class
		for (i = 0; i < num_classes; i++)
		{
			if (--h2_class_cursor < 0)
				h2_class_cursor = num_classes - 1;
			if (M_H2_IsClassAvailable(h2_class_cursor))
				break;
		}
		break;

	case K_ENTER:
	case K_KP_ENTER:
	case K_ABUTTON:
	case K_MOUSE1:
		m_entersound = true;
		Cvar_SetValue("_cl_playerclass", h2_class_cursor + 1);  // Convert 0-based to 1-based
		M_Menu_H2_Difficulty_f();
		break;
	}
}

/*
================================================================================
DIFFICULTY SELECTION MENU
================================================================================
*/

void M_Menu_H2_Difficulty_f(void)
{
	IN_DeactivateForMenu();
	key_dest = key_menu;
	m_state = m_difficulty;
	m_entersound = true;
	M_H2_ResetScrollTitle();
}

static void M_H2_Difficulty_Draw(void)
{
	int y, i;
	int class_idx;
	const char **diff_names;

	// Draw animated title and plaque (title5.lmp for difficulty)
	M_H2_ScrollTitle("gfx/menu/title5.lmp");

	// Clamp class index
	class_idx = h2_player_class - 1;
	if (class_idx < 0 || class_idx >= H2_MAX_PLAYER_CLASS)
		class_idx = 0;

	// Get class-specific difficulty names
	diff_names = h2_diff_names[class_idx];

	// Draw difficulty options
	y = 60;
	for (i = 0; i < H2_NUM_DIFFLEVELS; i++)
	{
		M_H2_DrawBigString(72, y, diff_names[i]);
		y += 20;
	}

	// Draw cursor
	M_H2_DrawCursor(43, 54 + h2_diff_cursor * 20);
}

static void M_H2_Difficulty_Key(int key)
{
	switch (key)
	{
	case K_ESCAPE:
	case K_BBUTTON:
	case K_MOUSE2:
		M_Menu_H2_Class_f();
		break;

	case K_DOWNARROW:
	case K_MWHEELDOWN:
		M_H2_NavSound();
		if (++h2_diff_cursor >= H2_NUM_DIFFLEVELS)
			h2_diff_cursor = 0;
		break;

	case K_UPARROW:
	case K_MWHEELUP:
		M_H2_NavSound();
		if (--h2_diff_cursor < 0)
			h2_diff_cursor = H2_NUM_DIFFLEVELS - 1;
		break;

	case K_ENTER:
	case K_KP_ENTER:
	case K_ABUTTON:
	case K_MOUSE1:
		m_entersound = true;

		// Set the skill level and player class, then start game
		Cvar_SetValue("skill", h2_diff_cursor);
		// Note: playerclass is set via console command for H2
		Cbuf_AddText(va("playerclass %d\n", h2_player_class));

		IN_Activate();
		key_dest = key_game;
		m_state = m_none;

		// Start the game
		if (sv.active)
			Cbuf_AddText("disconnect\n");
		Cbuf_AddText("maxplayers 1\n");

		if (h2_enter_portals)
		{
			// Portals campaign: show intro intermission, then start keep1
			// uhexen2 uses CL_SetupIntermission(12) then key press starts keep1
			// For now, just start keep1 directly (TODO: add intermission 12)
			Cbuf_AddText("map keep1\n");
		}
		else
		{
			// Base H2 campaign: start demo1
			Cbuf_AddText("map demo1\n");
		}
		break;
	}
}

/*
================================================================================
HELP MENU
================================================================================
*/

static void M_H2_Help_Draw(void)
{
	if (h2_help[h2_help_page])
		M_DrawPic(0, 0, h2_help[h2_help_page]);
	else
	{
		// Fallback if help graphics not found
		M_Print(100, 100, "Help page not available");
	}
}

static void M_H2_Help_Key(int key)
{
	switch (key)
	{
	case K_ESCAPE:
	case K_BBUTTON:
	case K_MOUSE2:
		M_Menu_Main_f();
		break;

	case K_UPARROW:
	case K_RIGHTARROW:
	case K_MWHEELDOWN:
		M_H2_NavSound();
		if (++h2_help_page >= H2_NUM_HELP_PAGES)
			h2_help_page = 0;
		break;

	case K_DOWNARROW:
	case K_LEFTARROW:
	case K_MWHEELUP:
		M_H2_NavSound();
		if (--h2_help_page < 0)
			h2_help_page = H2_NUM_HELP_PAGES - 1;
		break;
	}
}

/*
================================================================================
MULTIPLAYER MENU
================================================================================
*/

#define H2_MP_ITEMS		5
#define H2_MP_JOIN		0
#define H2_MP_NEWGAME	1
#define H2_MP_SETUP		2
#define H2_MP_LOAD		3
#define H2_MP_SAVE		4

static int h2_mp_cursor;

static void M_H2_MultiPlayer_Draw(void)
{
	int y;

	// Draw animated title and plaque (title4.lmp for multiplayer)
	M_H2_ScrollTitle("gfx/menu/title4.lmp");

	// Menu items
	y = 60;
	M_H2_DrawBigString(72, y, "JOIN A GAME");
	y += 20;
	M_H2_DrawBigString(72, y, "NEW GAME");
	y += 20;
	M_H2_DrawBigString(72, y, "SETUP");
	y += 20;
	M_H2_DrawBigString(72, y, "LOAD");
	y += 20;
	M_H2_DrawBigString(72, y, "SAVE");

	// Draw cursor
	M_H2_DrawCursor(43, 54 + h2_mp_cursor * 20);

	// Check for network availability (like uhexen2)
	if (!tcpipAvailable)
		M_PrintWhite((320/2) - ((27*8)/2), 160, "No Communications Available");
}

static void M_H2_MultiPlayer_Key(int key)
{
	switch (key)
	{
	case K_ESCAPE:
	case K_BBUTTON:
	case K_MOUSE2:
		M_Menu_Main_f();
		break;

	case K_DOWNARROW:
	case K_MWHEELDOWN:
		M_H2_NavSound();
		if (++h2_mp_cursor >= H2_MP_ITEMS)
			h2_mp_cursor = 0;
		break;

	case K_UPARROW:
	case K_MWHEELUP:
		M_H2_NavSound();
		if (--h2_mp_cursor < 0)
			h2_mp_cursor = H2_MP_ITEMS - 1;
		break;

	case K_ENTER:
	case K_KP_ENTER:
	case K_ABUTTON:
	case K_MOUSE1:
		m_entersound = true;

		switch (h2_mp_cursor)
		{
		case H2_MP_JOIN:
			// Join a game - go to LAN config
			M_Menu_LanConfig_f();
			break;

		case H2_MP_NEWGAME:
			// Host a new game - go to game options
			M_Menu_GameOptions_f();
			break;

		case H2_MP_SETUP:
			// Player setup (H2 version with class selection)
			M_Menu_H2_Setup_f();
			break;

		case H2_MP_LOAD:
			// Load multiplayer game
			M_Menu_Load_f();
			break;

		case H2_MP_SAVE:
			// Save multiplayer game
			if (sv.active)
				M_Menu_Save_f();
			break;
		}
		break;
	}
}

/*
================================================================================
PLAYER SETUP MENU (with class selection)
================================================================================
*/

#define H2_SETUP_ITEMS		6
#define H2_SETUP_HOSTNAME	0
#define H2_SETUP_NAME		1
#define H2_SETUP_CLASS		2
#define H2_SETUP_TOP		3
#define H2_SETUP_BOTTOM		4
#define H2_SETUP_ACCEPT		5

static int h2_setup_cursor;
static int h2_setup_class;
static int h2_setup_top, h2_setup_bottom;
static int h2_setup_oldtop, h2_setup_oldbottom;
static char h2_setup_hostname[16];
static char h2_setup_myname[16];

static int h2_setup_cursor_table[] = {40, 56, 80, 104, 128, 156};

extern cvar_t cl_name;
extern cvar_t hostname;
extern cvar_t cl_color;

void M_Menu_H2_Setup_f(void)
{
	IN_DeactivateForMenu();
	key_dest = key_menu;
	m_state = m_setup;
	m_entersound = true;
	M_H2_ResetScrollTitle();

	q_strlcpy(h2_setup_myname, cl_name.string, sizeof(h2_setup_myname));
	q_strlcpy(h2_setup_hostname, hostname.string, sizeof(h2_setup_hostname));
	h2_setup_top = h2_setup_oldtop = ((int)cl_color.value) >> 4;
	h2_setup_bottom = h2_setup_oldbottom = ((int)cl_color.value) & 15;

	// Initialize class from playerclass cvar
	h2_setup_class = h2_player_class;
	if (h2_setup_class < 1 || h2_setup_class > H2_MAX_PLAYER_CLASS)
		h2_setup_class = 1;

	// Restrict class based on game version
	if (hexen2_demo)
	{
		// Demo: only Paladin (1) and Assassin (4)
		if (h2_setup_class != 1 && h2_setup_class != 4)
			h2_setup_class = 1;
	}
	else if (!M_H2_HasPortals())
	{
		// No Portals: no Demoness
		if (h2_setup_class > H2_NUM_BASE_CLASSES)
			h2_setup_class = H2_NUM_BASE_CLASSES;
	}
}

static void M_H2_Setup_Draw(void)
{
	// Draw animated title (title4 = multiplayer)
	M_H2_ScrollTitle("gfx/menu/title4.lmp");

	M_Print(64, 40, "Hostname");
	M_DrawTextBox(160, 32, 16, 1);
	M_Print(168, 40, h2_setup_hostname);

	M_Print(64, 56, "Your name");
	M_DrawTextBox(160, 48, 16, 1);
	M_Print(168, 56, h2_setup_myname);

	M_Print(64, 80, "Current Class:");
	if (h2_setup_class >= 1 && h2_setup_class <= H2_MAX_PLAYER_CLASS)
		M_Print(88, 88, h2_class_names_upper[h2_setup_class - 1]);

	M_Print(64, 104, "First color patch");
	M_Print(64, 128, "Second color patch");

	M_DrawTextBox(64, 148, 14, 1);
	M_Print(72, 156, "Accept Changes");

	// Draw color patches
	M_DrawTextBox(160, 80, 6, 2);
	// Note: Would need translated player pic - for now show color values
	M_Print(176, 98, va("top: %d", h2_setup_top));
	M_Print(176, 106, va("btm: %d", h2_setup_bottom));

	// Draw cursor
	M_H2_DrawCursor(43, h2_setup_cursor_table[h2_setup_cursor] - 6);

	// Draw text cursors for text fields
	if (h2_setup_cursor == H2_SETUP_HOSTNAME)
		M_DrawCharacter(168 + 8*strlen(h2_setup_hostname), 40, 10 + ((int)(realtime*4)&1));
	if (h2_setup_cursor == H2_SETUP_NAME)
		M_DrawCharacter(168 + 8*strlen(h2_setup_myname), 56, 10 + ((int)(realtime*4)&1));
}

static void M_H2_Setup_Key(int key)
{
	int l;

	switch (key)
	{
	case K_ESCAPE:
	case K_BBUTTON:
	case K_MOUSE2:
		M_Menu_MultiPlayer_f();
		break;

	case K_UPARROW:
	case K_MWHEELUP:
		M_H2_NavSound();
		if (--h2_setup_cursor < 0)
			h2_setup_cursor = H2_SETUP_ITEMS - 1;
		break;

	case K_DOWNARROW:
	case K_MWHEELDOWN:
		M_H2_NavSound();
		if (++h2_setup_cursor >= H2_SETUP_ITEMS)
			h2_setup_cursor = 0;
		break;

	case K_LEFTARROW:
		if (h2_setup_cursor < H2_SETUP_CLASS)
			return;
		M_H2_NavSound();
		if (h2_setup_cursor == H2_SETUP_CLASS)
		{
			// Cycle class backwards
			if (hexen2_demo)
			{
				// Demo: toggle between Paladin (1) and Assassin (4)
				h2_setup_class = (h2_setup_class == 1) ? 4 : 1;
			}
			else
			{
				int max_class = M_H2_HasPortals() ? H2_MAX_PLAYER_CLASS : H2_NUM_BASE_CLASSES;
				if (--h2_setup_class < 1)
					h2_setup_class = max_class;
			}
		}
		else if (h2_setup_cursor == H2_SETUP_TOP)
		{
			if (--h2_setup_top < 0)
				h2_setup_top = 10;
		}
		else if (h2_setup_cursor == H2_SETUP_BOTTOM)
		{
			if (--h2_setup_bottom < 0)
				h2_setup_bottom = 10;
		}
		break;

	case K_RIGHTARROW:
		if (h2_setup_cursor < H2_SETUP_CLASS)
			return;
forward:
		M_H2_NavSound();
		if (h2_setup_cursor == H2_SETUP_CLASS)
		{
			// Cycle class forwards
			if (hexen2_demo)
			{
				// Demo: toggle between Paladin (1) and Assassin (4)
				h2_setup_class = (h2_setup_class == 1) ? 4 : 1;
			}
			else
			{
				int max_class = M_H2_HasPortals() ? H2_MAX_PLAYER_CLASS : H2_NUM_BASE_CLASSES;
				if (++h2_setup_class > max_class)
					h2_setup_class = 1;
			}
		}
		else if (h2_setup_cursor == H2_SETUP_TOP)
		{
			if (++h2_setup_top > 10)
				h2_setup_top = 0;
		}
		else if (h2_setup_cursor == H2_SETUP_BOTTOM)
		{
			if (++h2_setup_bottom > 10)
				h2_setup_bottom = 0;
		}
		break;

	case K_ENTER:
	case K_KP_ENTER:
	case K_ABUTTON:
	case K_MOUSE1:
		if (h2_setup_cursor == H2_SETUP_HOSTNAME || h2_setup_cursor == H2_SETUP_NAME)
			return;

		if (h2_setup_cursor >= H2_SETUP_CLASS && h2_setup_cursor <= H2_SETUP_BOTTOM)
			goto forward;

		// Accept changes
		if (h2_setup_cursor == H2_SETUP_ACCEPT)
		{
			if (strcmp(cl_name.string, h2_setup_myname) != 0)
				Cbuf_AddText(va("name \"%s\"\n", h2_setup_myname));
			if (strcmp(hostname.string, h2_setup_hostname) != 0)
				Cvar_Set("hostname", h2_setup_hostname);
			if (h2_setup_top != h2_setup_oldtop || h2_setup_bottom != h2_setup_oldbottom)
				Cbuf_AddText(va("color %d %d\n", h2_setup_top, h2_setup_bottom));
			Cvar_SetValue("_cl_playerclass", h2_setup_class);
			m_entersound = true;
			M_Menu_MultiPlayer_f();
		}
		break;

	case K_BACKSPACE:
		if (h2_setup_cursor == H2_SETUP_HOSTNAME)
		{
			l = strlen(h2_setup_hostname);
			if (l > 0)
				h2_setup_hostname[l - 1] = 0;
		}
		else if (h2_setup_cursor == H2_SETUP_NAME)
		{
			l = strlen(h2_setup_myname);
			if (l > 0)
				h2_setup_myname[l - 1] = 0;
		}
		break;
	}
}

static void M_H2_Setup_Char(int key)
{
	int l;

	if (key < 32 || key > 127)
		return;

	if (h2_setup_cursor == H2_SETUP_HOSTNAME)
	{
		l = strlen(h2_setup_hostname);
		if (l < 15)
		{
			h2_setup_hostname[l + 1] = 0;
			h2_setup_hostname[l] = key;
		}
	}
	else if (h2_setup_cursor == H2_SETUP_NAME)
	{
		l = strlen(h2_setup_myname);
		if (l < 15)
		{
			h2_setup_myname[l + 1] = 0;
			h2_setup_myname[l] = key;
		}
	}
}

/*
================================================================================
DISPATCH FUNCTIONS
================================================================================
*/

/*
================
M_H2_Draw

Main draw dispatcher for Hexen II menus
================
*/
void M_H2_Draw(void)
{
	// Lazy-load graphics on first draw (after video init)
	M_H2_LoadGraphics();

	switch (m_state)
	{
	case m_main:
		M_H2_Main_Draw();
		break;

	case m_singleplayer:
		M_H2_SinglePlayer_Draw();
		break;

	case m_class:
		M_H2_Class_Draw();
		break;

	case m_difficulty:
		M_H2_Difficulty_Draw();
		break;

	case m_help:
		M_H2_Help_Draw();
		break;

	case m_multiplayer:
		M_H2_MultiPlayer_Draw();
		break;

	case m_setup:
		M_H2_Setup_Draw();
		break;

	default:
		// For menus that don't need H2 customization (options, load, save, etc.),
		// return false to let the normal menu code handle it
		break;
	}
}

/*
================
M_H2_Keydown

Main keydown dispatcher for Hexen II menus
Returns true if the key was handled, false to use standard menu handling
================
*/
void M_H2_Keydown(int key)
{
	switch (m_state)
	{
	case m_main:
		M_H2_Main_Key(key);
		break;

	case m_singleplayer:
		M_H2_SinglePlayer_Key(key);
		break;

	case m_class:
		M_H2_Class_Key(key);
		break;

	case m_difficulty:
		M_H2_Difficulty_Key(key);
		break;

	case m_help:
		M_H2_Help_Key(key);
		break;

	case m_multiplayer:
		M_H2_MultiPlayer_Key(key);
		break;

	case m_setup:
		M_H2_Setup_Key(key);
		break;

	default:
		// Other menus handled by standard menu code
		break;
	}
}

/*
================
M_H2_ShouldHandle

Returns true if the current menu state should be handled by H2 menu code
================
*/
qboolean M_H2_ShouldHandle(enum m_state_e state)
{
	switch (state)
	{
	case m_main:
	case m_singleplayer:
	case m_class:
	case m_difficulty:
	case m_help:
	case m_multiplayer:  // For H2 multiplayer title/layout
	case m_setup:        // For H2 player setup with class selection
		return true;
	default:
		return false;
	}
}

/*
================
M_H2_Charinput

Handle character input for text fields in H2 menus
================
*/
void M_H2_Charinput(int key)
{
	switch (m_state)
	{
	case m_setup:
		M_H2_Setup_Char(key);
		break;
	default:
		break;
	}
}

/*
================
M_H2_TextEntry

Returns text entry mode for current menu
================
*/
enum textmode_t M_H2_TextEntry(void)
{
	switch (m_state)
	{
	case m_setup:
		// Text entry for hostname or player name fields
		if (h2_setup_cursor == H2_SETUP_HOSTNAME || h2_setup_cursor == H2_SETUP_NAME)
			return TEXTMODE_ON;
		return TEXTMODE_OFF;
	default:
		return TEXTMODE_OFF;
	}
}
