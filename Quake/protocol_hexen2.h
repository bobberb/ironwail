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

#ifndef _QUAKE_PROTOCOL_HEXEN2_H
#define _QUAKE_PROTOCOL_HEXEN2_H

// protocol_hexen2.h -- Hexen II communications protocols

// Hexen II protocol versions
#define	PROTOCOL_RAVEN_107		15	// CD version, aka 1.03 (not supported)
#define	PROTOCOL_RAVEN_109		17	// Official 1.09 update (not supported)
#define	PROTOCOL_RAVEN_111		18	// Official 1.11 update
#define	PROTOCOL_RAVEN_112		19	// 1.12, mission pack
#define	PROTOCOL_UQE_113		20	// Korax UQE patch 1.13

// The default Hexen II protocol
#define	H2_PROTOCOL_VERSION		PROTOCOL_RAVEN_112

//==================
// Hexen II specific server to client messages
// (Note: 0-35 are shared with Quake for compatibility)
//==================

// Hexen II specific svc messages
#define	svc_h2_raineffect			21	// Rain effect
#define	svc_h2_particle2			34	// [vec3] <variable>
#define	svc_h2_cutscene				35
#define	svc_h2_midi_name			36	// [string] name
#define	svc_h2_updateclass			37	// [byte] [byte]
#define	svc_h2_particle3			38
#define	svc_h2_particle4			39
#define	svc_h2_set_view_flags		40
#define	svc_h2_clear_view_flags		41
#define	svc_h2_start_effect			42	// Effect streaming start
#define	svc_h2_end_effect			43	// Effect streaming end
#define	svc_h2_plaque				44	// Display plaque
#define	svc_h2_particle_explosion	45	// Material-aware explosion
#define	svc_h2_set_view_tint		46	// Set view color tint
#define	svc_h2_reference			47
#define	svc_h2_clear_edicts			48
#define	svc_h2_update_inv			49	// Update inventory
#define	svc_h2_setangle_interpolate	50	// Interpolated angle setting
#define	svc_h2_update_kingofhill	51	// King of the hill update
#define	svc_h2_toggle_statbar		52	// Toggle status bar
#define	svc_h2_sound_update_pos		53	// [short] ent+channel [coord3] pos
#define	svc_h2_mod_name				54	// [string] name (UQE v1.13, music file name)
#define	svc_h2_skybox				55	// [string] name (UQE v1.13, skybox name)

//==================
// Hexen II specific client to server messages
//==================
#define	clc_h2_inv_select	5	// Inventory selection
#define	clc_h2_frame		6	// Frame update

//==================
// Hexen II entity update flags
// (These extend the Quake update flags)
//==================
#define	H2_U_CLEAR_ENT		(1<<11)	// Clear entity
#define	H2_U_ENT_OFF		(1<<13)	// Entity off
#define	H2_U_MOREBITS2		(1<<15)	// More update bits follow
#define	H2_U_SKIN			(1<<16)	// Skin changed
#define	H2_U_EFFECTS		(1<<17)	// Effects changed
#define	H2_U_SCALE			(1<<18)	// Scale changed (H2 specific)
#define	H2_U_COLORMAP		(1<<19)	// Colormap changed

//==================
// Hexen II client data update flags
//==================
#define	H2_SU_IDEALROLL		(1<<8)	// Ideal roll for view
#define	H2_SU_SC1			(1<<9)	// Stat changes 1
#define	H2_SU_SC2			(1<<15)	// Stat changes 2

