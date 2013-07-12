#include "cg_local.h"

#include "cg_main.h"
#include "cg_predict.h"
#include "cg_spawn.h"
#include "cg_syscalls.h"

#include "sc.h"

/*
====================
CG_AddSpawnVarToken
====================
*/
static char *CG_AddSpawnVarToken( const char *string ) {
    int             l;
    char    *dest;

    l = strlen( string );
    if ( cg.numSpawnVarChars + l + 1 > MAX_SPAWN_VARS_CHARS ) {
        CG_Error( "CG_AddSpawnVarToken: MAX_SPAWN_VARS_CHARS" );
    }

    dest = cg.spawnVarChars + cg.numSpawnVarChars;
    memcpy( dest, string, l+1 );

    cg.numSpawnVarChars += l + 1;

    return dest;
}

typedef enum {
    megaHealth,
    redArmor,
    yellowArmor,
    greenArmor,
    quad,
    battleSuit,
    noitem,
} titem_t;

static void AddTimedItem(int *num, timedItem_t *tlist, const vec3_t origin, int respawnLength)
{
    int i, j, k;
    int place;

    if (*num >= MAX_TIMED_ITEMS) {
        Com_Printf("FIXME %d >= MAX_TIMED_ITEMS\n", *num);
        return;
    }

    if (cg.maxItemRespawnTime < respawnLength) {
        cg.maxItemRespawnTime = respawnLength;
    }

    if (*num == 0) {
        place = 0;
        goto foundPlacement;
    }

    place = 0;

    for (i = 0;  i < *num;  i++) {
        if (origin[2] > tlist[i].origin[2]) {
            place = i;
            goto foundPlacement;
        } else if (origin[2] == tlist[i].origin[2]) {
            //Com_Printf("............ sorting by y\n");
            for (j = i;  j < *num;  j++) {
                if (origin[1] > tlist[i].origin[1]) {
                    place = j;
                    goto foundPlacement;
                } else if (origin[1] == tlist[i].origin[1]) {
                    //Com_Printf("....... sorting by x\n");
                    for (k = 0;  k < *num;  k++) {
                        if (origin[0] > tlist[i].origin[0]) {
                            place = k;
                            goto foundPlacement;
                        }
                    }
                }
            }
        }
    }

    if (place == 0)
        place = *num;

 foundPlacement:

    if (*num > 0) {
        for (i = *num;  i >= place;  i--) {
            VectorCopy(tlist[i].origin, tlist[i + 1].origin);
            tlist[i + 1].respawnLength = tlist[i].respawnLength;
        }
    }

    VectorCopy(origin, tlist[place].origin);
    tlist[place].respawnLength = respawnLength;
    (*num)++;

#if 0
    Com_Printf("-----------\n");
    for (i = 0;  i < *num;  i++) {
        Com_Printf(" %d (%f, %f, %f),", i, tlist[i].origin[0], tlist[i].origin[1], tlist[i].origin[2]);
    }
    Com_Printf("\n");
#endif

    return;
}

