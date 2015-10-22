/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#define ITEM_TYPE_TEXT 0                  // simple text
#define ITEM_TYPE_BUTTON 1                // button, basically text with a border 
#define ITEM_TYPE_RADIOBUTTON 2           // toggle button, may be grouped 
#define ITEM_TYPE_CHECKBOX 3              // check box
#define ITEM_TYPE_EDITFIELD 4             // editable text, associated with a cvar
#define ITEM_TYPE_COMBO 5                 // drop down list
#define ITEM_TYPE_LISTBOX 6               // scrollable list  
#define ITEM_TYPE_MODEL 7                 // model
#define ITEM_TYPE_OWNERDRAW 8             // owner draw, name specs what it is
#define ITEM_TYPE_NUMERICFIELD 9          // editable text, associated with a cvar
#define ITEM_TYPE_SLIDER 10               // mouse speed, volume, etc.
#define ITEM_TYPE_YESNO 11                // yes no cvar setting
#define ITEM_TYPE_MULTI 12                // multiple list setting, enumerated
#define ITEM_TYPE_BIND 13		              // multiple list setting, enumerated
#define ITEM_TYPE_SLIDER_COLOR 14		// ql team colors?  etc?
#define ITEM_TYPE_PRESET 15  //FIXME ?
#define ITEM_TYPE_PRESETLIST 16  //FIXME ?

#define ITEM_ALIGN_LEFT 0                 // left alignment
#define ITEM_ALIGN_CENTER 1               // center alignment
#define ITEM_ALIGN_RIGHT 2                // right alignment

#define ITEM_TEXTSTYLE_NORMAL 0           // normal text
#define ITEM_TEXTSTYLE_BLINK 1            // fast blinking
#define ITEM_TEXTSTYLE_PULSE 2            // slow pulsing
#define ITEM_TEXTSTYLE_SHADOWED 3         // drop shadow ( need a color for this )
#define ITEM_TEXTSTYLE_OUTLINED 4         // drop shadow ( need a color for this )
#define ITEM_TEXTSTYLE_OUTLINESHADOWED 5  // drop shadow ( need a color for this )
#define ITEM_TEXTSTYLE_SHADOWEDMORE 6         // drop shadow ( need a color for this )
                          
#define WINDOW_BORDER_NONE 0              // no border
#define WINDOW_BORDER_FULL 1              // full border based on border color ( single pixel )
#define WINDOW_BORDER_HORZ 2              // horizontal borders only
#define WINDOW_BORDER_VERT 3              // vertical borders only 
#define WINDOW_BORDER_KCGRADIENT 4        // horizontal border using the gradient bars
  
#define WINDOW_STYLE_EMPTY 0              // no background
#define WINDOW_STYLE_FILLED 1             // filled with background color
#define WINDOW_STYLE_GRADIENT 2           // gradient bar based on background color 
#define WINDOW_STYLE_SHADER   3           // gradient bar based on background color 
#define WINDOW_STYLE_TEAMCOLOR 4          // team color
#define WINDOW_STYLE_CINEMATIC 5          // cinematic

#define MENU_TRUE 1                       // uh.. true
#define MENU_FALSE 0                      // and false

#define HUD_VERTICAL				0x00
#define HUD_HORIZONTAL				0x01

// list box element types
#define LISTBOX_TEXT  0x00
#define LISTBOX_IMAGE 0x01

// list feeders
#define FEEDER_HEADS						0x00			// model heads
#define FEEDER_MAPS							0x01			// text maps based on game type
#define FEEDER_SERVERS						0x02			// servers
#define FEEDER_CLANS						0x03			// clan names
#define FEEDER_ALLMAPS						0x04			// all maps available, in graphic format
#define FEEDER_REDTEAM_LIST					0x05			// red team members
#define FEEDER_BLUETEAM_LIST				0x06			// blue team members
#define FEEDER_PLAYER_LIST					0x07			// players
#define FEEDER_TEAM_LIST					0x08			// team members for team voting
#define FEEDER_MODS							0x09			//
#define FEEDER_DEMOS 						0x0a			//
#define FEEDER_SCOREBOARD					0x0b			//
#define FEEDER_Q3HEADS		 				0x0c			// model heads
#define FEEDER_SERVERSTATUS					0x0d			// server status
#define FEEDER_FINDPLAYER					0x0e			// find player
#define FEEDER_CINEMATICS					0x0f			// cinematics
// ql update 2011-07-19
#define FEEDER_ENDSCOREBOARD				0x10			// premium scoreboard with country flags and quit players
#define FEEDER_REDTEAM_STATS				0x11			// premium stats
#define FEEDER_BLUETEAM_STATS				0x12			// premium stats
#define FEEDER_BAN_LIST						0x13			// list of players banned on the server

// changed with ql update 2014-08
// display flags
#define CG_SHOW_BLUE_TEAM_HAS_REDFLAG     0x00000001
#define CG_SHOW_RED_TEAM_HAS_BLUEFLAG     0x00000002

#define CG_SHOW_DUEL 0x00000004
#define CG_SHOW_CLAN_ARENA 0x00000008
#define CG_SHOW_CTF 0x00000010
#define CG_SHOW_ONEFLAG 0x00000020
#define CG_SHOW_OBELISK 0x00000040
#define CG_SHOW_HARVESTER 0x00000080
#define CG_SHOW_DOMINATION 0x00000100
#define CG_SHOW_ANYNONTEAMGAME 0x00000200
#define CG_SHOW_ANYTEAMGAME 0x00000400
#define CG_SHOW_HEALTHCRITICAL 0x00000800
#define CG_SHOW_IF_NOT_WARMUP 0x00001000
#define CG_SHOW_IF_PLAYER_HAS_FLAG 0x00002000
#define CG_SHOW_IF_WARMUP 0x00004000
#define CG_SHOW_IF_BLUE_IS_FIRST_PLACE 0x00008000
#define CG_SHOW_TEAMINFO 0x00010000
#define CG_SHOW_NOTEAMINFO 0x00020000
#define CG_SHOW_OTHERTEAMHASFLAG 0x00040000
#define CG_SHOW_YOURTEAMHASENEMYFLAG 0x00080000
#define CG_SHOW_INTERMISSION 0x00100000
#define CG_SHOW_NOTINTERMISSION 0x00200000
#define CG_SHOW_IF_MSG_PRESENT 0x00400000
#define CG_SHOW_IF_NOTICE_PRESENT 0x00800000
#define CG_SHOW_IF_CHAT_VISIBLE 0x01000000
#define CG_SHOW_IF_PLYR_IS_FIRST_PLACE 0x02000000
#define CG_SHOW_IF_PLYR_IS_NOT_FIRST_PLACE 		0x04000000
#define CG_SHOW_IF_RED_IS_FIRST_PLACE		   	0x08000000
#define CG_SHOW_2DONLY							0x10000000
#define CG_SHOW_IF_PLYR_IS_ON_RED				0x20000000
#define CG_SHOW_IF_PLYR_IS_ON_BLUE				0x40000000
#define CG_SHOW_PLAYERS_REMAINING				0x80000000


