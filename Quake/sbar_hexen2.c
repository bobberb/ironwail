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

// sbar_hexen2.c -- Hexen II status bar implementation

#include "quakedef.h"
#include "sbar_hexen2.h"
#include "cl_string_hexen2.h"

// Bar dimensions
#define BAR_TOP_HEIGHT		46
#define BAR_BOTTOM_HEIGHT	98
#define BAR_TOTAL_HEIGHT	(BAR_TOP_HEIGHT + BAR_BOTTOM_HEIGHT)
#define BAR_BUMP_HEIGHT		23

// Centering offset for 320-pixel-wide HUD
#define SB_XOFS				((vid.width - 320) / 2)

// Cached graphics
static qpic_t *sb_h2_nums[11];
static qpic_t *sb_h2_colon;
static qpic_t *sb_h2_slash;

// Top bar pieces
static qpic_t *sb_h2_topbar1;
static qpic_t *sb_h2_topbar2;
static qpic_t *sb_h2_topbumpl;
static qpic_t *sb_h2_topbumpm;
static qpic_t *sb_h2_topbumpr;

// Bottom bar pieces
static qpic_t *sb_h2_btmbar1;
static qpic_t *sb_h2_btmbar2;

// Mana graphics
static qpic_t *sb_h2_bmana;
static qpic_t *sb_h2_bmanacov;
static qpic_t *sb_h2_gmana;
static qpic_t *sb_h2_gmanacov;
static qpic_t *sb_h2_bmmana;
static qpic_t *sb_h2_gmmana;

// Health chain
static qpic_t *sb_h2_hpchain;
static qpic_t *sb_h2_hpgem;
static qpic_t *sb_h2_chnlcov;
static qpic_t *sb_h2_chnrcov;

// Armor icons
static qpic_t *sb_h2_armor[4];

// Ring graphics
static qpic_t *sb_h2_ring_f;
static qpic_t *sb_h2_ring_w;
static qpic_t *sb_h2_ring_t;
static qpic_t *sb_h2_ring_r;
static qpic_t *sb_h2_ringhlth;
static qpic_t *sb_h2_rhlthcvr;

// Artifact icons (loaded dynamically)
static qpic_t *sb_h2_arti[H2_INV_MAX];

// Class weapon icons
static qpic_t *sb_h2_weapons[5][4];	// [class][weapon]

// State
static float ChainPosition = 0;
static int inv_flg = 0;			// Inventory visible flag
static double inv_time = 0;		// Time inventory was shown
static qboolean sb_h2_loaded = false;

// Bar height for animated show/hide (default: only top bar visible)
static float BarHeight = BAR_TOP_HEIGHT;
static float BarTargetHeight = BAR_TOP_HEIGHT;
static qboolean sb_ShowInfo = false;
#define BAR_SPEED 15.0f		// Animation speed multiplier

// Forward declarations
static void Sbar_H2_DrawPic(int x, int y, qpic_t *pic);
static void Sbar_H2_DrawTransPic(int x, int y, qpic_t *pic);
static void Sbar_H2_DrawNum(int x, int y, int number, int digits);
static void Sbar_H2_DrawSmallNum(int x, int y, int number);
static void Sbar_H2_DrawTopBar(void);
static void Sbar_H2_DrawBottomBar(void);
static void Sbar_H2_DrawManaBar(int x, int y, int mana, int maxMana, qboolean blue);
static void Sbar_H2_DrawHealthChain(void);
static void Sbar_H2_DrawArmor(void);
static void Sbar_H2_DrawRings(void);
static void Sbar_H2_DrawArtifactInventory(void);
static void Sbar_H2_DrawInfoOverlay(void);
static qboolean Sbar_H2_SetChainPosition(float health, float maxHealth, qboolean immediate);

