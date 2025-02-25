/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell 
 * or otherwise commercially exploit the source or things you created based on the 
 * source.
 *
*/ 



#include "freespace.h"
#include "gamehelp/contexthelp.h"
#include "gamesequence/gamesequence.h"
#include "gamesnd/gamesnd.h"
#include "globalincs/alphacolors.h"
#include "globalincs/linklist.h"
#include "globalincs/pstypes.h"
#include "graphics/font.h"
#include "iff_defs/iff_defs.h"
#include "io/key.h"
#include "io/timer.h"
#include "mission/missionhotkey.h"
#include "missionui/missionscreencommon.h"
#include "mod_table/mod_table.h"
#include "object/object.h"
#include "parse/parselo.h"
#include "playerman/player.h"
#include "ship/ship.h"
#include "sound/audiostr.h"
#include "ui/ui.h"
#include "ui/uidefs.h"
#include "weapon/weapon.h"


static int Key_sets[MAX_KEYED_TARGETS] = {
	KEY_F5,
	KEY_F6,
	KEY_F7,
	KEY_F8,
	KEY_F9,
	KEY_F10,
	KEY_F11,
	KEY_F12
};

/////////////////////////////

static int Hotkey_bits[MAX_SHIPS];  // bitfield indicating which hotkeys are used by each ship

static int Hotkey_sets_saved;			// have we saved the sets for this mission

static int Mission_hotkey_save_timestamp;		// timestamp used to tell us when we can save
#define HOTKEY_SAVE_TIME				15000		// save sets this number of milliseconds into the mission

typedef struct {
	int setnum;
	char name[NAME_LENGTH];
} HK_save_info;

HK_save_info Hotkey_saved_info[MAX_HOTKEY_TARGET_ITEMS];
int Num_hotkeys_saved;


static const char *Hotkey_background_fname[GR_NUM_RESOLUTIONS] = {
	"Hotkeys",		// GR_640
	"2_Hotkeys"		// GR_1024
};

static const char *Hotkey_mask_fname[GR_NUM_RESOLUTIONS] = {
	"Hotkeys-M",		// GR_640
	"2_Hotkeys-M"	// GR_1024
};

//#define GROUP_LIST_X	40
//#define GROUP_LIST_W	160

// #define ICON_LIST_X	219
// #define ICON_LIST_W	8

// #define ICON_LIST_X	280
// #define ICON_LIST_W	8

//#define SHIP_LIST_X	242
//#define SHIP_LIST_X2	259
//#define SHIP_LIST_W	341
//#define SHIP_LIST_W2	324

// #define SHIP_LIST_X	302
// #define SHIP_LIST_X2	319
// #define SHIP_LIST_W	281
// #define SHIP_LIST_W2	264

// #define LIST_Y			70
// #define LIST_H			280

/*
#define HOTKEY_X		575
#define HOTKEY_Y		41
*/

#define WING_FLAG	0x80000

#define NUM_BUTTONS				10
#define LIST_BUTTONS_MAX		50

#define SCROLL_UP_BUTTON		0
#define SCROLL_DOWN_BUTTON		1
#define CANCEL_BUTTON			2
#define CLEAR_BUTTON				3
#define RESET_BUTTON				4
#define ADD_HOTKEY_BUTTON		5
#define REMOVE_HOTKEY_BUTTON	6
#define HELP_BUTTON				7
#define OPTIONS_BUTTON			8
#define ACCEPT_BUTTON			9

// coords for entire ship box
static int Hotkey_list_coords[GR_NUM_RESOLUTIONS][4] = {
	{
		// GR_640
		29,			// x
		22,			// y
		502,			// w
		315			// h
	},
	{
		// GR_1024
		47,			// x
		35,			// y
		802,			// w
		505			// h
	}
};

// coords for big "F9" thing in the corner
static int Hotkey_function_name_coords[GR_NUM_RESOLUTIONS][4] = {
	{
		// GR_640
		570,			// x
		14,			// y
		59,			// w
		22				// h
	},
	{
		// GR_1024
		912,			// x
		22,			// y
		94,			// w
		36				// h
	}
};

/*
#define FIELD_LEFT_EDGE		0
#define FIELD_F5				1
#define FIELD_F6				2
#define FIELD_F7				3
#define FIELD_F8				4
#define FIELD_F9				5
#define FIELD_F10				6
#define FIELD_F11				7
#define FIELD_F12				8
#define FIELD_ICON			9
#define FIELD_RIGHT_EDGE	10
// x coords of unseen field boundaries (  | field1 | field2 | ... |  )
// entried will all be centered in fields except FIELD_SHIP which will be left justified
// an edge is named by the field on its left
static int Hotkey_field_edge[GR_NUM_RESOLUTIONS][11] = {
	{
		29, 56, 83, 110, 137, 164, 191, 218, 245, 280, 531
	},
	{
		47, 91, 135, 179, 223, 267, 311, 355, 399, 448, 849
	}
}
*/

static int Hotkey_function_field_width[GR_NUM_RESOLUTIONS] = {
	27,			// GR_640
	44				// GR_1024
};
static int Hotkey_wing_icon_x[GR_NUM_RESOLUTIONS] = {
	246,			// GR_640
	400			// GR_1024
};
static int Hotkey_ship_x[GR_NUM_RESOLUTIONS] = {
	280,			// GR_640
	448			// GR_1024
};

struct hotkey_buttons {
	const char *filename;
	int x, y;
	int hotspot;
	UI_BUTTON button;  // because we have a class inside this struct, we need the constructor below..

	hotkey_buttons(const char *name, int x1, int y1, int h) : filename(name), x(x1), y(y1), hotspot(h) {}
};