//////////////  changed
/*
#define CG_SHOW_ANYTEAMGAME               0x00000004
#define CG_SHOW_HARVESTER                 0x00000008
#define CG_SHOW_ONEFLAG                   0x00000010
#define CG_SHOW_CTF                       0x00000020
#define CG_SHOW_OBELISK                   0x00000040
#define CG_SHOW_HEALTHCRITICAL            0x00000080
#define CG_SHOW_CLAN_ARENA                0x00000100
#define CG_SHOW_TOURNAMENT                0x00000200
#define CG_SHOW_IF_NOT_WARMUP       0x00000400
#define CG_SHOW_IF_PLAYER_HAS_FLAG				0x00000800
#define CG_SHOW_IF_WARMUP								0x00001000
#define CG_SHOW_IF_BLUE_IS_FIRST_PLACE						0x00002000
#define CG_SHOW_DOMINATION			            0x00004000
#define CG_SHOW_TEAMINFO			            0x00008000
#define CG_SHOW_NOTEAMINFO		            0x00010000
#define CG_SHOW_OTHERTEAMHASFLAG          0x00020000
#define CG_SHOW_YOURTEAMHASENEMYFLAG      0x00040000
#define CG_SHOW_ANYNONTEAMGAME            0x00080000

#define CG_SHOW_INTERMISSION                            0x00100000
#define CG_SHOW_NOTINTERMISSION                         0x00200000
#define CG_SHOW_IF_MSG_PRESENT                          0x00400000
#define CG_SHOW_IF_NOTICE_PRESENT                       0x00800000
#define CG_SHOW_IF_CHAT_VISIBLE                         0x01000000
#define CG_SHOW_IF_PLYR_IS_FIRST_PLACE          0x02000000
#define CG_SHOW_IF_PLYR_IS_NOT_FIRST_PLACE      0x04000000
#define CG_SHOW_IF_RED_IS_FIRST_PLACE           0x08000000

#define CG_SHOW_2DONLY										0x10000000

#define CG_SHOW_IF_PLYR_IS_ON_RED                       0x20000000
#define CG_SHOW_IF_PLYR_IS_ON_BLUE                      0x40000000
#define CG_SHOW_PLAYERS_REMAINING 0x80000000  // check for ca or freezetag?
*/

// 2010-12-14 new scoreboard
#define CG_SHOW_IF_PLYR1 	0x00000001
#define CG_SHOW_IF_PLYR2	0x00000002
#define CG_SHOW_IF_G_FIRED 	0x00000004
#define CG_SHOW_IF_MG_FIRED	0x00000008
#define CG_SHOW_IF_SG_FIRED	0x00000010
#define CG_SHOW_IF_GL_FIRED	0x00000020
#define CG_SHOW_IF_RL_FIRED	0x00000040
#define CG_SHOW_IF_LG_FIRED	0x00000080
#define CG_SHOW_IF_RG_FIRED	0x00000100
#define CG_SHOW_IF_PG_FIRED	0x00000200
#define CG_SHOW_IF_BFG_FIRED	0x00000400
#define CG_SHOW_IF_CG_FIRED	0x00000800
#define CG_SHOW_IF_NG_FIRED	0x00001000
#define CG_SHOW_IF_PL_FIRED	0x00002000
#define CG_SHOW_IF_HMG_FIRED 0x00004000

#define CG_SHOW_IF_PLYR_IS_ON_RED_OR_SPEC 0x00008000
#define CG_SHOW_IF_PLYR_IS_ON_BLUE_OR_SPEC 0x00010000
#define CG_SHOW_IF_PLYR_IS_ON_RED_NO_SPEC 0x00020000
#define CG_SHOW_IF_PLYR_IS_ON_BLUE_NO_SPEC 0x00040000

#define CG_SHOW_IF_LOADOUT_ENABLED 0x00080000
#define CG_SHOW_IF_LOADOUT_DISABLED 0x00100000


#define UI_SHOW_LEADER				            0x00000001
#define UI_SHOW_NOTLEADER			            0x00000002
#define UI_SHOW_FAVORITESERVERS						0x00000004
#define UI_SHOW_ANYNONTEAMGAME						0x00000008
#define UI_SHOW_ANYTEAMGAME								0x00000010
#define UI_SHOW_NEWHIGHSCORE							0x00000020
#define UI_SHOW_DEMOAVAILABLE							0x00000040
#define UI_SHOW_NEWBESTTIME								0x00000080
#define UI_SHOW_FFA												0x00000100
#define UI_SHOW_NOTFFA										0x00000200
#define UI_SHOW_NETANYNONTEAMGAME	 				0x00000400
#define UI_SHOW_NETANYTEAMGAME		 				0x00000800
#define UI_SHOW_NOTFAVORITESERVERS				0x00001000

#define UI_SHOW_IF_LOADOUT_ENABLED 0x00002000
#define UI_SHOW_IF_LOADOUT_DISABLED 0x00004000
#define UI_SHOW_IF_NOT_INTERMISSION 0x00008000
#define UI_SHOW_IF_WARMUP 0x00010000
#define UI_SHOW_IF_NOT_WARMUP 0x00020000

// owner draw types
// ideally these should be done outside of this file but
// this makes it much easier for the macro expansion to
// convert them for the designers ( from the .menu files )

