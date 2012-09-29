#include "cg_local.h"
#include "../client/keycodes.h"
#include "wolfcam_local.h"

//FIXME
extern void CG_ScoresUp_f (void);
extern void CG_ScoresDown_f (void);

void Wolfcam_DemoKeyClick (int key, qboolean down)
{
    int t;
    char binding[MAX_STRING_CHARS];
    char altBinding[MAX_STRING_CHARS];
    qboolean downPrev;
    char *s;
    char token[MAX_TOKEN_CHARS];
    qboolean newLine;
    //int timing;

    t = trap_Milliseconds();

    // Avoid active console keypress issues
    if (!down  &&  !cgs.fKeyPressed[key]) {
        memset(&cgs.fKeyRepeat, 0, sizeof(cgs.fKeyRepeat));
        memset(&cgs.fKeyPressedLastTime, 0, sizeof(cgs.fKeyPressedLastTime));
        return;
    }

    trap_Key_GetBinding(key, binding);
    if (SC_Cvar_Get_Int("debug_demokeyclick")) {
        CG_Printf("(%d)  key %d:  %s\n", t, key, binding);
    }

    downPrev = cgs.fKeyPressed[key];
    cgs.fKeyPressed[key] = down;


    if (!strcmp(binding, "+forward")) {
        if (down) {
            cg.keyf = 1;
        } else {
            cg.keyf = 0;
        }
    } else if (!strcmp(binding, "+back")) {
        if (down) {
            cg.keyb = 1;
        } else {
            cg.keyb = 0;
        }
    } else if (!strcmp(binding, "+moveleft")) {
        if (down) {
            cg.keyl = 1;
        } else {
            cg.keyl = 0;
        }
    } else if (!strcmp(binding, "+moveright")) {
        if (down) {
            cg.keyr = 1;
        } else {
            cg.keyr = 0;
        }
    } else if (!strcmp(binding, "+moveup")) {
        if (down) {
            cg.keyu = 1;
        } else {
            cg.keyu = 0;
        }
    } else if (!strcmp(binding, "+movedown")) {
        if (down) {
            cg.keyd = 1;
        } else {
            cg.keyd = 0;
        }
    } else if (!strcmp(binding, "+attack")) {
        if (down) {
            cg.keya = 1;
        } else {
            cg.keya = 0;
        }
    } else if (!strcmp(binding, "+rollright")) {
        if (down) {
            cg.keyrollright = 1;
        } else {
            cg.keyrollright = 0;
        }
    } else if (!strcmp(binding, "+rollleft")) {
        if (down) {
            cg.keyrollleft = 1;
        } else {
            cg.keyrollleft = 0;
        }
    } else if (!strcmp(binding, "+rollstopzero")) {
        if (down) {
            cg.keyrollstopzero = 1;
        } else {
            cg.keyrollstopzero = 0;
        }
    } else if (0) {  //(!strcmp(binding, "+scores")) {
        if (down) {
            CG_ScoresDown_f();
            cg.showDemoScores = qtrue;
        } else {
            CG_ScoresUp_f();
            cg.showDemoScores = qfalse;
        }
    } else if (0) {  //(!strcmp(binding, "+zoom")) {
        if (down) {
            CG_ZoomDown_f();
        } else {
            CG_ZoomUp_f();
        }
    } else if (!Q_stricmpn(binding, "+vstr", strlen("+vstr") - 1)) {
        //Com_Printf("+vstr '%s'\n", binding);
        if (down  &&  !downPrev) {
            //CG_GetToken (char *inputString, char *token, qboolean isFilename, qboolean *newLine);
            s = binding + strlen("+vstr");
            s = CG_GetToken(s, token, qfalse, &newLine);
            if (*token) {
                trap_SendConsoleCommand(va("vstr %s\n", token));
            }
        } else if (!down  &&  downPrev) {
            s = binding + strlen("+vstr");
            s = CG_GetToken(s, token, qfalse, &newLine);
            s = CG_GetToken(s, token, qfalse, &newLine);
            if (*token) {
                trap_SendConsoleCommand(va("vstr %s\n", token));
            }
        }
    } else if (!Q_stricmpn(binding, "++vstr", strlen("++vstr") - 1)) {
        if (down) {
#if 0
            s = binding + strlen("++vstr");
            s = CG_GetToken(s, token, qfalse, &newLine);  // timing
            timing = atoi(token);
            if (t - cgs.fKeyPressedLastTime[key] >= timing) {
                s = CG_GetToken(s, token, qfalse, &newLine);
                if (*token) {
                    trap_SendConsoleCommand(va("vstr %s\n", token));
                }
                cgs.fKeyPressedLastTime[key] = t;
                Com_Printf("yes %d  key[%d]\n", t, key);
            } else {
                Com_Printf("no %d - %d < %d key[%d]\n", t, cgs.fKeyPressedLastTime[key], timing, key);
            }
#endif
            // can't do it here since it depends on operating system's repeat
            // settings
            cgs.fKeyRepeat[key] = qtrue;
        } else if (!down  &&  downPrev) {
            s = binding + strlen("++vstr");
            s = CG_GetToken(s, token, qfalse, &newLine);  // timing
            s = CG_GetToken(s, token, qfalse, &newLine);
            s = CG_GetToken(s, token, qfalse, &newLine);
            if (*token) {
                trap_SendConsoleCommand(va("vstr %s\n", token));
            }
            cgs.fKeyPressedLastTime[key] = 0;
            cgs.fKeyRepeat[key] = qfalse;
        }
    } else {
        if (*binding) {
            if (binding[0] == '+') {
                if (down) {
                    //Com_Printf("^3+  %s\n", binding);
                    trap_SendConsoleCommand(va("%s\n", binding));
                } else {
                    Q_strncpyz(altBinding, binding, sizeof(binding));
                    altBinding[0] = '-';
                    //Com_Printf("^3-  %s\n", binding);
                    trap_SendConsoleCommand(va("%s\n", altBinding));
                }
            } else if (down  &&  !downPrev) {
                trap_SendConsoleCommand(va("%s\n", binding));
            }
        }
    }
}