// button definitions
static hotkey_buttons Buttons[GR_NUM_RESOLUTIONS][NUM_BUTTONS] = {
//XSTR:OFF
	{
		// GR_640
		hotkey_buttons("HKB_00",	1,		94,	0),
		hotkey_buttons("HKB_01",	1,		133,	1),
		hotkey_buttons("HKB_02",	15,	342,	2),
		hotkey_buttons("HKB_03",	84,	342,	3),
		hotkey_buttons("HKB_04",	161,	342,	4),
		hotkey_buttons("HKB_05",	539,	5,		5),
		hotkey_buttons("HKB_06",	539,	44,	6),
		hotkey_buttons("HKB_07",	539,	431,	7),
		hotkey_buttons("HKB_08",	539,	455,	8),
		hotkey_buttons("HKB_09",	575,	432,	9)
	},
	{
		// GR_1024
		hotkey_buttons("2_HKB_00",		2,		150,	0),
		hotkey_buttons("2_HKB_01",		2,		213,	1),
		hotkey_buttons("2_HKB_02",		24,	548,	2),
		hotkey_buttons("2_HKB_03",		135,	548,	3),
		hotkey_buttons("2_HKB_04",		258,	548,	4),
		hotkey_buttons("2_HKB_05",		862,	8,		5),
		hotkey_buttons("2_HKB_06",		862,	71,	6),
		hotkey_buttons("2_HKB_07",		863,	690,	7),
		hotkey_buttons("2_HKB_08",		862,	728,	8),
		hotkey_buttons("2_HKB_09",		920,	692,	9)
	}
//XSTR:ON
};

#define HOTKEY_NUM_TEXT		6
static UI_XSTR Hotkey_text[GR_NUM_RESOLUTIONS][HOTKEY_NUM_TEXT] = {
	{ 
		// GR_640
		{ "Cancel",		1516,	7,	392,		UI_XSTR_COLOR_GREEN, -1, &Buttons[GR_640][CANCEL_BUTTON].button },
		{ "Clear",		1517,	85, 392,		UI_XSTR_COLOR_GREEN, -1, &Buttons[GR_640][CLEAR_BUTTON].button },
		{ "Reset",		1518,	159, 392,	UI_XSTR_COLOR_GREEN, -1, &Buttons[GR_640][RESET_BUTTON].button },
		{ "Help",		1519,	500, 440,	UI_XSTR_COLOR_GREEN, -1, &Buttons[GR_640][HELP_BUTTON].button },
		{ "Options",	1520,	479, 464,	UI_XSTR_COLOR_GREEN, -1, &Buttons[GR_640][OPTIONS_BUTTON].button },
		{ "Accept",		1521,	573, 413,	UI_XSTR_COLOR_GREEN, -1, &Buttons[GR_640][ACCEPT_BUTTON].button }
	}, 
	{ 
		// GR_1024
		{ "Cancel",		1516,	30, 629,		UI_XSTR_COLOR_GREEN, -1, &Buttons[GR_1024][CANCEL_BUTTON].button },
		{ "Clear",		1517,	151, 629,	UI_XSTR_COLOR_GREEN, -1, &Buttons[GR_1024][CLEAR_BUTTON].button },
		{ "Reset",		1518,	269, 629,	UI_XSTR_COLOR_GREEN, -1, &Buttons[GR_1024][RESET_BUTTON].button },
		{ "Help",		1519,	800, 704,	UI_XSTR_COLOR_GREEN, -1, &Buttons[GR_1024][HELP_BUTTON].button },
		{ "Options",	1520,	797, 743,	UI_XSTR_COLOR_GREEN, -1, &Buttons[GR_1024][OPTIONS_BUTTON].button },
		{ "Accept",		1521,	902, 661,	UI_XSTR_COLOR_GREEN, -1, &Buttons[GR_1024][ACCEPT_BUTTON].button }	
	}
};

hotkey_line Hotkey_lines[MAX_LINES];
static int Cur_hotkey = 0;
static int Scroll_offset;
static int Num_lines;
static int Selected_line;
static int Background_bitmap;
static int Wing_bmp;
static UI_WINDOW Ui_window;
static UI_BUTTON List_buttons[LIST_BUTTONS_MAX];  // buttons for each line of text in list
//static UI_BUTTON List_region;

int Hotkey_overlay_id;

//////////////////////


// function used in a couple of places to get the actual hotkey set number from a key value.
// not trivial since our current keysets (F5 - F12) do not have sequential keycodes
int mission_hotkey_get_set_num( int k )
{

	for (int i = 0; i < MAX_KEYED_TARGETS; i++ ) {
		if ( Key_sets[i] == k ) {
			return i;
		}
	}

	Int3();		// get allender
	return 0;
}
		
// function to maybe restore some hotkeys during the first N seconds of the mission
void mission_hotkey_maybe_restore()
{

	for (int i = 0; i < Num_hotkeys_saved; i++ ) {
		// don't process something that has no set
		if ( Hotkey_saved_info[i].setnum == -1 )
			continue;

		// the ship is present, add it to the given set.
		int index = ship_name_lookup(Hotkey_saved_info[i].name);
		if ( index != -1 ) {
			hud_target_hotkey_add_remove( Hotkey_saved_info[i].setnum, &Objects[Ships[index].objnum], HOTKEY_USER_ADDED );
			Hotkey_saved_info[i].setnum = -1;
		}
	}
}