// old
/*
#define CG_OWNERDRAW_BASE 1
#define CG_PLAYER_ARMOR_ICON 1
#define CG_PLAYER_ARMOR_VALUE 2
#define CG_PLAYER_HEAD 3
#define CG_PLAYER_HEALTH 4
#define CG_PLAYER_AMMO_ICON 5
#define CG_PLAYER_AMMO_VALUE 6
#define CG_SELECTEDPLAYER_HEAD 7
#define CG_SELECTEDPLAYER_NAME 8
#define CG_SELECTEDPLAYER_LOCATION 9
#define CG_SELECTEDPLAYER_STATUS 10
#define CG_SELECTEDPLAYER_WEAPON 11
#define CG_SELECTEDPLAYER_POWERUP 12

#define CG_FLAGCARRIER_HEAD 13
#define CG_FLAGCARRIER_NAME 14
#define CG_FLAGCARRIER_LOCATION 15
#define CG_FLAGCARRIER_STATUS 16
#define CG_FLAGCARRIER_WEAPON 17
#define CG_FLAGCARRIER_POWERUP 18

#define CG_PLAYER_ITEM 19
#define CG_PLAYER_SCORE 20

#define CG_BLUE_FLAGHEAD 21
#define CG_BLUE_FLAGSTATUS 22
#define CG_BLUE_FLAGNAME 23
#define CG_RED_FLAGHEAD 24
#define CG_RED_FLAGSTATUS 25
#define CG_RED_FLAGNAME 26

#define CG_BLUE_SCORE 27
#define CG_RED_SCORE 28
#define CG_RED_NAME 29
#define CG_BLUE_NAME 30
#define CG_HARVESTER_SKULLS 31					// only shows in harvester
#define CG_ONEFLAG_STATUS 32						// only shows in one flag
#define CG_PLAYER_LOCATION 33
#define CG_TEAM_COLOR 34
#define CG_CTF_POWERUP 35
                                        
#define CG_AREA_POWERUP	36
#define CG_AREA_LAGOMETER	37            // painted with old system
#define CG_PLAYER_HASFLAG 38            
#define CG_GAME_TYPE 39                 // not done

#define CG_SELECTEDPLAYER_ARMOR 40      
#define CG_SELECTEDPLAYER_HEALTH 41
#define CG_PLAYER_STATUS 42
#define CG_FRAGGED_MSG 43               // painted with old system
#define CG_PROXMINED_MSG 44             // painted with old system
#define CG_AREA_FPSINFO 45              // painted with old system
#define CG_AREA_SYSTEMCHAT 46           // painted with old system
#define CG_AREA_TEAMCHAT 47             // painted with old system
#define CG_AREA_CHAT 48                 // painted with old system
#define CG_GAME_STATUS 49
#define CG_KILLER 50
#define CG_PLAYER_ARMOR_ICON2D 51              
#define CG_PLAYER_AMMO_ICON2D 52
#define CG_ACCURACY 53
#define CG_ASSISTS 54
#define CG_DEFEND 55
#define CG_EXCELLENT 56
#define CG_IMPRESSIVE 57
#define CG_PERFECT 58
#define CG_GAUNTLET 59
#define CG_SPECTATORS 60
#define CG_TEAMINFO 61
#define CG_VOICE_HEAD 62
#define CG_VOICE_NAME 63
#define CG_PLAYER_HASFLAG2D 64            
#define CG_HARVESTER_SKULLS2D 65					// only shows in harvester
#define CG_CAPFRAGLIMIT 66	 
#define CG_1STPLACE 67
#define CG_2NDPLACE 68
#define CG_CAPTURES 69
#define CG_FULLTEAMINFO 70
#define CG_LEVELTIMER 71
#define CG_PLAYER_SKILL 72
#define CG_PLAYER_OBIT 73
#define CG_PLAYER_HEALTH_BAR_100 74
#define CG_PLAYER_HEALTH_BAR_200 75
#define CG_PLAYER_ARMOR_BAR_100 76
#define CG_PLAYER_ARMOR_BAR_200 77
#define CG_AREA_NEW_CHAT 78
#define CG_TEAM_COLORIZED 79
#define CG_1ST_PLACE_SCORE 80
#define CG_2ND_PLACE_SCORE 81
#define CG_GAME_TYPE_ICON 82
#define CG_1STPLACE_PLYR_MODEL 83
#define CG_MATCH_WINNER 84
#define CG_MATCH_END_CONDITION 85
#define CG_PLYR_END_GAME_SCORE 86
#define CG_MAP_NAME 87
#define CG_PLYR_BEST_WEAPON_NAME 88
#define CG_SELECTED_PLYR_TEAM_COLOR 89
#define CG_SELECTED_PLYR_ACCURACY 90
#define CG_PLAYER_COUNTS 91
#define CG_RED_PLAYER_COUNT 92
#define CG_BLUE_PLAYER_COUNT 93
#define CG_FOLLOW_PLAYER_NAME 94
#define CG_RED_CLAN_PLYRS 95
#define CG_BLUE_CLAN_PLYRS 96
#define CG_GAME_LIMIT 97
#define CG_ROUNDTIMER 98
#define CG_VOTEMAP1 99
#define CG_RED_TIMEOUT_COUNT 100
#define CG_BLUE_TIMEOUT_COUNT 101
#define CG_1ST_PLACE_SCORE_EX 102
#define CG_2ND_PLACE_SCORE_EX 103
#define CG_FOLLOW_PLAYER_NAME_EX 104
#define CG_SPEEDOMETER 105
#define CG_WP_VERTICAL 106
#define CG_ACC_VERTICAL 107


#define CG_1STPLACE_PLYR_MODEL_ACTIVE 108
#define CG_1ST_PLYR 109
#define CG_1ST_PLYR_SCORE 110
#define CG_1ST_PLYR_FRAGS 111
#define CG_1ST_PLYR_DEATHS 112
#define CG_1ST_PLYR_DMG 113
#define CG_1ST_PLYR_TIME 114
#define CG_1ST_PLYR_PING 115
#define CG_1ST_PLYR_WINS 116
#define CG_1ST_PLYR_ACC 117
#define CG_1ST_PLYR_FLAG 118
#define CG_1ST_PLYR_FULLCLAN 119
#define CG_1ST_PLYR_TIMEOUT_COUNT 120
#define CG_2ND_PLYR 121
#define CG_2ND_PLYR_SCORE 122
#define CG_2ND_PLYR_FRAGS 123
#define CG_2ND_PLYR_DEATHS 124
#define CG_2ND_PLYR_DMG 125
#define CG_2ND_PLYR_TIME 126
#define CG_2ND_PLYR_PING 127
#define CG_2ND_PLYR_WINS 128
#define CG_2ND_PLYR_ACC 129
#define CG_2ND_PLYR_FLAG 130
#define CG_2ND_PLYR_FULLCLAN 131
#define CG_2ND_PLYR_TIMEOUT_COUNT 132
#define CG_RED_AVG_PING 133
#define CG_BLUE_AVG_PING 134
#define CG_VOTEMAP2 135
#define CG_VOTEMAP3 136
#define CG_VOTESHOT1 137
#define CG_VOTESHOT2 138
#define CG_VOTESHOT3 139
#define CG_VOTENAME1 140
#define CG_VOTENAME2 141
#define CG_VOTENAME3 142
#define CG_VOTECOUNT1 143
#define CG_VOTECOUNT2 144
#define CG_VOTECOUNT3 145
#define CG_VOTETIMER 146
#define CG_BEST_ITEMCONTROL_PLYR 147
#define CG_MOST_ACCURATE_PLYR 148
#define CG_MOST_DAMAGEDEALT_PLYR 149
#define CG_LOCALTIME 150
#define CG_MATCH_DETAILS 151
#define CG_1ST_PLYR_FRAGS_G 152
#define CG_1ST_PLYR_FRAGS_MG 153
#define CG_1ST_PLYR_FRAGS_SG 154
#define CG_1ST_PLYR_FRAGS_GL 155
#define CG_1ST_PLYR_FRAGS_RL 156
#define CG_1ST_PLYR_FRAGS_LG 157
#define CG_1ST_PLYR_FRAGS_RG 158
#define CG_1ST_PLYR_FRAGS_PG 159
#define CG_1ST_PLYR_FRAGS_BFG 160
#define CG_1ST_PLYR_FRAGS_CG 161
#define CG_1ST_PLYR_FRAGS_NG 162
#define CG_1ST_PLYR_FRAGS_PL 163
#define CG_1ST_PLYR_HITS_MG 164
#define CG_1ST_PLYR_HITS_SG 165
#define CG_1ST_PLYR_HITS_GL 166
#define CG_1ST_PLYR_HITS_RL 167
#define CG_1ST_PLYR_HITS_LG 168
#define CG_1ST_PLYR_HITS_RG 169
#define CG_1ST_PLYR_HITS_PG 170
#define CG_1ST_PLYR_HITS_BFG 171
#define CG_1ST_PLYR_HITS_CG 172
#define CG_1ST_PLYR_HITS_NG 173
#define CG_1ST_PLYR_HITS_PL 174
#define CG_1ST_PLYR_SHOTS_MG 175
#define CG_1ST_PLYR_SHOTS_SG 176
#define CG_1ST_PLYR_SHOTS_GL 177
#define CG_1ST_PLYR_SHOTS_RL 178
#define CG_1ST_PLYR_SHOTS_LG 179
#define CG_1ST_PLYR_SHOTS_RG 180
#define CG_1ST_PLYR_SHOTS_PG 181
#define CG_1ST_PLYR_SHOTS_BFG 182
#define CG_1ST_PLYR_SHOTS_CG 183
#define CG_1ST_PLYR_SHOTS_NG 184
#define CG_1ST_PLYR_SHOTS_PL 185
#define CG_1ST_PLYR_DMG_G 186
#define CG_1ST_PLYR_DMG_MG 187
#define CG_1ST_PLYR_DMG_SG 188
#define CG_1ST_PLYR_DMG_GL 189
#define CG_1ST_PLYR_DMG_RL 190
#define CG_1ST_PLYR_DMG_LG 191
#define CG_1ST_PLYR_DMG_RG 192
#define CG_1ST_PLYR_DMG_PG 193
#define CG_1ST_PLYR_DMG_BFG 194
#define CG_1ST_PLYR_DMG_CG 195
#define CG_1ST_PLYR_DMG_NG 196
#define CG_1ST_PLYR_DMG_PL 197
#define CG_1ST_PLYR_ACC_MG 198
#define CG_1ST_PLYR_ACC_SG 199
#define CG_1ST_PLYR_ACC_GL 200
#define CG_1ST_PLYR_ACC_RL 201
#define CG_1ST_PLYR_ACC_LG 202
#define CG_1ST_PLYR_ACC_RG 203
#define CG_1ST_PLYR_ACC_PG 204
#define CG_1ST_PLYR_ACC_BFG 205
#define CG_1ST_PLYR_ACC_CG 206
#define CG_1ST_PLYR_ACC_NG 207
#define CG_1ST_PLYR_ACC_PL 208
#define CG_1ST_PLYR_PICKUPS_RA 209
#define CG_1ST_PLYR_PICKUPS_YA 210
#define CG_1ST_PLYR_PICKUPS_GA 211
#define CG_1ST_PLYR_PICKUPS_MH 212
#define CG_1ST_PLYR_AVG_PICKUP_TIME_RA 213
#define CG_1ST_PLYR_AVG_PICKUP_TIME_YA 214
#define CG_1ST_PLYR_AVG_PICKUP_TIME_GA 215
#define CG_1ST_PLYR_AVG_PICKUP_TIME_MH 216
#define CG_2ND_PLYR_FRAGS_G 217
#define CG_2ND_PLYR_FRAGS_MG 218
#define CG_2ND_PLYR_FRAGS_SG 219
#define CG_2ND_PLYR_FRAGS_GL 220
#define CG_2ND_PLYR_FRAGS_RL 221
#define CG_2ND_PLYR_FRAGS_LG 222
#define CG_2ND_PLYR_FRAGS_RG 223
#define CG_2ND_PLYR_FRAGS_PG 224
#define CG_2ND_PLYR_FRAGS_BFG 225
#define CG_2ND_PLYR_FRAGS_CG 226
#define CG_2ND_PLYR_FRAGS_NG 227
#define CG_2ND_PLYR_FRAGS_PL 228
#define CG_2ND_PLYR_HITS_MG 229
#define CG_2ND_PLYR_HITS_SG 230
#define CG_2ND_PLYR_HITS_GL 231
#define CG_2ND_PLYR_HITS_RL 232
#define CG_2ND_PLYR_HITS_LG 233
#define CG_2ND_PLYR_HITS_RG 234
#define CG_2ND_PLYR_HITS_PG 235
#define CG_2ND_PLYR_HITS_BFG 236
#define CG_2ND_PLYR_HITS_CG 237
#define CG_2ND_PLYR_HITS_NG 238
#define CG_2ND_PLYR_HITS_PL 239
#define CG_2ND_PLYR_SHOTS_MG 240
#define CG_2ND_PLYR_SHOTS_SG 241
#define CG_2ND_PLYR_SHOTS_GL 242
#define CG_2ND_PLYR_SHOTS_RL 243
#define CG_2ND_PLYR_SHOTS_LG 244
#define CG_2ND_PLYR_SHOTS_RG 245
#define CG_2ND_PLYR_SHOTS_PG 246
#define CG_2ND_PLYR_SHOTS_BFG 247
#define CG_2ND_PLYR_SHOTS_CG 248
#define CG_2ND_PLYR_SHOTS_NG 249
#define CG_2ND_PLYR_SHOTS_PL 250
#define CG_2ND_PLYR_DMG_G 251
#define CG_2ND_PLYR_DMG_MG 252
#define CG_2ND_PLYR_DMG_SG 253
#define CG_2ND_PLYR_DMG_GL 254
#define CG_2ND_PLYR_DMG_RL 255
#define CG_2ND_PLYR_DMG_LG 256
#define CG_2ND_PLYR_DMG_RG 257
#define CG_2ND_PLYR_DMG_PG 258
#define CG_2ND_PLYR_DMG_BFG 259
#define CG_2ND_PLYR_DMG_CG 260
#define CG_2ND_PLYR_DMG_NG 261
#define CG_2ND_PLYR_DMG_PL 262
#define CG_2ND_PLYR_ACC_MG 263
#define CG_2ND_PLYR_ACC_SG 264
#define CG_2ND_PLYR_ACC_GL 265
#define CG_2ND_PLYR_ACC_RL 266
#define CG_2ND_PLYR_ACC_LG 267
#define CG_2ND_PLYR_ACC_RG 268
#define CG_2ND_PLYR_ACC_PG 269
#define CG_2ND_PLYR_ACC_BFG 270
#define CG_2ND_PLYR_ACC_CG 271
#define CG_2ND_PLYR_ACC_NG 272
#define CG_2ND_PLYR_ACC_PL 273
#define CG_2ND_PLYR_PICKUPS_RA 274
#define CG_2ND_PLYR_PICKUPS_YA 275
#define CG_2ND_PLYR_PICKUPS_GA 276
#define CG_2ND_PLYR_PICKUPS_MH 277
#define CG_2ND_PLYR_AVG_PICKUP_TIME_RA 278
#define CG_2ND_PLYR_AVG_PICKUP_TIME_YA 279
#define CG_2ND_PLYR_AVG_PICKUP_TIME_GA 280
#define CG_2ND_PLYR_AVG_PICKUP_TIME_MH 281
#define CG_1ST_PLYR_EXCELLENT 282
#define CG_1ST_PLYR_IMPRESSIVE 283
#define CG_1ST_PLYR_HUMILIATION 284
#define CG_2ND_PLYR_EXCELLENT 285
#define CG_2ND_PLYR_IMPRESSIVE 286
#define CG_2ND_PLYR_HUMILIATION 287
#define CG_1ST_PLYR_READY 288
#define CG_2ND_PLYR_READY 289
#define CG_SELECTED_PLYR_NAME 290
#define CG_SELECTED_PLYR_SCORE 291
#define CG_SELECTED_PLYR_DMG 292
#define CG_SELECTED_PLYR_ACC 293
#define CG_SELECTED_PLYR_FLAG 294
#define CG_SELECTED_PLYR_FULLCLAN 295
#define CG_SELECTED_PLYR_PICKUPS_RA 296
#define CG_SELECTED_PLYR_PICKUPS_YA 297
#define CG_SELECTED_PLYR_PICKUPS_GA 298
#define CG_SELECTED_PLYR_PICKUPS_MH 299

#define CG_1ST_PLYR_PR 300
#define CG_2ND_PLYR_PR 301
#define CG_1ST_PLYR_TIER 302
#define CG_2ND_PLYR_TIER 303
#define CG_SERVER_OWNER 304

#define CG_RED_TEAM_PICKUPS_RA 305
#define CG_RED_TEAM_PICKUPS_YA 306
#define CG_RED_TEAM_PICKUPS_GA 307
#define CG_RED_TEAM_PICKUPS_MH 308
#define CG_RED_TEAM_PICKUPS_QUAD 309
#define CG_RED_TEAM_PICKUPS_BS 310
#define CG_BLUE_TEAM_PICKUPS_RA 311
#define CG_BLUE_TEAM_PICKUPS_YA 312
#define CG_BLUE_TEAM_PICKUPS_GA 313
#define CG_BLUE_TEAM_PICKUPS_MH 314
#define CG_BLUE_TEAM_PICKUPS_QUAD 315
#define CG_BLUE_TEAM_PICKUPS_BS 316
#define CG_RED_TEAM_TIMEHELD_QUAD 317
#define CG_RED_TEAM_TIMEHELD_BS 318
#define CG_BLUE_TEAM_TIMEHELD_QUAD 319
#define CG_BLUE_TEAM_TIMEHELD_BS 320

#define CG_RED_TEAM_PICKUPS_FLAG 321
#define CG_RED_TEAM_PICKUPS_MEDKIT 322
#define CG_RED_TEAM_PICKUPS_REGEN 323
#define CG_RED_TEAM_PICKUPS_HASTE 324
#define CG_RED_TEAM_PICKUPS_INVIS 325
#define CG_BLUE_TEAM_PICKUPS_FLAG 326
#define CG_BLUE_TEAM_PICKUPS_MEDKIT 327
#define CG_BLUE_TEAM_PICKUPS_REGEN 328
#define CG_BLUE_TEAM_PICKUPS_HASTE 329
#define CG_BLUE_TEAM_PICKUPS_INVIS 330
#define CG_RED_TEAM_TIMEHELD_FLAG 331
#define CG_RED_TEAM_TIMEHELD_REGEN 332
#define CG_RED_TEAM_TIMEHELD_HASTE 333
#define CG_RED_TEAM_TIMEHELD_INVIS 334
#define CG_BLUE_TEAM_TIMEHELD_FLAG 335
#define CG_BLUE_TEAM_TIMEHELD_REGEN 336
#define CG_BLUE_TEAM_TIMEHELD_HASTE 337
#define CG_BLUE_TEAM_TIMEHELD_INVIS 338
#define CG_ARMORTIERED_COLORIZED 339
#define CG_RED_TEAM_MAP_PICKUPS 340
#define CG_BLUE_TEAM_MAP_PICKUPS 341
#define CG_1ST_PLYR_PICKUPS 342
#define CG_2ND_PLYR_PICKUPS 343
#define CG_SERVER_SETTINGS 344
#define CG_MOST_VALUABLE_OFFENSIVE_PLYR 345
#define CG_MOST_VALUABLE_DEFENSIVE_PLYR 346
#define CG_MOST_VALUABLE_PLYR 347
#define CG_RED_OWNED_FLAGS 348
#define CG_BLUE_OWNED_FLAGS 349
#define CG_TEAM_PLYR_COUNT 350
#define CG_ENEMY_PLYR_COUNT 351
#define CG_ROUND 352
#define CG_RACE_STATUS 353
#define CG_RACE_TIMES 354
#define CG_SPEC_MESSAGES 355

*/

