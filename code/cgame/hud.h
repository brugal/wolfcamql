#ifndef HUD_H_INCLUDED
#define HUD_H_INCLUDED

//#include "cg_local.h"
#include "../qcommon/q_shared.h"

typedef struct {
    qboolean valid;
    int type;
    int condition;  //FIXME multiple conditions

    int team;
    int posType;
    //int coord[4];
    int x;
    int y;
    int width;
    int height;
    float scale;
    vec4_t color;
    char text[MAX_STRING_CHARS];
    float adjust;
    int limit;
    int textType;
    int textStyle;
    fontInfo_t *font;
    qhandle_t shader;
    qhandle_t model;

} hudItem_t;

enum {
    HI_CROSSHAIR,
    HI_FOLLOWING,
    HI_WOLFCAM_FOLLOWING,
    HI_CROSSHAIRNAMES,
    HI_AMMOWARNING,
    //HI_PROXWARNING,
    HI_WEAPONSELECT,
    HI_HOLDABLEITEM,
    HI_POWERUP,  // ?
    HI_REWARD,
    HI_VOTE,
    HI_TEAMVOTE,
    HI_LAGOMETER,
    HI_WARMUP,
    //FIXME scoreboard
    HI_CENTERSTRING,
    HI_TEAMBACKGROUND,
    HI_STATUSBARHEAD,
    HI_STATUSBARFLAG,
    HI_HEALTH,  // strecth on damage
    HI_ARMOR,
    HI_TEAMOVERLAY,
    HI_SNAPSHOT,
    HI_FPS,
    HI_TIMER,
    HI_SPEED,
    HI_ATTACKER,
    HI_CLIENTITEMTIMER,
    HI_ORIGIN,
    HI_SCORES,
    HI_PLAYERSLEFT,
    HI_POWERUPS,
    HI_PICKUPITEM,

    HI_TEXT,
    HI_IMAGE,  //done
    //HI_MODEL,  //FIXME angles origin, etc
};

enum {
    POS_FIXED,
    POS_UPPERRIGHT,
    POS_LOWERRIGHT,
    POS_UPPERLEFT,
    POS_LOWERLEFT,
};


#endif  // HUD_H_INCLUDED