//==================
// Hexen II stat change bits (SC1)
// These indicate which player stats have changed
//==================
#define H2_SC1_HEALTH			(1<<0)	// Health changed
#define H2_SC1_LEVEL			(1<<1)	// Player level
#define H2_SC1_INTELLIGENCE		(1<<2)	// Intelligence stat
#define H2_SC1_WISDOM			(1<<3)	// Wisdom stat
#define H2_SC1_STRENGTH			(1<<4)	// Strength stat
#define H2_SC1_DEXTERITY		(1<<5)	// Dexterity stat
#define H2_SC1_WEAPON			(1<<6)	// Current weapon
#define H2_SC1_BLUEMANA			(1<<7)	// Blue mana amount
#define H2_SC1_GREENMANA		(1<<8)	// Green mana amount
#define H2_SC1_EXPERIENCE		(1<<9)	// Experience points
#define H2_SC1_CNT_TORCH		(1<<10)	// Torch artifact count
#define H2_SC1_CNT_H_BOOST		(1<<11)	// Health boost count
#define H2_SC1_CNT_SH_BOOST		(1<<12)	// Super health boost count
#define H2_SC1_CNT_MANA_BOOST	(1<<13)	// Mana boost count
#define H2_SC1_CNT_TELEPORT		(1<<14)	// Teleport artifact count
#define H2_SC1_CNT_TOME			(1<<15)	// Tome of power count
#define H2_SC1_CNT_SUMMON		(1<<16)	// Summon artifact count
#define H2_SC1_CNT_INVISIBILITY	(1<<17)	// Invisibility count
#define H2_SC1_CNT_GLYPH		(1<<18)	// Glyph artifact count
#define H2_SC1_CNT_HASTE		(1<<19)	// Haste artifact count
#define H2_SC1_CNT_BLAST		(1<<20)	// Blast artifact count
#define H2_SC1_CNT_POLYMORPH	(1<<21)	// Polymorph count
#define H2_SC1_CNT_FLIGHT		(1<<22)	// Flight artifact count
#define H2_SC1_CNT_CUBEOFFORCE	(1<<23)	// Cube of force count
#define H2_SC1_CNT_INVINCIBILITY (1<<24) // Invincibility count
#define H2_SC1_ARTIFACT_ACTIVE	(1<<25)	// Artifact currently active
#define H2_SC1_ARTIFACT_LOW		(1<<26)	// Artifact running low
#define H2_SC1_MOVETYPE			(1<<27)	// Movement type changed
#define H2_SC1_CAMERAMODE		(1<<28)	// Camera mode
#define H2_SC1_HASTED			(1<<29)	// Player hasted
#define H2_SC1_INVENTORY		(1<<30)	// Inventory mask
#define H2_SC1_RINGS_ACTIVE		(1<<31)	// Rings active

//==================
// Hexen II stat change bits (SC2)
//==================
#define H2_SC2_RINGS_LOW		(1<<0)	// Rings running low
#define H2_SC2_AMULET			(1<<1)	// Amulet equipped
#define H2_SC2_BRACER			(1<<2)	// Bracer equipped
#define H2_SC2_BREASTPLATE		(1<<3)	// Breastplate equipped
#define H2_SC2_HELMET			(1<<4)	// Helmet equipped
#define H2_SC2_FLIGHT_T			(1<<5)	// Flight time
#define H2_SC2_WATER_T			(1<<6)	// Water time
#define H2_SC2_TURNING_T		(1<<7)	// Turning time
#define H2_SC2_REGEN_T			(1<<8)	// Regeneration time
#define H2_SC2_HASTE_T			(1<<9)	// Haste time
#define H2_SC2_TOME_T			(1<<10)	// Tome time
#define H2_SC2_PUZZLE1			(1<<11)	// Puzzle piece 1
#define H2_SC2_PUZZLE2			(1<<12)	// Puzzle piece 2
#define H2_SC2_PUZZLE3			(1<<13)	// Puzzle piece 3
#define H2_SC2_PUZZLE4			(1<<14)	// Puzzle piece 4
#define H2_SC2_PUZZLE5			(1<<15)	// Puzzle piece 5
#define H2_SC2_PUZZLE6			(1<<16)	// Puzzle piece 6
#define H2_SC2_PUZZLE7			(1<<17)	// Puzzle piece 7
#define H2_SC2_PUZZLE8			(1<<18)	// Puzzle piece 8
#define H2_SC2_MAXHEALTH		(1<<19)	// Max health
#define H2_SC2_MAXMANA			(1<<20)	// Max mana
#define H2_SC2_FLAGS			(1<<21)	// Player flags
#define H2_SC2_OBJ				(1<<22)	// Objective
#define H2_SC2_OBJ2				(1<<23)	// Objective 2