/*
===============
Sbar_H2_Init

Load Hexen II HUD graphics
===============
*/
void Sbar_H2_Init(void)
{
	int i;
	char name[64];

	if (sb_h2_loaded)
		return;

	// Numbers
	for (i = 0; i < 10; i++)
	{
		q_snprintf(name, sizeof(name), "num_%i", i);
		sb_h2_nums[i] = Draw_PicFromWad(name);
	}
	sb_h2_nums[10] = Draw_PicFromWad("num_minus");
	sb_h2_colon = Draw_PicFromWad("num_colon");
	sb_h2_slash = Draw_PicFromWad("num_slash");

	// Top bar
	sb_h2_topbar1 = Draw_CachePic("gfx/topbar1.lmp");
	sb_h2_topbar2 = Draw_CachePic("gfx/topbar2.lmp");
	sb_h2_topbumpl = Draw_CachePic("gfx/topbumpl.lmp");
	sb_h2_topbumpm = Draw_CachePic("gfx/topbumpm.lmp");
	sb_h2_topbumpr = Draw_CachePic("gfx/topbumpr.lmp");

	// Bottom bar
	sb_h2_btmbar1 = Draw_CachePic("gfx/btmbar1.lmp");
	sb_h2_btmbar2 = Draw_CachePic("gfx/btmbar2.lmp");

	// Mana
	sb_h2_bmana = Draw_CachePic("gfx/bmana.lmp");
	sb_h2_bmanacov = Draw_CachePic("gfx/bmanacov.lmp");
	sb_h2_gmana = Draw_CachePic("gfx/gmana.lmp");
	sb_h2_gmanacov = Draw_CachePic("gfx/gmanacov.lmp");
	sb_h2_bmmana = Draw_CachePic("gfx/bmmana.lmp");
	sb_h2_gmmana = Draw_CachePic("gfx/gmmana.lmp");

	// Health chain
	sb_h2_hpchain = Draw_CachePic("gfx/hpchain.lmp");
	sb_h2_hpgem = Draw_CachePic("gfx/hpgem.lmp");
	sb_h2_chnlcov = Draw_CachePic("gfx/chnlcov.lmp");
	sb_h2_chnrcov = Draw_CachePic("gfx/chnrcov.lmp");

	// Armor
	sb_h2_armor[0] = Draw_CachePic("gfx/armor1.lmp");
	sb_h2_armor[1] = Draw_CachePic("gfx/armor2.lmp");
	sb_h2_armor[2] = Draw_CachePic("gfx/armor3.lmp");
	sb_h2_armor[3] = Draw_CachePic("gfx/armor4.lmp");

	// Rings
	sb_h2_ring_f = Draw_CachePic("gfx/ring_f.lmp");
	sb_h2_ring_w = Draw_CachePic("gfx/ring_w.lmp");
	sb_h2_ring_t = Draw_CachePic("gfx/ring_t.lmp");
	sb_h2_ring_r = Draw_CachePic("gfx/ring_r.lmp");
	sb_h2_ringhlth = Draw_CachePic("gfx/ringhlth.lmp");
	sb_h2_rhlthcvr = Draw_CachePic("gfx/rhlthcvr.lmp");

	// Artifacts
	for (i = 0; i < H2_INV_MAX; i++)
	{
		q_snprintf(name, sizeof(name), "gfx/arti%02d.lmp", i);
		sb_h2_arti[i] = Draw_CachePic(name);
	}

	// Class weapons would be loaded per-class
	// TODO: Load weapon icons for each class

	sb_h2_loaded = true;
	Con_DPrintf("Hexen II HUD graphics loaded\n");
}

/*
===============
Sbar_H2_DrawPic

Draw a pic at the given location (solid)
===============
*/
static void Sbar_H2_DrawPic(int x, int y, qpic_t *pic)
{
	if (!pic)
		return;
	Draw_Pic(x + SB_XOFS, y + (vid.height - (int)BarHeight), pic);
}

/*
===============
Sbar_H2_DrawTransPic

Draw a transparent pic
Note: Ironwail's Draw_Pic handles transparency via alpha channel
===============
*/
static void Sbar_H2_DrawTransPic(int x, int y, qpic_t *pic)
{
	if (!pic)
		return;
	Draw_Pic(x + SB_XOFS, y + (vid.height - (int)BarHeight), pic);
}

/*
===============
Sbar_H2_DrawNum

Draw a number with H2 graphics
===============
*/
static void Sbar_H2_DrawNum(int x, int y, int number, int digits)
{
	char str[16];
	char *ptr;
	int l, frame;

	l = q_snprintf(str, sizeof(str), "%d", number);
	ptr = str;

	if (l > digits)
		ptr += (l - digits);
	if (l < digits)
		x += (digits - l) * 13;

	while (*ptr)
	{
		if (*ptr == '-')
			frame = 10;
		else
			frame = *ptr - '0';

		Sbar_H2_DrawTransPic(x, y, sb_h2_nums[frame]);
		x += 13;
		ptr++;
	}
}