// new 2014-08 ql dm 90

#define CG_SERVER_SETTINGS 1 
#define CG_SERVER_OWNER 2
#define CG_GAME_LIMIT 3
#define CG_GAME_TYPE 4
#define CG_GAME_TYPE_ICON				5

#define CG_GAME_TYPE_MAP 6	// Gametype Fullname - Server Locaton - Map
#define CG_GAME_STATUS 7	// Scores message
#define CG_ARMORTIERED_COLORIZED		8
#define	CG_MATCH_DETAILS 9	// Game state - Gametype Shortname - Server Location - Map
#define CG_MATCH_END_CONDITION			10
#define CG_MATCH_STATUS 11	// Game State - Scores message
#define CG_CAPFRAGLIMIT 12
#define	CG_LOCALTIME 13
#define CG_LEVELTIMER 14
#define	CG_ROUND 15
#define CG_ROUNDTIMER 16
#define CG_PLAYER_COUNTS 17
#define CG_MAP_NAME 18
#define CG_VOTEMAP1 19
#define	CG_VOTEMAP2 20
#define	CG_VOTEMAP3 21
#define	CG_VOTESHOT1 22
#define	CG_VOTESHOT2 23
#define	CG_VOTESHOT3 24
#define	CG_VOTENAME1 25
#define	CG_VOTENAME2 26
#define	CG_VOTENAME3 27
#define	CG_VOTECOUNT1 28
#define	CG_VOTECOUNT2 29
#define	CG_VOTECOUNT3 30
#define	CG_VOTETIMER 31
#define CG_SPEC_MESSAGES 32
#define CG_PLAYER_HEAD 33
#define CG_PLAYERMODEL 34
#define CG_PLAYER_ARMOR_ICON 35
#define CG_PLAYER_ARMOR_ICON2D 36
#define CG_PLAYER_ARMOR_VALUE 37
#define CG_PLAYER_ARMOR_BAR_100 38
#define CG_PLAYER_ARMOR_BAR_200 39
#define CG_PLAYER_HEALTH 40
#define CG_PLAYER_HEALTH_BAR_100 41
#define CG_PLAYER_HEALTH_BAR_200 42
#define CG_PLAYER_AMMO_ICON 43
#define CG_PLAYER_AMMO_ICON2D 44
#define CG_PLAYER_AMMO_VALUE 45
#define CG_PLAYER_ITEM 46
#define CG_PLAYER_SCORE 47
#define CG_BLUE_SCORE 48
#define CG_RED_SCORE 49
#define CG_RED_NAME 50
#define CG_BLUE_NAME 51
#define CG_RACE_STATUS 52
#define CG_RACE_TIMES 53
#define CG_TEAM_PLYR_COUNT 54
#define CG_ENEMY_PLYR_COUNT 55
#define CG_BLUE_FLAGSTATUS 56
#define CG_RED_FLAGSTATUS 57
#define CG_ONEFLAG_STATUS 58
#define CG_PLAYER_HASFLAG 59
#define CG_PLAYER_HASFLAG2D 60
#define CG_HARVESTER_SKULLS 61
#define CG_HARVESTER_SKULLS2D 62
#define CG_RED_OWNED_FLAGS 63
#define CG_BLUE_OWNED_FLAGS 64
#define CG_TEAM_COLOR 65
#define CG_CTF_POWERUP 66
#define CG_AREA_POWERUP 67
#define CG_KILLER 68
#define CG_ACCURACY 69
#define CG_ASSISTS 70
#define CG_CAPTURES 71
#define CG_COMBOKILLS 72
#define CG_DEFEND 73
#define CG_EXCELLENT 74
#define CG_GAUNTLET 75
#define CG_IMPRESSIVE 76
#define CG_RAMPAGES 77
#define CG_MIDAIRS 78
#define CG_PERFECT 79
#define	CG_MOST_VALUABLE_OFFENSIVE_PLYR	80
#define	CG_MOST_VALUABLE_DEFENSIVE_PLYR 81
#define	CG_MOST_VALUABLE_PLYR 82
#define	CG_BEST_ITEMCONTROL_PLYR 83
#define	CG_MOST_ACCURATE_PLYR 84
#define	CG_MOST_DAMAGEDEALT_PLYR 85
#define CG_SPECTATORS 86
#define CG_MATCH_WINNER 87
#define CG_1STPLACE 88
#define CG_1ST_PLACE_SCORE 89
#define CG_1STPLACE_PLYR_MODEL 90
#define CG_2NDPLACE 91
#define CG_2ND_PLACE_SCORE 92
#define	CG_RED_AVG_PING 93
#define	CG_BLUE_AVG_PING 94
#define CG_PLAYER_SKILL 95
#define CG_PLAYER_OBIT 96
#define CG_AREA_NEW_CHAT 97
#define CG_TEAM_COLORIZED 98
#define CG_PLYR_END_GAME_SCORE 99
#define CG_PLYR_BEST_WEAPON_NAME 100
#define CG_SELECTED_PLYR_TEAM_COLOR 101
#define CG_SELECTED_PLYR_ACCURACY 102
#define CG_RED_PLAYER_COUNT 103
#define CG_BLUE_PLAYER_COUNT 104
#define CG_FOLLOW_PLAYER_NAME 105
#define CG_RED_CLAN_PLYRS 106
#define CG_BLUE_CLAN_PLYRS 107
#define CG_RED_TIMEOUT_COUNT 108
#define CG_BLUE_TIMEOUT_COUNT 109
#define CG_FOLLOW_PLAYER_NAME_EX 110
#define CG_SPEEDOMETER 111
#define CG_WP_VERTICAL 112
#define CG_ACC_VERTICAL 113
#define	CG_1STPLACE_PLYR_MODEL_ACTIVE 114
#define	CG_1ST_PLYR 115
#define	CG_1ST_PLYR_READY 116
#define	CG_1ST_PLYR_SCORE 117
#define	CG_1ST_PLYR_FRAGS 118
#define	CG_1ST_PLYR_DEATHS 119
#define	CG_1ST_PLYR_DMG 120
#define	CG_1ST_PLYR_TIME 121
#define	CG_1ST_PLYR_PING 122
#define	CG_1ST_PLYR_WINS 123
#define	CG_1ST_PLYR_ACC 124
#define	CG_1ST_PLYR_FLAG 125
#define	CG_1ST_PLYR_FULLCLAN 126
#define	CG_1ST_PLYR_TIMEOUT_COUNT 127
#define	CG_1ST_PLYR_HEALTH_ARMOR 128
#define CG_1ST_PLYR_FRAGS_G 129
#define CG_1ST_PLYR_FRAGS_MG 130
#define CG_1ST_PLYR_FRAGS_SG 131
#define CG_1ST_PLYR_FRAGS_GL 132
#define CG_1ST_PLYR_FRAGS_RL 133
#define CG_1ST_PLYR_FRAGS_LG 134
#define CG_1ST_PLYR_FRAGS_RG 135
#define CG_1ST_PLYR_FRAGS_PG 136
#define CG_1ST_PLYR_FRAGS_BFG 137
#define CG_1ST_PLYR_FRAGS_CG 138
#define CG_1ST_PLYR_FRAGS_NG 139
#define CG_1ST_PLYR_FRAGS_PL 140
#define CG_1ST_PLYR_FRAGS_HMG 141
#define CG_1ST_PLYR_HITS_MG 142
#define CG_1ST_PLYR_HITS_SG 143
#define CG_1ST_PLYR_HITS_GL 144
#define CG_1ST_PLYR_HITS_RL 145
#define CG_1ST_PLYR_HITS_LG 146
#define CG_1ST_PLYR_HITS_RG 147
#define CG_1ST_PLYR_HITS_PG 148
#define CG_1ST_PLYR_HITS_BFG 149
#define CG_1ST_PLYR_HITS_CG 150
#define CG_1ST_PLYR_HITS_NG 151
#define CG_1ST_PLYR_HITS_PL 152
#define CG_1ST_PLYR_HITS_HMG 153
#define CG_1ST_PLYR_SHOTS_MG 154
#define CG_1ST_PLYR_SHOTS_SG 155
#define CG_1ST_PLYR_SHOTS_GL 156
#define CG_1ST_PLYR_SHOTS_RL 157
#define CG_1ST_PLYR_SHOTS_LG 158
#define CG_1ST_PLYR_SHOTS_RG 159
#define CG_1ST_PLYR_SHOTS_PG 160
#define CG_1ST_PLYR_SHOTS_BFG 161
#define CG_1ST_PLYR_SHOTS_CG 162
#define CG_1ST_PLYR_SHOTS_NG 163
#define CG_1ST_PLYR_SHOTS_PL 164
#define CG_1ST_PLYR_SHOTS_HMG 165
#define CG_1ST_PLYR_DMG_G 166
#define CG_1ST_PLYR_DMG_MG 167
#define CG_1ST_PLYR_DMG_SG 168
#define CG_1ST_PLYR_DMG_GL 169
#define CG_1ST_PLYR_DMG_RL 170
#define CG_1ST_PLYR_DMG_LG 171
#define CG_1ST_PLYR_DMG_RG 172
#define CG_1ST_PLYR_DMG_PG 173
#define CG_1ST_PLYR_DMG_BFG 174
#define CG_1ST_PLYR_DMG_CG 175
#define CG_1ST_PLYR_DMG_NG 176
#define CG_1ST_PLYR_DMG_PL 177
#define CG_1ST_PLYR_DMG_HMG 178
#define	CG_1ST_PLYR_ACC_MG 179
#define	CG_1ST_PLYR_ACC_SG 180
#define	CG_1ST_PLYR_ACC_GL 181
#define	CG_1ST_PLYR_ACC_RL 182
#define	CG_1ST_PLYR_ACC_LG 183
#define	CG_1ST_PLYR_ACC_RG 184
#define	CG_1ST_PLYR_ACC_PG 185
#define	CG_1ST_PLYR_ACC_BFG 186
#define	CG_1ST_PLYR_ACC_CG 187
#define	CG_1ST_PLYR_ACC_NG 188
#define	CG_1ST_PLYR_ACC_PL 189
#define	CG_1ST_PLYR_ACC_HMG 190
#define CG_1ST_PLYR_PICKUPS 191
#define CG_1ST_PLYR_PICKUPS_RA 192
#define CG_1ST_PLYR_PICKUPS_YA 193
#define CG_1ST_PLYR_PICKUPS_GA 194
#define CG_1ST_PLYR_PICKUPS_MH 195
#define CG_1ST_PLYR_AVG_PICKUP_TIME_RA 196
#define CG_1ST_PLYR_AVG_PICKUP_TIME_YA 197
#define CG_1ST_PLYR_AVG_PICKUP_TIME_GA 198
#define CG_1ST_PLYR_AVG_PICKUP_TIME_MH 199
#define CG_1ST_PLYR_EXCELLENT 200
#define CG_1ST_PLYR_IMPRESSIVE 201
#define CG_1ST_PLYR_HUMILIATION 202
#define CG_1ST_PLYR_PR 203
#define CG_1ST_PLYR_TIER 204
#define	CG_2ND_PLYR 205
#define	CG_2ND_PLYR_READY 206
#define	CG_2ND_PLYR_SCORE 207
#define	CG_2ND_PLYR_FRAGS 208
#define	CG_2ND_PLYR_DEATHS 209
#define	CG_2ND_PLYR_DMG 210
#define	CG_2ND_PLYR_TIME 211
#define	CG_2ND_PLYR_PING 212
#define	CG_2ND_PLYR_WINS 213
#define	CG_2ND_PLYR_ACC 214
#define	CG_2ND_PLYR_FLAG 215
#define	CG_2ND_PLYR_FULLCLAN 216
#define	CG_2ND_PLYR_TIMEOUT_COUNT 217
#define	CG_2ND_PLYR_HEALTH_ARMOR 218
#define CG_2ND_PLYR_FRAGS_G 219
#define CG_2ND_PLYR_FRAGS_MG 220
#define CG_2ND_PLYR_FRAGS_SG 221
#define CG_2ND_PLYR_FRAGS_GL 222
#define CG_2ND_PLYR_FRAGS_RL 223
#define CG_2ND_PLYR_FRAGS_LG 224
#define CG_2ND_PLYR_FRAGS_RG 225
#define CG_2ND_PLYR_FRAGS_PG 226
#define CG_2ND_PLYR_FRAGS_BFG 227
#define CG_2ND_PLYR_FRAGS_CG 228
#define CG_2ND_PLYR_FRAGS_NG 229
#define CG_2ND_PLYR_FRAGS_PL 230
#define CG_2ND_PLYR_FRAGS_HMG 231
#define CG_2ND_PLYR_HITS_MG 232
#define CG_2ND_PLYR_HITS_SG 233
#define CG_2ND_PLYR_HITS_GL 234
#define CG_2ND_PLYR_HITS_RL 235
#define CG_2ND_PLYR_HITS_LG 236
#define CG_2ND_PLYR_HITS_RG 237
#define CG_2ND_PLYR_HITS_PG 238
#define CG_2ND_PLYR_HITS_BFG 239
#define CG_2ND_PLYR_HITS_CG 240
#define CG_2ND_PLYR_HITS_NG 241
#define CG_2ND_PLYR_HITS_PL 242
#define CG_2ND_PLYR_HITS_HMG 243
#define CG_2ND_PLYR_SHOTS_MG 244
#define CG_2ND_PLYR_SHOTS_SG 245
#define CG_2ND_PLYR_SHOTS_GL 246
#define CG_2ND_PLYR_SHOTS_RL 247
#define CG_2ND_PLYR_SHOTS_LG 248
#define CG_2ND_PLYR_SHOTS_RG 249
#define CG_2ND_PLYR_SHOTS_PG 250
#define CG_2ND_PLYR_SHOTS_BFG 251
#define CG_2ND_PLYR_SHOTS_CG 252
#define CG_2ND_PLYR_SHOTS_NG 253
#define CG_2ND_PLYR_SHOTS_PL 254
#define CG_2ND_PLYR_SHOTS_HMG 255
#define CG_2ND_PLYR_DMG_G 256
#define CG_2ND_PLYR_DMG_MG 257
#define CG_2ND_PLYR_DMG_SG 258
#define CG_2ND_PLYR_DMG_GL 259
#define CG_2ND_PLYR_DMG_RL 260
#define CG_2ND_PLYR_DMG_LG 261
#define CG_2ND_PLYR_DMG_RG 262
#define CG_2ND_PLYR_DMG_PG 263
#define CG_2ND_PLYR_DMG_BFG 264
#define CG_2ND_PLYR_DMG_CG 265
#define CG_2ND_PLYR_DMG_NG 266
#define CG_2ND_PLYR_DMG_PL 267
#define CG_2ND_PLYR_DMG_HMG 268
#define	CG_2ND_PLYR_ACC_MG 269
#define	CG_2ND_PLYR_ACC_SG 270
#define	CG_2ND_PLYR_ACC_GL 271
#define	CG_2ND_PLYR_ACC_RL 272
#define	CG_2ND_PLYR_ACC_LG 273
#define	CG_2ND_PLYR_ACC_RG 274
#define	CG_2ND_PLYR_ACC_PG 275
#define	CG_2ND_PLYR_ACC_BFG 276
#define	CG_2ND_PLYR_ACC_CG 277
#define	CG_2ND_PLYR_ACC_NG 278
#define	CG_2ND_PLYR_ACC_PL 279
#define	CG_2ND_PLYR_ACC_HMG 280
#define CG_2ND_PLYR_PICKUPS 281
#define CG_2ND_PLYR_PICKUPS_RA 282
#define CG_2ND_PLYR_PICKUPS_YA 283
#define CG_2ND_PLYR_PICKUPS_GA 284
#define CG_2ND_PLYR_PICKUPS_MH 285
#define CG_2ND_PLYR_AVG_PICKUP_TIME_RA 286
#define CG_2ND_PLYR_AVG_PICKUP_TIME_YA 287
#define CG_2ND_PLYR_AVG_PICKUP_TIME_GA 288
#define CG_2ND_PLYR_AVG_PICKUP_TIME_MH 289
#define CG_2ND_PLYR_EXCELLENT 290
#define CG_2ND_PLYR_IMPRESSIVE 291
#define CG_2ND_PLYR_HUMILIATION 292
#define CG_2ND_PLYR_PR 293
#define CG_2ND_PLYR_TIER 294
#define CG_RED_TEAM_MAP_PICKUPS 295
#define CG_RED_TEAM_PICKUPS_RA 296
#define CG_RED_TEAM_PICKUPS_YA 297
#define CG_RED_TEAM_PICKUPS_GA 298
#define CG_RED_TEAM_PICKUPS_MH 299
#define CG_RED_TEAM_PICKUPS_QUAD 300
#define CG_RED_TEAM_PICKUPS_BS 301
#define CG_RED_TEAM_TIMEHELD_QUAD 302
#define CG_RED_TEAM_TIMEHELD_BS 303
#define CG_RED_TEAM_PICKUPS_FLAG 304
#define CG_RED_TEAM_PICKUPS_MEDKIT 305
#define CG_RED_TEAM_PICKUPS_REGEN 306
#define CG_RED_TEAM_PICKUPS_HASTE 307
#define CG_RED_TEAM_PICKUPS_INVIS 308
#define CG_RED_TEAM_TIMEHELD_FLAG 309
#define CG_RED_TEAM_TIMEHELD_REGEN 310
#define CG_RED_TEAM_TIMEHELD_HASTE 311
#define CG_RED_TEAM_TIMEHELD_INVIS 312
#define CG_BLUE_TEAM_MAP_PICKUPS 313
#define CG_BLUE_TEAM_PICKUPS_RA 314
#define CG_BLUE_TEAM_PICKUPS_YA 315
#define CG_BLUE_TEAM_PICKUPS_GA 316
#define CG_BLUE_TEAM_PICKUPS_MH 317
#define CG_BLUE_TEAM_PICKUPS_QUAD 318
#define CG_BLUE_TEAM_PICKUPS_BS 319
#define CG_BLUE_TEAM_TIMEHELD_QUAD 320
#define CG_BLUE_TEAM_TIMEHELD_BS 321
#define CG_BLUE_TEAM_PICKUPS_FLAG 322
#define CG_BLUE_TEAM_PICKUPS_MEDKIT 323
#define CG_BLUE_TEAM_PICKUPS_REGEN 324
#define CG_BLUE_TEAM_PICKUPS_HASTE 325
#define CG_BLUE_TEAM_PICKUPS_INVIS 326
#define CG_BLUE_TEAM_TIMEHELD_FLAG 327
#define CG_BLUE_TEAM_TIMEHELD_REGEN 328
#define CG_BLUE_TEAM_TIMEHELD_HASTE 329
#define CG_BLUE_TEAM_TIMEHELD_INVIS 330