// ---------------------------------------------------------------------
// mission_hotkey_set_defaults()
//
// Set up the hotkey lists for the player based on the mission designer
// defaults. 
// Set restore to false to explicitely ignore player saved values. 
//
void mission_hotkey_set_defaults(bool restore)
{

	for (int i = 0; i < MAX_KEYED_TARGETS; i++ ) {
		hud_target_hotkey_clear(i);
	}

	// set the variable letting us know that we should save the hotkey sets
	Hotkey_sets_saved = 0;
	Mission_hotkey_save_timestamp = timestamp(HOTKEY_SAVE_TIME);

	// if we have hotkeys saved from the previous run of this mission, then simply keep the cleared
	// sets, and let the restore code take care of it!  This works because this function is currently
	// only called from one place -- after the mission loads.
	if ( restore && (Num_hotkeys_saved > 0) ) {
		mission_hotkey_maybe_restore();
		return;
	}

	// Check for ships with a hotkey assigned
	obj_merge_created_list();
	for (auto so: list_range(&Ship_obj_list)) {
		auto A = &Objects[so->objnum];
		if (A->flags[Object::Object_Flags::Should_be_dead])
			continue;
		if ( (A->type != OBJ_SHIP) || ((Game_mode & GM_NORMAL) && (A == Player_obj)) ) {
			continue;
		}

		Assert(A->instance >= 0 && A->instance < MAX_SHIPS);
		ship *sp = &Ships[A->instance];		

		if ( sp->hotkey == -1 )
			continue;

		// if the hotkey is the last hotkey in the list, then don't add it either since this hotkey is a special
		// marker to indicate that this ship should remain invisible in the hotkey screen until after mission
		// starts
		if ( sp->hotkey == MAX_KEYED_TARGETS )
			continue;

		Assert(sp->objnum >= 0);
		hud_target_hotkey_add_remove( sp->hotkey, &Objects[sp->objnum], HOTKEY_MISSION_FILE_ADDED );
	}

	// Check for wings with a hotkey assigned
	for (int i = 0; i < Num_wings; i++ ) {
		wing *wp = &Wings[i];

		if ( wp->hotkey == -1 )  
			continue;

		// like ships, skip this wing if the hotkey is the last hotkey item
		if ( wp->hotkey == MAX_KEYED_TARGETS )
			continue;

		for (int j = 0; j < wp->current_count; j++ ) {
			if ( wp->ship_index[j] == -1 )
				continue;

			ship *sp = &Ships[wp->ship_index[j]];
			hud_target_hotkey_add_remove( wp->hotkey, &Objects[sp->objnum], HOTKEY_MISSION_FILE_ADDED );
		}				
	}
}

// function to reset the saved hotkeys -- called when a new mission is loaded
void mission_hotkey_reset_saved()
{
	Num_hotkeys_saved = 0;
}

// next function called when we might want to save the hotkey sets for the player.  We will save the hotkey
// sets N seconds into the mission
void mission_hotkey_maybe_save_sets()
{
	if ( !timestamp_elapsed(Mission_hotkey_save_timestamp) ) {
		mission_hotkey_maybe_restore();
		return;
	}

	// no processing if we have saved them.
	if ( Hotkey_sets_saved )
		return;

	for (int i = 0; i < MAX_HOTKEY_TARGET_ITEMS; i++ )
		Hotkey_saved_info[i].setnum = -1;

	Num_hotkeys_saved = 0;
	HK_save_info *hkp = &(Hotkey_saved_info[0]);

	for (int i = 0; i < MAX_KEYED_TARGETS; i++ ) {

		// get the list.  do nothing if list is empty
		htarget_list *plist = &(Player->keyed_targets[i]);
		if ( EMPTY(plist) )
			continue;

		bool found_player_hotkey = false;
		htarget_list* hitem;
		for ( hitem = GET_FIRST(plist); hitem != END_OF_LIST(plist); hitem = GET_NEXT(hitem) )
			if (hitem->how_added == HOTKEY_USER_ADDED)
				found_player_hotkey = true;

		if (!found_player_hotkey)
			continue;

		for ( hitem = GET_FIRST(plist); hitem != END_OF_LIST(plist); hitem = GET_NEXT(hitem) ) {
			Assert( Num_hotkeys_saved < MAX_HOTKEY_TARGET_ITEMS );
			hkp->setnum = i;
			strcpy_s( hkp->name, Ships[hitem->objp->instance].ship_name );
			hkp++;
			Num_hotkeys_saved++;
		}
	}

	Hotkey_sets_saved = 1;
}

// function which gets called from MissionParse to maybe add a ship or wing to a hotkey set.
// this intermediate function is needed so that we don't blast over possibly saved hotkey sets
void mission_hotkey_mf_add( int set, int objnum, int how_to_add )
{
	// if we are restoring hotkeys, and the timer hasn't elapsed, then return and let the
	// hotkey restoration code deal with it
	if ( Num_hotkeys_saved && !timestamp_elapsed(Mission_hotkey_save_timestamp) )
		return;

	// we can add it to the set
	hud_target_hotkey_add_remove( set, &Objects[objnum], how_to_add );
}

void mission_hotkey_validate()
{
	for (int i = 0; i < MAX_KEYED_TARGETS; i++ ) {
		htarget_list* plist = &(Players[Player_num].keyed_targets[i]);
		if ( EMPTY( plist ) )			// no items in list, then do nothing
			continue;

		htarget_list *hitem = GET_FIRST(plist);
		while ( hitem != END_OF_LIST(plist) ) {

			// ensure this object is still valid and in the obj_used_list
			int obj_valid = FALSE;
			object* A;
			for ( A = GET_FIRST(&obj_used_list); A !=END_OF_LIST(&obj_used_list); A = GET_NEXT(A) ) {
				if (A->flags[Object::Object_Flags::Should_be_dead])
					continue;
				if ( A->signature == hitem->objp->signature ) {
					obj_valid = TRUE;
					break;
				}
			}
			if ( obj_valid == FALSE ) {
				htarget_list *temp;

				temp = GET_NEXT(hitem);
				list_remove( plist, hitem );
				list_append( &htarget_free_list, hitem );
				hitem->objp = nullptr;
				hitem = temp;
				continue;
			}
			hitem = GET_NEXT( hitem );
		}	// end while
	} // end for
}


// get the Hotkey_bits of a whole wing (bits must be set in all ships of wing for a hotkey bit to be set)
int get_wing_hotkeys(int n)
{
	int idx = Hotkey_lines[n].index;
	
	int total = 0xffffffff;

	Assert((idx >= 0) && (idx < Num_wings));
	for (int i = 0; i < Wings[idx].current_count; i++) {
		int ship_index;

		// don't count the player ship for the total -- you cannot assign the player since bad things
		// can happen on the hud.
		ship_index = Wings[idx].ship_index[i];
		if ( &Ships[ship_index] == Player_ship )
			continue;

		total &= Hotkey_bits[Wings[idx].ship_index[i]];
	}

	return total;
}