/*
===============
Sbar_H2_DrawSmallNum

Draw a small number for artifact counts
===============
*/
static void Sbar_H2_DrawSmallNum(int x, int y, int number)
{
	// Draw small numbers using half-size characters (4x4 instead of 8x8)
	char str[8];
	int i;
	float draw_y = y + (vid.height - (int)BarHeight);

	q_snprintf(str, sizeof(str), "%d", number);
	for (i = 0; str[i]; i++)
	{
		Draw_CharacterEx(x + i * 4, draw_y, 4, 4, str[i]);
	}
}

/*
===============
Sbar_H2_SetChainPosition

Update the health chain gem position
===============
*/
static qboolean Sbar_H2_SetChainPosition(float health, float maxHealth, qboolean immediate)
{
	float targetPos;
	float diff;

	if (maxHealth <= 0)
		maxHealth = 100;

	// Calculate target position (0-195 range for the chain)
	targetPos = (health / maxHealth) * 195.0f;
	if (targetPos < 0)
		targetPos = 0;
	if (targetPos > 195)
		targetPos = 195;

	if (immediate)
	{
		ChainPosition = targetPos;
		return false;
	}

	diff = targetPos - ChainPosition;
	if (fabs(diff) < 0.1f)
	{
		ChainPosition = targetPos;
		return false;
	}

	// Smooth movement
	if (diff > 0)
		ChainPosition += host_frametime * 80;
	else
		ChainPosition -= host_frametime * 80;

	// Clamp
	if ((diff > 0 && ChainPosition > targetPos) ||
		(diff < 0 && ChainPosition < targetPos))
		ChainPosition = targetPos;

	return true;
}

/*
===============
Sbar_H2_DrawHealthChain

Draw the health chain and gem
===============
*/
static void Sbar_H2_DrawHealthChain(void)
{
	int chainX;

	Sbar_H2_SetChainPosition((float)cl.stats[STAT_HEALTH], 100.0f, false);

	chainX = 45 + (int)ChainPosition;

	// Draw chain (repeating pattern)
	Sbar_H2_DrawTransPic(45 + ((int)ChainPosition & 7), 38, sb_h2_hpchain);

	// Draw gem
	Sbar_H2_DrawTransPic(chainX, 36, sb_h2_hpgem);

	// Draw chain covers
	Sbar_H2_DrawPic(43, 36, sb_h2_chnlcov);
	Sbar_H2_DrawPic(267, 36, sb_h2_chnrcov);
}

/*
===============
Sbar_H2_DrawManaBar

Draw a mana bar (blue or green)
===============
*/
static void Sbar_H2_DrawManaBar(int x, int y, int mana, int maxMana, qboolean blue)
{
	int fillHeight;
	qpic_t *fill, *cover;

	if (maxMana <= 0)
		maxMana = 1;

	fillHeight = (int)((mana * 18.0f) / (float)maxMana + 0.5f);
	if (fillHeight > 18)
		fillHeight = 18;
	if (fillHeight < 0)
		fillHeight = 0;

	fill = blue ? sb_h2_bmana : sb_h2_gmana;
	cover = blue ? sb_h2_bmanacov : sb_h2_gmanacov;

	// Draw mana fill (offset by fill amount)
	Sbar_H2_DrawPic(x, y + (18 - fillHeight), fill);

	// Draw cover
	Sbar_H2_DrawPic(x, y + 1, cover);
}

/*
===============
Sbar_H2_DrawTopBar

Draw the top portion of the H2 status bar
===============
*/
static void Sbar_H2_DrawTopBar(void)
{
	// Draw top bar background
	Sbar_H2_DrawPic(0, 0, sb_h2_topbar1);
	Sbar_H2_DrawPic(160, 0, sb_h2_topbar2);

	// Draw bumps
	Sbar_H2_DrawTransPic(0, -23, sb_h2_topbumpl);
	Sbar_H2_DrawTransPic(138, -8, sb_h2_topbumpm);
	Sbar_H2_DrawTransPic(269, -23, sb_h2_topbumpr);

	// Draw mana bars
	// Blue mana at x=190, green at x=232
	Sbar_H2_DrawManaBar(190, 26, cl.stats[STAT_SHELLS], 100, true);	// Using STAT_SHELLS as placeholder
	Sbar_H2_DrawManaBar(232, 26, cl.stats[STAT_NAILS], 100, false);	// Using STAT_NAILS as placeholder

	// Draw health chain
	Sbar_H2_DrawHealthChain();

	// Draw health number
	Sbar_H2_DrawNum(58, 14, cl.stats[STAT_HEALTH], 3);
}