#define CG_VOTEGAMETYPE1 331
#define CG_VOTEGAMETYPE2 332
#define CG_VOTEGAMETYPE3 333
#define CG_OVERTIME 334
#define CG_PLAYER_HASKEY 335
#define CG_STARTING_WEAPONS 336

//////////////////////


#define UI_OWNERDRAW_BASE 512
#define UI_HANDICAP 513

//#define UI_EFFECTS (6666 + 14)  //514

#define UI_PLAYERMODEL 514

//#define UI_CLANNAME (6666 + 1)  //516  // for compiling
//#define UI_CLANLOGO (6666 + 15)  //517

#define UI_GAMETYPE 515
#define UI_MAPPREVIEW 516
#define UI_SKILL 517

//#define UI_BLUETEAMNAME (6666 + 2)  //521  // for compiling
//#define UI_REDTEAMNAME (6666 + 3)  // 522  // for compiling
//#define UI_BLUETEAM1 (6666 + 4)  //523
//#define UI_BLUETEAM2 (6666 + 5)  //524
//#define UI_BLUETEAM3 (6666 + 6)  // 525
//#define UI_BLUETEAM4 (6666 + 7)  //526
//#define UI_BLUETEAM5 (6666 + 8)  //527
//#define UI_REDTEAM1 (6666 + 9)  //528
//#define UI_REDTEAM2 (6666 + 10)  //529
//#define UI_REDTEAM3 (6666 + 11)  //530
//#define UI_REDTEAM4 (6666 + 12)  //531
//#define UI_REDTEAM5 (6666 + 13)  //532

