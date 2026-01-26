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

// Title scrolling animation
static float title_start_time;
static const char *title_name;
static qboolean title_active;
static qpic_t *title_pic;

// Bigfont kerning table: 27 chars (A-Z plus /) x 27 chars
// Value = number of pixels to move left for the second character
static char BigCharWidth[27][27];
static qboolean bigfont_loaded = false;
static qpic_t *bigfont_pic = NULL;

// Player class for new game
int h2_player_class = 1;

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

/*
================
M_H2_HasPortals

Check if the Portals of Praevus expansion is available
================
*/
qboolean M_H2_HasPortals(void)
{
	// Check for portals pak file or command line parameter
	if (COM_CheckParm("-portals"))
		return true;

	// Check for portals directory with content
	// In H2, the Demoness class is only available with the mission pack
	// We can check for the presence of pak3.pak in the portals directory
	// or check for specific mission pack files

	// For now, also check if running from portals game dir
	if (q_strcasecmp(com_gamedir, "portals") == 0)
		return true;

	// TODO: Could also check for existence of specific mission pack files

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

	// Bigfont2.lmp is 160x80 pixels, containing 10 chars per row (16 pixels wide each)
	// Characters are arranged: A-Z (rows 0-2, 10 chars each minus a few), then /
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

	// Bigfont layout: 10 chars per row, each 16x20 pixels (approx)
	// Actually it's 160x80 total, so 16x26-27 per char
	row = num / 10;
	col = num % 10;

	// Draw the character as a subpic
	// Each char is roughly 16 pixels wide in the 160 wide texture
	srcx = col * 16;
	srcy = row * 26;

	M_DrawSubpic(x, y, bigfont_pic, srcx, srcy, 16, 26);
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

Set up animated title that scrolls down from top of screen
================
*/
void M_H2_ScrollTitle(const char *name)
{
	title_name = name;
	title_start_time = realtime;
	title_active = true;
	title_pic = Draw_CachePic(name);
}

/*
================
M_H2_DrawTitle

Draw the currently active scrolling title
Returns the Y position where menu content should start
================
*/
static int M_H2_DrawTitle(void)
{
	int y;
	float elapsed;
	float scroll_time = 0.5f;  // Time to complete scroll animation

	if (!title_active || !title_pic)
		return 28;  // Default starting position

	elapsed = realtime - title_start_time;

	if (elapsed < scroll_time)
	{
		// Animate from -title_pic->height to 0
		y = (int)(-title_pic->height * (1.0f - elapsed / scroll_time));
	}
	else
	{
		y = 0;
	}

	M_DrawTransPic((320 - title_pic->width) / 2, y, title_pic);

	// Return y position for menu content (below title)
	if (elapsed < scroll_time)
		return 28;  // While animating, use fixed position
	return title_pic->height + 4;
}

/*
================
M_H2_DrawPlaque

Draw the Hexen II plaque logo
================
*/
static void M_H2_DrawPlaque(void)
{
	if (h2_plaque)
		M_DrawTransPic(10, 4, h2_plaque);
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
	int cursor_offset;

	// Draw plaque
	M_H2_DrawPlaque();

	// Draw/animate title
	if (h2_titles[0])
	{
		M_H2_ScrollTitle("gfx/menu/title0.lmp");
		y = M_H2_DrawTitle();
	}
	else
	{
		y = 28;
	}

	// Draw menu items using bigfont
	y += 20;
	M_H2_DrawBigString(88, y, "SINGLE PLAYER");
	y += 26;
	M_H2_DrawBigString(88, y, "MULTIPLAYER");
	y += 26;
	M_H2_DrawBigString(88, y, "OPTIONS");
	y += 26;
	M_H2_DrawBigString(88, y, "MODS");
	y += 26;
	M_H2_DrawBigString(88, y, "HELP");
	y += 26;
	M_H2_DrawBigString(88, y, "QUIT");

	// Draw cursor
	cursor_offset = 48 + h2_main_cursor * 26;  // Adjusted for title position
	M_H2_DrawCursor(56, cursor_offset);
}

static void M_H2_Main_Key(int key)
{
	switch (key)
	{
	case K_ESCAPE:
	case K_BBUTTON:
	case K_MOUSE2:
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

#define H2_SP_ITEMS		3
#define H2_SP_NEWGAME		0
#define H2_SP_LOADGAME		1
#define H2_SP_SAVEGAME		2

static int h2_sp_cursor;

static void M_H2_SinglePlayer_Draw(void)
{
	int y;

	// Draw plaque
	M_H2_DrawPlaque();

	// Draw title
	if (h2_titles[1])
	{
		M_H2_ScrollTitle("gfx/menu/title1.lmp");
		y = M_H2_DrawTitle();
	}
	else
	{
		y = 28;
	}

	// Draw menu items
	y += 20;
	M_H2_DrawBigString(88, y, "NEW GAME");
	y += 26;
	M_H2_DrawBigString(88, y, "LOAD");
	y += 26;
	M_H2_DrawBigString(88, y, "SAVE");

	// Draw cursor
	M_H2_DrawCursor(56, 48 + h2_sp_cursor * 26);
}

static void M_H2_SinglePlayer_Key(int key)
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
		if (++h2_sp_cursor >= H2_SP_ITEMS)
			h2_sp_cursor = 0;
		break;

	case K_UPARROW:
	case K_MWHEELUP:
		M_H2_NavSound();
		if (--h2_sp_cursor < 0)
			h2_sp_cursor = H2_SP_ITEMS - 1;
		break;

	case K_ENTER:
	case K_KP_ENTER:
	case K_ABUTTON:
	case K_MOUSE1:
		m_entersound = true;

		switch (h2_sp_cursor)
		{
		case H2_SP_NEWGAME:
			M_Menu_H2_Class_f();
			break;

		case H2_SP_LOADGAME:
			M_Menu_Load_f();
			break;

		case H2_SP_SAVEGAME:
			if (sv.active)
				M_Menu_Save_f();
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

void M_Menu_H2_Class_f(void)
{
	IN_DeactivateForMenu();
	key_dest = key_menu;
	m_state = m_class;
	m_entersound = true;
	h2_class_cursor = h2_player_class - 1;  // Convert 1-based to 0-based
	if (h2_class_cursor < 0)
		h2_class_cursor = 0;
	title_active = false;
}

static void M_H2_Class_Draw(void)
{
	int y, i;
	int num_classes;
	int portrait_x, portrait_y;

	// Draw plaque
	M_H2_DrawPlaque();

	// Draw title
	if (h2_titles[2])
	{
		M_H2_ScrollTitle("gfx/menu/title2.lmp");
		y = M_H2_DrawTitle();
	}
	else
	{
		y = 28;
	}

	num_classes = M_H2_GetNumClasses();

	// Draw class list
	y = 64;
	for (i = 0; i < num_classes; i++)
	{
		M_H2_DrawBigString(88, y, h2_class_names_upper[i]);
		y += 26;
	}

	// Draw cursor
	M_H2_DrawCursor(56, 64 + h2_class_cursor * 26);

	// Draw class portrait on the right side
	portrait_x = 220;
	portrait_y = 64;

	// Draw frame first, then portrait inside
	if (h2_frame)
		M_DrawTransPic(portrait_x, portrait_y, h2_frame);

	if (h2_portraits[h2_class_cursor])
	{
		// Portrait goes inside the frame (offset by frame border)
		M_DrawTransPic(portrait_x + 8, portrait_y + 8, h2_portraits[h2_class_cursor]);
	}
}

static void M_H2_Class_Key(int key)
{
	int num_classes = M_H2_GetNumClasses();

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
		if (++h2_class_cursor >= num_classes)
			h2_class_cursor = 0;
		break;

	case K_UPARROW:
	case K_MWHEELUP:
		M_H2_NavSound();
		if (--h2_class_cursor < 0)
			h2_class_cursor = num_classes - 1;
		break;

	case K_ENTER:
	case K_KP_ENTER:
	case K_ABUTTON:
	case K_MOUSE1:
		m_entersound = true;
		h2_player_class = h2_class_cursor + 1;  // Convert 0-based to 1-based
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
	title_active = false;
}

static void M_H2_Difficulty_Draw(void)
{
	int y, i;
	const char **diff_names;

	// Draw plaque
	M_H2_DrawPlaque();

	// Draw title (title5.lmp for difficulty in H2)
	if (h2_titles[5])
	{
		M_H2_ScrollTitle("gfx/menu/title5.lmp");
		y = M_H2_DrawTitle();
	}
	else
	{
		y = 28;
	}

	// Show class name at top
	y = 50;
	M_Print(72, y, "Playing as:");
	y += 12;
	M_H2_DrawBigString(72, y, h2_class_names_upper[h2_player_class - 1]);
	y += 36;

	// Get class-specific difficulty names
	diff_names = h2_diff_names[h2_player_class - 1];

	// Draw difficulty options
	for (i = 0; i < H2_NUM_DIFFLEVELS; i++)
	{
		M_H2_DrawBigString(88, y, diff_names[i]);
		y += 26;
	}

	// Draw cursor
	M_H2_DrawCursor(56, 98 + h2_diff_cursor * 26);
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

		key_dest = key_game;
		m_state = m_none;

		// Start the game - H2 uses demo1 as first map
		if (sv.active)
			Cbuf_AddText("disconnect\n");
		Cbuf_AddText("maxplayers 1\n");
		Cbuf_AddText("map demo1\n");
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
		return true;
	default:
		return false;
	}
}