/*
===============
Sbar_H2_DrawArmor

Draw the armor slots
===============
*/
static void Sbar_H2_DrawArmor(void)
{
	// H2 has individual armor values (armor_amulet, armor_bracer, etc.)
	// which require entity-level data syncing to display accurately.
	// Using threshold-based approximation based on total armor value.
	int armor = cl.stats[STAT_ARMOR];

	if (armor > 0)
		Sbar_H2_DrawPic(164, 115, sb_h2_armor[0]);	// Amulet
	if (armor > 50)
		Sbar_H2_DrawPic(205, 115, sb_h2_armor[1]);	// Bracer
	if (armor > 100)
		Sbar_H2_DrawPic(246, 115, sb_h2_armor[2]);	// Breastplate
	if (armor > 150)
		Sbar_H2_DrawPic(285, 115, sb_h2_armor[3]);	// Helmet
}

/*
===============
Sbar_H2_DrawRings

Draw active ring status
===============
*/
static void Sbar_H2_DrawRings(void)
{
	int ringhealth;

	// Ring of Flight
	if (cl.ring_flight > 0)
	{
		Sbar_H2_DrawTransPic(6, 119, sb_h2_ring_f);

		ringhealth = (int)cl.ring_flight;
		if (ringhealth > 100)
			ringhealth = 100;
		Sbar_H2_DrawPic(35 - (int)(26 * (ringhealth / 100.0f)), 142, sb_h2_ringhlth);
		Sbar_H2_DrawPic(35, 142, sb_h2_rhlthcvr);
	}

	// Ring of Water Breathing
	if (cl.ring_water > 0)
	{
		Sbar_H2_DrawTransPic(44, 119, sb_h2_ring_w);

		ringhealth = (int)cl.ring_water;
		if (ringhealth > 100)
			ringhealth = 100;
		Sbar_H2_DrawPic(73 - (int)(26 * (ringhealth / 100.0f)), 142, sb_h2_ringhlth);
		Sbar_H2_DrawPic(73, 142, sb_h2_rhlthcvr);
	}

	// Ring of Turning
	if (cl.ring_turning > 0)
	{
		Sbar_H2_DrawTransPic(81, 119, sb_h2_ring_t);

		ringhealth = (int)cl.ring_turning;
		if (ringhealth > 100)
			ringhealth = 100;
		Sbar_H2_DrawPic(110 - (int)(26 * (ringhealth / 100.0f)), 142, sb_h2_ringhlth);
		Sbar_H2_DrawPic(110, 142, sb_h2_rhlthcvr);
	}

	// Ring of Regeneration
	if (cl.ring_regeneration > 0)
	{
		Sbar_H2_DrawTransPic(119, 119, sb_h2_ring_r);

		ringhealth = (int)cl.ring_regeneration;
		if (ringhealth > 100)
			ringhealth = 100;
		Sbar_H2_DrawPic(148 - (int)(26 * (ringhealth / 100.0f)), 142, sb_h2_ringhlth);
		Sbar_H2_DrawPic(148, 142, sb_h2_rhlthcvr);
	}
}

/*
===============
Sbar_H2_DrawBarArtifactIcon

Draw an artifact icon in the inventory bar
===============
*/
static void Sbar_H2_DrawBarArtifactIcon(int x, int y, int artifact)
{
	int count;
	float flashtime;
	int flashon;

	if (artifact < 0 || artifact >= H2_INV_MAX)
		return;

	// Check if item was recently acquired (blink for 1 second)
	flashtime = cl.time - cl.inv_gettime[artifact];
	if (flashtime >= 0 && flashtime < 1)
	{
		// Flash on/off 5 times per second
		flashon = (int)(flashtime * 10) % 2;
		if (!flashon)
			return;	// Don't draw during "off" phase of blink
	}

	Sbar_H2_DrawTransPic(x, y, sb_h2_arti[artifact]);

	count = cl.inv_cnt[artifact];
	if (count > 0)
	{
		Sbar_H2_DrawSmallNum(x + 20, y + 21, count);
	}
}