#define UI_NETSOURCE 518
#define UI_NETMAPPREVIEW 519
#define UI_NETFILTER 520
#define UI_TIER 521
#define UI_OPPONENTMODEL 522
#define UI_TIERMAP1 523
#define UI_TIERMAP2 524
#define UI_TIERMAP3 525

#define UI_PLAYERLOGO (6666 + 16)  //541
#define UI_OPPONENTLOGO (6666 + 17)  //542
#define UI_PLAYERLOGO_METAL (6666 + 18)  //543
#define UI_OPPONENTLOGO_METAL (6666 + 19)  //544
#define UI_PLAYERLOGO_NAME (6666 + 20)  //545
#define UI_OPPONENTLOGO_NAME (6666 + 21)  //546

#define UI_TIER_MAPNAME 526
#define UI_TIER_GAMETYPE 527
#define UI_ALLMAPS_SELECTION 528
#define UI_OPPONENT_NAME 529
#define UI_VOTE_KICK 530
#define UI_BOTNAME 531
#define UI_BOTSKILL 532
#define UI_REDBLUE 533
#define UI_CROSSHAIR 534
#define UI_SELECTEDPLAYER 535
#define UI_MAPCINEMATIC 536
#define UI_NETGAMETYPE 537
#define UI_NETMAPCINEMATIC 538
#define UI_SERVERREFRESHDATE 539
#define UI_SERVERMOTD 540
#define UI_GLINFO 541
#define UI_KEYBINDSTATUS 542
#define UI_CLANCINEMATIC 543
#define UI_MAP_TIMETOBEAT 544
#define UI_JOINGAMETYPE 545
#define UI_PREVIEWCINEMATIC 546
#define UI_STARTMAPCINEMATIC 547
#define UI_MAPS_SELECTION 548
#define UI_ADVERT 549
#define UI_CROSSHAIR_COLOR 550
#define UI_NEXTMAP 551
#define UI_VOTESTRING 552
#define UI_TEAMPLAYERMODEL 553
#define UI_ENEMYPLAYERMODEL 554
#define UI_REDTEAMMODEL 555
#define UI_BLUETEAMMODEL 556
#define UI_SERVER_OWNER 557
#define UI_SERVER_SETTINGS 558
#define UI_STARTING_WEAPONS 559