int get_ship_hotkeys(int n)
{
	return Hotkey_bits[Hotkey_lines[n].index];
}

// add a line of hotkey smuck to end of list
int hotkey_line_add(const char *text, HotkeyLineType type, int index, int y)
{
	if (Num_lines >= MAX_LINES)
		return 0;

	Hotkey_lines[Num_lines].label = text;
	Hotkey_lines[Num_lines].type = type;
	Hotkey_lines[Num_lines].index = index;
	Hotkey_lines[Num_lines].y = y;
	return Num_lines++;
}

// insert a line of hotkey smuck before line 'n'.
int hotkey_line_insert(int n, const char *text, HotkeyLineType type, int index)
{

	if (Num_lines >= MAX_LINES)
		return 0;

	int z = Num_lines++;
	while (z > n) {
		Hotkey_lines[z] = Hotkey_lines[z - 1];
		z--;
	}

	Hotkey_lines[z].label = text;
	Hotkey_lines[z].type = type;
	Hotkey_lines[z].index = index;
	return z;
}

// insert a line of hotkey smuck somewhere between 'start' and end of list such that it is
// sorted by name
int hotkey_line_add_sorted(const char *text, HotkeyLineType type, int index, int start)
{

	if (Num_lines >= MAX_LINES)
		return -1;

	int z = Num_lines - 1;
	while ((z >= start) && ((Hotkey_lines[z].type == HotkeyLineType::SUBSHIP) || (stricmp(text, Hotkey_lines[z].label.c_str()) < 0)))
		z--;

	z++;
	while ((z < Num_lines) && (Hotkey_lines[z].type == HotkeyLineType::SUBSHIP))
		z++;

	return hotkey_line_insert(z, text, type, index);
}

int hotkey_build_team_listing(int enemy_team_mask, int y, bool list_enemies)
{
	const char *str = nullptr;
	int team_mask;
	IFF_hotkey_team hotkey_team;

	if (list_enemies)
	{
		str = XSTR( "Enemy ships", 403);
		team_mask = enemy_team_mask;
		hotkey_team = IFF_hotkey_team::Hostile;
	}
	else
	{
		str = XSTR( "Friendly ships", 402);
		team_mask = ~enemy_team_mask;
		hotkey_team = IFF_hotkey_team::Friendly;
	}

	hotkey_line_add(str, HotkeyLineType::HEADING, 0, y);
	y += 2;

	int start = Num_lines;
	ship_obj* so;

	for ( so = GET_FIRST(&Ship_obj_list); so != END_OF_LIST(&Ship_obj_list); so = GET_NEXT(so) ) {
		bool add_it;

		if (Objects[so->objnum].flags[Object::Object_Flags::Should_be_dead])
			continue;

		// don't process non-ships, or the player ship
		if ( (Game_mode & GM_NORMAL) && (so->objnum == OBJ_INDEX(Player_obj)) )
			continue;

		int shipnum = Objects[so->objnum].instance;
		ship *shipp = &Ships[shipnum];

		// filter out ships in wings
		if ( shipp->wingnum >= 0 )
			continue;

		// filter out cargo containers, navbouys, etc, and non-ships
		if ( Ship_info[shipp->ship_info_index].class_type < 0 || !(Ship_types[Ship_info[shipp->ship_info_index].class_type].flags[Ship::Type_Info_Flags::Hotkey_on_list]) )
			continue;

		// don't process ships invisible to sensors, dying or departing
		if ( shipp->flags[Ship::Ship_Flags::Hidden_from_sensors] || shipp->is_dying_or_departing() )
			continue;

		// if a ship's hotkey is the last hotkey on the list, then maybe make the hotkey -1 if
		// we are now in mission.  Otherwise, skip this ship
		if ( shipp->hotkey == MAX_KEYED_TARGETS ) {
			if ((!(Game_mode & GM_IN_MISSION)) || Hotkey_always_hide_hidden_ships)
				continue;										// skip to next ship
			shipp->hotkey = -1;
		}

		// check IFF override and team mask
		if (Iff_info[shipp->team].hotkey_team == IFF_hotkey_team::Default) {
			add_it = iff_matches_mask(shipp->team, team_mask);
		} else {
			add_it = (Iff_info[shipp->team].hotkey_team == hotkey_team);
		}

		// add it if the teams match or if the IFF says to
		if (add_it) {
			hotkey_line_add_sorted(shipp->get_display_name(), HotkeyLineType::SHIP, shipnum, start);
		}
	}

	for (int i=0; i<Num_wings; i++) {
		bool add_it;
		char wing_name[NAME_LENGTH];

		// the wing has to be valid
		if (Wings[i].current_count && Wings[i].ship_index[Wings[i].special_ship] >= 0) {
			ship *shipp = &Ships[Wings[i].ship_index[Wings[i].special_ship]];

			// check IFF override and team mask
			if (Iff_info[shipp->team].hotkey_team == IFF_hotkey_team::Default) {
				add_it = iff_matches_mask(shipp->team, team_mask);
			} else {
				add_it = (Iff_info[shipp->team].hotkey_team == hotkey_team);
			}
		} else {
			add_it = false;
		}

		// add it if the teams match or if the IFF says to
		if (add_it) {
			// special check for the player's wing.  If he's in a wing, and the only guy left, don't
			// do anything
			if ( (Player_ship->wingnum == i) && (Wings[i].current_count == 1) )
				continue;

			// if a ship's hotkey is the last hotkey on the list, then maybe make the hotkey -1 if
			// we are now in mission.  Otherwise, skip this ship
			if ( Wings[i].hotkey == MAX_KEYED_TARGETS ) {
				if ( !(Game_mode & GM_IN_MISSION) )
					continue;										// skip to next ship
				Wings[i].hotkey = -1;
			}

			int j;

			// don't add any wing data whose ships are hidden from sensors
			for ( j = 0; j < Wings[i].current_count; j++ ) {
				if ( Ships[Wings[i].ship_index[j]].flags[Ship::Ship_Flags::Hidden_from_sensors] )
					break;
			}
			// if we didn't reach the end of the list, don't display the wing
			if ( j < Wings[i].current_count )
				continue;

			strcpy_s(wing_name, Wings[i].name);
			end_string_at_first_hash_symbol(wing_name);

			int z = hotkey_line_add_sorted(wing_name, HotkeyLineType::WING, i, start);
			if (Wings[i].flags[Ship::Wing_Flags::Expanded]) {
				for (j=0; j<Wings[i].current_count; j++) {
					int s = Wings[i].ship_index[j];
					if (!Ships[s].is_dying_or_departing()) {
						z = hotkey_line_insert(z + 1, Ships[s].get_display_name(), HotkeyLineType::SUBSHIP, s);
					}
				}
			}
		}
	}

	// see if we actually added any lines
	if (start == Num_lines) {
		// roll back the heading, then return as if nothing happened
		Num_lines--;
		return y - 2;
	}

	int font_height = gr_get_font_height();

	for (int i=start; i<Num_lines; i++) {
		if (Hotkey_lines[i].type == HotkeyLineType::SUBSHIP)
			y += font_height;
		else
			y += font_height + 2;

		Hotkey_lines[i].y = y;
	}

	y += font_height + 8;
	return y;
}