/*
===============
Sbar_H2_DrawArtifactInventory

Draw the artifact inventory bar
===============
*/
static qpic_t *sb_h2_artisel = NULL;

static void Sbar_H2_DrawArtifactInventory(void)
{
	int i;
	int x, y;

	// Check if inventory should be visible
	if (inv_time == 0 || realtime > inv_time + H2_INV_DISPLAY_TIME)
	{
		inv_flg = 0;
		return;
	}

	if (!inv_flg)
		return;

	if (!cl.inv_count)
		return;

	// Load selection highlight if needed
	if (!sb_h2_artisel)
		sb_h2_artisel = Draw_CachePic("gfx/artisel.lmp");

	y = -37;  // Above the top bar

	// Draw up to INV_MAX_ICON artifacts
	for (i = 0, x = 64; i < H2_INV_MAX_ICON; i++, x += 33)
	{
		if (i >= cl.inv_count)
			break;

		// Highlight selected item
		if ((cl.inv_startpos + i) % cl.inv_count == cl.inv_selected)
		{
			if (sb_h2_artisel)
				Sbar_H2_DrawTransPic(x + 9, y - 12, sb_h2_artisel);
		}

		Sbar_H2_DrawBarArtifactIcon(x, y, cl.inv_order[(cl.inv_startpos + i) % cl.inv_count]);
	}
}

/*
===============
Sbar_H2_DrawPuzzlePieces

Draw collected puzzle pieces
===============
*/
static void Sbar_H2_DrawPuzzlePieces(void)
{
	int i, piece;
	char puzPath[64];

	piece = 0;
	for (i = 0; i < 8; i++)
	{
		if (cl.puzzle_pieces[i][0] == 0)
			continue;

		// Draw puzzle piece icon (4 per row, 2 rows)
		q_snprintf(puzPath, sizeof(puzPath), "gfx/puzzle/%s.lmp", cl.puzzle_pieces[i]);
		Sbar_H2_DrawPic(194 + (piece % 4) * 31, piece < 4 ? 51 : 82, Draw_CachePic(puzPath));
		piece++;
	}
}

/*
===============
Sbar_H2_DrawBottomBar

Draw the bottom portion of the H2 status bar
===============
*/
static void Sbar_H2_DrawBottomBar(void)
{
	char classIcon[64];

	// Draw bottom bar background
	Sbar_H2_DrawPic(0, 46, sb_h2_btmbar1);
	Sbar_H2_DrawPic(160, 46, sb_h2_btmbar2);

	// Draw class/weapon icon
	q_snprintf(classIcon, sizeof(classIcon), "gfx/cport%d.lmp",
			   CLAMP(1, cl.playerclass, 5));
	Sbar_H2_DrawPic(134, 50, Draw_CachePic(classIcon));

	// Draw armor
	Sbar_H2_DrawArmor();

	// Draw rings
	Sbar_H2_DrawRings();

	// Draw puzzle pieces
	Sbar_H2_DrawPuzzlePieces();

	// Draw frags in deathmatch
	if (cl.gametype == GAME_DEATHMATCH)
	{
		Sbar_H2_DrawNum(194, 48, cl.stats[STAT_FRAGS], 4);
	}
}

/*
===============
Sbar_H2_Draw

Main H2 status bar drawing function
===============
*/
void Sbar_H2_Draw(void)
{
	float delta;

	if (!sb_h2_loaded)
	{
		// Try to load H2 graphics
		Sbar_H2_Init();
		if (!sb_h2_loaded)
		{
			// Fall back to simple text display
			Draw_String(8, vid.height - 16, va("HP:%3d  Armor:%3d",
				cl.stats[STAT_HEALTH], cl.stats[STAT_ARMOR]));
			return;
		}
	}

	// Animate bar height toward target
	if (BarHeight < BarTargetHeight)
	{
		delta = (BarTargetHeight - BarHeight) * BAR_SPEED * host_frametime;
		if (delta < 1)
			delta = 1;
		BarHeight += delta;
		if (BarHeight > BarTargetHeight)
			BarHeight = BarTargetHeight;
	}
	else if (BarHeight > BarTargetHeight)
	{
		delta = (BarHeight - BarTargetHeight) * BAR_SPEED * host_frametime;
		if (delta < 1)
			delta = 1;
		BarHeight -= delta;
		if (BarHeight < BarTargetHeight)
			BarHeight = BarTargetHeight;
	}

	// Always draw top bar
	Sbar_H2_DrawTopBar();

	// Only draw bottom bar if bar is raised
	if (BarHeight > BAR_TOP_HEIGHT)
		Sbar_H2_DrawBottomBar();

	// Draw artifact inventory above top bar
	Sbar_H2_DrawArtifactInventory();

	// Draw info overlay if active
	Sbar_H2_DrawInfoOverlay();
}