//==================
// Hexen II player classes
//==================
#define H2_CLASS_PALADIN		1
#define H2_CLASS_CRUSADER		2
#define H2_CLASS_NECROMANCER	3
#define H2_CLASS_ASSASSIN		4
#define H2_CLASS_DEMONESS		5	// Mission pack only
#define H2_NUM_CLASSES			5

//==================
// Hexen II artifact flags (artifact_active bits)
// Used for special powerup visual effects
//==================
#define H2_ART_INVISIBILITY			1
#define H2_ART_INVINCIBILITY		2
#define H2_ART_SUPER_HP				4
#define H2_ART_HASTE				8
#define H2_ART_CUBEOFFORCE			16
#define H2_ART_TOMEOFPOWER			32
#define H2_ARTFLAG_FROZEN			(1<<7)	// Player is frozen
#define H2_ARTFLAG_STONED			(1<<8)	// Player is turned to stone
#define H2_ARTFLAG_DIVINE_INTERVENTION	(1<<9)	// Divine intervention whiteout

//==================
// Hexen II inventory defines
//==================
#define H2_MAX_INVENTORY		15	// Maximum inventory items
#define H2_MAX_PUZZLE_PIECES	8	// Maximum puzzle pieces

//==================
// Hexen II effects (CE_ constants)
// Used with svc_h2_start_effect/svc_h2_end_effect
//==================
#define H2_CE_RAIN				0
#define H2_CE_FOUNTAIN			1
#define H2_CE_QUAKE				2
#define H2_CE_WHITE_SMOKE		3
#define H2_CE_BLUESPARK			4
#define H2_CE_YELLOWSPARK		5
#define H2_CE_SM_CIRCLE_EXP		6
#define H2_CE_BG_CIRCLE_EXP		7
#define H2_CE_SM_WHITE_FLASH	8
#define H2_CE_WHITE_FLASH		9
#define H2_CE_YELLOWRED_FLASH	10
#define H2_CE_BLUE_FLASH		11
#define H2_CE_SM_BLUE_FLASH		12
#define H2_CE_RED_FLASH			13
#define H2_CE_SM_EXPLOSION		14
#define H2_CE_LG_EXPLOSION		15
#define H2_CE_FLOOR_EXPLOSION	16
#define H2_CE_RIDER_DEATH		17
#define H2_CE_BLUE_EXPLOSION	18
#define H2_CE_GREEN_SMOKE		19
#define H2_CE_GREY_SMOKE		20
#define H2_CE_RED_SMOKE			21
#define H2_CE_SLOW_WHITE_SMOKE	22
#define H2_CE_REDSPARK			23
#define H2_CE_GREENSPARK		24
#define H2_CE_TELESMK1			25
#define H2_CE_TELESMK2			26
#define H2_CE_ICEHIT			27
#define H2_CE_MEDUSA_HIT		28
#define H2_CE_MEZZO_REFLECT		29
#define H2_CE_FLOOR_EXPLOSION2	30
#define H2_CE_XBOW_EXPLOSION	31
#define H2_CE_NEW_EXPLOSION		32
#define H2_CE_MAGIC_MISSILE_EXPLOSION 33
#define H2_CE_GHOST				34
#define H2_CE_BONE_EXPLOSION	35
#define H2_CE_REDCLOUD			36
#define H2_CE_TELEPORTERPUFFS	37
#define H2_CE_TELEPORTERBODY	38
#define H2_CE_BONESHARD			39
#define H2_CE_BONESHRAPNEL		40
#define H2_CE_FLAMESTREAM		41
#define H2_CE_SNOW				42
#define H2_CE_GRAVITYWELL		43
#define H2_CE_BLDRN_EXPL		44
#define H2_CE_ACID_MUZZFL		45
#define H2_CE_ACID_HIT			46
#define H2_CE_FIREWALL_SMALL	47
#define H2_CE_FIREWALL_MEDIUM	48
#define H2_CE_FIREWALL_LARGE	49
#define H2_CE_LBALL_EXPL		50
#define H2_CE_ACID_SPLAT		51
#define H2_CE_ACID_EXPL			52
#define H2_CE_FBOOM				53
#define H2_CE_CHUNK				54
#define H2_CE_BOMB				55
#define H2_CE_BRN_BOUNCE		56
#define H2_CE_LSHOCK			57
#define H2_CE_FLAMEWALL			58
#define H2_CE_FLAMEWALL2		59
#define H2_CE_FLOOR_EXPLOSION3	60
#define H2_CE_ONFIRE			61