void hotkey_set_selected_line(int line)
{
	Selected_line = line;
}

void hotkey_build_listing()
{
	int y;

	Num_lines = y = 0;

	int enemy_team_mask = iff_get_attackee_mask(Player_ship->team);

	y = hotkey_build_team_listing(enemy_team_mask, y, false);
	y = hotkey_build_team_listing(enemy_team_mask, y, true);
}

int hotkey_line_query_visible(int n)
{
	if ((n < 0) || (n >= Num_lines))
		return 0;
	
	int y = Hotkey_lines[n].y - Hotkey_lines[Scroll_offset].y;
	if ((y < 0) || (y + gr_get_font_height() > Hotkey_list_coords[gr_screen.res][3]))
		return 0;

	return 1;
}

void hotkey_scroll_screen_up()
{
	if (Scroll_offset) {
		Scroll_offset--;
		Assert(Selected_line > Scroll_offset);
		while (!hotkey_line_query_visible(Selected_line) || (Hotkey_lines[Selected_line].type == HotkeyLineType::HEADING))
			Selected_line--;

		gamesnd_play_iface(InterfaceSounds::SCROLL);

	} else
		gamesnd_play_iface(InterfaceSounds::GENERAL_FAIL);
}

void hotkey_scroll_line_up()
{
	if (Selected_line > 1) {
		Selected_line--;
		while (Hotkey_lines[Selected_line].type == HotkeyLineType::HEADING)
			Selected_line--;

		if (Selected_line < Scroll_offset)
			Scroll_offset = Selected_line;

		gamesnd_play_iface(InterfaceSounds::SCROLL);

	} else
		gamesnd_play_iface(InterfaceSounds::GENERAL_FAIL);
}

void hotkey_scroll_screen_down()
{
	if (Hotkey_lines[Num_lines - 1].y + gr_get_font_height() > Hotkey_lines[Scroll_offset].y + Hotkey_list_coords[gr_screen.res][3]) {
		Scroll_offset++;
		while (!hotkey_line_query_visible(Selected_line) || (Hotkey_lines[Selected_line].type == HotkeyLineType::HEADING)) {
			Selected_line++;
			Assert(Selected_line < Num_lines);
		}

		gamesnd_play_iface(InterfaceSounds::SCROLL);

	} else
		gamesnd_play_iface(InterfaceSounds::GENERAL_FAIL);
}

void hotkey_scroll_line_down()
{
	if (Selected_line < Num_lines - 1) {
		Selected_line++;
		while (Hotkey_lines[Selected_line].type == HotkeyLineType::HEADING)
			Selected_line++;

		Assert(Selected_line > Scroll_offset);
		while (!hotkey_line_query_visible(Selected_line))
			Scroll_offset++;

		gamesnd_play_iface(InterfaceSounds::SCROLL);

	} else
		gamesnd_play_iface(InterfaceSounds::GENERAL_FAIL);
}

void expand_wing(int line, bool forceExpand)
{
	if (Hotkey_lines[line].type == HotkeyLineType::WING) {
		int i = Hotkey_lines[line].index;
		if (forceExpand) {
			Wings[i].flags.set(Ship::Wing_Flags::Expanded);
		} else {
			Wings[i].flags.toggle(Ship::Wing_Flags::Expanded);
		}
		hotkey_build_listing();
		for (int z=0; z<Num_lines; z++)
			if ((Hotkey_lines[z].type == HotkeyLineType::WING) && (Hotkey_lines[z].index == i)) {
				Selected_line = z;
				break;
			}
	}
}

void reset_hotkeys()
{
	htarget_list *hitem, *plist;

	for (int i=0; i<MAX_SHIPS; i++)
		Hotkey_bits[i] = 0;

	for (int i=0; i<MAX_KEYED_TARGETS; i++ ) {
		plist = &(Players[Player_num].keyed_targets[i]);
		if ( EMPTY(plist) ) // no items in list, then do nothing
			continue;

		for ( hitem = GET_FIRST(plist); hitem != END_OF_LIST(plist); hitem = GET_NEXT(hitem) ) {
			Assert(hitem->objp->type == OBJ_SHIP);
			Hotkey_bits[hitem->objp->instance] |= (1 << i);
		}
	}
}

void clear_hotkeys(int line)
{
	auto type = Hotkey_lines[line].type;

	if (type == HotkeyLineType::WING) {
		int z = Hotkey_lines[line].index;
		int b = ~get_wing_hotkeys(line);
		for (int i=0; i<Wings[z].current_count; i++)
			Hotkey_bits[Wings[z].ship_index[i]] &= b;

	} else if ((type == HotkeyLineType::SHIP) || (type == HotkeyLineType::SUBSHIP)) {
		Hotkey_bits[Hotkey_lines[line].index] = 0;
	}
}