/*
===============
Sbar_H2_DrawMini

Minimal HUD for full-screen mode
===============
*/
void Sbar_H2_DrawMini(void)
{
	// Just draw health and mana numbers
	Sbar_H2_DrawNum(10, vid.height - 30, cl.stats[STAT_HEALTH], 3);

	// Draw small mana indicators
	if (sb_h2_bmmana)
		Draw_Pic(10, vid.height - 50, sb_h2_bmmana);
	if (sb_h2_gmmana)
		Draw_Pic(10, vid.height - 68, sb_h2_gmmana);
}

/*
===============
Sbar_H2_IntermissionOverlay

Draw intermission screen (level completion stats)
===============
*/
void Sbar_H2_IntermissionOverlay(void)
{
	qpic_t	*pic;
	char	str[80];
	int		y;

	// In deathmatch, show scoreboard instead
	if (cl.gametype == GAME_DEATHMATCH)
	{
		Sbar_DeathmatchOverlay();
		return;
	}

	GL_SetCanvas(CANVAS_MENU);

	// Draw H2 intermission background if available, otherwise plain background
	pic = Draw_CachePic("gfx/meso.lmp");
	if (pic)
		Draw_Pic((320 - pic->width) / 2, (200 - pic->height) / 2, pic);

	// Draw "Level Complete" text
	y = 40;
	Draw_String(160 - 7 * 4, y, "Level Complete");
	y += 24;

	// Draw level name
	if (cl.levelname[0])
	{
		char map[80];
		Mod_SanitizeMapDescription(map, sizeof(map), cl.levelname);
		COM_TintString(map, map, sizeof(map));
		Draw_String(160 - strlen(map) * 4, y, map);
	}
	else
	{
		Draw_String(160 - strlen(cl.mapname) * 4, y, cl.mapname);
	}
	y += 32;

	// Time
	q_snprintf(str, sizeof(str), "Time:    %d:%02d", cl.completed_time / 60, cl.completed_time % 60);
	Draw_String(100, y, str);
	y += 16;

	// Secrets
	q_snprintf(str, sizeof(str), "Secrets: %d/%d", cl.stats[STAT_SECRETS], cl.stats[STAT_TOTALSECRETS]);
	Draw_String(100, y, str);
	y += 16;

	// Monsters
	q_snprintf(str, sizeof(str), "Kills:   %d/%d", cl.stats[STAT_MONSTERS], cl.stats[STAT_TOTALMONSTERS]);
	Draw_String(100, y, str);
}

/*
===============
Sbar_H2_FinaleOverlay

Draw finale screen (end of episode/game)
===============
*/
void Sbar_H2_FinaleOverlay(void)
{
	qpic_t	*pic;

	GL_SetCanvas(CANVAS_MENU);

	// Try to load H2 finale graphic, fall back to generic message
	pic = Draw_CachePic("gfx/finale.lmp");
	if (pic)
	{
		Draw_Pic((320 - pic->width) / 2, 16, pic);
	}
	else
	{
		// Fallback text if no graphic
		Draw_String(160 - 6 * 4, 80, "The End");
	}
}

/*
===============
Inventory commands
===============
*/
void Sbar_H2_InvLeft(void)
{
	if (cl.inv_count == 0)
		return;

	// Scroll selection left, wrapping around
	if (cl.inv_selected > 0)
		cl.inv_selected--;
	else
		cl.inv_selected = cl.inv_count - 1;

	inv_flg = 1;
	inv_time = realtime;
}

void Sbar_H2_InvRight(void)
{
	if (cl.inv_count == 0)
		return;

	// Scroll selection right, wrapping around
	if (cl.inv_selected < cl.inv_count - 1)
		cl.inv_selected++;
	else
		cl.inv_selected = 0;

	inv_flg = 1;
	inv_time = realtime;
}