#define VOICECHAT_GETFLAG			"getflag"				// command someone to get the flag
#define VOICECHAT_OFFENSE			"offense"				// command someone to go on offense
#define VOICECHAT_DEFEND			"defend"				// command someone to go on defense
#define VOICECHAT_DEFENDFLAG		"defendflag"			// command someone to defend the flag
#define VOICECHAT_PATROL			"patrol"				// command someone to go on patrol (roam)
#define VOICECHAT_CAMP				"camp"					// command someone to camp (we don't have sounds for this one)
#define VOICECHAT_FOLLOWME			"followme"				// command someone to follow you
#define VOICECHAT_RETURNFLAG		"returnflag"			// command someone to return our flag
#define VOICECHAT_FOLLOWFLAGCARRIER	"followflagcarrier"		// command someone to follow the flag carrier
#define VOICECHAT_YES				"yes"					// yes, affirmative, etc.
#define VOICECHAT_NO				"no"					// no, negative, etc.
#define VOICECHAT_ONGETFLAG			"ongetflag"				// I'm getting the flag
#define VOICECHAT_ONOFFENSE			"onoffense"				// I'm on offense
#define VOICECHAT_ONDEFENSE			"ondefense"				// I'm on defense
#define VOICECHAT_ONPATROL			"onpatrol"				// I'm on patrol (roaming)
#define VOICECHAT_ONCAMPING			"oncamp"				// I'm camping somewhere
#define VOICECHAT_ONFOLLOW			"onfollow"				// I'm following
#define VOICECHAT_ONFOLLOWCARRIER	"onfollowcarrier"		// I'm following the flag carrier
#define VOICECHAT_ONRETURNFLAG		"onreturnflag"			// I'm returning our flag
#define VOICECHAT_INPOSITION		"inposition"			// I'm in position
#define VOICECHAT_IHAVEFLAG			"ihaveflag"				// I have the flag
#define VOICECHAT_BASEATTACK		"baseattack"			// the base is under attack
#define VOICECHAT_ENEMYHASFLAG		"enemyhasflag"			// the enemy has our flag (CTF)
#define VOICECHAT_STARTLEADER		"startleader"			// I'm the leader
#define VOICECHAT_STOPLEADER		"stopleader"			// I resign leadership
#define VOICECHAT_TRASH				"trash"					// lots of trash talk
#define VOICECHAT_WHOISLEADER		"whoisleader"			// who is the team leader
#define VOICECHAT_WANTONDEFENSE		"wantondefense"			// I want to be on defense
#define VOICECHAT_WANTONOFFENSE		"wantonoffense"			// I want to be on offense
#define VOICECHAT_KILLINSULT		"kill_insult"			// I just killed you
#define VOICECHAT_TAUNT				"taunt"					// I want to taunt you
#define VOICECHAT_DEATHINSULT		"death_insult"			// you just killed me
#define VOICECHAT_KILLGAUNTLET		"kill_gauntlet"			// I just killed you with the gauntlet
#define VOICECHAT_PRAISE			"praise"				// you did something good