void save_hotkeys()
{
	for (int i=0; i<MAX_KEYED_TARGETS; i++) {
		hud_target_hotkey_clear(i);
		ship_obj* so;

		for ( so = GET_FIRST(&Ship_obj_list); so != END_OF_LIST(&Ship_obj_list); so = GET_NEXT(so) ) {
			if (Objects[so->objnum].flags[Object::Object_Flags::Should_be_dead])
				continue;
			if ( Hotkey_bits[Objects[so->objnum].instance] & (1 << i) ) {
				hud_target_hotkey_add_remove(i, &Objects[so->objnum], HOTKEY_USER_ADDED );
			}
		}
	}
}

void add_hotkey(int hotkey, int line)
{
	auto type = Hotkey_lines[line].type;

	if (type == HotkeyLineType::WING) {
		int z = Hotkey_lines[line].index;
		for (int i=0; i<Wings[z].current_count; i++)
			Hotkey_bits[Wings[z].ship_index[i]] |= (1 << hotkey);

	} else if ((type == HotkeyLineType::SHIP) || (type == HotkeyLineType::SUBSHIP)) {
		Hotkey_bits[Hotkey_lines[line].index] |= (1 << hotkey);
	}
}

void remove_hotkey(int hotkey, int line)
{
	auto type = Hotkey_lines[line].type;

	if (type == HotkeyLineType::WING) {
		int z = Hotkey_lines[line].index;
		for (int i=0; i<Wings[z].current_count; i++)
			Hotkey_bits[Wings[z].ship_index[i]] &= ~(1 << hotkey);

	} else if ((type == HotkeyLineType::SHIP) || (type == HotkeyLineType::SUBSHIP)) {
		Hotkey_bits[Hotkey_lines[line].index] &= ~(1 << hotkey);
	}
}

void hotkey_button_pressed(int n)
{
	switch (n) {
		case SCROLL_UP_BUTTON:
			hotkey_scroll_screen_up();
			break;

		case SCROLL_DOWN_BUTTON:
			hotkey_scroll_screen_down();
			break;

		case ADD_HOTKEY_BUTTON:
			add_hotkey(Cur_hotkey, Selected_line);
			gamesnd_play_iface(InterfaceSounds::USER_SELECT);
			break;

		case REMOVE_HOTKEY_BUTTON:
			remove_hotkey(Cur_hotkey, Selected_line);
			gamesnd_play_iface(InterfaceSounds::USER_SELECT);
			break;

		case ACCEPT_BUTTON:
			save_hotkeys();
			// fall through to CANCEL_BUTTON
			FALLTHROUGH;

		case CANCEL_BUTTON:			
			mission_hotkey_exit();
			gamesnd_play_iface(InterfaceSounds::USER_SELECT);
			break;

		case HELP_BUTTON:
			launch_context_help();
			gamesnd_play_iface(InterfaceSounds::HELP_PRESSED);
			break;

		case OPTIONS_BUTTON:			
			gameseq_post_event(GS_EVENT_OPTIONS_MENU);
			gamesnd_play_iface(InterfaceSounds::USER_SELECT);
			break;

		case CLEAR_BUTTON:
			clear_hotkeys(Selected_line);
			gamesnd_play_iface(InterfaceSounds::USER_SELECT);
			break;

		case RESET_BUTTON:
			reset_hotkeys();
			gamesnd_play_iface(InterfaceSounds::USER_SELECT);
			break;
	}
}

// ---------------------------------------------------------------------
// mission_hotkey_init()
//
// Initialize the hotkey assignment screen system.  Called when GS_STATE_HOTKEY_SCREEN
// is entered.
//
void mission_hotkey_init()
{
	// pause all weapon sounds
	weapon_pause_sounds();

	// pause all game music
	audiostream_pause_all();

	reset_hotkeys();
	common_set_interface_palette();  // set the interface palette
	Ui_window.create(0, 0, gr_screen.max_w_unscaled, gr_screen.max_h_unscaled, 0);
	Ui_window.set_mask_bmap(Hotkey_mask_fname[gr_screen.res]);

	for (int i=0; i<NUM_BUTTONS; i++) {
		hotkey_buttons *b = &Buttons[gr_screen.res][i];

		b->button.create(&Ui_window, "", b->x, b->y, 60, 30, i < 2 ? 1 : 0, 1);
		// set up callback for when a mouse first goes over a button
		b->button.set_highlight_action(common_play_highlight_sound);
		b->button.set_bmaps(b->filename);
		b->button.link_hotspot(b->hotspot);
	}

	// add all xstr text
	for(int i=0; i<HOTKEY_NUM_TEXT; i++) {
		Ui_window.add_XSTR(&Hotkey_text[gr_screen.res][i]);
	}

	for (int i=0; i<LIST_BUTTONS_MAX; i++) {
		List_buttons[i].create(&Ui_window, "", 0, 0, 60, 30, (i < 2), 1);
		List_buttons[i].hide();
		List_buttons[i].disable();
	}

	// set up hotkeys for buttons so we draw the correct animation frame when a key is pressed
	Buttons[gr_screen.res][SCROLL_UP_BUTTON].button.set_hotkey(KEY_PAGEUP);
	Buttons[gr_screen.res][SCROLL_DOWN_BUTTON].button.set_hotkey(KEY_PAGEDOWN);

	// ensure help overlay is off
	Hotkey_overlay_id = help_overlay_get_index(HOTKEY_OVERLAY);
	help_overlay_set_state(Hotkey_overlay_id,gr_screen.res,0);

	// load in relevant bitmaps
	Background_bitmap = bm_load(Hotkey_background_fname[gr_screen.res]);
	if (Background_bitmap < 0) {
		// bitmap didnt load -- this is bad
		Int3();
	}
	Wing_bmp = bm_load("WingDesignator");
	if (Wing_bmp < 0) {
		// bitmap didnt load -- this is bad
		Int3();
	}

	Scroll_offset = 0;
	Selected_line = 1;
	hotkey_build_listing();
}