void Sbar_H2_InvUse(void)
{
	if (cl.inv_count == 0)
		return;

	// Send inventory use command to server
	MSG_WriteByte(&cls.message, clc_stringcmd);
	MSG_WriteString(&cls.message, va("impulse %d", 100 + cl.inv_order[cl.inv_selected]));
}

void Sbar_H2_InvOff(void)
{
	inv_flg = 0;
	inv_time = 0;
}

/*
===============
Sbar_H2_InvChanged

Called when artifact counts change. Rebuilds the inventory order list.
===============
*/
void Sbar_H2_InvChanged(void)
{
	int i, position;
	qboolean examined[H2_INV_MAX];
	qboolean force_update = false;

	memset(examined, 0, sizeof(examined));

	// Check if currently selected item was depleted
	if (cl.inv_selected >= 0 && cl.inv_selected < cl.inv_count)
	{
		if (cl.inv_cnt[cl.inv_order[cl.inv_selected]] == 0)
			force_update = true;
	}

	// Remove items we no longer have from the order
	for (i = position = 0; i < cl.inv_count; i++)
	{
		if (cl.inv_cnt[cl.inv_order[i]] > 0)
		{
			cl.inv_order[position] = cl.inv_order[i];
			examined[cl.inv_order[position]] = true;
			position++;
		}
	}

	// Add in new items that we have but aren't in the order yet
	for (i = 0; i < H2_INV_MAX; i++)
	{
		if (!examined[i] && cl.inv_cnt[i] > 0)
		{
			cl.inv_order[position] = i;
			position++;
		}
	}

	cl.inv_count = position;

	// Fix selection if out of bounds
	if (cl.inv_selected >= cl.inv_count)
	{
		cl.inv_selected = cl.inv_count - 1;
		force_update = true;
	}
	if (cl.inv_count > 0 && cl.inv_selected < 0)
	{
		cl.inv_selected = 0;
		force_update = true;
	}

	// Fix startpos if out of bounds
	if (cl.inv_count <= 1)
		cl.inv_startpos = 0;
	else if (cl.inv_startpos >= cl.inv_count)
		cl.inv_startpos = cl.inv_selected;
	else
	{
		// Make sure selected item is visible
		int vis_pos = cl.inv_selected - cl.inv_startpos;
		if (vis_pos < 0)
			vis_pos += cl.inv_count;
		if (vis_pos >= H2_INV_MAX_ICON)
			cl.inv_startpos = cl.inv_selected;
	}

	(void)force_update;  // Suppress unused warning for now
}

/*
===============
Info display toggles
===============
*/
static qboolean sb_h2_showinfo = false;
static qboolean sb_h2_showdm = false;

void Sbar_H2_ShowInfo(qboolean show)
{
	if (show && !sb_ShowInfo)
	{
		S_LocalSound("misc/barmovup.wav");
		BarTargetHeight = BAR_TOTAL_HEIGHT;
		sb_ShowInfo = true;
	}
	else if (!show && sb_ShowInfo)
	{
		BarTargetHeight = BAR_TOP_HEIGHT;
		sb_ShowInfo = false;
	}
	sb_h2_showinfo = show;
}

void Sbar_H2_ShowDM(qboolean show)
{
	sb_h2_showdm = show;
}

/*
===============
Sbar_H2_DrawInfoOverlay

Draw puzzle pieces and objectives when +showinfo is pressed
===============
*/
static void Sbar_H2_DrawInfoOverlay(void)
{
	int i, y, piece;
	char puzPath[64];

	if (!sb_h2_showinfo)
		return;

	// Draw puzzle pieces with names
	y = 32;
	piece = 0;

	Draw_String(80, y, "Puzzle Pieces:");
	y += 16;

	for (i = 0; i < 8; i++)
	{
		const char *fullname;

		if (cl.puzzle_pieces[i][0] == 0)
			continue;

		// Draw puzzle piece icon
		q_snprintf(puzPath, sizeof(puzPath), "gfx/puzzle/%s.lmp", cl.puzzle_pieces[i]);
		Draw_Pic(80, y, Draw_CachePic(puzPath));

		// Draw puzzle piece name (use full name from puzzles.txt if available)
		fullname = CL_FindPuzzleString(cl.puzzle_pieces[i]);
		Draw_String(120, y + 8, fullname ? fullname : cl.puzzle_pieces[i]);

		y += 36;
		piece++;
	}

	if (piece == 0)
	{
		Draw_String(80, y, "(none)");
	}
}