/*
====================
CG_ParseSpawnVars

Parses a brace bounded set of key / value pairs out of the
level's entity strings into cg.spawnVars[]

This does not actually spawn an entity.
====================
*/
qboolean CG_ParseSpawnVars( void ) {
    char            keyname[MAX_TOKEN_CHARS];
    char            com_token[MAX_TOKEN_CHARS];
    char    token[MAX_TOKEN_CHARS];
    //char buf[MAX_TOKEN_CHARS];
    const char *line;
    qboolean newLine;
    vec3_t origin;
    int wait;
    titem_t ti;
    int val;
    int skipItem;
    int i;
    //char *gametypeName;
    //char *value;
    int angle;
    int spawnflags;
    qboolean spawnPoint;
    qboolean redSpawn;
    qboolean blueSpawn;
    qboolean initial;
    qboolean deathmatch;
    qboolean portal;
    qboolean portalHasTarget;
    qboolean startTimer;
    qboolean stopTimer;
    qboolean gotOrigin;
    int pointContents;
    static char *gametypeNames[] = { "ffa", "tournament", "single", "team", /*FIXME clanarena*/ "ca",  "ctf", "oneflag", "obelisk", "harvester", "ft", "dom", "ad", "rr" };

    //Com_Printf("cgs.gametype : %d\n", cgs.gametype);
    cg.numSpawnVars = 0;
    cg.numSpawnVarChars = 0;

    // parse the opening brace
    if ( !trap_GetEntityToken( com_token, sizeof( com_token ) ) ) {
        // end of spawn string
        return qfalse;
    }
    if ( com_token[0] != '{' ) {
        CG_Error( "CG_ParseSpawnVars: found %s when expecting {",com_token );
    }

    // go through all the key / value pairs
    while ( 1 ) {
        // parse key
        if ( !trap_GetEntityToken( keyname, sizeof( keyname ) ) ) {
            CG_Error( "CG_ParseSpawnVars: EOF without closing brace" );
        }

        if ( keyname[0] == '}' ) {
            break;
        }

        // parse value
        if ( !trap_GetEntityToken( com_token, sizeof( com_token ) ) ) {
            CG_Error( "CG_ParseSpawnVars: EOF without closing brace" );
        }

        if ( com_token[0] == '}' ) {
            CG_Error( "CG_ParseSpawnVars: closing brace without data" );
        }
        if ( cg.numSpawnVars == MAX_SPAWN_VARS ) {
            CG_Error( "CG_ParseSpawnVars: MAX_SPAWN_VARS" );
        }
        cg.spawnVars[ cg.numSpawnVars ][0] = CG_AddSpawnVarToken( keyname );
        cg.spawnVars[ cg.numSpawnVars ][1] = CG_AddSpawnVarToken( com_token );
        cg.numSpawnVars++;

        if (!Q_stricmp(keyname, "enableDust")) {
            cg.mapEnableDust = atoi(com_token);
        } else if (!Q_stricmp(keyname, "enableBreath")) {
            cg.mapEnableBreath = atoi(com_token);
        }
        //CG_Printf("spawnvars  %s : %s\n", cg.spawnVars[ cg.numSpawnVars - 1 ][0], cg.spawnVars[ cg.numSpawnVars - 1 ][1]);
    }

    if (cgs.gametype == GT_CA) {
        //return qtrue;
    }

    ti = noitem;
    wait = 0;
    skipItem = 0;
    angle = 0;
    spawnflags = 0;
    spawnPoint = qfalse;
    redSpawn = qfalse;
    blueSpawn = qfalse;
    initial = qfalse;
    deathmatch = qfalse;
    gotOrigin = qfalse;

    // get the rest
    while (1) {
        // parse key
        if ( !trap_GetEntityToken( keyname, sizeof( keyname ) ) ) {
            //CG_Printf("all done parsing ents\n");
            break;
        }

        //Com_Printf("'%s'\n", keyname);

        if (keyname[0] == '{') {
            //CG_Printf("{\n");
            // clear values
            ti = noitem;
            wait = 0;
            skipItem = 0;
            angle = 0;
            spawnflags = 0;
            spawnPoint = qfalse;
            redSpawn = qfalse;
            blueSpawn = qfalse;
            initial = qfalse;
            deathmatch = qfalse;
            portal = qfalse;
            portalHasTarget = qfalse;
            startTimer = qfalse;
            stopTimer = qfalse;
            gotOrigin = qfalse;
            continue;
        }
        if ( keyname[0] == '}' ) {
            //CG_Printf("}\n");
            if (spawnPoint) {
                //Com_Printf("spawn: %f %f %f   angle:%d  spawnflags:%d  r:%d b:%d\n", origin[0], origin[1], origin[2], angle, spawnflags, redSpawn, blueSpawn);
                i = cg.numSpawnPoints;
                if (i >= MAX_SPAWN_POINTS) {
                    continue;
                }

                VectorCopy(origin, cg.spawnPoints[i].origin);
                cg.spawnPoints[i].angle = angle;
                cg.spawnPoints[i].spawnflags = spawnflags;
                cg.spawnPoints[i].redSpawn = redSpawn;
                cg.spawnPoints[i].blueSpawn = blueSpawn;
                cg.spawnPoints[i].initial = initial;
                cg.spawnPoints[i].deathmatch = deathmatch;
                cg.numSpawnPoints++;
                continue;
            }
            if (portal) {
                if (!portalHasTarget) {
                    if (cg.numMirrorSurfaces > MAX_MIRROR_SURFACES) {
                        Com_Printf("^3max mirror surfaces (%d)\n", MAX_MIRROR_SURFACES);
                        continue;
                    }
                    VectorCopy(origin, cg.mirrorSurfaces[cg.numMirrorSurfaces]);
                    cg.numMirrorSurfaces++;
                    //Com_Printf("origin: %f %f %f\n", origin[0], origin[1], origin[2]);
                }
                //Com_Printf("^1origin: %f %f %f  (hasTarget %d)\n", origin[0], origin[1], origin[2], portalHasTarget);
                continue;
            }
            if (startTimer) {
                cg.hasStartTimer = qtrue;
                VectorCopy(origin, cg.startTimerOrigin);
                Com_Printf("^5start timer %f %f %f\n", origin[0], origin[1], origin[2]);
                continue;
            }
            if (stopTimer) {
                cg.hasStopTimer = qtrue;
                VectorCopy(origin, cg.stopTimerOrigin);
                Com_Printf("^5stop timer %f %f %f\n", origin[0], origin[1], origin[2]);
                continue;
            }

            if (gotOrigin) {
                pointContents = CG_PointContents(origin, 0);
            } else {
                pointContents = 0;
            }

            if (skipItem) {
                continue;
            } else if (pointContents & 0x1) {
                // it's in an invalid place, some maps have this  qzdm20 has 2 megas
                continue;
            } else if (ti == redArmor) {
                if (!wait)
                    wait = 25;
                AddTimedItem(&cg.numRedArmors, cg.redArmors, origin, wait);
            } else if (ti == yellowArmor) {
                if (!wait)
                    wait = 25;
                AddTimedItem(&cg.numYellowArmors, cg.yellowArmors, origin, wait);
            } else if (ti == greenArmor) {
                if (!wait)
                    wait = 25;
                AddTimedItem(&cg.numGreenArmors, cg.greenArmors, origin, wait);
            } else if (ti == megaHealth) {
                if (!wait)
                    wait = 35;
                AddTimedItem(&cg.numMegaHealths, cg.megaHealths, origin, wait);
            } else if (ti == quad) {
                if (!wait)
                    wait = 120;
                AddTimedItem(&cg.numQuads, cg.quads, origin, wait);
            } else if (ti == battleSuit) {
                if (!wait)
                    wait = 120;
                AddTimedItem(&cg.numBattleSuits, cg.battleSuits, origin, wait);
            }

            continue;
        }

        // parse value
        if ( !trap_GetEntityToken( com_token, sizeof( com_token ) ) ) {
            CG_Error( "CG_ParseSpawnVars: EOF without closing brace" );
        }

        if ( com_token[0] == '}' ) {
            CG_Error( "CG_ParseSpawnVars: closing brace without data" );
        }

        //CG_Printf("^3ent  %s : %s\n", keyname, com_token);

        if (!Q_stricmp(keyname, "classname")) {
            if (!Q_stricmp(com_token, "info_player_deathmatch")  ||  !Q_stricmp(com_token, "info_player_start")) {
                //FIXME spawnflags 4
                spawnPoint = qtrue;
                deathmatch = qtrue;
                blueSpawn = qtrue;
                redSpawn = qtrue;
                //Com_Printf("deathmatch spawn %d\n", cg.numSpawnPoints);
            } else if (!Q_stricmp(com_token, "team_CTF_redspawn")) {
                spawnPoint = qtrue;
                redSpawn = qtrue;
            } else if (!Q_stricmp(com_token, "team_CTF_bluespawn")) {
                spawnPoint = qtrue;
                blueSpawn = qtrue;
            } else if (!Q_stricmp(com_token, "team_CTF_redplayer")) {
                spawnPoint = qtrue;
                redSpawn = qtrue;
                initial = qtrue;
            } else if (!Q_stricmp(com_token, "team_CTF_blueplayer")) {
                spawnPoint = qtrue;
                blueSpawn = qtrue;
                initial = qtrue;
            } else if (!Q_stricmp(com_token, "item_armor_body")) {
                // red armor
                //Com_Printf("red armor\n");
                ti = redArmor;
            } else if (!Q_stricmp(com_token, "item_armor_combat")) {
                // yellow armor
                //Com_Printf("yellow armor\n");
                ti = yellowArmor;
            } else if (!Q_stricmp(com_token, "item_armor_jacket")) {
                ti = greenArmor;
            } else if (!Q_stricmp(com_token, "item_health_mega")) {
                //Com_Printf("mega health\n");
                ti = megaHealth;
            } else if (!Q_stricmp(com_token, "item_quad")) {
                //Com_Printf("quad\n");
                ti = quad;
            } else if (!Q_stricmp(com_token, "item_enviro")) {
                // battlesuit
                //Com_Printf("battlesuit?\n");
                ti = battleSuit;
                // count ??
            } else if (!Q_stricmp(com_token, "misc_portal_surface")) {
                //Com_Printf("^3portal surface\n");
                portal = qtrue;
            } else if (!Q_stricmp(com_token, "target_starttimer")) {
                startTimer = qtrue;
            } else if (!Q_stricmp(com_token, "target_stoptimer")) {
                stopTimer = qtrue;
            } else {
                //FIXME other?  regen?
            }


        } else if (!Q_stricmp(keyname, "origin")) {
            if (SC_ParseVec3FromStr(com_token, origin) == -1) {
                //Com_Printf("FIXME getting origin for entitytoken %s", com_token);
                origin[0] = 0.0;
                origin[1] = 0.0;
                origin[2] = 0.0;
            }
            gotOrigin = qtrue;
            //Com_Printf("*** origin: %f %f %f\n", origin[0], origin[1], origin[2]);
        } else if (!Q_stricmp(keyname, "target")) {
            portalHasTarget = qtrue;
            //Q_strncpyz(buf, com_token, sizeof(buf));
            //Com_Printf("^5target == '%s'\n", buf);
        } else if (!Q_stricmp(keyname, "wait")) {
            wait = atoi(com_token);
        } else if (!Q_stricmp(keyname, "notteam")) {
            val = atoi(com_token);
            if (val  &&  cgs.gametype >= GT_TEAM) {
                skipItem = 1;
            }
        } else if (!Q_stricmp(keyname, "not_gametype")) {
            val = atoi(com_token);
            if (cgs.gametype == val) {
                skipItem = 1;
            }
        } else if (!Q_stricmp(keyname, "gametype")) {
            //FIXME this is wrong, see g_spawn.c
            //val = atoi(com_token);
            //if (val != cgs.gametype) {
            //    skipItem = 1;
            //}
            qboolean found = qfalse;

            skipItem = 1;
            line = com_token;
            while (*line) {
                int ln;
                //Com_Printf("line: '%s'\n", line);
                //FIXME const
                line = (char *)CG_GetTokenGameType(line, token, qfalse, &newLine);
                ln = strlen(token);
                if (ln  &&  *token) {
                    if (token[ln - 1] == ',') {
                        token[ln - 1] = '\0';
                    }
                }
                for (i = 0;  i < (sizeof(gametypeNames) / sizeof(char *));  i++) {
                    if (!Q_stricmp(token, gametypeNames[i])) {
                        found = qtrue;
                        break;
                    }
                }
                if (!found) {
                    Com_Printf("FIXME gametype : '%s'  '%s'\n", com_token, token);
                } else if (!Q_stricmp(token, gametypeNames[cgs.gametype])) {
                    skipItem = 0;
                }
            }
#if 0
            if (!found) {
                Com_Printf("FIXME gametype : %s\n", com_token);
            }

            if (Q_stricmp(com_token, gametypeNames[cgs.gametype])) {
                skipItem = 1;
            }
#endif
        } else if (!Q_stricmp(keyname, "notfree")) {
            val = atoi(com_token);
            if (val  &&  (cgs.gametype == GT_FFA ||  cgs.gametype == GT_TOURNAMENT)) {
                skipItem = 1;
            }
        } else if (!Q_stricmp(keyname, "notsingle")) {
            val = atoi(com_token);
            if (val  &&  cgs.gametype == GT_SINGLE_PLAYER) {
                skipItem = 1;
            }
        } else if (!Q_stricmp(keyname, "angle")) {
            angle = atoi(com_token);
        } else if (!Q_stricmp(keyname, "spawnflags")) {
            spawnflags = atoi(com_token);
        }

            //  count
    }

    return qtrue;
}