#define H2_MAX_EFFECTS			256

//==================
// Hexen II particle types
//==================
#define H2_PT_STATIC			0
#define H2_PT_GRAV				1
#define H2_PT_SLOWGRAV			2
#define H2_PT_FIRE				3
#define H2_PT_EXPLODE			4
#define H2_PT_EXPLODE2			5
#define H2_PT_BLOB				6
#define H2_PT_BLOB2				7
#define H2_PT_VORPAL			8
#define H2_PT_BLOODCLOUD		9
#define H2_PT_FAERIE			10
#define H2_PT_SPELL				11
#define H2_PT_SMOKE				12
#define H2_PT_RAIN				13
#define H2_PT_SPLASH			14
#define H2_PT_SPLASHCLOUD		15
#define H2_PT_ICE				16
#define H2_PT_ACID				17
#define H2_PT_REDFIRE			18
#define H2_PT_GRENSMOKE			19
#define H2_PT_BLOODCLOUD2		20
#define H2_PT_GRAVWELL			21
#define H2_PT_GRAVSMOKE			22
#define H2_PT_MAGICMISSILE		23
#define H2_PT_BONESHARD			24
#define H2_PT_SCARAB			25
#define H2_PT_ACIDBALL			26
#define H2_PT_DARKEN			27
#define H2_PT_SNOW				28
#define H2_PT_GRAVWELL2			29
#define H2_PT_REDSMOKE			30

//==================
// Hexen II material types for particle chunks
//==================
#define H2_THINGTYPE_GLASS		0
#define H2_THINGTYPE_METAL		1
#define H2_THINGTYPE_FLESH		2
#define H2_THINGTYPE_FIRE		3
#define H2_THINGTYPE_CLAY		4
#define H2_THINGTYPE_LEAVES		5
#define H2_THINGTYPE_HAY		6
#define H2_THINGTYPE_BROWNSTONE	7
#define H2_THINGTYPE_CLOTH		8
#define H2_THINGTYPE_WOOD		9
#define H2_THINGTYPE_GREYSTONE	10
#define H2_THINGTYPE_METAL2		11
#define H2_THINGTYPE_LEATHER	12
#define H2_THINGTYPE_ICE		13
#define H2_THINGTYPE_CLEARGLASS	14
#define H2_THINGTYPE_REDGLASS	15
#define H2_THINGTYPE_ACID		16
#define H2_THINGTYPE_METEOR		17
#define H2_THINGTYPE_GREENFLESH	18
#define H2_THINGTYPE_BONE		19
#define H2_THINGTYPE_WEB		20
#define H2_THINGTYPE_MIST		21
#define H2_THINGTYPE_REDICE		22
#define H2_THINGTYPE_MUSHROOM	23

//==================
// Hexen II drawflags (special rendering modes)
// The drawflags byte is structured as:
//   bits 0-2: Model Light Style (MLS)
//   bits 3-4: Scale Type
//   bits 5-6: Scale Origin
//   bit 7:    Translucent flag
//==================