// ---------------------------------------------------------------------
// mission_hotkey_close()
//
// Cleanup the hotkey assignment screen system.  Called when GS_STATE_HOTKEY_SCREEN
// is left.
//
void mission_hotkey_close()
{
	if (Background_bitmap)
		bm_release(Background_bitmap);
	if (Wing_bmp >= 0)
		bm_release(Wing_bmp);

	// unpause all weapon sounds
	weapon_unpause_sounds();

	// unpause all game music
	audiostream_unpause_all();

	Ui_window.destroy();
	common_free_interface_palette();		// restore game palette
	game_flush();
}

// ---------------------------------------------------------------------
// mission_hotkey_do_frame()
//
// Called once per frame to process user input for the Hotkey Assignment Screen
//
void mission_hotkey_do_frame(float  /*frametime*/)
{

	if ( help_overlay_active(Hotkey_overlay_id) ) {
		Buttons[gr_screen.res][HELP_BUTTON].button.reset_status();
		Ui_window.set_ignore_gadgets(1);
	}

	int k = Ui_window.process() & ~KEY_DEBUGGED;

	if ( (k > 0) || B1_JUST_RELEASED ) {
		if ( help_overlay_active(Hotkey_overlay_id) ) {
			help_overlay_set_state(Hotkey_overlay_id, gr_screen.res, 0);
			Ui_window.set_ignore_gadgets(0);
			k = 0;
		}
	}

	if ( !help_overlay_active(Hotkey_overlay_id) ) {
		Ui_window.set_ignore_gadgets(0);
	}

	switch (k) {
		case KEY_DOWN:  // scroll list down
			hotkey_scroll_line_down();
			break;

		case KEY_UP:  // scroll list up
			hotkey_scroll_line_up();
			break;

		case KEY_PAGEDOWN:  // scroll list down
			hotkey_scroll_screen_down();
			break;

		case KEY_PAGEUP:  // scroll list up
			hotkey_scroll_screen_up();
			break;

		case KEY_CTRLED | KEY_ENTER:
			save_hotkeys();
			// fall through to next state -- allender changed this behavior since ESC should always cancel, no?
			FALLTHROUGH;

		case KEY_ESC:			
			mission_hotkey_exit();
			break;

		case KEY_TAB:
		case KEY_ENTER:
		case KEY_PADENTER:
			expand_wing(Selected_line);
			break;

		case KEY_EQUAL:
		case KEY_PADPLUS:
			add_hotkey(Cur_hotkey, Selected_line);
			break;

		case KEY_MINUS:
		case KEY_PADMINUS:
			remove_hotkey(Cur_hotkey, Selected_line);
			break;

		case KEY_F2:			
			gameseq_post_event(GS_EVENT_OPTIONS_MENU);			
			break;

		case KEY_CTRLED | KEY_R:
			reset_hotkeys();
			break;

		case KEY_CTRLED | KEY_C:
			clear_hotkeys(Selected_line);
			break;
	}	// end switch

	// if the key that was pressed is one of the hotkeys
	// then set that as the active hotkey.
	// if the hotkey was shifted then add it to the current
	// selected line.
	for (int i=0; i<MAX_KEYED_TARGETS; i++) {
		if (k == Key_sets[i])
			Cur_hotkey = i;

		if (k == (Key_sets[i] | KEY_SHIFTED))
			add_hotkey(i, Selected_line);
	}

	// handle pressed buttons
	for (int i=0; i<NUM_BUTTONS; i++) {
		if (Buttons[gr_screen.res][i].button.pressed()) {
			hotkey_button_pressed(i);
			break;					// only need to handle 1 button @ a time
		}
	}

	int select_tease_line = -1; // line mouse is down on, but won't be selected until button released

	for (int i=0; i<LIST_BUTTONS_MAX; i++) {
		// check for tease line
		if (List_buttons[i].button_down()) {
			select_tease_line = i + Scroll_offset;
		}
	
		// check for selected list item
		if (List_buttons[i].pressed()) {
			Selected_line = i + Scroll_offset;

			int z;
			List_buttons[i].get_mouse_pos(&z, nullptr);
			z += Hotkey_list_coords[gr_screen.res][0];		// adjust to full screen space
			if ((z >= Hotkey_wing_icon_x[gr_screen.res]) && (z < (Hotkey_wing_icon_x[gr_screen.res]) + Hotkey_function_field_width[gr_screen.res])) {
				expand_wing(Selected_line);
			}
		}

		if (List_buttons[i].double_clicked()) {
			Selected_line = i + Scroll_offset;
			int hotkeys = -1;
			switch (Hotkey_lines[Selected_line].type) {
				case HotkeyLineType::WING:
					hotkeys = get_wing_hotkeys(Selected_line);
					break;

				case HotkeyLineType::SHIP:
				case HotkeyLineType::SUBSHIP:
					hotkeys = get_ship_hotkeys(Selected_line);
					break;

				default:
					break;
			}

			if (hotkeys != -1) {
				if (hotkeys & (1 << Cur_hotkey))
					remove_hotkey(Cur_hotkey, Selected_line);
				else
					add_hotkey(Cur_hotkey, Selected_line);
			}
		}
	}

	GR_MAYBE_CLEAR_RES(Background_bitmap);
	if (Background_bitmap >= 0) {
		gr_set_bitmap(Background_bitmap);
		gr_bitmap(0, 0, GR_RESIZE_MENU);

	} else
		gr_clear();

	Ui_window.draw();
	color circle_color;
	gr_init_color(&circle_color, 160, 160, 0);

	// draw the big "F10" in the little box	
	font::set_font(font::FONT2);
	gr_set_color_fast(&Color_text_normal);

	char buf[256];
	strcpy_s(buf, textify_scancode(Key_sets[Cur_hotkey]));

	int w, h;
	gr_get_string_size(&w, &h, buf);
	gr_printf_menu(Hotkey_function_name_coords[gr_screen.res][0] + (Hotkey_function_name_coords[gr_screen.res][2] - w) / 2, Hotkey_function_name_coords[gr_screen.res][1], "%s", buf);

	font::set_font(font::FONT1);
	int line = Scroll_offset;
	while (hotkey_line_query_visible(line)) {
		//int z = Hotkey_lines[line].index;
		int y = Hotkey_list_coords[gr_screen.res][1] + Hotkey_lines[line].y - Hotkey_lines[Scroll_offset].y;
		int hotkeys = 0;
		int font_height = gr_get_font_height();
		int width = 0;

		switch (Hotkey_lines[line].type) {
			case HotkeyLineType::HEADING:
				gr_set_color_fast(&Color_text_heading);

				gr_get_string_size(&w, &h, Hotkey_lines[line].label.c_str());
				width = y + h / 2 - 1;
				gr_line(Hotkey_list_coords[gr_screen.res][0], width, Hotkey_ship_x[gr_screen.res] - 2, width, GR_RESIZE_MENU);
				gr_line(Hotkey_ship_x[gr_screen.res] + w + 1, width, Hotkey_list_coords[gr_screen.res][0] + Hotkey_list_coords[gr_screen.res][2], width, GR_RESIZE_MENU);
				break;

			case HotkeyLineType::WING:
				gr_set_bitmap(Wing_bmp);
				bm_get_info(Wing_bmp, nullptr, &h, nullptr);
				width = y + font_height / 2 - h / 2 - 1;
				gr_bitmap(Hotkey_wing_icon_x[gr_screen.res], width, GR_RESIZE_MENU);

//				i = y + font_height / 2 - 1;
//				gr_set_color_fast(&circle_color);
//				gr_circle(ICON_LIST_X + 4, i, 5, GR_RESIZE_MENU);

//				gr_set_color_fast(&Color_bright);
//				gr_line(ICON_LIST_X, i, ICON_LIST_X + 2, i, GR_RESIZE_MENU);
//				gr_line(ICON_LIST_X + 4, i - 4, ICON_LIST_X + 4, i - 2, GR_RESIZE_MENU);
//				gr_line(ICON_LIST_X + 6, i, ICON_LIST_X + 8, i, GR_RESIZE_MENU);
//				gr_line(ICON_LIST_X + 4, i + 2, ICON_LIST_X + 4, i + 4, GR_RESIZE_MENU);

				hotkeys = get_wing_hotkeys(line);
				break;

			case HotkeyLineType::SHIP:
			case HotkeyLineType::SUBSHIP:
				hotkeys = get_ship_hotkeys(line);
				break;

			default:
				Int3();
		}

		if (Hotkey_lines[line].type != HotkeyLineType::HEADING) {
			Assert( (line - Scroll_offset) < LIST_BUTTONS_MAX );
			List_buttons[line - Scroll_offset].update_dimensions(Hotkey_list_coords[gr_screen.res][0], y, Hotkey_list_coords[gr_screen.res][0] + Hotkey_list_coords[gr_screen.res][2] - Hotkey_list_coords[gr_screen.res][0], font_height);
			List_buttons[line - Scroll_offset].enable();
			if (hotkeys & (1 << Cur_hotkey)) {
				gr_set_color_fast(&Color_text_active);

			} else {
				if (line == Selected_line)
					gr_set_color_fast(&Color_text_selected);
				else if (line == select_tease_line)
					gr_set_color_fast(&Color_text_subselected);
				else
					gr_set_color_fast(&Color_text_normal);
			}

		} else {
			Assert( (line - Scroll_offset) < LIST_BUTTONS_MAX );
			List_buttons[line - Scroll_offset].disable();
		}

		// print active hotkeys associated for this line
		if (hotkeys) {
			for (int i=0; i<MAX_KEYED_TARGETS; i++) {
				if (hotkeys & (1 << i)) {
					gr_printf_menu(Hotkey_list_coords[gr_screen.res][0] + Hotkey_function_field_width[gr_screen.res]*i, y, "%s", textify_scancode(Key_sets[i]));
				}
			}
/*
			*buf = 0;
			for (i=0; i<MAX_KEYED_TARGETS; i++) {
				if (hotkeys & (1 << i)) {
					strcat_s(buf, Scan_code_text[Key_sets[i]]);
					strcat_s(buf, ", ");
				}
			}

			Assert(strlen(buf) > 1);
			buf[strlen(buf) - 2] = 0;  // lose the ", " on the end

			font::force_fit_string(buf, 255, GROUP_LIST_W);
			gr_printf_menu(GROUP_LIST_X, y, buf);*/
		}
	
		// draw ship/wing name
		strcpy_s(buf, Hotkey_lines[line].label.c_str());
		if (Hotkey_lines[line].type == HotkeyLineType::SUBSHIP) {
			// indent
			font::force_fit_string(buf, 255, Hotkey_list_coords[gr_screen.res][0] + Hotkey_list_coords[gr_screen.res][2] - (Hotkey_ship_x[gr_screen.res]+20));
			gr_printf_menu(Hotkey_ship_x[gr_screen.res]+20, y, "%s", buf);
		} else {
			font::force_fit_string(buf, 255, Hotkey_list_coords[gr_screen.res][0] + Hotkey_list_coords[gr_screen.res][2] - Hotkey_ship_x[gr_screen.res]);
			gr_printf_menu(Hotkey_ship_x[gr_screen.res], y, "%s", buf);
		}

		line++;
	}

	int i = line - Scroll_offset;
	while (i < LIST_BUTTONS_MAX)
		List_buttons[i++].disable();

	// blit help overlay if active
	help_overlay_maybe_blit(Hotkey_overlay_id, gr_screen.res);

	gr_flip();
}

void mission_hotkey_exit()
{
	gameseq_post_event(GS_EVENT_PREVIOUS_STATE);
}