// Model Light Style (MLS) - bits 0-2
#define H2_MLS_MASKIN			7		// Mask for MLS bits
#define H2_MLS_MASKOUT			248		// Inverse mask
#define H2_MLS_NONE				0		// Normal lighting
#define H2_MLS_FULLBRIGHT		1		// Fullbright (uses lightstyle 25-30)
#define H2_MLS_POWERMODE		2		// Power mode pulsing light
#define H2_MLS_TORCH			3		// Torch flickering light
#define H2_MLS_TOTALDARK		4		// Total darkness
#define H2_MLS_ABSLIGHT			7		// Use entity's abslight value

// Scale Type - bits 3-4
#define H2_SCALE_TYPE_MASKIN	24		// Mask for scale type bits
#define H2_SCALE_TYPE_MASKOUT	231		// Inverse mask
#define H2_SCALE_TYPE_UNIFORM	0		// Scale X, Y, and Z equally
#define H2_SCALE_TYPE_XYONLY	8		// Scale X and Y only
#define H2_SCALE_TYPE_ZONLY		16		// Scale Z only

// Scale Origin - bits 5-6
#define H2_SCALE_ORIGIN_MASKIN	96		// Mask for scale origin bits
#define H2_SCALE_ORIGIN_MASKOUT	159		// Inverse mask
#define H2_SCALE_ORIGIN_CENTER	0		// Scale from object center
#define H2_SCALE_ORIGIN_BOTTOM	32		// Scale from object bottom
#define H2_SCALE_ORIGIN_TOP		64		// Scale from object top

// Translucent flag - bit 7
#define H2_DRF_TRANSLUCENT		128		// Entity is translucent

// DRF_ANIMATEONCE - defined in original H2 but never actually used in game code.
// Can't fit in byte-sized drawflags over network, but we support it for local use.
// Could potentially be set via effects field or other means.
#define H2_DRF_ANIMATEONCE		256		// Animate once then freeze on last frame

//==================
// Hexen II particle flags (SFL_* constants for snow/weather effects)
//==================
#define H2_SFL_FLUFFY			1		// All largish flakes
#define H2_SFL_MIXED			2		// Mixed flake sizes
#define H2_SFL_HALF_BRIGHT		4		// All flakes start darker
#define H2_SFL_NO_MELT			8		// Flakes don't melt when hitting surface
#define H2_SFL_IN_BOUNDS		16		// Flakes cannot leave their bounding box
#define H2_SFL_NO_TRANS			32		// All flakes start non-translucent
#define H2_SFL_64				64		// Reserved flag
#define H2_SFL_128				128		// Reserved flag

//==================
// Hexen II intermission flags (used by CL_SetupIntermission)
// These control the behavior and display of intermission screens
//==================
#define H2_INTERMISSION_NOT_CONNECTED	(1<<0)	// Can't use cl.time, use realtime
#define H2_INTERMISSION_NO_MENUS		(1<<1)	// Don't allow drawing menus
#define H2_INTERMISSION_NO_MESSAGE		(1<<2)	// Doesn't need a valid message index
#define H2_INTERMISSION_PRINT_TOP		(1<<3)	// Print centered in top half
#define H2_INTERMISSION_PRINT_TOPMOST	(1<<4)	// Print at topmost side
#define H2_INTERMISSION_PRINT_WHITE		(1<<5)	// Print in white, not red
#define H2_INTERMISSION_PRINT_DELAY		(1<<6)	// Delay message print for ~2.5s

//==================
// Global flags to indicate Hexen II mode and mission pack
//==================
extern qboolean hexen2_mode;
extern qboolean hexen2_missionpack;	// Portal of Praevus detected
extern int h2_kingofhill;			// Deathmatch: current king of the hill (-1 = none)

//==================
// Function declarations
//==================
void H2_Protocol_Init(void);
void H2_DetectGameType(void);
void H2_SetProtocol(int protocol);
qboolean H2_IsHexen2Protocol(int protocol);
const char *H2_GetProtocolName(int protocol);

#endif	/* _QUAKE_PROTOCOL_HEXEN2_H */
