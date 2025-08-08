// Copyright (C) 1999-2000 Id Software, Inc.
//
// cg_draw.c -- draw all of the graphical elements during
// active (after loading) gameplay

#include "../qcommon/q_shared.h"
#include "cg_local.h"

#include "cg_draw.h"
#include "cg_freeze.h"
#include "cg_info.h"
#include "cg_newdraw.h"
#include "cg_drawtools.h"
#include "cg_localents.h"
#include "cg_main.h"
#include "cg_players.h"  // color from string
#include "cg_predict.h"
#include "cg_q3mme_demos_camera.h"
#include "cg_scoreboard.h"
#include "cg_sound.h"
#include "cg_syscalls.h"
#include "cg_view.h"
#include "cg_weapons.h"
#include "sc.h"
#include "wolfcam_main.h"
#include "wolfcam_predict.h"

#include "wolfcam_local.h"

#if 1  //def MPACK
#include "../ui/ui_shared.h"

// used for scoreboard

#else

#endif

int drawTeamOverlayModificationCount = -1;

int sortedTeamPlayers[TEAM_MAXOVERLAY];
int	numSortedTeamPlayers;

char systemChat[256];
char teamChat1[256];
char teamChat2[256];


static void CG_DrawEchoPopup (void);
static void CG_DrawErrorPopup (void);
static void CG_DrawAccStats (void);
static void CG_DrawFade (void);

//FIXME float for SP_x, *SP_y, and SP_h
static int SP_x;
static int *SP_y;
static int SP_h;
static int SP_align;
static float SP_scale;
static vec4_t SP_color;
static int SP_style;
static const fontInfo_t *SP_font;
static int SP_count;  //FIXME hack
static vec4_t SP_selectedColor;
static vec4_t SP_backgroundColor;
static qboolean SP_drawSelected = qtrue;

static void CG_SPrintInit (int x, int *y, int h, int align, float scale, const vec4_t color, int style, const fontInfo_t *font, qboolean drawSelected)
{
	SP_x = x;
	SP_y = y;
	SP_h = h;
	SP_align = align;
	SP_scale = scale;
	//SP_color = color;
	Vector4Copy(color, SP_color);
	//Vector4Set(SP_selectedColor, 1, 0.35, 0.35, 1);
	SC_Vec4ColorFromCvars(SP_selectedColor, &cg_drawCameraPointInfoSelectedColor, &cg_drawCameraPointInfoAlpha);
	SC_Vec4ColorFromCvars(SP_backgroundColor, &cg_drawCameraPointInfoBackgroundColor, &cg_drawCameraPointInfoBackgroundAlpha);
	SP_style = style;
	SP_font = font;
	SP_count = 0;

	if (drawSelected) {
		SP_drawSelected = qtrue;
	} else {
		SP_drawSelected = qfalse;
	}
}

static void CG_SPrint (const char *s)
{
	float x;
	float w;

	x = SP_x;
	w = CG_Text_Width(s, SP_scale, 0, SP_font);
	if (SP_align == 1) {
		x -= w / 2;
	} else if (SP_align == 2) {
		x -= w;
	}

	CG_FillRect(x, *SP_y, w, SP_h, SP_backgroundColor);

	if (SP_count == cg.selectedCameraPointField  &&  SP_drawSelected) {
		CG_Text_Paint_Bottom(x, *SP_y, SP_scale, SP_selectedColor, s, 0, 0, SP_style, SP_font);
	} else {
		CG_Text_Paint_Bottom(x, *SP_y, SP_scale, SP_color, s, 0, 0, SP_style, SP_font);
	}

	*SP_y += SP_h;
	SP_count++;
}

static void CG_DrawQ3mmeCameraPointInfo (void);

static void CG_DrawCameraPointInfo (void)
{
	vec4_t color;
	int x, y;
	float h;
	int style, align;
	float scale;
	const fontInfo_t *font;
	const char *s;
	const cameraPoint_t *cp;
	const cameraPoint_t *cpOriginNext;
	const cameraPoint_t *cpOriginPrev;
	const cameraPoint_t *cpAnglesNext;
	const cameraPoint_t *cpAnglesPrev;
	const cameraPoint_t *cpFovNext;
	const cameraPoint_t *cpFovPrev;
	char buf[32];
	qboolean badOrigin;
	qboolean badAngles;
	int currentPoint;

	if (!cg_drawCameraPointInfo.integer) {
		return;
	}

	if (cg_drawCameraPointInfo.integer == 3) {
		CG_DrawQ3mmeCameraPointInfo();
		return;
	}

	if (cg.numCameraPoints < 1) {
		// 1: draw either if present
		if (cg_drawCameraPointInfo.integer == 1) {
			CG_DrawQ3mmeCameraPointInfo();
		}
		return;
	}

	SC_Vec4ColorFromCvars(color, &cg_drawCameraPointInfoColor, &cg_drawCameraPointInfoAlpha);

	if (color[3] <= 0.0) {
		return;
	}

	QLWideScreen = WIDESCREEN_LEFT;

	align = cg_drawCameraPointInfoAlign.integer;
	scale = cg_drawCameraPointInfoScale.value;
	style = cg_drawCameraPointInfoStyle.integer;
	if (*cg_drawCameraPointInfoFont.string) {
		font = &cgs.media.cameraPointInfoFont;
	} else {
		font = &cgDC.Assets.textFont;
	}

	x = cg_drawCameraPointInfoX.integer;
	y = cg_drawCameraPointInfoY.integer;

	//FIXME store max height/point size
	//h = CG_Text_Height("1!IPUTY|", scale, 0, font);
	h = CG_Text_Height("1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ~`!@#$%^&*()-_=+[{]}\\|;:'\",./?", scale, 0, font);

	cp = &cg.cameraPoints[cg.selectedCameraPointMin];

	cpOriginNext = cp->next;
	while (cpOriginNext  &&  !(cpOriginNext->flags & CAM_ORIGIN)) {
		cpOriginNext = cpOriginNext->next;
	}
	cpOriginPrev = cp->prev;
	while (cpOriginPrev  &&  !(cpOriginPrev->flags & CAM_ORIGIN)) {
		cpOriginPrev = cpOriginPrev->prev;
	}

	cpAnglesNext = cp->next;
	while (cpAnglesNext  &&  !(cpAnglesNext->flags & CAM_ANGLES)) {
		cpAnglesNext = cpAnglesNext->next;
	}
	cpAnglesPrev = cp->prev;
	while (cpAnglesPrev  &&  !(cpAnglesPrev->flags & CAM_ANGLES)) {
		cpAnglesPrev = cpAnglesPrev->prev;
	}

	cpFovNext = cp->next;
	while (cpFovNext  &&  !(cpFovNext->flags & CAM_FOV)) {
		cpFovNext = cpFovNext->next;
	}
	cpFovPrev = cp->prev;
	while (cpFovPrev  &&  !(cpFovPrev->flags & CAM_FOV)) {
		cpFovPrev = cpFovPrev->prev;
	}

	CG_SPrintInit(x, &y, h, align, scale, color, style, font, qtrue);

	if (cg.selectedCameraPointMin == cg.selectedCameraPointMax) {
		Com_sprintf(buf, sizeof(buf), "  :  ^3%d", cg.selectedCameraPointMin);
	} else {
		Com_sprintf(buf, sizeof(buf), "  :  ^3%d - %d", cg.selectedCameraPointMin, cg.selectedCameraPointMax);
	}

	if (cp->type == CAMERA_CURVE  &&  cp->flags & CAM_ORIGIN) {
		if (cp->curveCount % 2 == 0) {
			CG_SPrint(va("camera point %d/%d (start/end curve)%s", cg.selectedCameraPointMin, cg.numCameraPoints - 1, buf));
		} else {
			CG_SPrint(va("camera point %d/%d (mid point of curve)%s", cg.selectedCameraPointMin, cg.numCameraPoints - 1, buf));
		}
	} else {
		CG_SPrint(va("camera point %d/%d%s", cg.selectedCameraPointMin, cg.numCameraPoints - 1, buf));
	}

	// find current camera point
	//FIXME epsilon
	if (cg.ftime < cg.cameraPoints[0].cgtime) {
		currentPoint = -1;
	} else if (cg.ftime > cg.cameraPoints[cg.numCameraPoints - 1].cgtime) {
		currentPoint = cg.numCameraPoints;
	} else {  // within a camera point
		for (currentPoint = 0;  currentPoint < cg.numCameraPoints;  currentPoint++) {
			if (cg.ftime < cg.cameraPoints[currentPoint].cgtime) {
				currentPoint--;
				break;
			}
		}
	}

	// current camera point
	if (currentPoint < 0) {
		CG_SPrint(va("current time: %f ^5[ - 0]", cg.ftime));
	} else if (currentPoint >= cg.numCameraPoints) {
		CG_SPrint(va("current time: %f ^5[%d +]", cg.ftime, cg.numCameraPoints - 1));
	} else {  // within camera point
		CG_SPrint(va("current time: %f ^5[%d - %d]", cg.ftime, currentPoint, currentPoint + 1));
	}

	if (cp->next) {
		CG_SPrint(va("camera time: %f  %f sec", cp->cgtime, (cp->next->cgtime - cp->cgtime) / 1000.0));
	} else {
		CG_SPrint(va("camera time: %f", cp->cgtime));
	}

	badOrigin = qfalse;
	badAngles = qfalse;

	if (cpOriginPrev  &&  cp->flags & CAM_ORIGIN  &&  cpOriginNext) {
		if ((cp->originImmediateInitialVelocity * cg_cameraSmoothFactor.value) < cpOriginPrev->originImmediateFinalVelocity) {
			badOrigin = qtrue;
		}
	}

	if (cpAnglesPrev  &&  cp->flags & CAM_ANGLES  &&  cpAnglesNext) {
		if ((cp->anglesImmediateInitialVelocity * cg_cameraSmoothFactor.value) < cpAnglesPrev->anglesImmediateFinalVelocity) {
			badAngles = qtrue;
		}
	}

	if (badOrigin) {
		s = va("origin: %d  %d  %d  ^1(not smooth)", (int)cp->origin[0], (int)cp->origin[1], (int)cp->origin[2]);
	} else {
		s = va("origin: %d  %d  %d", (int)cp->origin[0], (int)cp->origin[1], (int)cp->origin[2]);
	}
	CG_SPrint(s);

	if (badAngles) {
		s = va("angles: %d  %d  %d  ^1(not smooth)", (int)cp->angles[0], (int)cp->angles[1], (int)cp->angles[2]);
	} else {
		s = va("angles: %d  %d  %d", (int)cp->angles[0], (int)cp->angles[1], (int)cp->angles[2]);
	}
	CG_SPrint(s);

	if (cp->type == CAMERA_SPLINE) {
		s = va("camera type: spline");
	} else if (cp->type == CAMERA_INTERP) {
		s = va("camera type: interp");
	} else if (cp->type == CAMERA_JUMP) {
		s = va("camera type: jump");
	} else if (cp->type == CAMERA_CURVE) {
		s = va("camera type: curve");
	} else if (cp->type == CAMERA_SPLINE_BEZIER) {
		s = va("camera type: splineBezier");
	} else if (cp->type == CAMERA_SPLINE_CATMULLROM) {
		s = va("camera type: splineCatmullRom");
	} else {
		s = va("camera type: ???");
	}
	CG_SPrint(s);

	if (cp->viewType == CAMERA_ANGLES_INTERP) {
		s = va("angles type: interp");
	} else if (cp->viewType == CAMERA_ANGLES_SPLINE) {
		s = va("angles type: spline");
	} else if (cp->viewType == CAMERA_ANGLES_ENT) {
		s = va("angles type: view entity");
	} else if (cp->viewType == CAMERA_ANGLES_VIEWPOINT_INTERP) {
		s = va("angles type: view point interp");  //FIXME too long
	} else if (cp->viewType == CAMERA_ANGLES_INTERP_USE_PREVIOUS) {
		s = va("angles type: interp use prev");
	} else if (cp->viewType == CAMERA_ANGLES_FIXED) {
		s = va("angles type: fixed");
	} else if (cp->viewType == CAMERA_ANGLES_FIXED_USE_PREVIOUS) {
		s = va("angles type: fixed use prev");
	} else if (cp->viewType == CAMERA_ANGLES_VIEWPOINT_PASS) {
		s = va("angles type: view point pass");
	} else if (cp->viewType == CAMERA_ANGLES_VIEWPOINT_FIXED) {
		s = va("angles type: view point fixed");
	} else {
		s = va("angles type: ???");
	}
	CG_SPrint(s);

	//FIXME show disabled for angles type spline
	if (cp->rollType == CAMERA_ROLL_INTERP) {
		s = va("camera roll: interp");
	} else if (cp->rollType == CAMERA_ROLL_FIXED) {
		s = va("camera roll: fixed");
	} else if (cp->rollType == CAMERA_ROLL_PASS) {
		s = va("camera roll: pass");
	} else if (cp->rollType == CAMERA_ROLL_AS_ANGLES) {
		s = va("camera roll: angles");
	} else {
		s = va("camera roll: ???");
	}
	CG_SPrint(s);

	// flags
	CG_SPrint(va("flags: %s%s%s%s", cp->flags & CAM_ORIGIN ? "origin " : "", cp->flags & CAM_ANGLES ? "angles " : "", cp->flags & CAM_FOV ? "fov " : "", cp->flags & CAM_TIME ? "time " : ""));

	CG_SPrint(va("number of splines: %d", cp->numSplines));
	CG_SPrint(va("view point: %d  %d  %d", (int)cp->viewPointOrigin[0], (int)cp->viewPointOrigin[1], (int)cp->viewPointOrigin[2]));

	if (cp->viewEnt < 0) {
		CG_SPrint("view ent:");
	} else {
		CG_SPrint(va("view ent: %d", cp->viewEnt));
	}

	CG_SPrint(va("view ent offsets: %d  %d  %d", (int)cp->xoffset, (int)cp->yoffset, (int)cp->zoffset));

	if (cp->offsetType == CAMERA_OFFSET_INTERP) {
		CG_SPrint("offset type: interp");
	} else if (cp->offsetType == CAMERA_OFFSET_FIXED) {
		CG_SPrint("offset type: fixed");
	} else if (cp->offsetType == CAMERA_OFFSET_PASS) {
		CG_SPrint("offset type: pass");
	} else {
		CG_SPrint("offset type: ???");
	}

	if (cp->fov > 0) {
		CG_SPrint(va("fov: %f", cp->fov));
	} else {
		CG_SPrint("fov:");
	}

	if (cp->fovType == CAMERA_FOV_USE_CURRENT) {
		CG_SPrint("fov type: current");
	} else if (cp->fovType == CAMERA_FOV_INTERP) {
		CG_SPrint("fov type: interp");
	} else if (cp->fovType == CAMERA_FOV_FIXED) {
		CG_SPrint("fov type: fixed");
	} else if (cp->fovType == CAMERA_FOV_PASS) {
		CG_SPrint("fov type: pass");
	} else if (cp->fovType == CAMERA_FOV_SPLINE) {
		CG_SPrint("fov type: spline");
	} else {
		CG_SPrint("fov type:  ???");
	}

	if (cp->flags & CAM_ORIGIN) {
		if (!cp->useOriginVelocity) {
			if (cpOriginNext) {
				CG_SPrint(va("initial velocity: %f(avg) (initial avg not set) ^3%f(imm)", cp->originDistance / (cpOriginNext->cgtime - cp->cgtime) * 1000.0, cp->originImmediateInitialVelocity));
			} else {
				CG_SPrint("initial velocity:");
			}
		} else {
			CG_SPrint(va("initial velocity: %f(avg) %f(initial avg) ^3%f(imm)", cp->originDistance / (cpOriginNext->cgtime - cp->cgtime) * 1000.0, cp->originInitialVelocity, cp->originImmediateInitialVelocity));
		}

		if (!cp->useOriginVelocity) {
			if (cpOriginNext) {
				CG_SPrint(va("final velocity: %f(avg) (final avg not set) %f(imm)", cp->originDistance / (cpOriginNext->cgtime - cp->cgtime) * 1000.0, cp->originImmediateFinalVelocity));
			} else {
				CG_SPrint("final velocity:");
			}
		} else {
			CG_SPrint(va("final velocity: %f(avg) %f(final avg) %f(imm)", cp->originDistance / (cpOriginNext->cgtime - cp->cgtime) * 1000.0, cp->originFinalVelocity, cp->originImmediateFinalVelocity));
		}

		if (cpOriginPrev) {
			if (!cpOriginPrev->useOriginVelocity) {
				CG_SPrint(va("prev final velocity: %f(avg) (final avg not set) ^3%f(imm)", cpOriginPrev->originDistance / (cp->cgtime - cpOriginPrev->cgtime) * 1000.0, cpOriginPrev->originImmediateFinalVelocity));
			} else {
				CG_SPrint(va("prev final velocity: %f(avg) %f(final avg) ^3%f(imm)", cpOriginPrev->originDistance / (cp->cgtime - cpOriginPrev->cgtime) * 1000.0, cpOriginPrev->originFinalVelocity, cpOriginPrev->originImmediateFinalVelocity));
			}
		} else {
			CG_SPrint(va("prev final velocity:"));
		}
	} else {  // !(cp->flags & CAM_ORIGIN)
		CG_SPrint("initial velocity:");
		CG_SPrint("final velocity:");
		CG_SPrint("prev final velocity:");
	}

	// angles

	if (cp->flags & CAM_ANGLES) {
		if (!cp->useAnglesVelocity) {
			if (cpAnglesNext) {
				CG_SPrint(va("angles initial velocity: %f(avg) (initial avg not set) ^6%f(imm)", cp->anglesAvgVelocity, cp->anglesImmediateInitialVelocity));
			} else {
				CG_SPrint("angles initial velocity:");
			}
		} else {
			CG_SPrint(va("angles initial velocity: %f(avg) %f(initial avg) ^6%f(imm)", cp->anglesAvgVelocity, cp->anglesInitialVelocity, cp->anglesImmediateInitialVelocity));
		}

		if (!cp->useAnglesVelocity) {
			if (cpAnglesNext) {
				CG_SPrint(va("angles final velocity: %f(avg) (final avg not set) %f(imm)", cp->anglesAvgVelocity, cp->anglesImmediateFinalVelocity));
			} else {
				CG_SPrint("angles final velocity:");
			}
		} else {
			CG_SPrint(va("angles final velocity: %f(avg) %f(final avg) %f(imm)", cp->anglesAvgVelocity, cp->anglesFinalVelocity, cp->anglesImmediateFinalVelocity));
		}

		if (cpAnglesPrev) {
			if (!cpAnglesPrev->useAnglesVelocity) {
				CG_SPrint(va("angles prev final velocity: %f(avg) (final avg not set) ^6%f(imm)", cpAnglesPrev->anglesAvgVelocity, cpAnglesPrev->anglesImmediateFinalVelocity));
			} else {
				CG_SPrint(va("angles prev final velocity: %f(avg) %f(final avg) ^6%f(imm)", cpAnglesPrev->anglesAvgVelocity, cpAnglesPrev->anglesFinalVelocity, cpAnglesPrev->anglesImmediateFinalVelocity));
			}
		} else {
			CG_SPrint(va("angles prev final velocity:"));
		}
	} else {  // !(cp->flags & CAM_ANGLES)
		CG_SPrint("angles initial velocity:");
		CG_SPrint("angles final velocity:");
		CG_SPrint("angles prev final velocity:");
	}

	// xoffset

	if (cp->flags & CAM_ANGLES) {
		if (!cp->useXoffsetVelocity) {
			if (cpAnglesNext) {
				CG_SPrint(va("xoffset initial velocity: %f", fabs(cpAnglesNext->xoffset - cp->xoffset) / (cpAnglesNext->cgtime - cp->cgtime) * 1000.0));
			} else {
				CG_SPrint("xoffset initial velocity:");
			}
		} else {
			CG_SPrint(va("xoffset initial velocity: %f", cp->xoffsetInitialVelocity));
		}

		if (!cp->useXoffsetVelocity) {
			if (cpAnglesNext) {
				CG_SPrint(va("xoffset final velocity: %f", fabs(cpAnglesNext->xoffset - cp->xoffset) / (cpAnglesNext->cgtime - cp->cgtime) * 1000.0));
			} else {
				CG_SPrint("xoffset final velocity:");
			}
		} else {
			CG_SPrint(va("xoffset final velocity: %f", cp->xoffsetFinalVelocity));
		}

		if (cpAnglesPrev) {
			if (!cpAnglesPrev->useXoffsetVelocity) {
				CG_SPrint(va("xoffset prev final velocity: %f", fabs(cp->xoffset - cpAnglesPrev->xoffset) / (cp->cgtime - cpAnglesPrev->cgtime) * 1000.0));
			} else {
				CG_SPrint(va("xoffset prev final velocity: %f", cpAnglesPrev->xoffsetFinalVelocity));
			}
		} else {
			CG_SPrint(va("xoffset prev final velocity:"));
		}
	} else {  // !(cp->flags & CAM_ANGLES)
		CG_SPrint("xoffset initial velocity:");
		CG_SPrint("xoffset final velocity:");
		CG_SPrint("xoffset prev final velocity:");
	}

	// yoffset

	if (cp->flags & CAM_ANGLES) {
		if (!cp->useYoffsetVelocity) {
			if (cpAnglesNext) {
				CG_SPrint(va("yoffset initial velocity: %f", fabs(cpAnglesNext->yoffset - cp->yoffset) / (cpAnglesNext->cgtime - cp->cgtime) * 1000.0));
			} else {
				CG_SPrint("yoffset initial velocity:");
			}
		} else {
			CG_SPrint(va("yoffset initial velocity: %f", cp->yoffsetInitialVelocity));
		}

		if (!cp->useYoffsetVelocity) {
			if (cpAnglesNext) {
				CG_SPrint(va("yoffset final velocity: %f", fabs(cpAnglesNext->yoffset - cp->yoffset) / (cpAnglesNext->cgtime - cp->cgtime) * 1000.0));
			} else {
				CG_SPrint("yoffset final velocity:");
			}
		} else {
			CG_SPrint(va("yoffset final velocity: %f", cp->yoffsetFinalVelocity));
		}

		if (cpAnglesPrev) {
			if (!cpAnglesPrev->useYoffsetVelocity) {
				CG_SPrint(va("yoffset prev final velocity: %f", fabs(cp->yoffset - cpAnglesPrev->yoffset) / (cp->cgtime - cpAnglesPrev->cgtime) * 1000.0));
			} else {
				CG_SPrint(va("yoffset prev final velocity: %f", cpAnglesPrev->yoffsetFinalVelocity));
			}
		} else {
			CG_SPrint(va("yoffset prev final velocity:"));
		}
	} else {  // !(cp->flags & CAM_ANGLES)
		CG_SPrint("yoffset initial velocity:");
		CG_SPrint("yoffset final velocity:");
		CG_SPrint("yoffset prev final velocity:");
	}

	// zoffset

	if (cp->flags & CAM_ANGLES) {
		if (!cp->useZoffsetVelocity) {
			if (cpAnglesNext) {
				CG_SPrint(va("zoffset initial velocity: %f", fabs(cpAnglesNext->zoffset - cp->zoffset) / (cpAnglesNext->cgtime - cp->cgtime) * 1000.0));
			} else {
				CG_SPrint("zoffset initial velocity:");
			}
		} else {
			CG_SPrint(va("zoffset initial velocity: %f", cp->zoffsetInitialVelocity));
		}

		if (!cp->useZoffsetVelocity) {
			if (cpAnglesNext) {
				CG_SPrint(va("zoffset final velocity: %f", fabs(cpAnglesNext->zoffset - cp->zoffset) / (cpAnglesNext->cgtime - cp->cgtime) * 1000.0));
			} else {
				CG_SPrint("zoffset final velocity:");
			}
		} else {
			CG_SPrint(va("zoffset final velocity: %f", cp->zoffsetFinalVelocity));
		}

		if (cpAnglesPrev) {
			if (!cpAnglesPrev->useZoffsetVelocity) {
				CG_SPrint(va("zoffset prev final velocity: %f", fabs(cp->zoffset - cpAnglesPrev->zoffset) / (cp->cgtime - cpAnglesPrev->cgtime) * 1000.0));
			} else {
				CG_SPrint(va("zoffset prev final velocity: %f", cpAnglesPrev->zoffsetFinalVelocity));
			}
		} else {
			CG_SPrint(va("zoffset prev final velocity:"));
		}
	} else {  // !(cp->flags & CAM_ANGLES)
		CG_SPrint("zoffset initial velocity:");
		CG_SPrint("zoffset final velocity:");
		CG_SPrint("zoffset prev final velocity:");
	}

	// fov
	if (cp->flags & CAM_FOV) {
		if (!cp->useFovVelocity) {
			if (cpFovNext) {
				CG_SPrint(va("fov initial velocity: %f", fabs(cpFovNext->fov - cp->fov) / (cpFovNext->cgtime - cp->cgtime) * 1000.0));
			} else {
				CG_SPrint("fov initial velocity:");
			}
		} else {
			CG_SPrint(va("fov initial velocity: %f", cp->fovInitialVelocity));
		}

		if (!cp->useFovVelocity) {
			if (cpFovNext) {
				CG_SPrint(va("fov final velocity: %f", fabs(cpFovNext->fov - cp->fov) / (cpFovNext->cgtime - cp->cgtime) * 1000.0));
			} else {
				CG_SPrint("fov final velocity:");
			}
		} else {
			CG_SPrint(va("fov final velocity: %f", cp->fovFinalVelocity));
		}

		if (cpFovPrev) {
			if (!cpFovPrev->useFovVelocity) {
				CG_SPrint(va("fov prev final velocity: %f", fabs(cp->fov - cpFovPrev->fov) / (cp->cgtime - cpFovPrev->cgtime) * 1000.0));
			} else {
				CG_SPrint(va("fov prev final velocity: %f", cpFovPrev->fovFinalVelocity));
			}
		} else {
			CG_SPrint(va("fov prev final velocity:"));
		}
	} else {  // !(cp->flags & CAM_FOV)
		CG_SPrint("fov initial velocity:");
		CG_SPrint("fov final velocity:");
		CG_SPrint("fov prev final velocity:");
	}

	// roll

	if (cp->flags & CAM_ANGLES) {
		if (!cp->useRollVelocity) {
			if (cpAnglesNext) {
				CG_SPrint(va("roll initial velocity: %f", fabs(AngleSubtract(cpAnglesNext->angles[ROLL], cp->angles[ROLL])) / (cpAnglesNext->cgtime - cp->cgtime) * 1000.0));
			} else {
				CG_SPrint("roll initial velocity:");
			}
		} else {
			CG_SPrint(va("roll initial velocity: %f", cp->rollInitialVelocity));
		}

		if (!cp->useRollVelocity) {
			if (cpAnglesNext) {
				CG_SPrint(va("roll final velocity: %f", fabs(AngleSubtract(cpAnglesNext->angles[ROLL], cp->angles[ROLL])) / (cpAnglesNext->cgtime - cp->cgtime) * 1000.0));
			} else {
				CG_SPrint("roll final velocity:");
			}
		} else {
			CG_SPrint(va("roll final velocity: %f", cp->rollFinalVelocity));
		}

		if (cpAnglesPrev) {
			if (!cpAnglesPrev->useRollVelocity) {
				CG_SPrint(va("roll prev final velocity: %f", fabs(AngleSubtract(cp->angles[ROLL], cpAnglesPrev->angles[ROLL])) / (cp->cgtime - cpAnglesPrev->cgtime) * 1000.0));
			} else {
				CG_SPrint(va("roll prev final velocity: %f", cpAnglesPrev->rollFinalVelocity));
			}
		} else {
			CG_SPrint(va("roll prev final velocity:"));
		}
	} else {  // !(cp->flags & CAM_ANGLES)
		CG_SPrint("roll initial velocity:");
		CG_SPrint("roll final velocity:");
		CG_SPrint("roll prev final velocity:");
	}

	CG_SPrint(va("command: %s", cp->command));
	CG_SPrint(va("distance: %f", cp->originDistance));
}

static void CG_DrawQ3mmeCameraPointInfo (void)
{
	vec4_t color;
	int x, y;
	float h;
	int style, align;
	float scale;
	const fontInfo_t *font;
	const demoCameraPoint_t *cp;
	const demoCameraPoint_t *currentCameraPoint;
	const demoCameraPoint_t *cameraPointFirst;
	const demoCameraPoint_t *cameraPointLast;
	int numCameraPoints;
	int currentPointNum;
	const char *smoothPos;

	if (!cg_drawCameraPointInfo.integer) {
		return;
	}

	if (!demo.camera.points) {
		return;
	}

	SC_Vec4ColorFromCvars(color, &cg_drawCameraPointInfoColor, &cg_drawCameraPointInfoAlpha);

	if (color[3] <= 0.0) {
		return;
	}

	QLWideScreen = WIDESCREEN_LEFT;

	align = cg_drawCameraPointInfoAlign.integer;
	scale = cg_drawCameraPointInfoScale.value;
	style = cg_drawCameraPointInfoStyle.integer;
	if (*cg_drawCameraPointInfoFont.string) {
		font = &cgs.media.cameraPointInfoFont;
	} else {
		font = &cgDC.Assets.textFont;
	}

	x = cg_drawCameraPointInfoX.integer;
	y = cg_drawCameraPointInfoY.integer;

	//FIXME store max height/point size
	//h = CG_Text_Height("1!IPUTY|", scale, 0, font);
	h = CG_Text_Height("1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ~`!@#$%^&*()-_=+[{]}\\|;:'\",./?", scale, 0, font);

	cameraPointFirst = demo.camera.points;
	cameraPointLast = demo.camera.points;

	numCameraPoints = 1;
	cp = demo.camera.points;
	currentCameraPoint = cp;
	currentPointNum = 0;

	while (cp  &&  cp->next) {
		if (cp->next->time <= cg.time) {
			currentCameraPoint = cp->next;
			currentPointNum = numCameraPoints;
		} else {

		}
		cp = cp->next;
		numCameraPoints++;
	}

	cameraPointLast = cp;

	CG_SPrintInit(x, &y, h, align, scale, color, style, font, qfalse);

	CG_SPrint(va("camera points: %d", numCameraPoints));

	// current camera point
	if (cg.time < cameraPointFirst->time) {
		CG_SPrint(va("current time: %f ^5[ - 0]", cg.ftime));
		cp = cameraPointFirst;
	} else if (cg.time == cameraPointFirst->time) {
		CG_SPrint(va("current time: %f ^5[0]", cg.ftime));
		cp = cameraPointFirst;
	} else if (cg.time > cameraPointLast->time) {
		CG_SPrint(va("current time: %f ^5[%d +]", cg.ftime, numCameraPoints - 1));
		cp = cameraPointLast;
	} else if (cg.time == cameraPointLast->time) {
		CG_SPrint(va("current time: %f ^5[%d]", cg.ftime, numCameraPoints - 1));
		cp = cameraPointLast;
	} else if (cg.time == currentCameraPoint->time) {
		CG_SPrint(va("current time: %f ^5[%d]", cg.ftime, currentPointNum));
		cp = currentCameraPoint;
	} else {  // within camera point
		CG_SPrint(va("current time: %f ^5[%d - %d]", cg.ftime, currentPointNum, currentPointNum + 1));
		cp = currentCameraPoint;
	}

	if (cp->next) {
		CG_SPrint(va("camera time: %d  %f sec", cp->time, (float)(cp->next->time - cp->time) / 1000.0));
	} else {
		CG_SPrint(va("camera time: %d", cp->time));
	}

	CG_SPrint(va("origin: %f  %f  %f", cp->origin[0], cp->origin[1], cp->origin[2]));
	CG_SPrint(va("angles: %f  %f  %f", cp->angles[0], cp->angles[1], cp->angles[2]));
	CG_SPrint(va("fov: %f", cp->fov));
	CG_SPrint(va("flags: %s%s%s%s",
				 cp->flags & CAM_ORIGIN ? "origin " : "",
				 cp->flags & CAM_ANGLES ? "angles " : "",
				 cp->flags & CAM_FOV ? "fov " : "",
				 cp->flags & CAM_TIME ? "time " : ""));

	if (cp == cameraPointLast) {
		CG_SPrint(va("length:"));
	} else {
		CG_SPrint(va("length: %f", cp->len));
	}
	CG_SPrint(va("target: %d%s", demo.camera.target, demo.camera.target == -1 ? " (not set)" : ""));

	switch (demo.camera.smoothPos) {
	case posLinear:
		smoothPos = "linear";
		break;
	case posCatmullRom:
		smoothPos = "catmull-rom";
		break;
	case posBezier:
		smoothPos = "bezier";
		break;
	default:
		smoothPos = "unknown";
		break;
	}
	CG_SPrint(va("smooth position: %s", smoothPos));
}

static float Wolfcam_DrawSpeed (float y)
{
    const char *s;
    float speed;
    int c;
	vec4_t color;
	float x;
	int style, align;
	float scale;
	const fontInfo_t *font;
	float h;
	rectDef_t rect;

	if (!cg_drawSpeed.integer) {
		return y;
	}

	SC_Vec4ColorFromCvars(color, &cg_drawSpeedColor, &cg_drawSpeedAlpha);
	align = cg_drawSpeedAlign.integer;
	scale = cg_drawSpeedScale.value;
	style = cg_drawSpeedStyle.integer;
	QLWideScreen = cg_drawSpeedWideScreen.integer;

	if (*cg_drawSpeedFont.string) {
		font = &cgs.media.speedFont;
	} else {
		font = &cgDC.Assets.textFont;
	}

	x = cg_drawSpeedX.value;
	if (*cg_drawSpeedY.string) {
		y = cg_drawSpeedY.value;
	} else {
		y = y + 2;
	}

    if (!wolfcam_following) {
		if (cg.cameraPlaying) {
			speed = sqrt(cg.cameraVelocity[0] * cg.cameraVelocity[0] + cg.cameraVelocity[1] * cg.cameraVelocity[1] + cg.cameraVelocity[2] * cg.cameraVelocity[2]);
		} else if (cg.freecam) {
			//speed = sqrt(cg.freecamPlayerState.velocity[0] * cg.freecamPlayerState.velocity[0] + cg.freecamPlayerState.velocity[1] * cg.freecamPlayerState.velocity[1]);
			speed = sqrt(cg.freecamPlayerState.velocity[0] * cg.freecamPlayerState.velocity[0] + cg.freecamPlayerState.velocity[1] * cg.freecamPlayerState.velocity[1] + cg.freecamPlayerState.velocity[2] * cg.freecamPlayerState.velocity[2]);
		} else {
			speed = cg.xyspeed;
		}
    } else {
        if (wcg.clientNum != cg.snap->ps.clientNum) {
            const entityState_t *es;

            es = &cg_entities[wcg.clientNum].currentState;
            speed = sqrt (es->pos.trDelta[0] * es->pos.trDelta[0] + es->pos.trDelta[1] * es->pos.trDelta[1]);
        } else {
            const playerState_t *ps;

            ps = &cg.snap->ps;
            speed = sqrt( ps->velocity[0] * ps->velocity[0] +
                          ps->velocity[1] * ps->velocity[1] );
        }
    }

    if (speed >= 820.0)
        c = 6;
    else if (speed >= 720.0)
        c = 5;
    else if (speed >= 620.0)
        c = 1;
    else if (speed >= 520.0)
        c = 4;
    else if (speed >= 420.0)
        c = 3;
    else if (speed >= 320.0)
        c = 2;
    else
		c = 7;

	if (cg_drawSpeedChangeColor.integer == 0) {
		c = 7;
	}

	if (cg_drawSpeedNoText.integer) {
		s = va("^%d%d", c, (int)speed);
	} else {
		s = va("^%d%d^7ups", c, (int)speed);
	}

	h = CG_Text_Height(s, scale, 0, font);

	memset(&rect, 0, sizeof(rectDef_t));
	rect.x = x;
	rect.y = y + h + (12.0f * scale);
	CG_Text_Paint_Align(&rect, scale, color, s, 0, 0, style, font, align);

	return y + h + (12.0f * scale);
}

static void CG_DrawJumpSpeeds (void)
{
	vec4_t color;
	float x, y, w;
	int style, align;
	float scale;
	const fontInfo_t *font;
	const char *s;
	char buffer[1024];
	int i;
	int maxShown;

	if (!cg.numJumps) {
		return;
	}

	SC_Vec4ColorFromCvars(color, &cg_drawJumpSpeedsColor, &cg_drawJumpSpeedsAlpha);
	if (color[3] <= 0.0) {
		return;
	}

	align = cg_drawJumpSpeedsAlign.integer;
	scale = cg_drawJumpSpeedsScale.value;
	style = cg_drawJumpSpeedsStyle.integer;
	QLWideScreen = cg_drawJumpSpeedsWideScreen.integer;

	if (*cg_drawJumpSpeedsFont.string) {
		font = &cgs.media.jumpSpeedsFont;
	} else {
		font = &cgDC.Assets.textFont;
	}

	x = cg_drawJumpSpeedsX.value;
	y = cg_drawJumpSpeedsY.value;

	buffer[0] = '\0';
	if (!cg_drawJumpSpeedsNoText.integer) {
		Q_strncpyz(buffer, "jump speeds: ", sizeof(buffer));
	}

	maxShown = cg_drawJumpSpeedsMax.integer;
	if (maxShown >= MAX_JUMPS_INFO) {
		maxShown = MAX_JUMPS_INFO - 1;
	}
	i = cg.numJumps - maxShown;
	if (i < 0) {
		i = 0;
	}

	for ( ;  i < cg.numJumps;  i++) {
		Q_strcat(buffer, sizeof(buffer), va("%d ", cg.jumps[i % MAX_JUMPS_INFO].speed));
	}

	s = buffer;
	w = CG_Text_Width(s, scale, 0, font);
	if (align == 1) {
		x -= w / 2;
	} else if (align == 2) {
		x -= w;
	}
	CG_Text_Paint_Bottom(x, y, scale, color, s, 0, 0, style, font);
}

static void CG_DrawJumpSpeedsTime (void)
{
	vec4_t color;
	float x, y, w;
	int style, align;
	float scale;
	const fontInfo_t *font;
	const char *s;
	char buffer[1024];

	if (!cg.numJumps) {
		return;
	}

	SC_Vec4ColorFromCvars(color, &cg_drawJumpSpeedsTimeColor, &cg_drawJumpSpeedsTimeAlpha);
	if (color[3] <= 0.0) {
		return;
	}

	align = cg_drawJumpSpeedsTimeAlign.integer;
	scale = cg_drawJumpSpeedsTimeScale.value;
	style = cg_drawJumpSpeedsTimeStyle.integer;
	QLWideScreen = cg_drawJumpSpeedsTimeWideScreen.integer;

	if (*cg_drawJumpSpeedsTimeFont.string) {
		font = &cgs.media.jumpSpeedsTimeFont;
	} else {
		font = &cgDC.Assets.textFont;
	}

	x = cg_drawJumpSpeedsTimeX.value;
	y = cg_drawJumpSpeedsTimeY.value;

	buffer[0] = '\0';
	if (!cg_drawJumpSpeedsTimeNoText.integer) {
		Q_strncpyz(buffer, "jump time: ", sizeof(buffer));
	}

	Q_strcat(buffer, sizeof(buffer), va("%f", (float)(cg.jumps[(cg.numJumps - 1) % MAX_JUMPS_INFO].time - cg.jumpsFirstTime) / 1000.0));

	if (!cg_drawJumpSpeedsTimeNoText.integer) {
		Q_strcat(buffer, sizeof(buffer), " seconds");
	}
	s = buffer;
	w = CG_Text_Width(s, scale, 0, font);
	if (align == 1) {
		x -= w / 2;
	} else if (align == 2) {
		x -= w;
	}
	CG_Text_Paint_Bottom(x, y, scale, color, s, 0, 0, style, font);
}

static void CG_DrawRaceTime (void)
{
	vec4_t color;
	float x, y, w;
	int style, align;
	float scale;
	const fontInfo_t *font;
	const char *s;
	int nextCheckPointEnt;
	int start;
	int end;
	const char *timeString;

	if (cg_qlhud.integer) {
		return;
	}

	if (cgs.gametype != GT_RACE) {
		return;
	}

	if (cg.freecam  &&  cg_freecam_useTeamSettings.integer == 0) {
		return;
	}

	SC_Vec4ColorFromCvars(color, &cg_drawRaceTimeColor, &cg_drawRaceTimeAlpha);
	if (color[3] <= 0.0) {
		return;
	}

	align = cg_drawRaceTimeAlign.integer;
	scale = cg_drawRaceTimeScale.value;
	style = cg_drawRaceTimeStyle.integer;
	QLWideScreen = cg_drawRaceTimeWideScreen.integer;

	if (*cg_drawRaceTimeFont.string) {
		font = &cgs.media.raceTimeFont;
	} else {
		font = &cgDC.Assets.textFont;
	}

	x = cg_drawRaceTimeX.value;
	y = cg_drawRaceTimeY.value;

	//FIXME duplicate code in cg_newdraw.c CG_RACE_TIMES
	if (wolfcam_following) {
		nextCheckPointEnt = cg_entities[wcg.clientNum].pe.raceCheckPointNextEnt;
		start = cg_entities[wcg.clientNum].pe.raceStartTime;
		end = cg_entities[wcg.clientNum].pe.raceEndTime;
	} else {
		nextCheckPointEnt = cg.predictedPlayerEntity.pe.raceCheckPointNextEnt;
		start = cg.predictedPlayerEntity.pe.raceStartTime;
		end = cg.predictedPlayerEntity.pe.raceEndTime;
	}

	if (nextCheckPointEnt > 0) {
		timeString = CG_GetTimeString(cg.time - start);
		if (cg_drawRaceTimeNoText.integer) {
			s = timeString;
		} else {
			s = va("CURRENT RUN: %s", timeString);
		}
	} else {
		timeString = CG_GetTimeString(end);
		if (cg_drawRaceTimeNoText.integer) {
			s = timeString;
		} else {
			s = va("LAST TIME: %s", timeString);
		}
	}

	w = CG_Text_Width(s, scale, 0, font);
	if (align == 1) {
		x -= w / 2;
	} else if (align == 2) {
		x -= w;
	}
	CG_Text_Paint_Bottom(x, y, scale, color, s, 0, 0, style, font);
}

static float Wolfcam_DrawMouseSpeed (float y)
{
    //char *s;
    //int w;
    //float sy, sx;
    //playerState_t *ps;
    //static vec3_t oldangles = { 0.0, 0.0, 0.0 };
    //static int oldServerTime = -1;
    //vec3_t oldAngles;
    //vec3_t newAngles;
	//int i;

	//QLWideScreen

	return y;

#if 0
    s = va ("%f %f %f",
			cg.snap->ps.origin[0],
			cg.snap->ps.origin[1],
			cg.snap->ps.origin[2]

			//sy, sx
            );
    w = CG_DrawStrlen(s, &cgs.media.bigchar);

    CG_DrawBigString (635 - w, y + 2, s, 1.0f);

	return y + BIGCHAR_HEIGHT + 4;

	if (wolfcam_following) {
		//FIXME wolfcam
		//return y;
	}

	sy = 0.0;
	sx = 0.0;

#if 0
	for (i = 30;  i > 0;  i--) {
		VectorCopy (wcg.snaps[(wcg.curSnapshotNumber - i - 1) % MAX_SNAPSHOT_BACKUP].ps.viewangles, oldAngles);
		VectorCopy (wcg.snaps[(wcg.curSnapshotNumber - i) % MAX_SNAPSHOT_BACKUP].ps.viewangles, newAngles);
		sy += newAngles[0] - oldAngles[0];
		sx += newAngles[1] - oldAngles[1];
	}
	sy /= 30.0;
	sx /= 30.0;
#endif

	VectorCopy (wcg.snaps[(wcg.curSnapshotNumber - 2) % MAX_SNAPSHOT_BACKUP].ps.viewangles, oldAngles);
    VectorCopy (cg.snap->ps.viewangles, newAngles);



#if 0
    speed = sqrt (ps->delta_angles[0] * ps->delta_angles[0]  +
                  ps->delta_angles[1] * ps->delta_angles[1]  +
                  ps->delta_angles[2] * ps->delta_angles[2]  +
                  ps->delta_angles[3] * ps->delta_angles[3]
                  );
#endif
    //s = va ("%f %f %f", speed);
    s = va ("%0.3f %0.3f",
            (newAngles[0] - oldAngles[0]) * 20,
            (newAngles[1] - oldAngles[1]) * 20
			//sy, sx
            );
    w = CG_DrawStrlen(s, &cgs.media.bigchar);

    CG_DrawBigString (635 - w, y + 2, s, 1.0f);

#if 0
    CG_Printf ("%f %f %f\n",
               cg.predictedPlayerState.delta_angles[0],
               cg.predictedPlayerState.delta_angles[1],
               cg.predictedPlayerState.delta_angles[2],
               cg.predictedPlayerState.delta_angles[3]
               );
#endif

    return y + BIGCHAR_HEIGHT + 4;
#endif
}

static void Wolfcam_DrawFollowing (void)
{
    const char *s;
    const char *scolor;
    qboolean visible;
	float x, y, w;
	int style, align;
	float scale;
	const fontInfo_t *font;
	char *clanTag;
	vec4_t color;

    if (!wolfcam_following)
		return;

	if (!wolfcam_drawFollowing.integer) {
		return;
	}

	SC_Vec4ColorFromCvars(color, &cg_drawFollowingColor, &cg_drawFollowingAlpha);
	align = cg_drawFollowingAlign.integer;
	scale = cg_drawFollowingScale.value;
	style = cg_drawFollowingStyle.integer;
	QLWideScreen = cg_drawFollowingWideScreen.integer;

	if (*cg_drawFollowingFont.string) {
		font = &cgs.media.followingFont;
	} else {
		font = &cgDC.Assets.textFont;
	}


    //FIXME wolfcam could be in limbo or not visible
    //if (cg_entities[cgs.clientinfo[wcg.clientNum].clientNum].currentValid  ||
    //    cg.snap->ps.clientNum == wcg.clientNum)
    if (wclients[wcg.clientNum].currentValid)
        visible = qtrue;
    else
        visible = qfalse;

    if (visible  ||  CG_IsCpmaMvd())
        scolor = S_COLOR_WHITE;
    else
		scolor = S_COLOR_RED;

	if (wolfcam_drawFollowing.integer == 2) {
		if (wcg.followMode == WOLFCAM_FOLLOW_KILLER) {
			if (wcg.clientNum == wcg.nextKiller) {
				scolor = S_COLOR_GREEN;
			} else if (wcg.clientNum == wcg.nextVictim) {
				scolor = S_COLOR_YELLOW;
			}
		} else if (wcg.followMode == WOLFCAM_FOLLOW_VICTIM) {
			if (wcg.clientNum == wcg.nextVictim) {
				scolor = S_COLOR_YELLOW;
			} else if (wcg.clientNum == wcg.nextKiller) {
				scolor = S_COLOR_GREEN;
			}
		}
	}

	if (wolfcam_drawFollowing.integer == 2  &&  wcg.followMode == WOLFCAM_FOLLOW_SELECTED_PLAYER) {
		// special case first
		if (wcg.nextKiller == wcg.nextVictim  &&  wcg.clientNum == wcg.nextKiller) {
			if (wcg.nextKillerServerTime > -1  &&  wcg.nextKillerServerTime < wcg.nextVictimServerTime) {
				scolor = S_COLOR_GREEN;
			}
		} else if (wcg.clientNum == wcg.nextKiller) {
			scolor = S_COLOR_GREEN;
		} else if (wcg.clientNum == wcg.nextVictim) {
			scolor = S_COLOR_YELLOW;
		}
	}

	if (cg_entities[wcg.clientNum].currentState.eFlags & EF_DEAD) {
		//scolor = S_COLOR_BLUE;
	}

	clanTag = cgs.clientinfo[wcg.clientNum].clanTag;

	if (wolfcam_drawFollowingOnlyName.integer) {
		if (*clanTag) {
			s = va("%s ^7%s", clanTag, cgs.clientinfo[wcg.clientNum].name);
		} else {
			s = cgs.clientinfo[wcg.clientNum].name;
		}
	} else {
		if (*clanTag) {
			s = va("%sFollowing^7 %s ^7%s", scolor, clanTag, cgs.clientinfo[wcg.clientNum].name);
		} else {
			s = va("%sFollowing^7 %s", scolor, cgs.clientinfo[wcg.clientNum].name);
		}
	}

	x = cg_drawFollowingX.value;
	y = cg_drawFollowingY.value;

	w = CG_Text_Width(s, scale, 0, font);
	if (align == 1) {
		x -= w / 2;
	} else if (align == 2) {
		x -= w;
	}
	//CG_Text_Paint_Bottom(x, y, scale, colorWhite, s, 0, 0, style, font);
	CG_Text_Paint_Bottom(x, y, scale, color, s, 0, 0, style, font);

	return;
}

static void CG_DrawCpmaMvdIndicator (void)
{
    const char *s;
	float x, y, w;
	int style, align;
	float scale;
	const fontInfo_t *font;
	vec4_t color;

	if (!cg_drawCpmaMvdIndicator.integer) {
		return;
	}

    if (wolfcam_following) {
		return;
	}

	if (!cg.snap) {
		return;
	}

	// only with demo taker view
	// check if cg.clientNum is invalid since it might be GTV
	if (cgs.clientinfo[cg.clientNum].infoValid  &&  cg.snap->ps.clientNum != cg.clientNum) {
		return;
	}

	if (!CG_IsCpmaMvd()) {
		return;
	}

	SC_Vec4ColorFromCvars(color, &cg_drawCpmaMvdIndicatorColor, &cg_drawCpmaMvdIndicatorAlpha);
	align = cg_drawCpmaMvdIndicatorAlign.integer;
	scale = cg_drawCpmaMvdIndicatorScale.value;
	style = cg_drawCpmaMvdIndicatorStyle.integer;
	QLWideScreen = cg_drawCpmaMvdIndicatorWideScreen.integer;

	if (*cg_drawCpmaMvdIndicatorFont.string) {
		font = &cgs.media.cpmaMvdIndicatorFont;
	} else {
		font = &cgDC.Assets.textFont;
	}

	s = ("[CPMA MVD]");

	x = cg_drawCpmaMvdIndicatorX.value;
	y = cg_drawCpmaMvdIndicatorY.value;

	w = CG_Text_Width(s, scale, 0, font);
	if (align == 1) {
		x -= w / 2;
	} else if (align == 2) {
		x -= w;
	}
	//CG_Text_Paint_Bottom(x, y, scale, colorWhite, s, 0, 0, style, font);
	CG_Text_Paint_Bottom(x, y, scale, color, s, 0, 0, style, font);

	return;
}

#if 1  //def MPACK

//FIXME depends on textHeight
float CG_Text_Pic_Width (const floatint_t *text, float scale, float iconScale, int limit, float textHeight, const fontInfo_t *fontOrig)
{
	int byteCount;  // check overflow
	int charCount;
	int len;
	float out;
	glyphInfo_t glyph;
	float useScale;
	float xscale;
	const fontInfo_t *font;
	int i;
	fontInfo_t newFont;

	font = fontOrig;

	if (cg_qlFontScaling.integer  &&  font == &cgDC.Assets.textFont) {
		if (scale <= cg_smallFont.value) {
			font = &cgDC.Assets.smallFont;
		} else if (scale > cg_bigFont.value) {
			font = &cgDC.Assets.bigFont;
		}
	}

	xscale = 1.0;

	if (!Q_stricmp(font->name, "q3tiny")) {
		xscale = 0.5;
	} else if (!Q_stricmp(font->name, "q3small")) {
		xscale = 0.5;
	} else if (!Q_stricmp(font->name, "q3giant")) {
		xscale = 2.0;
	}

	// hack to match quakelive behavior of not scaling text even if WIDESCREEN_STRETCH is set
	if ((cg_wideScreen.integer == 5  ||  cg_wideScreen.integer == 7)  &&  QLWideScreen == WIDESCREEN_STRETCH) {
		float ratio;

		ratio = (float)cgs.glconfig.vidWidth / (float)cgs.glconfig.vidHeight;

		if (ratio > (640.0f / 480.0f)) {
			xscale /= ratio / (640.0f / 480.0f);
		}
	}

	useScale = scale * font->glyphScale;

	font = CG_ScaleFont(font, &scale, &useScale);

	out = 0;
	if (text) {
		const floatint_t *s;
		char stmp[5] = "\0\0\0\0\0";
		int cp;
		int bytes;
		qboolean error;

		s = text;
		i = 0;
		while (1) {
			if (s[0].i == 0) {
				break;
			}
			if (s[0].i < 0  ||  s[0].i > 255) {
				s += 2;
				i += 2;
				continue;
			}
			//Com_Printf("^1  count '%c' : %d\n", s[0].i, i);
			s++;
			i++;
		}

		len = i;
		s = text;

		byteCount = 0;
		charCount = 0;
		while (s  &&  s[0].i  &&  byteCount < len) {
			if (limit > 0) {
				if (charCount >= limit) {
					break;
				}
			}

			if (s[0].i < 0  ||  s[0].i > 255) {
				if (s[0].i == TEXT_PIC_PAINT_ICON) {
					out += (textHeight * iconScale);
				} else if (s[0].i == TEXT_PIC_PAINT_XOFFSET) {
					//FIXME maybe not

				} else if (s[0].i == TEXT_PIC_PAINT_FONT) {
					if (!trap_R_GetFontInfo(s[1].i, &newFont)) {
						Com_Printf("^3CG_Text_Pic_Width: couldn't get new font info for %d\n", s[1].i);
						font = fontOrig;
					} else {
						font = &newFont;
					}

					//FIXME duplicate code

					//FIXME will never be true unless frag token parsing can pass font == ""
					if (cg_qlFontScaling.integer  &&  font == &cgDC.Assets.textFont) {
						if (scale <= cg_smallFont.value) {
							font = &cgDC.Assets.smallFont;
						} else if (scale > cg_bigFont.value) {
							font = &cgDC.Assets.bigFont;
						}
					}

					xscale = 1.0;
					//yscale = 1.0;

					if (!Q_stricmp(font->name, "q3tiny")) {
						xscale = 0.5;
						//yscale = 0.5;
					} else if (!Q_stricmp(font->name, "q3small")) {
						xscale = 0.5;
					} else if (!Q_stricmp(font->name, "q3giant")) {
						xscale = 2.0;
						//yscale = 3.0;
					}

					// hack to match quakelive behavior of not scaling text even if WIDESCREEN_STRETCH is set
					if ((cg_wideScreen.integer == 5  ||  cg_wideScreen.integer == 7)  &&  QLWideScreen == WIDESCREEN_STRETCH) {
						float ratio;

						ratio = (float)cgs.glconfig.vidWidth / (float)cgs.glconfig.vidHeight;

						if (ratio > (640.0f / 480.0f)) {
							xscale /= ratio / (640.0f / 480.0f);
						}
					}

					useScale = scale * font->glyphScale;

					font = CG_ScaleFont(font, &scale, &useScale);
				} else if (s[0].i == TEXT_PIC_PAINT_SCALE) {
					scale *= s[1].f;
					useScale = scale * font->glyphScale;
					font = CG_ScaleFont(font, &scale, &useScale);
				} else if (s[0].i == TEXT_PIC_PAINT_ICONSCALE) {
					iconScale *= s[1].f;
				} else {
					//Com_Printf("^1CG_Text_Pic_Width:  unknown command: %d\n", s[0].i);
				}

				//Com_Printf("^5cp: command %d\n", s[0].i);
				s += 2;
				byteCount += 2;
				continue;
			}

			stmp[0] = s[0].i & 255;
			if (s[1].i != 0) {
				stmp[1] = s[1].i & 255;
				if (s[2].i != 0) {
					stmp[2] = s[2].i & 255;
					if (s[3].i != 0) {
						stmp[3] = s[3].i & 255;
					}
				}
			}

			bytes = 0;
			cp = Q_GetCpFromUtf8(stmp, &bytes, &error);
			if (bytes > 1) {
				//Com_Printf("^2text_width cp unicode: %d\n", cp);
			}
			if (error) {
				const floatint_t *s1 = text;
				Com_Printf("cg_text_pic_width error: '%c'... \n", text[0].i);
				while (s1[0].i != '\0') {
					Com_Printf("char: %u\n", s1[0].i & 255);
					s1++;
				}
			}

			trap_R_GetGlyphInfo(font, cp, &glyph);

			if ( Q_IsColorStringPicString(s) ) {
				if (cgs.osp) {
					if (s[1].i == 'x'  ||  s[1].i == 'X') {
						s += 8;
						byteCount += 8;
					} else {
						s += 2;
						byteCount += 2;
					}
				} else {
					//Com_Printf("^6cp: color %c\n", s[1].i);
					s += 2;
					byteCount += 2;
				}
				continue;
			} else {

				out += ((float)glyph.xSkip * useScale * xscale);
				//FIXME adjust?

				//Com_Printf("^2cp: '%c'  byteCount:%d  len:%d\n", cp, byteCount, len);

				s += bytes;
				byteCount += bytes;
				charCount++;
			}
		}
	}

	//Com_Printf("width: %f (%d)\n", out, charCount);
	//CG_PrintPicString(text);

	return out;
}

static float CG_Text_Width_orig (const char *text, float scale, int limit, const fontInfo_t *fontOrig)
{
	int byteCount;
	int charCount;
	int len;
	float out;
	glyphInfo_t glyph;
	float useScale;
	float xscale;
	const char *s = text;
	const fontInfo_t *font;


	font = fontOrig;

	if (cg_qlFontScaling.integer  &&  font == &cgDC.Assets.textFont) {
		if (scale <= cg_smallFont.value) {
			font = &cgDC.Assets.smallFont;
		} else if (scale > cg_bigFont.value) {
			font = &cgDC.Assets.bigFont;
		}
	}

	xscale = 1.0;

	if (!Q_stricmp(font->name, "q3tiny")) {
		xscale = 0.5;
	} else if (!Q_stricmp(font->name, "q3small")) {
		xscale = 0.5;
	} else if (!Q_stricmp(font->name, "q3giant")) {
		xscale = 2.0;
	}

#if 0  // debugging widescreen
	if (!Q_stricmp(text, "hello world")) {
		Com_Printf("^2hello:  widescreen: %d\n", QLWideScreen);
	}
#endif

	// quake live bug with WIDESCREEN_STRETCH and right + center align can be simulated by not running this block (cg_wideScreen.integer == 7)

	// hack to match quakelive behavior of not scaling text even if WIDESCREEN_STRETCH is set
	if (cg_wideScreen.integer == 5  &&  QLWideScreen == WIDESCREEN_STRETCH) {
		float ratio;

		ratio = (float)cgs.glconfig.vidWidth / (float)cgs.glconfig.vidHeight;

		if (ratio > (640.0f / 480.0f)) {
			xscale /= ratio / (640.0f / 480.0f);
		}
	}

	useScale = scale * font->glyphScale;

	font = CG_ScaleFont(font, &scale, &useScale);

	out = 0;
	if (text) {
		len = strlen(text);

		byteCount = 0;
		charCount = 0;
		while (s  &&  *s  &&  byteCount < len) {
			if (limit > 0) {
				if (charCount >= limit) {
					break;
				}
			}

			if ( Q_IsColorString(s) ) {
				if (cgs.osp) {
					if (s[1] == 'x'  ||  s[1] == 'X') {
						s += 8;
						byteCount += 8;
					} else {
						s += 2;
						byteCount += 2;
					}
				} else {
					s += 2;
					byteCount += 2;
				}
				continue;
			} else {
				int cp;
				int bytes;
				qboolean error;

				bytes = 0;
				cp = Q_GetCpFromUtf8(s, &bytes, &error);

				if (bytes > 1) {
					//Com_Printf("^2text_width cp unicode: %d\n", cp);
				}
				if (error) {
					const char *s1 = text;
					Com_Printf("text_width error: '%s'\n", text);
					while (s1[0] != '\0') {
						Com_Printf("char: %u\n", s1[0] & 255);
						s1++;
					}
				}
				trap_R_GetGlyphInfo(font, cp, &glyph);
				out += glyph.xSkip;

				s += bytes;
				byteCount += bytes;
				charCount++;
			}
		}
	}

	return out * useScale * xscale;
}

float CG_Text_Width (const char *text, float scale, int limit, const fontInfo_t *font)
{
	return CG_Text_Width_orig(text, scale, limit, font);
}

float CG_Text_Width_old(const char *text, float scale, int limit, int fontIndex)
{
	const fontInfo_t *font;

	if (fontIndex <= 0) {
		font = &cgDC.Assets.textFont;
	} else {
		font = &cgDC.Assets.extraFonts[fontIndex];
	}

	return CG_Text_Width(text, scale, limit, font);
}

static float CG_Text_Height_orig (const char *text, float scale, int limit, const fontInfo_t *fontOrig)
{
	int len;
	int byteCount;
	int charCount;
	float max;
	glyphInfo_t glyph;
	float useScale;
	float yscale;
	const char *s = text;
	const fontInfo_t *font;

	font = fontOrig;

	if (cg_qlFontScaling.integer  &&  font == &cgDC.Assets.textFont) {
		if (scale <= cg_smallFont.value) {
			font = &cgDC.Assets.smallFont;
		} else if (scale > cg_bigFont.value) {
			font = &cgDC.Assets.bigFont;
		}
	}

	yscale = 1.0;

	if (!Q_stricmp(font->name, "q3tiny")) {
		yscale = 0.5;
	} else if (!Q_stricmp(font->name, "q3small")) {
		//
	} else if (!Q_stricmp(font->name, "q3giant")) {
		yscale = 3.0;
	}

  useScale = scale * font->glyphScale;
  font = CG_ScaleFont(font, &scale, &useScale);

  max = 0;
  if (text) {
	    len = strlen(text);

		byteCount = 0;
		charCount = 0;
		while (s  &&  *s  &&  byteCount < len) {
			if (limit > 0) {
				if (charCount >= limit) {
					break;
				}
			}

			if ( Q_IsColorString(s) ) {
				if (cgs.osp) {
					if (s[1] == 'x'  ||  s[1] == 'X') {
						s += 8;
						byteCount += 8;
					} else {
						s += 2;
						byteCount += 2;
					}
				} else {
					s += 2;
					byteCount += 2;
				}
				continue;
			} else {
				int cp;
				int bytes;
				qboolean error;

				bytes = 0;
				cp = Q_GetCpFromUtf8(s, &bytes, &error);

				if (error) {
					const char *s1 = text;
					Com_Printf("text_width error: '%s'\n", text);
					while (s1[0] != '\0') {
						Com_Printf("char: %u\n", s1[0] & 255);
						s1++;
					}
				}
				trap_R_GetGlyphInfo(font, cp, &glyph);
				if (max < glyph.height) {
					max = glyph.height;
				}

				s += bytes;
				byteCount += bytes;
				charCount++;
			}
    }
  }

  return max * useScale * yscale;;
}

float CG_Text_Height (const char *text, float scale, int limit, const fontInfo_t *font)
{
	return CG_Text_Height_orig(text, scale, limit, font);
}

float CG_Text_Height_old(const char *text, float scale, int limit, int fontIndex)
{
	const fontInfo_t *font;

	if (fontIndex <= 0) {
		font = &cgDC.Assets.textFont;
	} else {
		font = &cgDC.Assets.extraFonts[fontIndex];
	}

	return CG_Text_Height(text, scale, limit, font);
}

#if 0  // unused
void CG_Text_PaintChar(float x, float y, float width, float height, float scale, float s, float t, float s2, float t2, qhandle_t hShader) {
  float w, h;

  w = width * scale;
  h = height * scale;
  CG_AdjustFrom640( &x, &y, &w, &h );
  trap_R_DrawStretchPic( x, y, w, h, s, t, s2, t2, hShader );
}
#endif

void CG_Text_PaintCharScale (float x, float y, float width, float height, float xscale, float yscale, float s, float t, float s2, float t2, qhandle_t hShader)
{
  float w, h;

  w = width * xscale;
  h = height * yscale;
  CG_AdjustFrom640( &x, &y, &w, &h );
  trap_R_DrawStretchPic( x, y, w, h, s, t, s2, t2, hShader );
}


const fontInfo_t *CG_ScaleFont (const fontInfo_t *font, float *scale, float *useScale)
{
	static fontInfo_t scaledFont;
	int threshold;

	if (!cg_autoFontScaling.integer) {
		*useScale *= cg_overallFontScale.value;
		return font;
	}

	if (font->bitmapFont) {
		*useScale *= cg_overallFontScale.value;
		return font;
	}

	//FIXME based on point size
	//if (*useScale > 1.1f  ||  *useScale < 0.9f) {
	if (*useScale <= 0.5f  ||  *useScale >= 2.0) {
		//float newScale;
		int newPointSize;
		int oldPointSize;

		//Com_Printf("trying new scaled font: %f (%f) '%s' '%s'\n", *useScale, *scale, font->registerName, font->name);
		Com_Memset(&scaledFont, 0, sizeof(fontInfo_t));
		oldPointSize = font->pointSize;
		newPointSize = font->pointSize * *useScale;

		//newPointSize = font->pointSize / 4;
		// set to 6, 12, 24, 48, 96, or 192
		if (newPointSize < 12) {
			newPointSize = 6;
		} else if (newPointSize < 24) {
			newPointSize = 12;
		} else if (newPointSize < 48) {
			newPointSize = 24;
		} else if (newPointSize < 96) {
			newPointSize = 48;
		} else if (newPointSize < 192) {
			newPointSize = 96;
		} else {  // don't go higher
			newPointSize = 192;
		}

		threshold = cg_autoFontScalingThreshold.integer;
		if (threshold < 1) {
			threshold = 10;
		}

		if (newPointSize < threshold) {
			newPointSize = threshold;
		}

		trap_R_RegisterFont(font->qlDefaultFont ? DEFAULT_FONT : font->registerName, newPointSize, &scaledFont);
		if (scaledFont.name[0] == '\0') {
			Com_Printf("^3couldn't register new scaled point size for '%s' '%s' qlDefaultFont: %d\n", font->registerName, font->name, font->qlDefaultFont);
		} else {
			//float origGlyphScale;

			//origGlyphScale = font->glyphScale;
			// got it
		    font = &scaledFont;
			//useScale /= ((float)newPointSize / (float)oldPointSize);

			//useScale = scale * font->glyphScale;
			*useScale *= ((float)oldPointSize / (float)newPointSize);
			//useScale = scale * ((float)oldPointSize / (float)newPointSize) * origGlyphScale;
		}
	}

	*useScale *= cg_overallFontScale.value;

	return font;
}

//FIXME xy scale factors
void CG_Text_Paint (float x, float y, float scale, const vec4_t color, const char *text, float adjust, int limit, int style, const fontInfo_t *fontOrig)
{
	int len;
	int byteCount;
	int charCount;
	vec4_t newColor;
	glyphInfo_t glyph;
	float useScale;
	const fontInfo_t *font;
	float xscale;
	float yscale;

	//Com_Printf("CG_Text_Paint %d  %s\n", limit, text);

	font = fontOrig;

	if (cg_qlFontScaling.integer  &&  font == &cgDC.Assets.textFont) {
		if (scale <= cg_smallFont.value) {
			font = &cgDC.Assets.smallFont;
		} else if (scale > cg_bigFont.value) {
			font = &cgDC.Assets.bigFont;
		}
	}

	xscale = 1.0;
	yscale = 1.0;

	if (!Q_stricmp(font->name, "q3tiny")) {
		xscale = 0.5;
		yscale = 0.5;
	} else if (!Q_stricmp(font->name, "q3small")) {
		xscale = 0.5;
	} else if (!Q_stricmp(font->name, "q3giant")) {
		xscale = 2.0;
		yscale = 3.0;
	}

	// hack to match quakelive behavior of not scaling text even if WIDESCREEN_STRETCH is set
	if ((cg_wideScreen.integer == 5  ||  cg_wideScreen.integer == 7)  &&  QLWideScreen == WIDESCREEN_STRETCH) {
		float ratio;

		ratio = (float)cgs.glconfig.vidWidth / (float)cgs.glconfig.vidHeight;

		if (ratio > (640.0f / 480.0f)) {
			xscale /= ratio / (640.0f / 480.0f);
		}
	}

	useScale = scale * font->glyphScale;

	//FIXME eliminate font->glyphScale hack

	font = CG_ScaleFont(font, &scale, &useScale);

#if 0  // testing
	if (DebugFont) {
		Com_Printf("'%s' scale:%f  useScale:%f\n", font->name, scale, useScale);
	}
#endif

  if (text) {
		const char *s = text;
		trap_R_SetColor( color );
		memcpy(&newColor[0], &color[0], sizeof(vec4_t));

		len = strlen(text);

		byteCount = 0;
		charCount = 0;
		while (s  &&  *s  &&  byteCount < len) {
			int cp;
			int bytes;
			qboolean error;

			if (limit > 0) {
				if (charCount >= limit) {
					break;
				}
			}

			bytes = 0;
			cp = Q_GetCpFromUtf8(s, &bytes, &error);

			if (bytes > 1) {
				//Com_Printf("^2cp unicode: %d\n", cp);
			}
			//Com_Printf("^2cp: %lu\n", cp);
			if (error) {
				const char *s1 = text;
				Com_Printf("error: '%s'\n", text);
				while (s1[0] != '\0') {
					Com_Printf("char: %u\n", s1[0] & 255);
					s1++;
				}
			}
			//FIXME do below
			trap_R_GetGlyphInfo(font, cp, &glyph);

			if ( Q_IsColorString( s ) ) {
				if (cgs.cpma) {
					CG_CpmaColorFromString(s + 1, newColor);
				} else if (cgs.osp) {
					CG_OspColorFromString(s + 1, newColor);
				} else {
					memcpy( newColor, g_color_table[ColorIndex(*(s+1))], sizeof( newColor ) );
				}
				// g_color_table_ql_0_1_0_303
				//memcpy( newColor, g_color_table_ql_0_1_0_303[ColorIndex(*(s+1))], sizeof( newColor ) );

				if (cg.colorCodeUseForegroundAlpha) {
					newColor[3] = color[3];
				}

				if (s[1] == '7'  &&  cg.colorCodeWhiteUseForegroundColor) {
					VectorCopy(color, newColor);
				}
				trap_R_SetColor( newColor );
				if (cgs.osp) {
					if (s[1] == 'x'  ||  s[1] == 'X') {
						s += 8;
						byteCount += 8;
					} else {
						s += 2;
						byteCount += 2;
					}
				} else {
					s += 2;
					byteCount += 2;
				}
				continue;
			} else {
				float yadj = useScale * yscale * glyph.top;
				float xadj = useScale * xscale * glyph.left;

				if (style == ITEM_TEXTSTYLE_SHADOWED || style == ITEM_TEXTSTYLE_SHADOWEDMORE) {
					float ofs = style == ITEM_TEXTSTYLE_SHADOWED ? 1 : 2;
					float xofs = ofs;
					float yofs = ofs;

					xofs *= useScale * xscale;
					yofs *= useScale * yscale;

					if (style == ITEM_TEXTSTYLE_SHADOWED) {
						if (xofs < 1.0f) {
							xofs = 1.0f;
						}
						if (yofs < 1.0f) {
							yofs = 1.0f;
						}
					} else if (style == ITEM_TEXTSTYLE_SHADOWEDMORE) {
						if (xofs < 2.0f) {
							xofs = 2.0f;
						}
						if (yofs < 2.0f) {
							yofs = 2.0f;
						}
					}

					colorBlack[3] = newColor[3];
					trap_R_SetColor( colorBlack );
					//FIXME ofs should use scale value
					CG_Text_PaintCharScale(
										   x + xadj + xofs ,
										   y - yadj + yofs,
														glyph.imageWidth,
														glyph.imageHeight,
														useScale * xscale,
														useScale * yscale,
														glyph.s,
														glyph.t,
														glyph.s2,
														glyph.t2,
														glyph.glyph);
					colorBlack[3] = 1.0;
					trap_R_SetColor( newColor );
				}

#if 0
				if (style == ITEM_TEXTSTYLE_BLINK) {  //  &&  !((cg.realTime/BLINK_DIVISOR) & 1)) {
					vec4_t blinkColor;
					vec4_t lowLight;

					lowLight[0] = 0.8 * newColor[0];
					lowLight[1] = 0.8 * newColor[1];
					lowLight[2] = 0.8 * newColor[2];
					lowLight[3] = 0.8 * newColor[3];

					//Vector4Set(lowLight, newColor[0], newColor[1], newColor[2], 0);
					Vector4Set(lowLight, 0, 0, 0, 0);

					//LerpColor(newColor, lowLight, blinkColor, 0.5 + 0.5*sin(cg.realTime / PULSE_DIVISOR));
					LerpColor(newColor, lowLight, blinkColor, 0.5 + 0.5*sin(cg.realTime / 1000));
					//Vector4Set(blinkColor, 1, 0, 0, 1);

					trap_R_SetColor(blinkColor);
				}
#endif

				CG_Text_PaintCharScale(
									   x + xadj,
									   y - yadj,
													glyph.imageWidth,
													glyph.imageHeight,
													useScale * xscale,
													useScale * yscale,
													glyph.s,
													glyph.t,
													glyph.s2,
													glyph.t2,
													glyph.glyph);


				x += (glyph.xSkip * useScale * xscale) + adjust;
				//Com_Printf("%c  xSkip %d\n", s[0], glyph->xSkip);
				s += bytes;
				byteCount += bytes;
				charCount++;
			}
		}
		trap_R_SetColor( NULL );
  }
}

void CG_PrintPicString (const floatint_t *s)
{
	while (s  &&  s[0].i != 0) {
		if (s[0].i >= 0  &&  s[0].i <= 255) {
			Com_Printf("%c:%d ", s[0].i, s[0].i);
		} else {
			Com_Printf(" [%d .] ", s[0].i);
			s += 2;
			continue;
		}
		s++;
	}

	Com_Printf("\n");
}

//FIXME xy scale factors
void CG_Text_Pic_Paint (float x, float y, float scale, const vec4_t color, const floatint_t *text, float adjust, int limit, int style, const fontInfo_t *fontOrig, float textHeight, float iconScale)
{
	int len;
	int byteCount;  // check text overflow
	int charCount;
	vec4_t newColor;
	glyphInfo_t glyph;
	float useScale;
	const fontInfo_t *font;
	float xscale;
	float yscale;
	int i;
	fontInfo_t newFont;
	float outWidth;

	font = fontOrig;

	outWidth = 0;
	//printPicString(text);
	if (cg_qlFontScaling.integer  &&  font == &cgDC.Assets.textFont) {
		if (scale <= cg_smallFont.value) {
			font = &cgDC.Assets.smallFont;
		} else if (scale > cg_bigFont.value) {
			font = &cgDC.Assets.bigFont;
		}
	}

	xscale = 1.0;
	yscale = 1.0;

	if (!Q_stricmp(font->name, "q3tiny")) {
		xscale = 0.5;
		yscale = 0.5;
	} else if (!Q_stricmp(font->name, "q3small")) {
		xscale = 0.5;
	} else if (!Q_stricmp(font->name, "q3giant")) {
		xscale = 2.0;
		yscale = 3.0;
	}

	// hack to match quakelive behavior of not scaling text even if WIDESCREEN_STRETCH is set
	if ((cg_wideScreen.integer == 5  ||  cg_wideScreen.integer == 7)  &&  QLWideScreen == WIDESCREEN_STRETCH) {
		float ratio;

		ratio = (float)cgs.glconfig.vidWidth / (float)cgs.glconfig.vidHeight;

		if (ratio > (640.0f / 480.0f)) {
			xscale /= ratio / (640.0f / 480.0f);
		}
	}

	useScale = scale * font->glyphScale;

	font = CG_ScaleFont(font, &scale, &useScale);

	if (text) {
		const floatint_t *s = text;
		int cp;
		int bytes;
		qboolean error;

		trap_R_SetColor( color );
		memcpy(&newColor[0], &color[0], sizeof(vec4_t));

		i = 0;
		while (1) {
			if (s[0].i == 0) {
				break;
			}
			if (s[0].i < 0  ||  s[0].i > 255) {
				s += 2;
				i += 2;
				continue;
			}

			// char value
			s++;
			i++;
		}
		len = i;
		s = text;

		byteCount = 0;
		charCount = 0;
		while (s  &&  s[0].i  &&  byteCount < len) {
			if (limit > 0) {
				if (charCount >= limit) {
					break;
				}
			}

			if (s[0].i < 0  ||  s[0].i > 255) {
				if (s[0].i == TEXT_PIC_PAINT_ICON) {
					float picScale;
					float picWidth;
					float picHeight;
					float theight;

					if (cg_fragIconHeightFixed.integer) {
						theight = CG_Text_Height("ABCXYZ", scale, 0, fontOrig);
					} else {
						theight = textHeight;
					}
					picScale = iconScale;
					picWidth = theight * picScale;
					picHeight = picWidth;

					CG_DrawPic(x, y - theight - (picHeight - theight) / 2, picWidth, picHeight, s[1].i);
					x += picWidth;
				} else if (s[0].i == TEXT_PIC_PAINT_COLOR) {
					int c;

					c = s[1].i;
					c &= 0xff0000;
					c /= 0x010000;
					newColor[0] = (float)c / 255.0;
					c = s[1].i;
					c &= 0x00ff00;
					c /= 0x000100;
					newColor[1] = (float)c / 255.0;
					c = s[1].i;
					c &= 0x0000ff;
					c /= 0x000001;
					newColor[2] = (float)c / 255.0;
					newColor[3] = color[3];
					trap_R_SetColor(newColor);
				} else if (s[0].i == TEXT_PIC_PAINT_XOFFSET) {
					//FIXME text_width()
					x += s[1].f;
				} else if (s[0].i == TEXT_PIC_PAINT_YOFFSET) {
					//FIXME text_height()
					y += s[1].f;
				} else if (s[0].i == TEXT_PIC_PAINT_FONT) {

					if (!trap_R_GetFontInfo(s[1].i, &newFont)) {
						Com_Printf("^3CG_Text_Pic_Paint: couldn't get new font info for %d\n", s[1].i);
						font = fontOrig;
					} else {
						font = &newFont;
					}

					//FIXME duplicate code

					//FIXME will never be true unless frag token parsing can pass font == ""
					if (cg_qlFontScaling.integer  &&  font == &cgDC.Assets.textFont) {
						if (scale <= cg_smallFont.value) {
							font = &cgDC.Assets.smallFont;
						} else if (scale > cg_bigFont.value) {
							font = &cgDC.Assets.bigFont;
						}
					}

					xscale = 1.0;
					yscale = 1.0;

					if (!Q_stricmp(font->name, "q3tiny")) {
						xscale = 0.5;
						yscale = 0.5;
					} else if (!Q_stricmp(font->name, "q3small")) {
						xscale = 0.5;
					} else if (!Q_stricmp(font->name, "q3giant")) {
						xscale = 2.0;
						yscale = 3.0;
					}

					// hack to match quakelive behavior of not scaling text even if WIDESCREEN_STRETCH is set
					if ((cg_wideScreen.integer == 5  ||  cg_wideScreen.integer == 7)  &&  QLWideScreen == WIDESCREEN_STRETCH) {
						float ratio;

						ratio = (float)cgs.glconfig.vidWidth / (float)cgs.glconfig.vidHeight;

						if (ratio > (640.0f / 480.0f)) {
							xscale /= ratio / (640.0f / 480.0f);
						}
					}

					useScale = scale * font->glyphScale;

					font = CG_ScaleFont(font, &scale, &useScale);

				} else if (s[0].i == TEXT_PIC_PAINT_SCALE) {
					scale *= s[1].f;
					useScale = scale * font->glyphScale;
					font = CG_ScaleFont(font, &scale, &useScale);
				} else if (s[0].i == TEXT_PIC_PAINT_ICONSCALE) {
					iconScale *= s[1].f;
				} else if (s[0].i == TEXT_PIC_PAINT_STYLE) {
					style = s[1].i;
					//Com_Printf("style: %d\n", style);
				} else {
					Com_Printf("^1CG_Text_Pic_Paint:  unknown command: %d\n", s[0].i);
				}

				s += 2;
				byteCount += 2;
				continue;
			} // *s < 0  ||  *s > 255

			{
				char stmp[5] = "\0\0\0\0\0";

				stmp[0] = s[0].i & 255;
				if (s[1].i != 0) {
					stmp[1] = s[1].i & 255;
					if (s[2].i != 0) {
						stmp[2] = s[2].i & 255;
						if (s[3].i != 0) {
							stmp[3] = s[3].i & 255;
						}
					}
				}

#if 0
				//FIXME testing
				stmp[0] = s[0].i & 255;
				stmp[1] = s[1].i & 255;
				stmp[2] = s[2].i & 255;
				stmp[3] = s[3].i & 255;
#endif

				//Com_Printf("^2xxxxxxx '%s' 0x%x 0x%x 0x%x 0x%x\n", stmp, s[0], s[1], s[2], s[3]);
				bytes = 0;
				cp = Q_GetCpFromUtf8(stmp, &bytes, &error);
				if (bytes > 1) {
					//Com_Printf("^5got cp %d 0x%x '%s'\n", cp, cp, stmp);
				}
			}

			//Com_Printf("cp: %d '%c'\n", cp, (unsigned char)cp);
			
			trap_R_GetGlyphInfo(font, cp, &glyph);

			if ( Q_IsColorStringPicString( s ) ) {
				if (cgs.cpma) {
					CG_CpmaColorFromPicString(s + 1, newColor);
				} else if (cgs.osp) {
					CG_OspColorFromPicString(s + 1, newColor);
				} else {
					memcpy( newColor, g_color_table[ColorIndex(s[1].i)], sizeof( newColor ) );
				}
				//memcpy( newColor, g_color_table[ColorIndex(*(s+1))], sizeof( newColor ) );
				if (cg.colorCodeUseForegroundAlpha) {
					newColor[3] = color[3];
				}

				if (s[1].i == '7'  &&  cg.colorCodeWhiteUseForegroundColor) {
					VectorCopy(color, newColor);
				}
				trap_R_SetColor( newColor );
				if (cgs.osp) {
					if (s[1].i == 'x'  ||  s[1].i == 'X') {
						s += 8;
						byteCount += 8;
					} else {
						s += 2;
						byteCount += 2;
					}
				} else {
					s += 2;
					byteCount += 2;
				}
				continue;
			} else {   // not Q_IsColorString()
				float yadj = useScale * yscale * glyph.top;
				float xadj = useScale * xscale * glyph.left;

				if (style == ITEM_TEXTSTYLE_SHADOWED || style == ITEM_TEXTSTYLE_SHADOWEDMORE) {
					float ofs = style == ITEM_TEXTSTYLE_SHADOWED ? 1 : 2;
					float xofs = ofs;
					float yofs = ofs;

					xofs *= useScale * xscale;
					yofs *= useScale * yscale;

					colorBlack[3] = newColor[3];  //FIXME wtf?????
					trap_R_SetColor( colorBlack );
					//trap_R_SetColor(colorWhite);
					CG_Text_PaintCharScale(
										   x + xadj + xofs,
										   y - yadj + yofs,
														glyph.imageWidth,
														glyph.imageHeight,
														useScale * xscale,
														useScale * yscale,
														glyph.s,
														glyph.t,
														glyph.s2,
														glyph.t2,
														glyph.glyph);
					colorBlack[3] = 1.0;
					trap_R_SetColor( newColor );
				}
#if 1
				CG_Text_PaintCharScale(
									   x + xadj,
									   y - yadj,
													glyph.imageWidth,
													glyph.imageHeight,
													useScale * xscale,
													useScale * yscale,
													glyph.s,
													glyph.t,
													glyph.s2,
													glyph.t2,
													glyph.glyph);

#endif
				x += (glyph.xSkip * useScale * xscale) + adjust;
				//Com_Printf("%c  xSkip %d\n", s[0], glyph->xSkip);
				outWidth += ((float)glyph.xSkip * useScale * xscale);
				s += bytes;
				byteCount += bytes;
				charCount++;
			}
		}

		trap_R_SetColor( NULL );
	}

	//Com_Printf("outWidth: %f (%d)\n", outWidth, charCount);
	//CG_PrintPicString(text);
}


// assumes 512x512 font image, and ... font widths
#define FONT_DIMENSIONS 512  //256  // ql fonts now 512

//FIXME assuming worst case where dim == FONT_DIMENSIONS
#define NAME_SPRITE_GLYPH_DIMENSION FONT_DIMENSIONS  // 96  //48
#define NAME_SPRITE_SHADOW_OFFSET 16  //8


#define IMGBUFFSIZE (NAME_SPRITE_GLYPH_DIMENSION * (NAME_SPRITE_GLYPH_DIMENSION + NAME_SPRITE_SHADOW_OFFSET * 2)     * MAX_QPATH * 2)
static ubyte imgBuff[IMGBUFFSIZE];
static ubyte finalImgBuff[IMGBUFFSIZE];


#define TMPBUFF_SIZE (NAME_SPRITE_GLYPH_DIMENSION * NAME_SPRITE_GLYPH_DIMENSION * 4)

static ubyte fontImageData[FONT_DIMENSIONS * FONT_DIMENSIONS * 4];
static ubyte tmpBuff[TMPBUFF_SIZE];
static int fontImageWidth;
static int fontImageHeight;

//FIXME font x offset
//FIXME xy scale factors
//FIXME scaling is done when you add the sprite
void CG_CreateNameSprite (float xf, float yf, float scale, const vec4_t color, const char *text, float adjust, int limit, int style, const fontInfo_t *fontOrig, qhandle_t h)
{
	int len, count;
	vec4_t newColor;
	glyphInfo_t glyph;
	//float useScale;
	const fontInfo_t *font;
	//float xscale;
	//float yscale;
	int subx;
	int suby;
	int n;
	int i;
	int j;
	int k;
	int x;
	//int y;
	int destWidth;
	int destHeight;
	int alpha;
	int shadowOffset;

	//Com_Printf("replace shader %d for '%s'\n", h, text);

#if 0
	{
		int randIntr;
		int randIntg;
		int randIntb;

	randIntr = rand() % 256;
	randIntg = rand() % 256;
	randIntb = rand() % 256;



	for (i = 0;  i < NAME_SPRITE_GLYPH_DIMENSION * NAME_SPRITE_GLYPH_DIMENSION * MAX_QPATH * 1; i += 4) {
		finalImgBuff[i + 0] = randIntr;
		finalImgBuff[i + 1] = randIntg;
		finalImgBuff[i + 2] = randIntb;
		finalImgBuff[i + 3] = 255;
	}

    trap_ReplaceShaderImage(h, finalImgBuff, 64, 64);

	return;
	}
#endif

	if (!cg.menusLoaded) {
		// user might change default fonts in menu
		//Com_Printf("can't create name sprite yet, menus aren't loaded\n");
		return;
	}

	//Com_Printf("name sprite: '%s'\n", fontOrig->name);

	x = xf;
	//y = yf;

	x = 0;
	//y = 0;

	font = fontOrig;  //FIXME check font?

	if (cg_qlFontScaling.integer  &&  font == &cgDC.Assets.textFont) {
		if (scale <= cg_smallFont.value) {
			font = &cgDC.Assets.smallFont;
		} else if (scale > cg_bigFont.value) {
			font = &cgDC.Assets.bigFont;
		}
	}

	//Com_Printf("new name sprite font %d: '%s'\n", font == &cgDC.Assets.textFont, font->name);

	alpha = cg_drawPlayerNamesAlpha.integer;

	shadowOffset = 0;
	if (cg_drawPlayerNamesStyle.integer == 3) {
		shadowOffset = 1;
	} else if (cg_drawPlayerNamesStyle.integer == 6) {
		shadowOffset = 2;
	}

	//xscale = 1.0;
	//yscale = 1.0;

#if 0
	//FIXME
	if (!Q_stricmp(font->name, "q3tiny")) {
		xscale = 0.5;
		yscale = 0.5;
	} else if (!Q_stricmp(font->name, "q3small")) {
		xscale = 0.5;
	} else if (!Q_stricmp(font->name, "q3giant")) {
		xscale = 2.0;
		yscale = 3.0;
	}
#endif

	//useScale = scale * font->glyphScale;
	//useScale = 1;
	destWidth = IMGBUFFSIZE / (NAME_SPRITE_GLYPH_DIMENSION + NAME_SPRITE_SHADOW_OFFSET * 2) / 4;
	destHeight = NAME_SPRITE_GLYPH_DIMENSION + NAME_SPRITE_SHADOW_OFFSET * 2;
	memset(imgBuff, 0, sizeof(imgBuff));
	memset(finalImgBuff, 0, sizeof(finalImgBuff));

	if (text) {
		const char *s = text;

		if (*cg_drawPlayerNamesColor.string) {
			SC_Vec3ColorFromCvar(newColor, &cg_drawPlayerNamesColor);
			newColor[3] = alpha;
		} else {
			memcpy(&newColor[0], &color[0], sizeof(vec4_t));
		}
		len = strlen(text);
		if (limit > 0 && len > limit) {
			len = limit;
		}
		count = 0;
		while (s && *s && count < len) {
			if ( Q_IsColorString( s ) ) {
				if (!*cg_drawPlayerNamesColor.string) {
				if (cgs.cpma) {
					CG_CpmaColorFromString(s + 1, newColor);
				} else if (cgs.osp) {
					CG_OspColorFromString(s + 1, newColor);
				} else {
					memcpy( newColor, g_color_table[ColorIndex(*(s+1))], sizeof( newColor ) );
				}
				//memcpy( newColor, g_color_table[ColorIndex(*(s+1))], sizeof( newColor ) );
					newColor[3] = alpha;
				}
				if (cgs.osp) {
					if (s[1] == 'x'  ||  s[1] == 'X') {
						s += 8;
						count += 8;
					} else {
						s += 2;
						count += 2;
					}
				} else {
					s += 2;
					count += 2;
				}
				continue;
			} else {
				int codePoint;
				int numUtf8Bytes;
				qboolean error;

				codePoint = Q_GetCpFromUtf8(s, &numUtf8Bytes, &error);
				s += (numUtf8Bytes - 1);
				//trap_R_GetGlyphInfo(font, *s & 255, &glyph);
				trap_R_GetGlyphInfo(font, codePoint, &glyph);


				trap_GetShaderImageDimensions(glyph.glyph, &fontImageWidth, &fontImageHeight);
				//if (fontImageWidth != FONT_DIMENSIONS  ||  fontImageHeight != FONT_DIMENSIONS) {
				if (fontImageWidth > FONT_DIMENSIONS  ||  fontImageHeight > FONT_DIMENSIONS) {
					Com_Printf("^3WARNING: CG_CreateNameSprite() disabling name sprites, font image dimensions are not supported: %d x %d '%s'\n", fontImageWidth, fontImageHeight, font->name);
					return;
				}

				trap_GetShaderImageData(glyph.glyph, fontImageData);

				// get sub image
				n = glyph.s * (float)(fontImageWidth);
				subx = n;
				n = glyph.t * (float)(fontImageHeight);
				suby = n;

				//Com_Printf("[%c] glpyh %d x %d\n", s[0], glyph->imageWidth, glyph->imageHeight);

				if (glyph.imageWidth > NAME_SPRITE_GLYPH_DIMENSION  ||  glyph.imageHeight > NAME_SPRITE_GLYPH_DIMENSION) {
					Com_Printf("^3WARNING: CG_CreateNameSprite() skipping glyph, '%c' dimensions are invalid: %d x %d '%s'\n", s[0], glyph.imageWidth, glyph.imageHeight, font->name);
					break;
				}
				if (glyph.imageWidth < 0  ||  glyph.imageHeight < 0) {
					Com_Printf("^3WARNING: CG_CreateNameSprite() disabling name sprites, font glyph image dimensions are invalid: %d x %d '%s'\n", glyph.imageWidth, glyph.imageHeight, font->name);
					return;
				}

				j = (destHeight - glyph.top - (NAME_SPRITE_SHADOW_OFFSET * 2)) * destWidth * 4;
				if ((x + j) + (NAME_SPRITE_GLYPH_DIMENSION + NAME_SPRITE_SHADOW_OFFSET * 2) * NAME_SPRITE_GLYPH_DIMENSION >= (IMGBUFFSIZE - ((NAME_SPRITE_GLYPH_DIMENSION + NAME_SPRITE_SHADOW_OFFSET * 2) * NAME_SPRITE_GLYPH_DIMENSION) * 2)) {
					break;
				}

				n = (suby * fontImageWidth * 4) + (subx * 4);

				for (i = 0;  i < glyph.imageHeight  &&  i < destHeight;  i++) {
					memset(tmpBuff, 0, sizeof(tmpBuff));
					if (TMPBUFF_SIZE < glyph.imageWidth * 4) {
						Com_Printf("^3WARNING:  CG_CreateNameSprite() disabling name sprites, tmp buffer is too small  %d < %d '%s'\n", TMPBUFF_SIZE, glyph.imageWidth * 4, font->name);
						return;
					}
					memcpy(tmpBuff, fontImageData + n, glyph.imageWidth * 4);

					for (k = 0;  k < glyph.imageWidth * 4;  k += 4) {
						float whiteScale;
						float alphaScale;

						whiteScale = (float)tmpBuff[k + 0] / 255.0;
						alphaScale = (float)tmpBuff[k + 3] / 255.0;
						tmpBuff[k + 0] = newColor[0] * 255.0 * whiteScale;
						tmpBuff[k + 1] = newColor[1] * 255.0 * whiteScale;
						tmpBuff[k + 2] = newColor[2] * 255.0 * whiteScale;
						tmpBuff[k + 3] = alpha * alphaScale;
					}

					if (IMGBUFFSIZE - (x + j) < (glyph.imageWidth * 4)) {
						Com_Printf("^3WARNING:  CG_CreateNameSprite() disabling name sprites, img buffer is too small %d, %d, %d, %d %d '%s'\n", IMGBUFFSIZE, x, j, glyph.imageWidth * 4, glyph.top, font->name);
						return;
					}
					memcpy(imgBuff + x + j, tmpBuff, glyph.imageWidth * 4);
					j += destWidth * 4;
					n += fontImageWidth * 4;
				}

				x += glyph.xSkip * 4;

				s++;
				count++;
			}  //  !color string
		}  // while (*s ...

		memset(finalImgBuff, 0, sizeof(finalImgBuff));
		x += NAME_SPRITE_SHADOW_OFFSET;  // to allow for shadow

		for (i = 0;  i < destHeight;  i++) {
			if (shadowOffset) {
				for (j = 0;  j < x;  j += 4) {
					int sx, nx;

					sx = (i + shadowOffset) * x + (shadowOffset * 4) + j;
					nx = i * x + j;

					// shadow first
					if (sx + 4 >= IMGBUFFSIZE) {
						Com_Printf("^3WARNING:  CG_CreateNameSprite() disabling name sprites, final buffer is too small  %d < %d '%s'\n", IMGBUFFSIZE, sx + 4, font->name);
						return;
					}

					finalImgBuff[sx + 0] = 0;
					finalImgBuff[sx + 1] = 0;
					finalImgBuff[sx + 2] = 0;

					if (i * destWidth * 4 + j + 3 >= IMGBUFFSIZE) {
						Com_Printf("^3WARNING:  CG_CreateNameSprite() disabling name sprites, read buffer is too small  %d < %d '%s'\n", IMGBUFFSIZE, i * destWidth * 4 + j + 3, font->name);
						return;
					}

					finalImgBuff[sx + 3] = imgBuff[i * destWidth * 4 + j + 3];

					// now the actual image
					if (imgBuff[i * destWidth * 4 + j + 3]) {

						if (nx + 3 >= IMGBUFFSIZE) {
							Com_Printf("^3WARNING:  CG_CreateNameSprite() disabling name sprites, final nx buffer is too small  %d < %d '%s'\n", IMGBUFFSIZE, i * destWidth * 4 + j + 3, font->name);
							return;
						}

						finalImgBuff[nx + 0] = imgBuff[i * destWidth * 4 + j + 0];
						finalImgBuff[nx + 1] = imgBuff[i * destWidth * 4 + j + 1];
						finalImgBuff[nx + 2] = imgBuff[i * destWidth * 4 + j + 2];
						finalImgBuff[nx + 3] = imgBuff[i * destWidth * 4 + j + 3];
					}

				}
			} else {
				if (i * x >= IMGBUFFSIZE) {
					Com_Printf("^3WARNING:  CG_CreateNameSprite() disabling name sprites, final copy buffer is too small  %d < %d '%s'\n", IMGBUFFSIZE, i * x, font->name);
					return;
				}
				memcpy(finalImgBuff + i * x, imgBuff + i * destWidth * 4, x);
			}
		}

#if 0
		// testing
		for (i = 0;  i < (destWidth * destHeight * 4);  i += 4) {
			finalImgBuff[i + 0] = 125;
			finalImgBuff[i + 3] = 125;
		}
#endif
		//Com_Printf("replacing shader %d\n", h);
		trap_ReplaceShaderImage(h, finalImgBuff, x / 4, destHeight);
	}
}

void CG_Text_Paint_Bottom (float x, float y, float scale, const vec4_t color, const char *text, float adjust, int limit, int style, const fontInfo_t *font)
{
	float th;

	//th = CG_Text_Height(text, scale, limit, font);
	//th = CG_Text_Height("1IPUTY", scale, limit, font);  //FIXME fontInfo should store max height
	th = CG_Text_Height("1IPUTYgj", scale, limit, font);  //FIXME fontInfo should store max height
	CG_Text_Paint(x, y + th, scale, color, text, adjust, limit, style, font);
}

void CG_Text_Paint_old (float x, float y, float scale, const vec4_t color, const char *text, float adjust, int limit, int style, int fontIndex)
{
	const fontInfo_t *font;
	//FIXME testing limit * 2  team scoreboard tmp fix
	// limit = 0
	limit = 0;
	//limit = 20;

	if (fontIndex <= 0) {
		font = &cgDC.Assets.textFont;
	} else {
		font = &cgDC.Assets.extraFonts[fontIndex];
	}

#if 0
	{  //FIXME test
		float w;

		//w = CG_Text_Width(text, scale, 0, font);
		w = CG_Text_Width(text, scale, 0, &cgs.media.smallchar);
		Com_Printf("w %f   limit %d  '%s'\n", w, limit, text);
	}
#endif

	CG_Text_Paint(x, y, scale, color, text, adjust, limit, style, font);
}

#endif  // MPACK

/*
==============
CG_DrawField

Draws large numbers for status bar and powerups
==============
*/
#ifndef MISSIONPACK
static void CG_DrawField (int x, int y, int width, int value) {
	char	num[16], *ptr;
	int		l;
	int		frame;

	if ( width < 1 ) {
		return;
	}

	// draw number string
	if ( width > 5 ) {
		width = 5;
	}

	switch ( width ) {
	case 1:
		value = value > 9 ? 9 : value;
		value = value < 0 ? 0 : value;
		break;
	case 2:
		value = value > 99 ? 99 : value;
		value = value < -9 ? -9 : value;
		break;
	case 3:
		value = value > 999 ? 999 : value;
		value = value < -99 ? -99 : value;
		break;
	case 4:
		value = value > 9999 ? 9999 : value;
		value = value < -999 ? -999 : value;
		break;
	}

	Com_sprintf (num, sizeof(num), "%i", value);
	l = strlen(num);
	if (l > width)
		l = width;
	x += 2 + CHAR_WIDTH*(width - l);

	ptr = num;
	while (*ptr && l)
	{
		if (*ptr == '-')
			frame = STAT_MINUS;
		else
			frame = *ptr -'0';

		CG_DrawPic( x,y, CHAR_WIDTH, CHAR_HEIGHT, cgs.media.numberShaders[frame] );
		x += CHAR_WIDTH;
		ptr++;
		l--;
	}
}
#endif // MISSIONPACK

static void CG_Draw3DModelExt (float x, float y, float w, float h, qhandle_t model, qhandle_t skin, const vec3_t origin, const vec3_t angles, const vec4_t color)
{
	refdef_t		refdef;
	refEntity_t		ent;

	if ( !cg_draw3dIcons.integer || !cg_drawIcons.integer ) {
		return;
	}

	//Com_Printf("draw 3d model\n");

	CG_AdjustFrom640( &x, &y, &w, &h );

	memset( &refdef, 0, sizeof( refdef ) );

	memset( &ent, 0, sizeof( ent ) );
	AnglesToAxis( angles, ent.axis );
	VectorCopy( origin, ent.origin );
	ent.hModel = model;
	ent.customSkin = skin;
	ent.renderfx = RF_NOSHADOW;		// no stencil shadows
	ent.shaderRGBA[0] = 255 * color[0];
	ent.shaderRGBA[1] = 255 * color[1];
	ent.shaderRGBA[2] = 255 * color[2];
	ent.shaderRGBA[3] = 255 * color[3];

	refdef.rdflags = RDF_NOWORLDMODEL;

	AxisClear( refdef.viewaxis );

	refdef.fov_x = 30;
	refdef.fov_y = 30;

	refdef.x = x;
	refdef.y = y;
	refdef.width = w;
	refdef.height = h;

	refdef.time = cg.time;

	trap_R_ClearScene();
	CG_AddRefEntity(&ent);
	trap_R_RenderScene( &refdef );
}

/*
================
CG_Draw3DModel

================
*/
void CG_Draw3DModel (float x, float y, float w, float h, qhandle_t model, qhandle_t skin, const vec3_t origin, const vec3_t angles)
{
	vec4_t color = { 0, 0, 0, 0 };

	CG_Draw3DModelExt(x, y, w, h, model, skin, origin, angles, color);
}

/*
================
CG_DrawHead

Used for both the status bar and the scoreboard
================
*/
void CG_DrawHead( float x, float y, float w, float h, int clientNum, const vec3_t headAngles, qboolean useDefaultTeamSkin, qboolean useChangedModel, qboolean isScoreboard ) {
	clipHandle_t	cm;
	const clientInfo_t	*ci;
	float			len;
	vec3_t			origin;
	vec3_t			mins, maxs;
	vec4_t color;

	if (useChangedModel) {
		ci = &cgs.clientinfo[clientNum];
	} else {
		ci = &cgs.clientinfoOrig[clientNum];
	}

	if (isScoreboard  &&  cgs.cpma  &&  cgs.gametype == GT_NTF  &&  cg_cpmaNtfScoreboardClassModel.integer) {
		ci = &cg.ntfClassModel[cgs.clientinfo[clientNum].ntfClass];
		useDefaultTeamSkin = qtrue;
	}

	// useDefaultTeamSkin always true except for WCG_REAL_PLAYER_HEAD

	if ( cg_draw3dIcons.integer ) {
		cm = ci->headModel;
		if ( !cm ) {
			return;
		}

		// offset the origin y and z to center the head
		trap_R_ModelBounds( cm, mins, maxs );

		origin[2] = -0.5 * ( mins[2] + maxs[2] );
		origin[1] = 0.5 * ( mins[1] + maxs[1] );

		// calculate distance so the head nearly fills the box
		// assume heads are taller than wide
		len = 0.7 * ( maxs[2] - mins[2] );
		origin[0] = len / 0.268;	// len / tan( fov/2 )

		// allow per-model tweaking
		VectorAdd( origin, ci->headOffset, origin );

		color[0] = (float)ci->selectedHeadColor[0] / 255.0;
		color[1] = (float)ci->selectedHeadColor[1] / 255.0;
		color[2] = (float)ci->selectedHeadColor[2] / 255.0;
		color[3] = (float)ci->selectedHeadColor[3] / 255.0;

		if (isScoreboard  &&  cgs.cpma  &&  cgs.gametype == GT_NTF  &&  cg_cpmaNtfScoreboardClassModel.integer) {
			if (cgs.clientinfo[clientNum].team == TEAM_RED) {
				SC_Vec3ColorFromCvar(color, &cg_cpmaNtfRedHeadColor);
				color[3] = 1.0f;
			} else if (cgs.clientinfo[clientNum].team == TEAM_BLUE) {
				SC_Vec3ColorFromCvar(color, &cg_cpmaNtfBlueHeadColor);
				color[3] = 1.0f;
			} else {
				color[0] = color[1] = color[2] = color[3] = 1.0f;
			}
		}

		if (useDefaultTeamSkin) {
			CG_Draw3DModelExt(x, y, w, h, ci->headModel, ci->headSkin, origin, headAngles, color);
		} else {
			CG_Draw3DModelExt(x, y, w, h, ci->headModel, ci->setHeadSkin, origin, headAngles, color);
		}
	} else if ( cg_drawIcons.integer ) {
		if (useDefaultTeamSkin) {
			CG_DrawPic( x, y, w, h, ci->modelIcon );
		} else {
			CG_DrawPic( x, y, w, h, ci->setModelIcon );
		}
	}

	// if they are deferred, draw a cross out
	if ( ci->deferred ) {
		CG_DrawPic( x, y, w, h, cgs.media.deferShader );
	}
}

/*
================
CG_DrawFlagModel

Used for both the status bar and the scoreboard
================
*/
void CG_DrawFlagModel( float x, float y, float w, float h, int team, qboolean force2D ) {
	qhandle_t		cm;
	float			len;
	vec3_t			origin, angles;
	vec3_t			mins, maxs;
	qhandle_t		handle;

	if ( !force2D && cg_draw3dIcons.integer ) {

		VectorClear( angles );

		cm = cgs.media.redFlagModel;

		// offset the origin y and z to center the flag
		trap_R_ModelBounds( cm, mins, maxs );

		origin[2] = -0.5 * ( mins[2] + maxs[2] );
		origin[1] = 0.5 * ( mins[1] + maxs[1] );

		// calculate distance so the flag nearly fills the box
		// assume heads are taller than wide
		len = 0.5 * ( maxs[2] - mins[2] );
		origin[0] = len / 0.268;	// len / tan( fov/2 )

		angles[YAW] = 60 * sin( cg.time / 2000.0 );;

		if( team == TEAM_RED ) {
			handle = cgs.media.redFlagModel;
		} else if( team == TEAM_BLUE ) {
			handle = cgs.media.blueFlagModel;
		} else if( team == TEAM_FREE ) {
			handle = cgs.media.neutralFlagModel;
		} else {
			return;
		}
		CG_Draw3DModel( x, y, w, h, handle, 0, origin, angles );
	} else if ( cg_drawIcons.integer ) {
		gitem_t *item;

		if( team == TEAM_RED ) {
			item = BG_FindItemForPowerup( PW_REDFLAG );
		} else if( team == TEAM_BLUE ) {
			item = BG_FindItemForPowerup( PW_BLUEFLAG );
		} else if( team == TEAM_FREE ) {
			item = BG_FindItemForPowerup( PW_NEUTRALFLAG );
		} else {
			return;
		}
		if (item) {
		  CG_DrawPic( x, y, w, h, cg_items[ ITEM_INDEX(item) ].icon );
		}
	}
}

/*
================
CG_DrawStatusBarHead

================
*/
#ifndef MISSIONPACK

static void CG_DrawStatusBarHead( float x ) {
	vec3_t		angles;
	float		size, stretch;
	float		frac;
	int y;
	int clientNum;
	qboolean useChangedModel = qfalse;

	//if (!cg_drawStatusBarHead.integer) {
	//	return;
	//}

	//if (cg_drawStatusBarHeadX.string[0] != '\0') {
	//	x = cg_drawStatusBarHeadX.value;
	//}

	VectorClear( angles );

	if (!wolfcam_following  &&  (cg.damageTime && cg.time - cg.damageTime < DAMAGE_TIME)) {
		frac = (float)(cg.time - cg.damageTime ) / DAMAGE_TIME;
		size = ICON_SIZE * 1.25 * ( 1.5 - frac * 0.5 );

		stretch = size - ICON_SIZE * 1.25;
		// kick in the direction of damage
		x -= stretch * 0.5 + cg.damageX * stretch * 0.5;

		cg.headStartYaw = 180 + cg.damageX * 45;

		cg.headEndYaw = 180 + 20 * cos( crandom()*M_PI );
		cg.headEndPitch = 5 * cos( crandom()*M_PI );

		cg.headStartTime = cg.time;
		cg.headEndTime = cg.time + 100 + random() * 2000;
	} else {
		if ( cg.time >= cg.headEndTime ) {
			// select a new head angle
			cg.headStartYaw = cg.headEndYaw;
			cg.headStartPitch = cg.headEndPitch;
			cg.headStartTime = cg.headEndTime;
			cg.headEndTime = cg.time + 100 + random() * 2000;

			cg.headEndYaw = 180 + 20 * cos( crandom()*M_PI );
			cg.headEndPitch = 5 * cos( crandom()*M_PI );
		}

		size = ICON_SIZE * 1.25;
	}

	// if the server was frozen for a while we may have a bad head start time
	if ( cg.headStartTime > cg.time ) {
		cg.headStartTime = cg.time;
	}

	frac = ( cg.time - cg.headStartTime ) / (float)( cg.headEndTime - cg.headStartTime );
	frac = frac * frac * ( 3 - 2 * frac );
	angles[YAW] = cg.headStartYaw + ( cg.headEndYaw - cg.headStartYaw ) * frac;
	angles[PITCH] = cg.headStartPitch + ( cg.headEndPitch - cg.headStartPitch ) * frac;

#if 0
	if (cg_drawStatusBarHeadY.string[0] != '\0') {
		y = cg_drawStatusBarHeadY.value;
	} else {
		y = 480 - size;
	}
#endif
	y = 480 - size;

	if (wolfcam_following) {
		clientNum = wcg.clientNum;
	} else {
		clientNum = cg.snap->ps.clientNum;
	}

	if (cg_statusBarHeadStyle.integer == 1) {
		useChangedModel = qtrue;
	}

	CG_DrawHead( x, y, size, size,
				 clientNum, angles, qtrue, useChangedModel, qfalse );
}
#endif // MISSIONPACK

/*
================
CG_DrawStatusBarFlag

================
*/
#ifndef MISSIONPACK
static void CG_DrawStatusBarFlag( float x, int team ) {
	CG_DrawFlagModel( x, 480 - ICON_SIZE, ICON_SIZE, ICON_SIZE, team, qfalse );
}
#endif // MISSIONPACK

/*
================
CG_DrawTeamBackground

================
*/
void CG_DrawTeamBackground( int x, int y, int w, int h, float alpha, int team )
{
	vec4_t		hcolor;

	if (!cg_drawTeamBackground.integer) {
		return;
	}

	hcolor[3] = alpha;
	if ( team == TEAM_RED ) {
		hcolor[0] = 1;
		hcolor[1] = 0;
		hcolor[2] = 0;
	} else if ( team == TEAM_BLUE ) {
		hcolor[0] = 0;
		hcolor[1] = 0;
		hcolor[2] = 1;
	} else {
		return;
	}
	trap_R_SetColor( hcolor );
	CG_DrawPic( x, y, w, h, cgs.media.teamStatusBar );
	trap_R_SetColor( NULL );
}

/*
================
CG_DrawStatusBar

================
*/
#ifndef MISSIONPACK
static void CG_DrawStatusBar( void ) {
	int			color;
	const centity_t	*cent;
	const playerState_t	*ps;
	int			value;
	vec4_t		hcolor;
	vec3_t		angles;
	vec3_t		origin;
	int health;
	int armor;
	qboolean painHealth = qfalse;
#ifdef MISSIONPACK
	qhandle_t	handle;
#endif
	qboolean haveRedFlag = qfalse;
	qboolean haveBlueFlag = qfalse;
	qboolean haveNeutralFlag = qfalse;
	const static float colors[4][4] = {
//		{ 0.2, 1.0, 0.2, 1.0 } , { 1.0, 0.2, 0.2, 1.0 }, {0.5, 0.5, 0.5, 1} };
		{ 1.0f, 0.69f, 0.0f, 1.0f },    // normal
		{ 1.0f, 0.2f, 0.2f, 1.0f },     // low health
		{ 0.5f, 0.5f, 0.5f, 1.0f },     // weapon firing
		{ 1.0f, 1.0f, 1.0f, 1.0f } };   // health > 100

	if ( cg_drawStatus.integer == 0 ) {
		return;
	}
	QLWideScreen = WIDESCREEN_CENTER;

	// draw the team background
	if (wolfcam_following) {
		CG_DrawTeamBackground(0, 420, 640, 60, 0.33f, cgs.clientinfo[wcg.clientNum].team);
	} else {
		CG_DrawTeamBackground( 0, 420, 640, 60, 0.33f, cg.snap->ps.persistant[PERS_TEAM] );
	}

	ps = &cg.snap->ps;
	if (wolfcam_following) {
		cent = &cg_entities[wcg.clientNum];
		armor = Wolfcam_PlayerArmor(wcg.clientNum);

		health = Wolfcam_PlayerHealth(wcg.clientNum, qfalse);
		if (health <= INVALID_WOLFCAM_HEALTH) {
			health = Wolfcam_PlayerHealth(wcg.clientNum, qtrue);
			if (health > INVALID_WOLFCAM_HEALTH) {
				painHealth = qtrue;
			}
		}
	} else {
		cent = &cg_entities[cg.snap->ps.clientNum];
		armor = ps->stats[STAT_ARMOR];
		health = ps->stats[STAT_HEALTH];
	}


	VectorClear( angles );

	// draw any 3D icons first, so the changes back to 2D are minimized
	if ( cent->currentState.weapon && cg_weapons[ cent->currentState.weapon ].ammoModel ) {
		origin[0] = 70;
		origin[1] = 0;
		origin[2] = 0;
		angles[YAW] = 90 + 20 * sin( cg.time / 1000.0 );
		CG_Draw3DModel( CHAR_WIDTH*3 + TEXT_ICON_SPACE, 432, ICON_SIZE, ICON_SIZE,
					   cg_weapons[ cent->currentState.weapon ].ammoModel, 0, origin, angles );
	}

	CG_DrawStatusBarHead( 185 + CHAR_WIDTH*3 + TEXT_ICON_SPACE );

	if (wolfcam_following) {
		int powerups;

		powerups = cg_entities[wcg.clientNum].currentState.powerups;
		haveRedFlag = powerups & (1 << PW_REDFLAG);
		haveBlueFlag = powerups & (1 << PW_BLUEFLAG);
		haveNeutralFlag = powerups & (1 << PW_NEUTRALFLAG);
	} else {
		haveRedFlag = cg.predictedPlayerState.powerups[PW_REDFLAG];
		haveBlueFlag = cg.predictedPlayerState.powerups[PW_BLUEFLAG];
		haveNeutralFlag = cg.predictedPlayerState.powerups[PW_NEUTRALFLAG];
	}

	if (haveRedFlag) {
		CG_DrawStatusBarFlag( 185 + CHAR_WIDTH*3 + TEXT_ICON_SPACE + ICON_SIZE, TEAM_RED );
	} else if (haveBlueFlag) {
		CG_DrawStatusBarFlag( 185 + CHAR_WIDTH*3 + TEXT_ICON_SPACE + ICON_SIZE, TEAM_BLUE );
	} else if (haveNeutralFlag) {
		CG_DrawStatusBarFlag( 185 + CHAR_WIDTH*3 + TEXT_ICON_SPACE + ICON_SIZE, TEAM_FREE );
	}


	if (armor > 0) {
		origin[0] = 90;
		origin[1] = 0;
		origin[2] = -10;
		angles[YAW] = ( cg.time & 2047 ) * 360 / 2048.0;
		CG_Draw3DModel( 370 + CHAR_WIDTH*3 + TEXT_ICON_SPACE, 432, ICON_SIZE, ICON_SIZE,
					   cgs.media.armorModel, 0, origin, angles );
	}
	//FIXME wolfcam
#ifdef MISSIONPACK
	if( cgs.gametype == GT_HARVESTER ) {
		origin[0] = 90;
		origin[1] = 0;
		origin[2] = -10;
		angles[YAW] = ( cg.time & 2047 ) * 360 / 2048.0;
		if( cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE ) {
			handle = cgs.media.redCubeModel;
		} else {
			handle = cgs.media.blueCubeModel;
		}
		CG_Draw3DModel( 640 - (TEXT_ICON_SPACE + ICON_SIZE), 416, ICON_SIZE, ICON_SIZE, handle, 0, origin, angles );
	}
#endif
	//
	// ammo
	//
	if (cent->currentState.weapon) {
		value = -1;

		if (CG_IsCpmaMvd()) {
			if (wolfcam_following) {
				if (cent->currentState.weapon != WP_GAUNTLET  &&  cent->currentState.weapon != WP_GRAPPLING_HOOK) {
					value = (unsigned int)ps->ammo[wcg.clientNum] & 0xff;
				}
			}
		} else {
			if (!wolfcam_following) {
				value = ps->ammo[cent->currentState.weapon];
			}
		}

		if ( value > -1 ) {
			if ( !wolfcam_following  &&  (cg.predictedPlayerState.weaponstate == WEAPON_FIRING
										  && cg.predictedPlayerState.weaponTime > 100) ) {
				// draw as dark grey when reloading
				color = 2;	// dark grey
			} else {
				if ( value >= 0 ) {
					color = 0;	// green
				} else {
					color = 1;	// red
				}
			}
			trap_R_SetColor( colors[color] );

			CG_DrawField (0, 432, 3, value);
			trap_R_SetColor( NULL );
		}

		if (wolfcam_following  ||  (!wolfcam_following  &&  value > -1)) {
			// if we didn't draw a 3D icon, draw a 2D icon for ammo
			if ( !cg_draw3dIcons.integer && cg_drawIcons.integer ) {
				qhandle_t	icon;

				icon = cg_weapons[cent->currentState.weapon].ammoIcon;
				if ( icon ) {
					CG_DrawPic( CHAR_WIDTH*3 + TEXT_ICON_SPACE, 432, ICON_SIZE, ICON_SIZE, icon );
				}
			}
		}
	}

	//
	// health
	//
	value = health;
	if (painHealth) {
		vec4_t painHealthColor;
		float *fcolor;

		SC_Vec4ColorFromCvars(painHealthColor, &wolfcam_painHealthColor, &wolfcam_painHealthAlpha);

		// fade
		if (wolfcam_painHealthFade.integer) {
			fcolor = CG_FadeColor(wclients[wcg.clientNum].painValueTime, wolfcam_painHealthFadeTime.integer);

			if (!fcolor) {
				painHealthColor[3] = 0;
			} else {
				painHealthColor[3] -= (1.0 - fcolor[3]);
			}

			if (painHealthColor[3] < 0.0) {
				painHealthColor[3] = 0;
			}
		}
		trap_R_SetColor(painHealthColor);
	} else if ( value > 100 ) {
		trap_R_SetColor( colors[3] );		// white
	} else if (value > 25) {
		trap_R_SetColor( colors[0] );	// green
	} else if (value > 0) {
		color = (cg.time >> 8) & 1;	// flash
		trap_R_SetColor( colors[color] );
	} else {
		trap_R_SetColor( colors[1] );	// red
	}

	// stretch the health up when taking damage  2016-07-15 wc what?
	if (!wolfcam_following  ||  (wolfcam_following  &&  value != INVALID_WOLFCAM_HEALTH)) {
		if (painHealth  &&  wolfcam_painHealthStyle.integer == 1  &&  cgs.realProtocol >= 90  &&  value >= 80) {
			// since protocol 90 ql doesn't send real health value just 20, 40, 60, 80
			// x += 2 + CHAR_WIDTH*(3 - l)
			CG_DrawPic(185 + (2 + CHAR_WIDTH*2), 432, CHAR_WIDTH, CHAR_HEIGHT, cgs.media.numberShaders[STAT_MINUS]);
		} else {
			CG_DrawField ( 185, 432, 3, value);
		}

		CG_ColorForHealth( hcolor );
		trap_R_SetColor( hcolor );
	}

	//
	// armor
	//
	value = armor;
	if (value > 0 ) {
		trap_R_SetColor( colors[0] );
		CG_DrawField (370, 432, 3, value);
		trap_R_SetColor( NULL );
		// if we didn't draw a 3D icon, draw a 2D icon for armor
		if ( !cg_draw3dIcons.integer && cg_drawIcons.integer ) {
			CG_DrawPic( 370 + CHAR_WIDTH*3 + TEXT_ICON_SPACE, 432, ICON_SIZE, ICON_SIZE, cgs.media.yellowArmorIcon );
		}

	}
	//FIXME wolfcam
#ifdef MISSIONPACK
	//
	// cubes
	//
	if( cgs.gametype == GT_HARVESTER ) {
		value = ps->generic1 & 0x3f;
		if( value > 99 ) {
			value = 99;
		}
		trap_R_SetColor( colors[0] );
		CG_DrawField (640 - (CHAR_WIDTH*2 + TEXT_ICON_SPACE + ICON_SIZE), 432, 2, value);
		trap_R_SetColor( NULL );
		// if we didn't draw a 3D icon, draw a 2D icon for armor
		if ( !cg_draw3dIcons.integer && cg_drawIcons.integer ) {
			if( cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE ) {
				handle = cgs.media.redCubeIcon;
			} else {
				handle = cgs.media.blueCubeIcon;
			}
			CG_DrawPic( 640 - (TEXT_ICON_SPACE + ICON_SIZE), 432, ICON_SIZE, ICON_SIZE, handle );
		}
	}
#endif
}
# else // MISSIONPACK
static void CG_DrawStatusBar (void)
{
}
#endif

/*
===========================================================================================

  UPPER RIGHT CORNER

===========================================================================================
*/

#if 0
static int attacker_num = -1;

//static float Wolfcam_DrawAttacker( float y ) {
static int Wolfcam_DrawAttacker( float y ) {
	//int			t;
	float		size;
	vec3_t		angles;
	const char	*info;
	const char	*name;
	int			clientNum;
	qboolean useChangedModel = qfalse;
    trace_t trace;
    vec3_t start, forward, end;
    //int content;
	float x;

    VectorCopy (cg_entities[wcg.clientNum].currentState.pos.trBase, start);
    AngleVectors (cg_entities[wcg.clientNum].currentState.apos.trBase, forward, NULL, NULL);
    VectorMA (start, 131072, forward, end );
    Wolfcam_WeaponTrace (&trace, start, vec3_origin, vec3_origin, end, wcg.clientNum, CONTENTS_SOLID|CONTENTS_BODY);

    if (trace.entityNum >= MAX_CLIENTS) {
        goto L1;
        //return y;
    } else {
        attacker_num = trace.entityNum;
        goto L1;
    }

#if 0  //FIXME wolfcam double check this
    // if the player is in fog, don't show it
    content = CG_PointContents( trace.endpos, 0 );
    if ( content & CONTENTS_FOG ) {
        return -1;  //y;
    }

    // if the player is invisible, don't show it
    if ( cg_entities[ trace.entityNum ].currentState.powerups & ( 1 << PW_INVIS ) ) {
        return -1;  //y;
    }

	//clientNum = cg.predictedPlayerState.persistant[PERS_ATTACKER];
    clientNum = trace.entityNum;
	if ( clientNum < 0 || clientNum >= MAX_CLIENTS || clientNum == wcg.clientNum ) {
		return -1;  //y;
	}

#if 0
	t = cg.time - cg.attackerTime;
	if ( t > ATTACKER_HEAD_TIME ) {
		cg.attackerTime = 0;
		return -1;  //y;
	}
#endif
#endif  //FIXME wolfcam double check it

 L1:

    if (attacker_num == -1)
        return -1; //y;

    clientNum = attacker_num;

	return clientNum;

#if 0
	size = ICON_SIZE * cg_drawAttackerImageScale.value;  //1.25;

	if (cg_drawAttackerX.string[0] != '\0') {
		x = cg_drawAttackerX.value;
	} else {
		x = 640 - size;
	}

	if (cg_drawAttackerY.string[0] != '\0') {
		y = cg_drawAttackerY.value;
	}

	angles[PITCH] = 0;
	angles[YAW] = 180;
	angles[ROLL] = 0;

	if (cg_drawAttacker.integer == 2) {
		useChangedModel = qtrue;
	}
	CG_DrawHead( x, y, size, size, clientNum, angles, qtrue, qfalse, useChangedModel, qfalse );

	info = CG_ConfigString( CS_PLAYERS + clientNum );
	name = Info_ValueForKey(  info, "n" );
	y += size;

	//FIXME smaller font for ql
	//CG_DrawBigString( (x + size) - ( Q_PrintStrlen( name ) * BIGCHAR_WIDTH), y, name, 0.5 );
	//FIXME hack to keep right alignment
	if (cg_drawAttackerX.string[0] == '\0') {
		CG_DrawBigString( (x + size) - ( Q_PrintStrlen( name ) * BIGCHAR_WIDTH), y, name, 0.5 );
	} else {
		CG_DrawBigString( (x + size / 2)  - ( Q_PrintStrlen( name ) * BIGCHAR_WIDTH) / 2, y, name, 0.5 );
	}

	//CG_DrawBigString( 640 - ( Q_PrintStrlen( name ) * BIGCHAR_WIDTH), y, name, 0.5 );

	return y + BIGCHAR_HEIGHT + 2;
#endif
}
#endif

/*
================
CG_DrawAttacker

================
*/
static float CG_DrawAttacker( float y ) {
	int			t;
	float		size;
	vec3_t		angles;
	//const char	*info;
	//	const char	*name;
	int			clientNum = 0;  // silence compiler
	qboolean useChangedModel = qtrue;
	vec4_t color;
	float x, w;
	int style, align;
	float scale;
	const fontInfo_t *font;
	float *fcolor;
	char *s;
	float h;

	if (cg_drawAttacker.integer == 0) {
		return y;
	}

    if (wolfcam_following) {
        //return Wolfcam_DrawAttacker(y);
		//clientNum = Wolfcam_DrawAttacker(y);
		//Com_Printf("%d\n", clientNum);
		//FIXME
		return y;
    }

	if (!wolfcam_following  &&  cg.predictedPlayerState.stats[STAT_HEALTH] <= 0 ) {
		return y;
	}

	if (!wolfcam_following  &&  !cg.attackerTime ) {
		return y;
	}

	SC_Vec4ColorFromCvars(color, &cg_drawAttackerColor, &cg_drawAttackerAlpha);
	if (cg_drawAttackerFade.integer) {
		fcolor = CG_FadeColor(cg.attackerTime, cg_drawAttackerFadeTime.integer);

		if (!fcolor) {
			return y;
		}
		color[3] -= (1.0 - fcolor[3]);
    }

	if (color[3] <= 0.0) {
		return y;
	}
	align = cg_drawAttackerAlign.integer;
	scale = cg_drawAttackerScale.value;
	style = cg_drawAttackerStyle.integer;
	QLWideScreen = cg_drawAttackerWideScreen.integer;

	if (*cg_drawAttackerFont.string) {
		font = &cgs.media.attackerFont;
	} else {
		font = &cgDC.Assets.textFont;
	}

	x = cg_drawAttackerX.value;
	if (*cg_drawAttackerY.string) {
		y = cg_drawAttackerY.value;
	}

	if (!wolfcam_following) {
		clientNum = cg.predictedPlayerState.persistant[PERS_ATTACKER];
	}

	if ( clientNum < 0 || clientNum >= MAX_CLIENTS || clientNum == cg.snap->ps.clientNum ) {
		return y;
	}

	if (!cgs.clientinfo[clientNum].infoValid) {
		cg.attackerTime = 0;
		return y;
	}

	if (!wolfcam_following) {
		t = cg.time - cg.attackerTime;
		//if ( t > ATTACKER_HEAD_TIME ) {  //FIXME
		if (t > cg_drawAttackerTime.integer) {
			cg.attackerTime = 0;
			return y;
		}
	}
	s = cgs.clientinfo[clientNum].name;

	w = CG_Text_Width(s, scale, 0, font);
	h = CG_Text_Height(s, scale, 0, font);

#if 0
	if (align == 1) {
		x -= w / 2;
	} else if (align == 2) {
		x -= w;
	}
#endif

	angles[PITCH] = 0;
	angles[YAW] = 180;
	angles[ROLL] = 0;

	size = ICON_SIZE * cg_drawAttackerImageScale.value * scale;
	if (align == 1) {
		x -= size / 2;
	} else if (align == 2) {
		x -= size;
	}

	if (cg_drawAttacker.integer == 2) {
		useChangedModel = qfalse;
	}

	trap_R_SetColor(color);
	CG_DrawHead( x, y, size, size, clientNum, angles, qtrue, useChangedModel, qfalse );

	x = cg_drawAttackerX.value;
	if (align == 1) {
		x -= w / 2;
	} else if (align == 2) {
		x -= w;
	}
	y += size;
	CG_Text_Paint_Bottom(x, y, scale, color, s, 0, 0, style, font);

	return y + h + 2;
#if 0
	///////////////////////////
#if 1

#if 0
	//FIXME
	//size = ICON_SIZE * 1;  //0.5;  //* 1.25;
	if (cg_qlhud.integer  &&  !wolfcam_following) {
		size = ICON_SIZE * 0.75;
	} else {
		size = ICON_SIZE * 1.25;
	}
#endif


#if 0
	if (cg_drawAttackerX.string[0] != '\0') {
		x = cg_drawAttackerX.value;
	} else {
		x = 640 - size;
	}
#endif

#if 0
	if (cg_drawAttackerY.string[0] != '\0') {
		y = cg_drawAttackerY.value;
	}
#endif

	//FIXME fixmefont
	//font = cg_drawAttackerFont.string;
	//fontScale = cg_drawAttackerFontScale.value;


	s = cgs.clientinfo[clientNum].name;
	w = CG_Text_Width(s, scale, 0, font);
	if (align == 1) {
		x -= size / 2;
	} else if (align == 2) {
		x -= size;
	}
	//CG_DrawHead( 640 - size, y, size, size, clientNum, angles, qtrue, qfalse, qfalse );


	info = CG_ConfigString( CS_PLAYERS + clientNum );
	name = cgs.clientinfo[clientNum].name;

	y += size;
	//FIXME smaller font for ql
	//CG_DrawBigString( (x + size) - ( Q_PrintStrlen( name ) * BIGCHAR_WIDTH), y, name, 0.5 );
	//FIXME hack to keep right alignment
	if (cg_drawAttackerX.string[0] == '\0') {
		CG_DrawBigString( (x + size) - ( Q_PrintStrlen( name ) * BIGCHAR_WIDTH), y, name, 0.5 );
	} else {
		CG_DrawBigString( (x + size / 2)  - ( Q_PrintStrlen( name ) * BIGCHAR_WIDTH) / 2, y, name, 0.5 );
	}
#endif

	return y + BIGCHAR_HEIGHT + 2;
#endif
}

/*
==================
CG_DrawSnapshot
==================
*/
static float CG_DrawSnapshot( float y ) {
	char		*s;
	float w, x;
	float scale;
	vec4_t color;
	int align;
	const fontInfo_t *font;

	if (*cg_drawSnapshotFont.string) {
		font = &cgs.media.snapshotFont;
	} else {
		font = &cgDC.Assets.textFont;
	}
	if (*cg_drawSnapshotColor.string) {
		SC_Vec3ColorFromCvar(color, &cg_drawSnapshotColor);
	} else {
		Vector4Set(color, 1.0, 1.0, 1.0, 1.0);
	}
	color[3] = (float)cg_drawSnapshotAlpha.integer / 255.0;

	scale = cg_drawSnapshotScale.value;
	align = cg_drawSnapshotAlign.integer;
	QLWideScreen = cg_drawSnapshotWideScreen.integer;

	s = va( "time:%d snap:%i cmd:%i", cg.snap->serverTime,
		cg.latestSnapshotNum, cgs.serverCommandSequence );
	//w = CG_DrawStrlen( s, &cgs.media.bigchar );
	w = CG_Text_Width(s, scale, 0, &cgs.media.snapshotFont);

	if (cg_drawSnapshotX.string[0] != '\0') {
		x = cg_drawSnapshotX.value;
	} else {
		x = 635;
	}

	if (align == 1) {
		x -= w / 2;
	} else if (align == 2) {
		x -= w;
	}

	if (cg_drawSnapshotY.string[0] != '\0') {
		y = cg_drawSnapshotY.value;
	} else {
		y = y + 2;
	}


	//Com_Printf("draw snapshot %d %d\n", x, (int)y);
	//CG_DrawBigString( x, y, s, 1.0F);
	CG_Text_Paint_Bottom(x, y, scale, color, s, 0, 0, cg_drawSnapshotStyle.integer, font);

	//return y - 2 + BIGCHAR_HEIGHT + 4;
	return y - 2 + CG_Text_Height(s, scale, 0, &cgs.media.snapshotFont) + 4;
}

/*
==================
CG_DrawFPS
==================
*/
#define	FPS_FRAMES	4
static float CG_DrawFPS( float y ) {
	char		*s;
	float h;
	float		scale;
	static int	previousTimes[FPS_FRAMES];
	static int	index = 0;
	int		i, total;
	int		fps;
	double fpsf;
	static	int	previous;
	int		t, frameTime;
	static double lastFtime = 0;
	float x;
	int align;
	vec4_t color;
	const fontInfo_t *font;
	rectDef_t rect;

	if (*cg_drawFPSFont.string) {
		font = &cgs.media.fpsFont;
	} else {
		font = &cgDC.Assets.textFont;
	}
	QLWideScreen = cg_drawFPSWideScreen.integer;

	// don't use serverTime, because that will be drifting to
	// correct for internet lag changes, timescales, timedemos, etc
	t = trap_Milliseconds();
	frameTime = t - previous;
	previous = t;

	h = 0;
	scale = 0.0f;

	previousTimes[index % FPS_FRAMES] = frameTime;
	index++;
	if ( index > FPS_FRAMES ) {
		// average multiple frames together to smooth changes out a bit
		total = 0;
		for ( i = 0 ; i < FPS_FRAMES ; i++ ) {
			total += previousTimes[i];
		}
		if ( !total ) {
			total = 1;
		}
		fps = 1000 * FPS_FRAMES / total;

		if (cg_drawFPS.integer == 3  &&  frameTime > 0) {
			fps = 1000 / frameTime;
		}

		if (cg_drawFPS.integer == 2) {
			if (cg.ftime <= lastFtime) {
				fpsf = fps;
			} else {
				fpsf = 1000.0 / (cg.ftime - lastFtime);
			}
			if (cg_drawFPSNoText.integer) {
				s = va("%.3f", fpsf);
			} else {
				s = va( "%.3ffps", fpsf);
			}
		} else {
			if (cg_drawFPSNoText.integer) {
				s = va("%d", fps);
			} else {
				s = va( "%ifps", fps );
			}
		}

		scale = cg_drawFPSScale.value;
		h = CG_Text_Height(s, scale, 0, font);

		align = cg_drawFPSAlign.integer;

		x = cg_drawFPSX.value;

#if 0
		if (align == 1) {
			x -= w / 2;
		} else if (align == 2) {
			x -= w;
		}
#endif
		if (cg_drawFPSY.string[0] != '\0') {
			y = cg_drawFPSY.value;
		} else {
			y = y + 2;
		}

		SC_Vec3ColorFromCvar(color, &cg_drawFPSColor);
		color[3] = (float)cg_drawFPSAlpha.integer / 255.0;
		memset(&rect, 0, sizeof(rectDef_t));
		rect.x = x;
		rect.y = y + h + (12.0f *  scale);

		//DebugFont = qtrue;
		CG_Text_Paint_Align(&rect, scale, color, s, 0, 0, cg_drawFPSStyle.integer, font, align);
		//DebugFont = qfalse;
	}

	lastFtime = cg.ftime;

	//Com_Printf("fps height: %f + (12.0f * %f : %f)\n", h, scale, 12.0f * scale);
	return y + h + (12.0f * scale);
	//return y + h + 4;
}

static float CG_DrawRedArmorTimer (float x, float y, int cgtime, int ourClientNum, qboolean useTextColor, int align, qboolean useIcon, float iconX, float iconY, float iconSize, vec4_t color, float scale, const fontInfo_t *font, int textStyle, int spacing)
{
	int i;
	timedItem_t *titem;
	int t;
	int ts;
	int pickupTime;
	const char *s;
	float w;
	float xAlign;

	if (cgs.cpma  &&  cgs.gametype == GT_NTF) {
		return y;
	}

	for (i = 0;  i < cg.numRedArmors;  i++) {
		titem = &cg.redArmors[i];

		if (titem->specPickupTime > titem->pickupTime) {
			titem->clientNum = -1;
			pickupTime = titem->specPickupTime;
		} else {
			pickupTime = titem->pickupTime;
		}

		t = ((pickupTime / 1000) + titem->respawnLength) - (cgtime / 1000);
		if (t < -5) {
			titem->clientNum = -1;
		}

		if (t < 0)
			ts = 0;
		else
			ts = t;

		if (titem->specPickupTime  &&  titem->specPickupTime >= titem->pickupTime  &&  t < 0) {
			//Com_Printf("%d:  %d > %d\n", cg.time, titem->specPickupTime, titem->pickupTime);
			t = 0;
			ts = 0;
		}

		if (t < -5  &&  cg_drawClientItemTimer.integer == 1) {
			s = "^1  -";
		} else if (titem->clientNum == ourClientNum  &&  cg_drawClientItemTimer.integer == 1) {
			s = va("^1* %d", ts);
		} else {
			s = va("^1  %d", ts);
		}

		if (useTextColor) {
			s += 2;
		}

		w = CG_Text_Width(s, scale, 0, font);
		//h = CG_Text_Height(s, scale, 0, font);

		xAlign = x;
		if (align == 1) {
			xAlign -= (w / 2);
		} else if (align == 2) {
			xAlign -= w;
		}

		CG_Text_Paint_Bottom(xAlign, y, scale, color, s, 0, 0, textStyle, font);
		if (useIcon) {
			CG_DrawPic(x + iconX, y + iconY, iconSize, iconSize, cgs.media.redArmorIcon);
		}

		y += spacing;
	}

	return y;
}

static float CG_DrawYellowArmorTimer (float x, float y, int cgtime, int ourClientNum, qboolean useTextColor, int align, qboolean useIcon, float iconX, float iconY, float iconSize, vec4_t color, float scale, const fontInfo_t *font, int textStyle, int spacing)
{
	int i;
	timedItem_t *titem;
	int t;
	int ts;
	int pickupTime;
	const char *s;
	float w;
	float xAlign;

	if (cgs.cpma  &&  cgs.gametype == GT_NTF) {
		return y;
	}

	for (i = 0;  i < cg.numYellowArmors;  i++) {
		titem = &cg.yellowArmors[i];

		if (titem->specPickupTime > titem->pickupTime) {
			titem->clientNum = -1;
			pickupTime = titem->specPickupTime;
		} else {
			pickupTime = titem->pickupTime;
		}

		t = ((pickupTime / 1000) + titem->respawnLength) - (cgtime / 1000);
		if (t < -5) {
			titem->clientNum = -1;
		}

		if (t < 0)
			ts = 0;
		else
			ts = t;

		if (titem->specPickupTime  &&  titem->specPickupTime >= titem->pickupTime  &&  t < 0) {
			t = 0;
			ts = 0;
		}

		if (t < -5  &&  cg_drawClientItemTimer.integer == 1) {
			s = "^3  -";
		} else if (titem->clientNum == ourClientNum  &&  cg_drawClientItemTimer.integer == 1) {
			s = va("^3* %d", ts);
		} else {
			s = va("^3  %d", ts);
		}

		if (useTextColor) {
			s += 2;
		}

		w = CG_Text_Width(s, scale, 0, font);
		//h = CG_Text_Height(s, scale, 0, font);

		xAlign = x;
		if (align == 1) {
			xAlign -= (w / 2);
		} else if (align == 2) {
			xAlign -= w;
		}

		CG_Text_Paint_Bottom(xAlign, y, scale, color, s, 0, 0, textStyle, font);
		if (useIcon) {
			CG_DrawPic(x + iconX, y + iconY, iconSize, iconSize, cgs.media.yellowArmorIcon);
		}

		y += spacing;
	}

	return y;
}

static float CG_DrawGreenArmorTimer (float x, float y, int cgtime, int ourClientNum, qboolean useTextColor, int align, qboolean useIcon, float iconX, float iconY, float iconSize, vec4_t color, float scale, const fontInfo_t *font, int textStyle, int spacing)
{
	int i;
	timedItem_t *titem;
	int t;
	int ts;
	int pickupTime;
	const char *s;
	float w;
	float xAlign;

	if (cgs.cpma  &&  cgs.gametype == GT_NTF) {
		return y;
	}

	for (i = 0;  i < cg.numGreenArmors;  i++) {
		titem = &cg.greenArmors[i];

		if (titem->specPickupTime > titem->pickupTime) {
			titem->clientNum = -1;
			pickupTime = titem->specPickupTime;
		} else {
			pickupTime = titem->pickupTime;
		}

		t = ((pickupTime / 1000) + titem->respawnLength) - (cgtime / 1000);
		if (t < -5) {
			titem->clientNum = -1;
		}

		if (t < 0)
			ts = 0;
		else
			ts = t;

		if (titem->specPickupTime  &&  titem->specPickupTime >= titem->pickupTime  &&  t < 0) {
			t = 0;
			ts = 0;
		}

		if (t < -5  &&  cg_drawClientItemTimer.integer == 1) {
			s = "^2  -";
		} else if (titem->clientNum == ourClientNum  &&  cg_drawClientItemTimer.integer == 1) {
			s = va("^2* %d", ts);
		} else {
			s = va("^2  %d", ts);
		}

		if (useTextColor) {
			s += 2;
		}

		w = CG_Text_Width(s, scale, 0, font);
		//h = CG_Text_Height(s, scale, 0, font);

		xAlign = x;
		if (align == 1) {
			xAlign -= (w / 2);
		} else if (align == 2) {
			xAlign -= w;
		}

		CG_Text_Paint_Bottom(xAlign, y, scale, color, s, 0, 0, textStyle, font);
		if (useIcon) {
			CG_DrawPic(x + iconX, y + iconY, iconSize, iconSize, cgs.media.greenArmorIcon);
		}

		y += spacing;
	}

	return y;
}

static float CG_DrawMegaHealthTimer (float x, float y, int cgtime, int ourClientNum, qboolean useTextColor, int align, qboolean useIcon, float iconX, float iconY, float iconSize, vec4_t color, float scale, const fontInfo_t *font, int textStyle, int spacing)
{
	int i;
	timedItem_t *titem;
	int t;
	int ts;
	int pickupTime;
	const char *s;
	float w;
	float xAlign;
	qboolean useCpmWearOff;

	useCpmWearOff = qfalse;
	if (cg_drawClientItemTimerForceMegaHealthWearOff.integer == -1) {
		// -1 : wear off disabled even if wear off game mode detected
		useCpmWearOff = qfalse;
	} else if (cg_drawClientItemTimerForceMegaHealthWearOff.integer == 0  &&  cgs.cpm) {
		// based on detected game mode
		useCpmWearOff = qtrue;
	} else if (cg_drawClientItemTimerForceMegaHealthWearOff.integer == 1) {
		// forced regardless of detected game mode
		useCpmWearOff = qtrue;
	}

	for (i = 0;  i < cg.numMegaHealths;  i++) {
		titem = &cg.megaHealths[i];

		//Com_Printf("^6 mh client %d  pickupTime %d  countDownTrigger %d  specPickupTime %d\n", titem->clientNum, titem->pickupTime, titem->countDownTrigger, titem->specPickupTime);

		if (titem->specPickupTime > titem->pickupTime) {  //FIXME cpm
			titem->clientNum = -1;
			pickupTime = titem->specPickupTime;
		} else {
			pickupTime = titem->pickupTime;
		}

		t = ((pickupTime / 1000) + titem->respawnLength) - (cgtime / 1000);
		//Com_Printf("t %d\n", t);
		if (t < -5  &&  !useCpmWearOff) {
			titem->clientNum = -1;
		}
		if (t < 0)
			ts = 0;
		else
			ts = t;

		if (titem->specPickupTime  &&  titem->specPickupTime >= titem->pickupTime  &&  t < 0) {
			t = 0;
			ts = 0;
		}

		// mega health wear off time
		if (useCpmWearOff) {
			if ((CG_IsCpmaMvd()  ||  titem->clientNum == cg.snap->ps.clientNum)  &&  titem->countDownTrigger >= 0) {
				//FIXME always 20 for mega ??
				t = ((titem->countDownTrigger / 1000) + 20) - (cgtime / 1000);
				if (t < 0) {
					ts = 0;
				} else {
					ts = t;
				}
			} else {
				t = -999;
				ts = 0;
			}

			//Com_Printf("t %d  ts %d  trigger %d client %d\n", t, ts, titem->countDownTrigger, titem->clientNum);
		}

		if (t < -5  &&  cg_drawClientItemTimer.integer == 1) {
			s = "^4  -";
		} else if (titem->clientNum == ourClientNum  &&  cg_drawClientItemTimer.integer == 1) {
			//Com_Printf("%d == %d\n", titem->clientNum, ourClientNum);
			s = va("^4* %d", ts);
		} else {
			s = va("^4  %d", ts);
		}

		if (useTextColor) {
			s += 2;
		}

		w = CG_Text_Width(s, scale, 0, font);
		//h = CG_Text_Height(s, scale, 0, font);

		xAlign = x;
		if (align == 1) {
			xAlign -= (w / 2);
		} else if (align == 2) {
			xAlign -= w;
		}

		CG_Text_Paint_Bottom(xAlign, y, scale, color, s, 0, 0, textStyle, font);

		if (useIcon) {
			CG_DrawPic(x + iconX, y + iconY, iconSize, iconSize, cgs.media.megaHealthIcon);
		}
		;
		y += spacing;
	}

	return y;
}

static float CG_DrawQuadDamageTimer (float x, float y, int cgtime, int ourClientNum, qboolean useTextColor, int align, qboolean useIcon, float iconX, float iconY, float iconSize, vec4_t color, float scale, const fontInfo_t *font, int textStyle, int spacing)
{
	int i;
	int t;
	const char *s;
	float w;
	float xAlign;

	for (i = 0;  i < cg.numQuads;  i++) {
		t = ((cg.quads[i].pickupTime / 1000) + cg.quads[i].respawnLength) - (cgtime / 1000);
		if (t < 0)
			t = 0;

		s = va("^5  %d", t);
		if (useTextColor) {
			s += 2;
		}

		w = CG_Text_Width(s, scale, 0, font);
		//h = CG_Text_Height(s, scale, 0, font);

		xAlign = x;
		if (align == 1) {
			xAlign -= (w / 2);
		} else if (align == 2) {
			xAlign -= w;
		}

		CG_Text_Paint_Bottom(xAlign, y, scale, color, s, 0, 0, textStyle, font);
		if (useIcon) {
			CG_DrawPic(x + iconX, y + iconY, iconSize, iconSize, cgs.media.quadIcon);
		}

		y += spacing;
	}

	return y;
}

static float CG_DrawBattleSuitTimer (float x, float y, int cgtime, int ourClientNum, qboolean useTextColor, int align, qboolean useIcon, float iconX, float iconY, float iconSize, vec4_t color, float scale, const fontInfo_t *font, int textStyle, int spacing)
{
	int i;
	int t;
	const char *s;
	float w;
	float xAlign;

	for (i = 0;  i < cg.numBattleSuits;  i++) {
		t = ((cg.battleSuits[i].pickupTime / 1000) + cg.battleSuits[i].respawnLength) - (cgtime / 1000);
		if (t < 0)
			t = 0;

		s = va("^7  %d", t);
		if (useTextColor) {
			s += 2;
		}

		w = CG_Text_Width(s, scale, 0, font);
		//h = CG_Text_Height(s, scale, 0, font);

		xAlign = x;
		if (align == 1) {
			xAlign -= (w / 2);
		} else if (align == 2) {
			xAlign -= w;
		}

		CG_Text_Paint_Bottom(xAlign, y, scale, color, s, 0, 0, textStyle, font);
		if (useIcon) {
			CG_DrawPic(x + iconX, y + iconY, iconSize, iconSize, cgs.media.battleSuitIcon);
		}

		y += spacing;
	}

	return y;
}

static void CG_DrawClientItemTimer (void)
{
	vec4_t color;
	int ourClientNum;
	int cgtime;
	float x;
	float y;
	float scale;
	int alpha;
	int textStyle;
	const fontInfo_t *font;
	int spacing;
	int align;
	float iconSize;
	qboolean useIcon;
	float iconX;
	float iconY;
	qboolean useTextColor;
	qboolean shownMH, shownRA, shownYA, shownGA, shownQuad, shownBS;
	const char *str;

	if (cgs.gametype == GT_CA  ||  cgs.gametype == GT_DOMINATION  ||  cgs.gametype == GT_CTFS  ||  cgs.gametype == GT_RED_ROVER  ||  cgs.gametype == GT_RACE) {
		return;
	}

	x = cg_drawClientItemTimerX.value;
	y = cg_drawClientItemTimerY.value;

	scale = cg_drawClientItemTimerScale.value;
	alpha = (float)cg_drawClientItemTimerAlpha.integer / 255.0;
	textStyle = cg_drawClientItemTimerStyle.integer;
	QLWideScreen = cg_drawClientItemTimerWideScreen.integer;

	if (*cg_drawClientItemTimerFont.string) {
		font = &cgs.media.clientItemTimerFont;
	} else {
		font = &cgDC.Assets.textFont;
	}

	if (cg_drawClientItemTimerIcon.integer == 0) {
		useIcon = qfalse;
	} else {
		useIcon = qtrue;
	}
	iconSize = cg_drawClientItemTimerIconSize.value;
	iconX = cg_drawClientItemTimerIconXoffset.value;
	iconY = cg_drawClientItemTimerIconYoffset.value;

	align = cg_drawClientItemTimerAlign.integer;

	if (*cg_drawClientItemTimerSpacing.string) {
		spacing = cg_drawClientItemTimerSpacing.integer;
	} else {
		spacing = CG_Text_Height("0123456789", scale, 0, font);  //FIXME
		if (spacing < 4) {
			spacing++;
		} else {
			spacing = (float)spacing * 1.25;
		}
		spacing++;
	}

	if (wolfcam_following) {  //  &&  !cgs.cpm) {
		ourClientNum = wcg.clientNum;
	} else {
		ourClientNum = cg.snap->ps.clientNum;
	}

	if (CG_GameTimeout()) {
		cgtime = cgs.timeoutBeginTime;
	} else {
		cgtime = cg.time;
	}

	if (*cg_drawClientItemTimerTextColor.string) {
		useTextColor = qtrue;
		SC_Vec4ColorFromCvars(color, &cg_drawClientItemTimerTextColor, &cg_drawClientItemTimerAlpha);
	} else {
		useTextColor = qfalse;
		color[0] = color[1] = color[2] = 1.0f;
	}
	color[3] = alpha;

	shownMH = shownRA = shownYA = shownGA = shownQuad = shownBS = qfalse;

	str = cg_drawClientItemTimerFilter.string;
	while (*str) {
		switch (*str) {
		case 'b':
			if (!shownBS) {
				shownBS = qtrue;
				y = CG_DrawBattleSuitTimer(x, y, cgtime, ourClientNum, useTextColor, align, useIcon, iconX, iconY, iconSize, color, scale, font, textStyle, spacing);
			}
			break;
		case 'g':
			if (!shownGA) {
				shownGA = qtrue;
				y = CG_DrawGreenArmorTimer(x, y, cgtime, ourClientNum, useTextColor, align, useIcon, iconX, iconY, iconSize, color, scale, font, textStyle, spacing);
			}
			break;
		case 'm':
			if (!shownMH) {
				shownMH = qtrue;
				y = CG_DrawMegaHealthTimer(x, y, cgtime, ourClientNum, useTextColor, align, useIcon, iconX, iconY, iconSize, color, scale, font, textStyle, spacing);
			}
			break;
		case 'q':
			if (!shownQuad) {
				shownQuad = qtrue;
				y = CG_DrawQuadDamageTimer(x, y, cgtime, ourClientNum, useTextColor, align, useIcon, iconX, iconY, iconSize, color, scale, font, textStyle, spacing);
			}
			break;
		case 'r':
			if (!shownRA) {
				shownRA = qtrue;
				y = CG_DrawRedArmorTimer(x, y, cgtime, ourClientNum, useTextColor, align, useIcon, iconX, iconY, iconSize, color, scale, font, textStyle, spacing);

			}
			break;
		case 'y':
			if (!shownYA) {
				shownYA = qtrue;
				y = CG_DrawYellowArmorTimer(x, y, cgtime, ourClientNum, useTextColor, align, useIcon, iconX, iconY, iconSize, color, scale, font, textStyle, spacing);
			}
			break;
		default:
			// pass
			break;
		}

		str++;
	}
}

static void CG_DrawFxDebugEntities (void)
{
	QLWideScreen = WIDESCREEN_LEFT;
	CG_DrawSmallString(5, 480 - 70, va("local entities: %d", cg.numLocalEntities), 6.0);
}

static float CG_DrawOrigin (float y)
{
	char *s;
	vec3_t origin;
	vec3_t angles;
	vec4_t color;
	float x, w;
	int style, align;
	float scale;
	const fontInfo_t *font;
	float h;

	if (cg.playPath) {
		VectorCopy(cg.refdef.vieworg, origin);
		VectorCopy(cg.refdefViewAngles, angles);
	} else if (cg.freecam) {
		//VectorCopy(cg.fpos, origin);
		//VectorCopy(cg.fang, angles);
		VectorCopy(cg.refdef.vieworg, origin);
		VectorCopy(cg.refdefViewAngles, angles);
	} else if (wolfcam_following) {
		VectorCopy(cg_entities[wcg.clientNum].currentState.pos.trBase, origin);
		VectorCopy(cg_entities[wcg.clientNum].currentState.apos.trBase, angles);
	} else {
		VectorCopy(cg.snap->ps.origin, origin);
		VectorCopy(cg.snap->ps.viewangles, angles);
	}

	SC_Vec4ColorFromCvars(color, &cg_drawOriginColor, &cg_drawOriginAlpha);

	s = va("%0.2f %0.2f %0.2f  %0.2f %0.2f %0.2f    %f", origin[0], origin[1], origin[2], angles[0], angles[1], angles[2], cg.ftime);
	//Com_Printf("%d  %f  %f\n", cg.time, cg.ftime, cg.foverf);

	align = cg_drawOriginAlign.integer;
	scale = cg_drawOriginScale.value;
	style = cg_drawOriginStyle.integer;
	QLWideScreen = cg_drawOriginWideScreen.integer;

	if (*cg_drawOriginFont.string) {
		font = &cgs.media.originFont;
	} else {
		font = &cgDC.Assets.textFont;
	}

	x = cg_drawOriginX.value;
	y = cg_drawOriginY.value;

	w = CG_Text_Width(s, scale, 0, font);
	h = CG_Text_Height(s, scale, 0, font);
	if (align == 1) {
		x -= w / 2;
	} else if (align == 2) {
		x -= w;
	}
	CG_Text_Paint_Bottom(x, y, scale, color, s, 0, 0, style, font);

	y += h + 4;  //FIXME + 4
	return y - 2;
}

/*
=================
CG_DrawTimer
=================
*/
static float CG_DrawTimer( float y ) {
	char		*s;
	float w;
	int			mins, seconds, tens;
	int			msec;

	if (cg_qlhud.integer) {
		return y;
	}

	msec = CG_GetCurrentTimeWithDirection(NULL);

	seconds = msec / 1000;
	mins = seconds / 60;
	seconds -= mins * 60;
	tens = seconds / 10;
	seconds -= tens * 10;

	if (cg.warmup) {
		switch (cg_warmupTime.integer) {
		case 0:
			s = va("0:00");
			break;
		case 1:
			s = va("%i:%i%i (warmup)", mins, tens, seconds);
			break;
		case 2:
			s = va("0:00 (warmup)");
			break;
		case 3:
			s = va("%i:%i%i", mins, tens, seconds);
			break;
		default:
			s = va("0:00");
			break;
		}
	} else {
		s = va( "%i:%i%i", mins, tens, seconds );
	}

	QLWideScreen = WIDESCREEN_RIGHT;
	w = CG_DrawStrlen( s, &cgs.media.bigchar );
	CG_DrawBigString( 635 - w, y + 2, s, 1.0F);

	return y + BIGCHAR_HEIGHT + 4;
	//return y + BIGCHAR_HEIGHT + 0;
}


/*
=================
CG_DrawTeamOverlay
=================
*/

static int get_player_ping (int clientNum)
{
	int i;

	for (i = 0;  i < cg.numScores;  i++) {
		if (cg.scores[i].client == clientNum) {
			return cg.scores[i].ping;
		}
	}

	return 0;
}

#if 0
static float CG_DrawTeamOverlay_orig( float y, qboolean right, qboolean upper ) {
	int x, w, h, xx;
	int i, j, len;
	const char *p;
	vec4_t		hcolor;
	int pwidth, lwidth;
	int plyrs;
	char st[16];
	const clientInfo_t *ci;
	gitem_t	*item;
	int ret_y, count;

	if ( !cg_drawTeamOverlay.integer ) {
		return y;
	}
	if ( cg.snap->ps.persistant[PERS_TEAM] != TEAM_RED && cg.snap->ps.persistant[PERS_TEAM] != TEAM_BLUE ) {
		return y; // Not on any team
	}

	plyrs = 0;

	// max player name width
	pwidth = 0;
	count = (numSortedTeamPlayers > 8) ? 8 : numSortedTeamPlayers;
	for (i = 0; i < count; i++) {
		ci = cgs.clientinfo + sortedTeamPlayers[i];
		if ( ci->infoValid && ci->team == cg.snap->ps.persistant[PERS_TEAM]) {
			plyrs++;
			//FIXME width
			len = CG_DrawStrlen(ci->name, &cgs.media.tinychar) / TINYCHAR_WIDTH;
			if (len > pwidth)
				pwidth = len;
		}
	}

	if (!plyrs)
		return y;

	if (pwidth > TEAM_OVERLAY_MAXNAME_WIDTH)
		pwidth = TEAM_OVERLAY_MAXNAME_WIDTH;

	// max location name width
	lwidth = 0;
	for (i = 1; i < MAX_LOCATIONS; i++) {
		p = CG_ConfigString(CS_LOCATIONS + i);
		if (p && *p) {
			//FIXME width
			len = CG_DrawStrlen(p, &cgs.media.tinychar) / TINYCHAR_WIDTH;
			if (len > lwidth)
				lwidth = len;
		}
	}

	if (lwidth > TEAM_OVERLAY_MAXLOCATION_WIDTH)
		lwidth = TEAM_OVERLAY_MAXLOCATION_WIDTH;

	w = (pwidth + lwidth + 4 + 7) * TINYCHAR_WIDTH;

	if (1)  //( right )
		x = 640 - w;
	else
		x = 0;

	h = plyrs * TINYCHAR_HEIGHT;

	y = h;

	if ( upper ) {
		ret_y = y + h;
	} else {
		y -= h;
		ret_y = y;
	}

	ret_y = h;
	//y = 0;

	if (cg_drawTeamOverlayX.string[0] != '\0') {
		//x = cg_drawTeamOverlayX.value;
	}
	if (cg_drawTeamOverlayY.string[0] != '\0') {
		//y = cg_drawTeamOverlayY.value;
	}

	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED ) {
		hcolor[0] = 1.0f;
		hcolor[1] = 0.0f;
		hcolor[2] = 0.0f;
		hcolor[3] = 0.33f;
	} else { // if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE )
		hcolor[0] = 0.0f;
		hcolor[1] = 0.0f;
		hcolor[2] = 1.0f;
		hcolor[3] = 0.33f;
	}
	trap_R_SetColor( hcolor );
	CG_DrawPic( x, y, w, h, cgs.media.teamStatusBar );
	trap_R_SetColor( NULL );

	for (i = 0; i < count; i++) {
		ci = cgs.clientinfo + sortedTeamPlayers[i];
		if ( ci->infoValid && ci->team == cg.snap->ps.persistant[PERS_TEAM]) {
			if (get_player_ping(sortedTeamPlayers[i]) < 0) {
				VectorCopy(colorYellow, hcolor);
				hcolor[3] = 1;
			} else {
				hcolor[0] = hcolor[1] = hcolor[2] = hcolor[3] = 1.0;
			}

			xx = x + TINYCHAR_WIDTH;

			CG_DrawStringExt( xx, y,
				ci->name, hcolor, qfalse, qfalse,
							  TINYCHAR_WIDTH, TINYCHAR_HEIGHT, TEAM_OVERLAY_MAXNAME_WIDTH, &cgs.media.tinychar);

			if (lwidth) {
				p = CG_ConfigString(CS_LOCATIONS + ci->location);
				if (!p || !*p)
					p = "unknown";
				//FIXME width
//				len = CG_DrawStrlen(p, &cgs.media.tinychar) / TINYCHAR_WIDTH;
//				if (len > lwidth)
//					len = lwidth;

//				xx = x + TINYCHAR_WIDTH * 2 + TINYCHAR_WIDTH * pwidth +
//					((lwidth/2 - len/2) * TINYCHAR_WIDTH);
				xx = x + TINYCHAR_WIDTH * 2 + TINYCHAR_WIDTH * pwidth;
				CG_DrawStringExt( xx, y,
					p, hcolor, qfalse, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT,
								  TEAM_OVERLAY_MAXLOCATION_WIDTH, &cgs.media.tinychar);
			}

			CG_GetColorForHealth( ci->health, ci->armor, hcolor );

			Com_sprintf (st, sizeof(st), "%3i %3i", ci->health,	ci->armor);

			xx = x + TINYCHAR_WIDTH * 3 +
				TINYCHAR_WIDTH * pwidth + TINYCHAR_WIDTH * lwidth;

			CG_DrawStringExt( xx, y,
				st, hcolor, qfalse, qfalse,
							  TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0, &cgs.media.tinychar );

			// draw weapon icon
			xx += TINYCHAR_WIDTH * 3;

			if ( cg_weapons[ci->curWeapon].weaponIcon ) {
				CG_DrawPic( xx, y, TINYCHAR_WIDTH, TINYCHAR_HEIGHT,
					cg_weapons[ci->curWeapon].weaponIcon );
			} else {
				CG_DrawPic( xx, y, TINYCHAR_WIDTH, TINYCHAR_HEIGHT,
					cgs.media.deferShader );
			}

			// Draw powerup icons
			if (right) {
				xx = x;
			} else {
				xx = x + w - TINYCHAR_WIDTH;
			}
			for (j = 0; j <= PW_NUM_POWERUPS; j++) {
				if (ci->powerups & (1 << j)) {

					item = BG_FindItemForPowerup( j );

					if (item) {
						CG_DrawPic( xx, y, TINYCHAR_WIDTH, TINYCHAR_HEIGHT,
						trap_R_RegisterShader( item->icon ) );
						if (right) {
							xx -= TINYCHAR_WIDTH;
						} else {
							xx += TINYCHAR_WIDTH;
						}
					}
				}
			}

			y += TINYCHAR_HEIGHT;
		}
	}

	return ret_y;
//#endif
}
#endif

static float CG_DrawTeamOverlay (float y, qboolean right, qboolean upper)
{
	float x, w, h, xx;
	int i, j, len;
	const char *p;
	vec4_t		hcolor;
	int pwidth, lwidth;
	int plyrs;
	char st[16];
	const clientInfo_t *ci;
	const gitem_t *item;
	int ret_y, count;
	const fontInfo_t *font;
	float scale;
	float cheight;
	float cwidth;
	float wlimit;
	float alpha;
	int align;
	int picy;
	qboolean q3font = qfalse;
	float lineOffset;
	int ourClientNum;
	int ourTeam;

	if ( !cg_drawTeamOverlay.integer ) {
		return y;
	}

#if 0  // can be a free float spec with protocol 91 or cpma mvd
	//FIXME can skip this check with protocol >= 91 ?
	if ( cg.snap->ps.persistant[PERS_TEAM] != TEAM_RED && cg.snap->ps.persistant[PERS_TEAM] != TEAM_BLUE ) {
		return y; // Not on any team
	}
#endif

	//FIXME quicker short circuit here...


	//SC_Vec4ColorFromCvars(color, &cg_drawTeamOverlayColor, &cg_drawTeamOverlayAlpha);
	scale = cg_drawTeamOverlayScale.value;
	if (*cg_drawTeamOverlayFont.string) {
		font = &cgs.media.teamOverlayFont;
	} else {
		font = &cgDC.Assets.textFont;
	}
	QLWideScreen = cg_drawTeamOverlayWideScreen.integer;

	align = cg_drawTeamOverlayAlign.integer;
	//alpha = cg_drawTeamOverlayAlpha.value / 255.0;
	alpha = 1.0;
	plyrs = 0;

	cheight = CG_Text_Height(" !\"#$%&'()*+-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[]-`abcdefghijklmnopqrstuvwxyz{}|", scale, 0, font);  //FIXME
	//FIXME fontPointSize.integer
	//cheight = cg_drawTeamOverlayPointSize.value * font->glyphScale * scale;
	cwidth = cheight;

	if (cg_drawTeamOverlayLineOffset.value < 0.0f) {
		lineOffset = 0;
	} else {
		lineOffset = cg_drawTeamOverlayLineOffset.value * cheight;
	}

	if (!Q_stricmp(font->name, "q3tiny")) {
		picy = -cheight;
		q3font = qtrue;
	} else if (!Q_stricmp(font->name, "q3small")) {
		picy = -cheight;
		q3font = qtrue;
	} else if (!Q_stricmp(font->name, "q3giant")) {
		picy = -cheight;
		q3font = qtrue;
	} else {
		picy = -(cheight / 4);
	}

	// max player name width
	pwidth = 0;

	// note cg*maxPlayers unlimited or greater than numSortedPlayers only works with quake live protocol >= 91
	if (cg_drawTeamOverlayMaxPlayers.integer < 0) {
		count = numSortedTeamPlayers;
	} else {
		count = (numSortedTeamPlayers > cg_drawTeamOverlayMaxPlayers.integer) ? cg_drawTeamOverlayMaxPlayers.integer : numSortedTeamPlayers;
	}

	if (wolfcam_following) {
		ourClientNum = wcg.clientNum;
		ourTeam = cgs.clientinfo[wcg.clientNum].team;

		if (cgs.realProtocol >= 91) {
			// information not available for other team, but it is available for both teams in free spec
			if (cg.snap->ps.persistant[PERS_TEAM] != TEAM_SPECTATOR) {
				if (ourTeam != cg.snap->ps.persistant[PERS_TEAM]) {
					return y;
				}
			}
		}
	} else {
		ourClientNum = cg.snap->ps.clientNum;
		ourTeam = cg.snap->ps.persistant[PERS_TEAM];
	}

	for (i = 0; i < count; i++) {
		if (cg_selfOnTeamOverlay.integer == 0  &&  ourClientNum == sortedTeamPlayers[i]) {
			continue;
		}

		ci = cgs.clientinfo + sortedTeamPlayers[i];
		if ( ci->infoValid && ci->team == ourTeam ) {
			plyrs++;
			//FIXME width
			//len = CG_DrawStrlen(ci->name, &cgs.media.tinychar) / TINYCHAR_WIDTH;
			len = CG_Text_Width(ci->name, scale, 0, font);
			if (len > pwidth)
				pwidth = len;
		}
	}

	if (!plyrs)
		return y;

	if (pwidth > (TEAM_OVERLAY_MAXNAME_WIDTH * cwidth))  // 8
		pwidth = (TEAM_OVERLAY_MAXNAME_WIDTH * cwidth);

	// max location name width
	lwidth = 0;
	for (i = 1; i < MAX_LOCATIONS; i++) {
		p = CG_ConfigString(CS_LOCATIONS + i);
		if (p && *p) {
			//FIXME width
			//len = CG_DrawStrlen(p, &cgs.media.tinychar) / TINYCHAR_WIDTH;
			len = CG_Text_Width(p, scale, 0, font);
			if (len > lwidth)
				lwidth = len;
		}
	}

	if (lwidth > (TEAM_OVERLAY_MAXLOCATION_WIDTH * cwidth))  // 8
		lwidth = (TEAM_OVERLAY_MAXLOCATION_WIDTH * cwidth);

	//w = (pwidth + lwidth + 4 + 7) * TINYCHAR_WIDTH;  //FIXME width
	//w = (pwidth + lwidth + 4 + 7);  // * cwidth;  //FIXME width
	w = (pwidth + lwidth + 4 * cwidth + 7 * cwidth);  // * cwidth;  //FIXME width
	//Com_Printf("w %d  tc%d\n", w, TINYCHAR_WIDTH);
#if 0
	if ( right )
		x = 640 - w;
	else
		x = 0;
#endif

	//h = plyrs * TINYCHAR_HEIGHT;
	h = plyrs * cheight;
	h += (plyrs - 1) * lineOffset;

	x = cg_drawTeamOverlayX.value;
	if (*cg_drawTeamOverlayY.string) {
		y = cg_drawTeamOverlayY.value;
	}

	if (align == 1) {
		x -= w / 2;
	} else if (align == 2) {
		x -= w;
	}

	if ( upper ) {
		ret_y = y + h + (12.0 * scale);
	} else {
		//y -= h;
		//ret_y = y;
		ret_y = y - h - (12.0 * scale);
	}

	if (q3font) {
		if (upper) {
			y += cheight;
		} else {
			y -= cheight;
		}
	}

	//FIXME
	//y += 2;

	if ( ourTeam == TEAM_RED ) {
		hcolor[0] = 1.0f;
		hcolor[1] = 0.0f;
		hcolor[2] = 0.0f;
		hcolor[3] = 0.33f * alpha;
	} else { // if ( ourTeam == TEAM_BLUE )
		hcolor[0] = 0.0f;
		hcolor[1] = 0.0f;
		hcolor[2] = 1.0f;
		hcolor[3] = 0.33f * alpha;
	}
	trap_R_SetColor( hcolor );
	if (q3font) {
		if (upper) {
			if (q3font) {
				CG_DrawPic(x, y - cheight, w, h, cgs.media.teamStatusBar);
			} else {
				CG_DrawPic(x, y, w, h, cgs.media.teamStatusBar);
			}
		} else {
			if (q3font) {
				CG_DrawPic(x, y - h, w, h, cgs.media.teamStatusBar);
			} else {
				//FIXME double check
				CG_DrawPic(x, y - h, w, h, cgs.media.teamStatusBar);
			}
		}
	} else {
		if (upper) {
			//Com_Printf("sdlkfj\n");
			//CG_DrawPic(x, y + h - cheight, w, h, cgs.media.teamStatusBar);
			CG_DrawPic(x, y - cheight / 4, w, h + cheight / 2, cgs.media.teamStatusBar);
		} else {
			//CG_DrawPic(x, y - h + cheight, w, h, cgs.media.teamStatusBar);
			CG_DrawPic(x, y - h + cheight / 2, w, h + cheight / 2, cgs.media.teamStatusBar);
		}
	}
	trap_R_SetColor( NULL );

	if (!q3font) {
		if (upper) {
			y += 12.0f * scale;
		} else {
			y -= 12.0f * scale;
		}
	}

	for (i = 0; i < count; i++) {
		const entityState_t *es;
		int powerups;
		//FIXME what if it is negative?
		int location;
		int health;
		int armor;
		int curWeapon;

		if (cg_selfOnTeamOverlay.integer == 0  &&  ourClientNum == sortedTeamPlayers[i]) {
			continue;
		}

		es = &cg_entities[sortedTeamPlayers[i]].currentState;
		ci = cgs.clientinfo + sortedTeamPlayers[i];

		powerups = ci->powerups;
		location = ci->location;
		// 2019-04-19 could use painEvent health
		health = ci->health;
		armor = ci->armor;
		curWeapon = ci->curWeapon;

		if (cgs.realProtocol >= 91) {
			powerups = es->powerups;
			location = es->location;

			// these are 16 bit signed values

			health = SIGNED_16_BIT(es->health);
			//FIXME armor not showing correctly when dead specing ca
			armor = SIGNED_16_BIT(es->armor);

			curWeapon = es->weapon;
		} else if (CG_IsCpmaMvd()) {
			int cn;

			cn = sortedTeamPlayers[i];
			powerups = es->powerups;
			location = ((unsigned int)cg.snap->ps.ammo[cn] >> 8) & 0x3f;

			health = ((unsigned int)cg.snap->ps.powerups[cn] >> 24) & 0xff;
			armor = ((unsigned int)cg.snap->ps.powerups[cn] >> 16) & 0xff;

			curWeapon = es->weapon;
		}

		if ( ci->infoValid && ci->team == ourTeam ) {
			//Com_Printf("%d %s\n", i, ci->name);
			if (get_player_ping(sortedTeamPlayers[i]) < 0) {
				VectorCopy(colorYellow, hcolor);
				hcolor[3] = alpha;
#if 0
			} else if (CG_FreezeTagFrozen(sortedTeamPlayers[i])) {  //FIXME
				VectorCopy(colorBlue, hcolor);
				hcolor[3] = alpha;
#endif
			} else {
				hcolor[0] = hcolor[1] = hcolor[2] = 1.0;
				hcolor[3] = alpha;
			}

			//xx = x + TINYCHAR_WIDTH;
			xx = x + cwidth;  //TINYCHAR_WIDTH;

			// +cwidth hack to avoid truncating largest string by one char
			wlimit = xx + pwidth + cwidth;
			//Com_Printf("%d %f\n", i, wlimit);
			CG_Text_Paint_Limit_Bottom(&wlimit, xx, y, scale, hcolor, ci->name, 0, 0, font);  // FIXME width
			//wlimit = xx + pwidth;
			//CG_Text_Paint_Bottom(wlimit, y, scale, hcolor, "test", 0, 0, 0, font);

			if (lwidth) {
				p = CG_ConfigString(CS_LOCATIONS + location);
				if (!p || !*p)
					p = "unknown";
				//FIXME width
				//len = CG_DrawStrlen(p, &cgs.media.tinychar) / TINYCHAR_WIDTH;
				len = CG_Text_Width(p, scale, 0, font);
				if (len > lwidth)
					len = lwidth;

				xx = x + cwidth * 2 + pwidth;
				// +cwidth hack to avoid truncating largest string by one char
				wlimit = xx + lwidth + cwidth;
				CG_Text_Paint_Limit_Bottom(&wlimit, xx, y, scale, hcolor, p, 0, 0, font);
			}

			// health and armor
			if (CG_FreezeTagFrozen(sortedTeamPlayers[i])) {
				//FIXME
				p = "FROZEN";
				Vector4Copy(colorBlue, hcolor);
				//Com_Printf("frozen\n");
				w = CG_Text_Width(p, scale, 0, font);
				//xx = x + w + pwidth + lwidth;
				xx = x + cwidth * 3 + pwidth + lwidth;
				wlimit = xx + w;
				//FIXME style
				//CG_Text_Paint_Bottom(xx, y, scale, hcolor, p, 0, 0, style, font);
				CG_Text_Paint_Limit_Bottom(&wlimit, xx, y, scale, hcolor, p, 0, 0, font);
			} else {
				CG_GetColorForHealth(health, armor, hcolor);
				hcolor[3] = alpha;

				//FIXME ql shows 'DEAD' for <= 0 health
				//Com_sprintf (st, sizeof(st), "%3i %3i", health, armor);
				Com_sprintf(st, sizeof(st), "%3i", health);

				//xx = x + TINYCHAR_WIDTH * 3 + TINYCHAR_WIDTH * pwidth + TINYCHAR_WIDTH * lwidth;
				//xx = x + cwidth * 3 + cwidth * pwidth + cwidth * lwidth;
				xx = x + cwidth * 3 + pwidth + lwidth;
				wlimit = xx + cwidth * 3;
				//CG_DrawStringExt( xx, y, st, hcolor, qfalse, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0, &cgs.media.tinychar );
				//CG_DrawStringExt( xx, y, st, hcolor, qfalse, qfalse, cwidth, cheight, 0, font );
				CG_Text_Paint_Limit_Bottom(&wlimit, xx, y, scale, hcolor, st, 0, 0, font);

				// draw weapon icon
				//xx += TINYCHAR_WIDTH * 3;
				if (q3font) {
					xx += cwidth * 3;
				} else {
					xx += cwidth * 2.5;  // xskip hack
				}

				//FIXME check curWeapon range
				if (curWeapon == WP_NONE) {
					CG_DrawPic(xx, y + picy, cwidth, cheight, cgs.media.deferShader);
				} else if ( cg_weapons[curWeapon].weaponIcon ) {
					CG_DrawPic( xx, y + picy, cwidth, cheight, cg_weapons[curWeapon].weaponIcon );
				} else {
					// no y adjust, looks fine :sdf
					CG_DrawPic( xx, y + picy, cwidth, cheight, cgs.media.deferShader );
				}
				if (q3font) {
					xx += cwidth * 1;
				} else {
					xx += cwidth * 1.5;  // xskip hack
				}
				wlimit = xx + cwidth * 3;
				Com_sprintf (st, sizeof(st), "%3i", armor);
				//trap_R_SetColor(hcolor);
				//CG_DrawStringExt( xx, y, st, hcolor, qfalse, qfalse, cwidth, cheight, 0, font );
				CG_Text_Paint_Limit_Bottom(&wlimit, xx, y, scale, hcolor, st, 0, 0, font);
			}
			// powerups

			// Draw powerup icons
			if (right) {
				xx = x;
			} else {
				//xx = x + w - TINYCHAR_WIDTH;
				xx = x + w - cwidth;
			}

			for (j = 0; j <= PW_NUM_POWERUPS; j++) {
				if (powerups & (1 << j)) {

					item = BG_FindItemForPowerup( j );

					// 2010-08-08 new ql, spawn protection powerup
					if (item &&  !(powerups & PWEX_SPAWNPROTECTION)) {
						//CG_DrawPic( xx, y, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, trap_R_RegisterShader( item->icon ) );
						if (q3font) {
							CG_DrawPic( xx, y - cwidth / 1, cwidth, cheight, trap_R_RegisterShader( item->icon ) );
						} else {
							CG_DrawPic(xx, y - (12.0f * scale), cwidth, cheight, trap_R_RegisterShader(item->icon));
						}
						if (right) {
							//xx -= TINYCHAR_WIDTH;
							xx -= cwidth;
						} else {
							//xx += TINYCHAR_WIDTH;
							xx += cwidth;
						}
					}
				}
			}

			if (upper) {
				y += cheight;
				y += lineOffset;
			} else {
				y -= cheight;
				y -= lineOffset;
			}
		}
	}

	return ret_y;
//#endif
}

/*
=====================
CG_DrawUpperRight

=====================
*/
static void CG_DrawUpperRight( void ) {
	float	y;

	y = 0;

	if ( CG_IsTeamGame(cgs.gametype) && cg_drawTeamOverlay.integer == 1 ) {
		y = CG_DrawTeamOverlay( y, qtrue, qtrue );
	}
	if ( cg_drawSnapshot.integer ) {
		y = CG_DrawSnapshot( y );
	}
	if ( cg_drawFPS.integer ) {
		y = CG_DrawFPS( y );
	}
	if (cg_drawSpeed.integer) {
        y = Wolfcam_DrawSpeed(y);
    }
	y = Wolfcam_DrawMouseSpeed(y);

	if ( cg_drawTimer.integer ) {
		y = CG_DrawTimer( y );
	}
	if ( cg_drawAttacker.integer ) {
		CG_DrawAttacker( y );
	}
}

static void CG_DrawUpperLeft( void ) {
	float	y;

	y = 100;

    if (cg_drawOrigin.integer) {
		y = CG_DrawOrigin(y);
	}
}

/*
===========================================================================================

  LOWER RIGHT CORNER

===========================================================================================
*/

static float CG_DrawPlayersLeft (float y)
{
	const char *s;
	int s1, s2;
	float x, w;
	float y1;

	if (cg_qlhud.integer) {
		return y;
	}

	if (cgs.gametype != GT_CA  &&  cgs.gametype != GT_FREEZETAG  &&  cgs.gametype != GT_RED_ROVER  &&  cgs.gametype != GT_CTFS  &&  cgs.gametype != GT_DOMINATION) {
		return y;
	}

	QLWideScreen = WIDESCREEN_RIGHT;

	if (cgs.gametype == GT_DOMINATION) {
		s1 = cgs.dominationRedPoints;
		s2 = cgs.dominationBluePoints;
	} else {
		s1 = cgs.redPlayersLeft;
		s2 = cgs.bluePlayersLeft;
	}

	y -=  BIGCHAR_HEIGHT + 8;

	y1 = y;

	// draw from the right side to left
	x = 640;
	s = va( "%2i", s2 );
	w = CG_DrawStrlen( s, &cgs.media.bigchar ) + 8;
	x -= w;

	CG_DrawPic(x, y - 4, w, BIGCHAR_HEIGHT + 8, cgs.media.caScoreBlue);
	CG_DrawBigString( x + 4, y, s, 1.0F);

	s = va( "%2i", s1 );
	w = CG_DrawStrlen( s, &cgs.media.bigchar ) + 8;
	x -= w;

	CG_DrawPic(x, y - 4, w, BIGCHAR_HEIGHT + 8, cgs.media.caScoreRed);
	CG_DrawBigString( x + 4, y, s, 1.0F);

	return y1 - 8;
}

/*
=================
CG_DrawScores

Draw the small two score display
=================
*/
#ifndef MISSIONPACK

static float CG_DrawScores( float y ) {
	const char	*s;
	int			s1, s2, score;
	float x, w;
	int			v;
	vec4_t		color;
	float		y1;
	const gitem_t*item;
	int ourTeam;

	if (cg_qlhud.integer) {
		return y;
	}

	QLWideScreen = WIDESCREEN_RIGHT;

	if (wolfcam_following) {
		ourTeam = cgs.clientinfo[wcg.clientNum].team;
	} else {
		ourTeam = cg.snap->ps.persistant[PERS_TEAM];
		if (CG_IsCpmaMvd()) {
			ourTeam = TEAM_SPECTATOR;
		}
	}

	s1 = cgs.scores1;
	s2 = cgs.scores2;

	y -=  BIGCHAR_HEIGHT + 8;

	y1 = y;

	// draw from the right side to left
	if (CG_IsTeamGame(cgs.gametype)) {
		x = 640;
		color[0] = 0.0f;
		color[1] = 0.0f;
		color[2] = 1.0f;
		color[3] = 0.33f;
		s = va( "%2i", s2 );
		w = CG_DrawStrlen( s, &cgs.media.bigchar ) + 8;
		x -= w;
		CG_FillRect( x, y-4,  w, BIGCHAR_HEIGHT+8, color );
		if ( ourTeam == TEAM_BLUE ) {
			CG_DrawPic( x, y-4, w, BIGCHAR_HEIGHT+8, cgs.media.selectShader );
		}
		CG_DrawBigString( x + 4, y, s, 1.0F);

		if (cgs.gametype == GT_CTF  ||  cgs.gametype == GT_CTFS  ||  cgs.gametype == GT_NTF) {
			// Display flag status
			item = BG_FindItemForPowerup( PW_BLUEFLAG );

			if (item) {
				y1 = y - BIGCHAR_HEIGHT - 8;
				if( cgs.blueflag >= 0 && cgs.blueflag <= 2 ) {
					qhandle_t shader;

					switch (cgs.blueflag) {
					default:
					case 0:
						shader = cgs.media.blueFlagAtBaseShader;
						break;
					case 1:
						shader = cgs.media.blueFlagTakenShader;
						break;
					case 2:
						shader = cgs.media.blueFlagDroppedShader;
						break;
					}

					CG_DrawPic( x, y1-4, w, BIGCHAR_HEIGHT+8, shader );

#if 0  // if cg_hudBlueTeamColor implemented for q3 hud, above shader would be neutral flag but colorized
					if (cgs.blueflag == 2) {
						// little white arrow like quake live original icon
						trap_R_SetColor(colorWhite);
						CG_DrawPic( rect->x, rect->y, rect->w, rect->h, cgs.media.flagDroppedArrowShader );
					}
#endif
				}
			}
		}
		color[0] = 1.0f;
		color[1] = 0.0f;
		color[2] = 0.0f;
		color[3] = 0.33f;
		s = va( "%2i", s1 );
		w = CG_DrawStrlen( s, &cgs.media.bigchar ) + 8;
		x -= w;
		CG_FillRect( x, y-4,  w, BIGCHAR_HEIGHT+8, color );
		if ( ourTeam == TEAM_RED ) {
			CG_DrawPic( x, y-4, w, BIGCHAR_HEIGHT+8, cgs.media.selectShader );
		}
		CG_DrawBigString( x + 4, y, s, 1.0F);

		if (cgs.gametype == GT_CTF  ||  cgs.gametype == GT_CTFS  ||  cgs.gametype == GT_NTF) {
			// Display flag status
			item = BG_FindItemForPowerup( PW_REDFLAG );

			if (item) {
				y1 = y - BIGCHAR_HEIGHT - 8;
				if( cgs.redflag >= 0 && cgs.redflag <= 2 ) {
					qhandle_t shader;

					switch (cgs.redflag) {
					default:
					case 0:
						shader = cgs.media.redFlagAtBaseShader;
						break;
					case 1:
						shader = cgs.media.redFlagTakenShader;
						break;
					case 2:
						shader = cgs.media.redFlagDroppedShader;
						break;
					}

					CG_DrawPic( x, y1-4, w, BIGCHAR_HEIGHT+8, shader );

#if 0  // if cg_hudRedTeamColor implemented for q3 hud, above shader would be neutral flag but colorized
					if (cgs.redflag == 2) {
						// little white arrow like quake live original icon
						trap_R_SetColor(colorWhite);
						CG_DrawPic( rect->x, rect->y, rect->w, rect->h, cgs.media.flagDroppedArrowShader );
					}
#endif
				}
			}
		}

#if 1  // MPACK
		if ( cgs.gametype == GT_1FCTF ) {
			// Display flag status
			item = BG_FindItemForPowerup( PW_NEUTRALFLAG );

			if (item) {
				y1 = y - BIGCHAR_HEIGHT - 8;

				if( cgs.flagStatus >= 0 && cgs.flagStatus <= 4 ) {
					if (cgs.realProtocol < 91) {
						qhandle_t shader;

						// like q3
						switch (cgs.flagStatus) {
						default:
						case 0:
							shader = cgs.media.neutralFlagAtBaseShader;
							break;
						case 1:  // unused
							shader = 0;
							break;
						case 2:
							shader = cgs.media.redFlagTakenShader;
							break;
						case 3:
							shader = cgs.media.blueFlagTakenShader;
							break;
						}

						CG_DrawPic( x, y1-4, w, BIGCHAR_HEIGHT+8, shader );
					} else {
						qhandle_t shader;

						// 2018-07-20  now
						// 0:  at base
						// 1:  unused
						// 2:  dropped
						// 3:  red taken
						// 4:  blue taken

						switch (cgs.flagStatus) {
						default:
						case 0:
							shader = cgs.media.neutralFlagAtBaseShader;
							break;
						case 1:  // unused
							shader = 0;
							break;
						case 2:
							shader = cgs.media.neutralFlagDroppedShader;
							break;
						case 3:
							shader = cgs.media.redFlagTakenShader;
							break;
						case 4:
							shader = cgs.media.blueFlagTakenShader;
							break;
						}

						CG_DrawPic( x, y1-4, w, BIGCHAR_HEIGHT+8, shader );
					}

					// debugging
					//CG_Text_Paint(x, y - 10, 0.3, colorGreen, va("%d", cgs.flagStatus), 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgDC.Assets.textFont);
				}
			}
		}
#endif
		//if ( cgs.gametype >= GT_CTF  &&  cgs.gametype < GT_FREEZETAG) {
		if (cgs.gametype == GT_CTF  ||  cgs.gametype == GT_1FCTF  ||  cgs.gametype == GT_OBELISK  ||  cgs.gametype == GT_HARVESTER  ||  cgs.gametype == GT_NTF) {
			v = cgs.capturelimit;
		} else if (cgs.gametype == GT_CA  ||  cgs.gametype == GT_FREEZETAG) {
			v = cgs.roundlimit;
		} else {
			v = cgs.fraglimit;
		}
		if ( v ) {
			s = va( "%2i", v );
			w = CG_DrawStrlen( s, &cgs.media.bigchar ) + 8;
			x -= w;
			CG_DrawBigString( x + 4, y, s, 1.0F);
		}

	} else {
		qboolean spectator;
		int ourClientNum;

		x = 640;

		if (wolfcam_following) {
			ourClientNum = wcg.clientNum;

			//FIXME when will this not be valid?  what games don't update cgs.clientinfo score?
			if (wcg.clientNum == cg.snap->ps.clientNum) {
				score = cg.snap->ps.persistant[PERS_SCORE];
			} else {
				score = cgs.clientinfo[wcg.clientNum].score;
			}
		} else {
			ourClientNum = cg.snap->ps.clientNum;
			// free float spec
			if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR) {
				score = s2;
			} else {
				score = cg.snap->ps.persistant[PERS_SCORE];
			}
		}

		spectator = (cgs.clientinfo[ourClientNum].team == TEAM_SPECTATOR);

		// always show your score in the second box if not in first place
		if (!CG_ScoresEqual(s1, score)) {
			s2 = score;
		}
		if ( s2 != SCORE_NOT_PRESENT ) {
			if (cgs.gametype == GT_RACE) {
				s = va( " %.3fs ", (float)s2 / 1000.0f);
			} else {
				s = va( "%2i", s2 );
			}
			w = CG_DrawStrlen( s, &cgs.media.bigchar ) + 8;
			x -= w;
			if ( !spectator && score == s2 && score != s1 ) {
				color[0] = 1.0f;
				color[1] = 0.0f;
				color[2] = 0.0f;
				color[3] = 0.33f;
				CG_FillRect( x, y-4,  w, BIGCHAR_HEIGHT+8, color );
				CG_DrawPic( x, y-4, w, BIGCHAR_HEIGHT+8, cgs.media.selectShader );
			} else {
				color[0] = 0.5f;
				color[1] = 0.5f;
				color[2] = 0.5f;
				color[3] = 0.33f;
				CG_FillRect( x, y-4,  w, BIGCHAR_HEIGHT+8, color );
			}
			CG_DrawBigString( x + 4, y, s, 1.0F);
		}

		// first place
		if ( s1 != SCORE_NOT_PRESENT ) {
			if (cgs.gametype == GT_RACE) {
				s = va( " %.3fs ", (float)s1 / 1000.0f);
			} else {
				s = va( "%2i", s1 );
			}
			w = CG_DrawStrlen( s, &cgs.media.bigchar ) + 8;
			x -= w;
			if ( !spectator && score == s1 ) {
				color[0] = 0.0f;
				color[1] = 0.0f;
				color[2] = 1.0f;
				color[3] = 0.33f;
				CG_FillRect( x, y-4,  w, BIGCHAR_HEIGHT+8, color );
				CG_DrawPic( x, y-4, w, BIGCHAR_HEIGHT+8, cgs.media.selectShader );
			} else {
				color[0] = 0.5f;
				color[1] = 0.5f;
				color[2] = 0.5f;
				color[3] = 0.33f;
				CG_FillRect( x, y-4,  w, BIGCHAR_HEIGHT+8, color );
			}
			CG_DrawBigString( x + 4, y, s, 1.0F);
		}

		if (!CG_IsDuelGame(cgs.gametype)  &&  cgs.gametype != GT_RACE) {
			if ( cgs.fraglimit ) {
				s = va( "%2i", cgs.fraglimit );
				w = CG_DrawStrlen( s, &cgs.media.bigchar ) + 8;
				x -= w;
				CG_DrawBigString( x + 4, y, s, 1.0F);
			} else  if ( cgs.roundlimit ) {
				s = va( "%2i", cgs.roundlimit );
				w = CG_DrawStrlen( s, &cgs.media.bigchar ) + 8;
				x -= w;
				CG_DrawBigString( x + 4, y, s, 1.0F);
			}
		}
	}

	return y1 - 8;
}
#endif // MISSIONPACK

/*
================
CG_DrawPowerups
================
*/
#ifndef MISSIONPACK
static float CG_DrawPowerups( float y ) {
	int		sorted[MAX_POWERUPS];
	int		sortedTime[MAX_POWERUPS];
	int		i, j, k;
	int		active;
	const playerState_t *ps;
	int		t;
	const gitem_t *item;
	int		x;
	int		color;
	float	size;
	float	f;
	//clientInfo_t *ci;
	const static float colors[2][4] = {
    { 0.2f, 1.0f, 0.2f, 1.0f } ,
    { 1.0f, 0.2f, 0.2f, 1.0f }
  };

	if (cg_qlhud.integer) {  //FIXME
		return y;
	}

	QLWideScreen = WIDESCREEN_RIGHT;

	if (CG_IsCpmaMvd()  &&  !wolfcam_following) {
		return y;
	}

	if (!wolfcam_following) {
		ps = &cg.snap->ps;
		//ci = &cgs.clientinfo[ps->clientNum];

		if ( ps->stats[STAT_HEALTH] <= 0 ) {
			return y;
		}

		// sort the list by time remaining
		active = 0;
		for ( i = 0 ; i < MAX_POWERUPS ; i++ ) {
			if ( !ps->powerups[ i ] ) {
				continue;
			}

			// ZOID--don't draw if the power up has unlimited time
			// This is true of the CTF flags
			if ( ps->powerups[ i ] == INT_MAX ) {
				continue;
			}

			t = ps->powerups[ i ] - cg.time;
			if ( t <= 0 ) {
				continue;
			}

			// insert into the list
			for ( j = 0 ; j < active ; j++ ) {
				if ( sortedTime[j] >= t ) {
					for ( k = active - 1 ; k >= j ; k-- ) {
						sorted[k+1] = sorted[k];
						sortedTime[k+1] = sortedTime[k];
					}
					break;
				}
			}
			sorted[j] = i;
			sortedTime[j] = t;
			active++;
		}
	} else {  // wolfcam_following
		int powerups;
		int *powerupsCheck[] = { &PW_QUAD, &PW_BATTLESUIT, &PW_HASTE, &PW_INVIS, &PW_REGEN, &PW_FLIGHT, &PW_INVULNERABILITY };  // PW_ not static values

		ps = NULL;
		powerups = cg_entities[wcg.clientNum].currentState.powerups;

		if (ARRAY_LEN(powerupsCheck) >= MAX_POWERUPS) {
			Com_Printf("^1invalid number of powerups to check: %d\n", (int)ARRAY_LEN(powerupsCheck));
			return y;
		}

		active = 0;
		for (i = 0;  i < ARRAY_LEN(powerupsCheck);  i++) {
			if (powerups & (1 << *(powerupsCheck[i]))) {
				sorted[active] = *(powerupsCheck[i]);

				//FIXME can we get this?
				sortedTime[active] = -1;
				active++;
			}
		}
	}

	// draw the icons and timers
	x = 640 - ICON_SIZE - CHAR_WIDTH * 2;
	for ( i = 0 ; i < active ; i++ ) {
		item = BG_FindItemForPowerup( sorted[i] );

		// item 0 is ql spawn protection
		if (item  &&  sorted[i] != 0)  {  //!(ci->powerups & PWEX_SPAWNPROTECTION)) {
		  y -= ICON_SIZE;

		  if (wolfcam_following) {
			  t = -1;
			  if (sorted[i] == PW_QUAD) {
				  if (cg.numQuads == 1) {  //FIXME more than one?
					  t = cg.quads[0].pickupTime;
					  sortedTime[i] = (cg.quads[0].pickupTime + (item->quantity * 1000)) - cg.time;
				  }
			  } else if (sorted[i] == PW_BATTLESUIT) {
				  if (cg.numBattleSuits == 1) {  //FIXME more than one?
					  t = cg.battleSuits[0].pickupTime;
					  sortedTime[i] = (cg.battleSuits[0].pickupTime + (item->quantity * 1000)) - cg.time;
				  }
			  }
		  } else {
			  t = ps->powerups[ sorted[i] ];
		  }

		  color = 1;
		  trap_R_SetColor( colors[color] );

		  if (sortedTime[i] >= 0) {
			  CG_DrawField( x, y, 2, sortedTime[ i ] / 1000 );
		  }

		  if (cg_powerupBlink.integer) {
			  if ( t - cg.time >= POWERUP_BLINKS * POWERUP_BLINK_TIME ) {
				  trap_R_SetColor( NULL );
			  } else {
				  vec4_t	modulate;

				  f = (float)( t - cg.time ) / POWERUP_BLINK_TIME;
				  f -= (int)f;
				  modulate[0] = modulate[1] = modulate[2] = modulate[3] = f;
				  trap_R_SetColor( modulate );
			  }
		  } else {
			  trap_R_SetColor(colorWhite);
		  }

		  //Com_Printf("^5active %d sorted %d  time %d\n", cg.powerupActive, sorted[i], cg.powerupTime);

		  // 2018-07-22 doesn't work very well since powerups might only be sent as EV_GLOBAL_ITEM_PICKUP which doesn't set cg.powerupActive or cg.powerupTime.  Doesn't look very good anyways.
		  if ( cg.powerupActive == sorted[i] &&
			  cg.time - cg.powerupTime < PULSE_TIME ) {
			  f = 1.0 - ( ( (float)cg.time - cg.powerupTime ) / PULSE_TIME );
			  size = ICON_SIZE * ( 1.0 + ( PULSE_SCALE - 1.0 ) * f );
			  //Com_Printf("^5pulse size: %f\n", size);
		  } else {
			  size = ICON_SIZE;
		  }

		  //Com_Printf("%d  %s %d\n", trap_R_RegisterShader( item->icon ), item->pickup_name, sorted[i]);
		  CG_DrawPic( 640 - size, y + ICON_SIZE / 2 - size / 2,
			  size, size, trap_R_RegisterShader( item->icon ) );

		  //FIXME ql kill counters
		}
	}
	trap_R_SetColor( NULL );

	return y;
}
#endif // MISSIONPACK

/*
=====================
CG_DrawLowerRight

=====================
*/
#ifndef MISSIONPACK
static void CG_DrawLowerRight( void ) {
	float	y;

	y = 480 - ICON_SIZE;

	if ( CG_IsTeamGame(cgs.gametype) && cg_drawTeamOverlay.integer == 2 ) {
		y = CG_DrawTeamOverlay( y, qtrue, qfalse );
	}
	if (cg_drawScores.integer) {
		y = CG_DrawScores( y );
	}
	if (cg_drawPlayersLeft.integer) {
		y = CG_DrawPlayersLeft(y);
	}
	if (cg_drawPowerups.integer) {
		CG_DrawPowerups( y );
	}
}
#endif // MISSIONPACK

/*
===================
CG_DrawPickupItem
===================
*/
#if 1  //ndef MPACK


//FIXME return proper y value
static float CG_DrawPickupItem (float y)
{
	int value;
	float *fadeColor;
	int iconSize;
	int mins, seconds, tens, msec;
	char timeStr[128];
	float textHeight;
	vec4_t color;
	float x, w;
	int style, align;
	float scale;
	const fontInfo_t *font;
	float *fcolor;
	char pickupCountStr[128];
	int pickupCountStrWidth;
	const char *s;

	if (cg.snap->ps.stats[STAT_HEALTH] <= 0) {
		return y;
	}

	//FIXME wolfcam  -- currently disabled for /follow in cg_event.c EV_ITEM_PICKUP

	//FIXME testing
	//cg.itemPickupTime = cg.time;

	if (cg.time - cg.itemPickupTime > cg_drawItemPickupsTime.integer) {
		return y;
	}

	SC_Vec4ColorFromCvars(color, &cg_drawItemPickupsColor, &cg_drawItemPickupsAlpha);

	if (cg_drawItemPickupsFade.integer) {
		fcolor = CG_FadeColor(cg.itemPickupTime, cg_drawItemPickupsFadeTime.integer);
		if (!fcolor) {
			return y;
		}
		color[3] -= (1.0 - fcolor[3]);
    }

	if (color[3] <= 0.0) {
		return y;
	}

	fadeColor = color;

	align = cg_drawItemPickupsAlign.integer;
	scale = cg_drawItemPickupsScale.value;
	iconSize = cg_drawItemPickupsImageScale.value * (float)ICON_SIZE;
	style = cg_drawItemPickupsStyle.integer;
	QLWideScreen = cg_drawItemPickupsWideScreen.integer;

	if (*cg_drawItemPickupsFont.string) {
		font = &cgs.media.itemPickupsFont;
	} else {
		font = &cgDC.Assets.textFont;
	}

	x = cg_drawItemPickupsX.value;
	y = cg_drawItemPickupsY.value;

	value = cg.itemPickup;
	if (value) {
		msec = cg.itemPickupClockTime;
		seconds = msec / 1000;
		mins = seconds / 60;
		seconds -= mins * 60;
		tens = seconds / 10;
		seconds -= tens * 10;

		Com_sprintf(timeStr, sizeof(timeStr), "%i:%i%i", mins, tens, seconds);
		Com_sprintf(pickupCountStr, sizeof(pickupCountStr), " x%d", cg.itemPickupCount);
		CG_RegisterItemVisuals(value);
		trap_R_SetColor(fadeColor);

		if (cg.itemPickupCount > 1) {
			s = va("%s%s", bg_itemlist[value].pickup_name, pickupCountStr);
			textHeight = CG_Text_Height(s, scale, 0, font);
			pickupCountStrWidth = CG_Text_Width(pickupCountStr, scale, 0, font);
		} else {
			textHeight = CG_Text_Height(bg_itemlist[value].pickup_name, scale, 0, font);
			pickupCountStrWidth = 0;
		}

		if (cg_drawItemPickups.integer == 1) {
			if (align == 1) {
				x -= (iconSize + pickupCountStrWidth) / 2;
			} else if (align == 2) {
				x -= (iconSize + pickupCountStrWidth);
			}
			CG_DrawPic(x, y, iconSize, iconSize, cg_items[value].icon);
			if (cg.itemPickupCount > 1) {
				x += iconSize;
				CG_Text_Paint(x, y + (iconSize/2 - textHeight/2) + textHeight, scale, fadeColor, pickupCountStr, 0, 0, style, font);
			}
		} else if (cg_drawItemPickups.integer == 2) {
			w = CG_Text_Width(bg_itemlist[value].pickup_name, scale, 0, font);
			if (align == 1) {
				x -= (w + pickupCountStrWidth) / 2;
			} else if (align == 2) {
				x -= (w + pickupCountStrWidth);
			}

#if 0
			//FIXME testing
			if (1) {
				vec4_t color = { 1.0f, 0.0f, 0.0f, 0.33f };

				x += 20;
				CG_FillRect(x , y + (iconSize/2 - textHeight/2) + textHeight, w, 20 /*height*/, color);
				
				CG_Text_Paint(x, y + (iconSize/2 - textHeight/2) + textHeight, scale, fadeColor, bg_itemlist[value].pickup_name, 0, 0, style, font);
				x += 1;
				CG_Text_Paint(x + w, y + (iconSize/2 - textHeight/2) + textHeight, scale, fadeColor, bg_itemlist[value].pickup_name, 0, 0, style, font);

				return y;
			}
#endif

			
			CG_Text_Paint(x, y + (iconSize/2 - textHeight/2) + textHeight, scale, fadeColor, bg_itemlist[value].pickup_name, 0, 0, style, font);
			if (cg.itemPickupCount > 1) {
				x += w;
				CG_Text_Paint(x, y + (iconSize/2 - textHeight/2) + textHeight, scale, fadeColor, pickupCountStr, 0, 0, style, font);
			}
		} else if (cg_drawItemPickups.integer == 3) {
			w = CG_Text_Width(bg_itemlist[value].pickup_name, scale, 0, font);
			w += iconSize + 8;  //FIXME + 8, should be based on image scale
			if (align == 1) {
				x -= (w + pickupCountStrWidth) / 2;
			} else if (align == 2) {
				x -= (w + pickupCountStrWidth);
			}

			CG_DrawPic(x, y, iconSize, iconSize, cg_items[value].icon);
			trap_R_SetColor(fadeColor);
			CG_Text_Paint(x + iconSize + 8, y + (iconSize/2 - textHeight/2) + textHeight, scale, fadeColor, bg_itemlist[value].pickup_name, 0, 0, style, font);
			if (cg.itemPickupCount > 1) {
				//FIXME
				x += CG_Text_Width(bg_itemlist[value].pickup_name, scale, 0, font);
				CG_Text_Paint(x + iconSize + 8, y + (iconSize/2 - textHeight/2) + textHeight, scale, fadeColor, pickupCountStr, 0, 0, style, font);
			}
		} else if (cg_drawItemPickups.integer == 4) {
			w = CG_Text_Width(timeStr, scale, 0, font);
			if (align == 1) {
				x -= (w + pickupCountStrWidth) / 2;
			} else if (align == 2) {
				x -= (w + pickupCountStrWidth);
			}

			//CG_Text_Paint(x, y + textHeight, scale, fadeColor, timeStr, 0, 0, style, font);
			CG_Text_Paint(x, y + (iconSize/2 - textHeight/2) + textHeight, scale, fadeColor, timeStr, 0, 0, style, font);
			if (cg.itemPickupCount > 1) {
				x += w;
				//CG_Text_Paint(x, y + textHeight, scale, fadeColor, pickupCountStr, 0, 0, style, font);
				CG_Text_Paint(x, y + (iconSize/2 - textHeight/2) + textHeight, scale, fadeColor, pickupCountStr, 0, 0, style, font);
			}
		} else if (cg_drawItemPickups.integer == 5) {
			w = CG_Text_Width(timeStr, scale, 0, font);
			w += iconSize + 8;  //FIXME + 8
			if (align == 1) {
				x -= (w + pickupCountStrWidth) / 2;
			} else if (align == 2) {
				x -= (w + pickupCountStrWidth);
			}

			CG_Text_Paint(x, y + (iconSize/2 - textHeight/2) + textHeight, scale, fadeColor, timeStr, 0, 0, style, font);
			w = CG_Text_Width(timeStr, scale, 0, font);
			trap_R_SetColor(fadeColor);
			CG_DrawPic(x + w + 8, y, iconSize, iconSize, cg_items[value].icon);
			if (cg.itemPickupCount > 1) {
				x += w + 8 + iconSize + 8;  //FIXME + 8
				CG_Text_Paint(x, y + (iconSize/2 - textHeight/2) + textHeight, scale, fadeColor, pickupCountStr, 0, 0, style, font);
			}
		} else if (cg_drawItemPickups.integer == 6) {
			char buffer[1024];

			Q_strncpyz(buffer, timeStr, sizeof(buffer));

			w = CG_Text_Width(va("%s %s", buffer, bg_itemlist[value].pickup_name), scale, 0, font);
			if (align == 1) {
				x -= (w + pickupCountStrWidth) / 2;
			} else if (align == 2) {
				x -= (w + pickupCountStrWidth);
			}

			CG_Text_Paint(x, y + (iconSize/2 - textHeight/2) + textHeight, scale, fadeColor, va("%s %s", buffer, bg_itemlist[value].pickup_name), 0, 0, style, font);
			if (cg.itemPickupCount > 1) {
				x += w;
				CG_Text_Paint(x, y + (iconSize/2 - textHeight/2) + textHeight, scale, fadeColor, pickupCountStr, 0, 0, style, font);
			}
		} else if (cg_drawItemPickups.integer == 7) {
			w = CG_Text_Width(timeStr, scale, 0, font);
			w += iconSize + 8;  //FIXME + 8
			w += CG_Text_Width(bg_itemlist[value].pickup_name, scale, 0, font) + 8;  //FIXME + 8
			if (align == 1) {
				x -= (w + pickupCountStrWidth) / 2;
			} else if (align == 2) {
				x -= (w + pickupCountStrWidth);
			}

			CG_Text_Paint(x, y + (iconSize/2 - textHeight/2) + textHeight, scale, fadeColor, timeStr, 0, 0, style, font);
			w = CG_Text_Width(timeStr, scale, 0, font);
			trap_R_SetColor(fadeColor);
			CG_DrawPic(x + w + 8, y, iconSize, iconSize, cg_items[value].icon);
			trap_R_SetColor(fadeColor);
			CG_Text_Paint(x + w + iconSize + 16, y + (iconSize/2 - textHeight/2) + textHeight, scale, fadeColor, bg_itemlist[value].pickup_name, 0, 0, style, font);
			if (cg.itemPickupCount > 1) {
				x += w + iconSize + 16 + CG_Text_Width(bg_itemlist[value].pickup_name, scale, 0, font);
				CG_Text_Paint(x, y + (iconSize/2 - textHeight/2) + textHeight, scale, fadeColor, pickupCountStr, 0, 0, style, font);
			}
		}

		trap_R_SetColor(NULL);
	}

	return y + iconSize;  //FIXME test, just guessed for a value
}

#endif // ndef MPACK

/*
=====================
CG_DrawLowerLeft

=====================
*/
#ifndef MISSIONPACK
static void CG_DrawLowerLeft( void ) {
	float	y;

	y = 480 - ICON_SIZE;

	if ( CG_IsTeamGame(cgs.gametype) && cg_drawTeamOverlay.integer == 3 ) {
		y = CG_DrawTeamOverlay( y, qfalse, qfalse );
	}

	if (cg_drawItemPickups.integer) {
		CG_DrawPickupItem( y );
	}
}
#endif // MISSIONPACK


//===========================================================================================

/*
=================
CG_DrawTeamInfo
=================
*/
#ifndef MISSIONPACK
static void CG_DrawTeamInfo( void ) {
	float h;
	int i;
	vec4_t		hcolor;
	int		chatHeight;

#define CHATLOC_Y 420 // bottom end
#define CHATLOC_X 0

	QLWideScreen = WIDESCREEN_LEFT;

	if (cg_teamChatHeight.integer < TEAMCHAT_HEIGHT)
		chatHeight = cg_teamChatHeight.integer;
	else
		chatHeight = TEAMCHAT_HEIGHT;
	if (chatHeight <= 0)
		return; // disabled

	if (cgs.teamLastChatPos != cgs.teamChatPos) {
		if (cg.time - cgs.teamChatMsgTimes[cgs.teamLastChatPos % chatHeight] > cg_teamChatTime.integer) {
			cgs.teamLastChatPos++;
		}

		h = (cgs.teamChatPos - cgs.teamLastChatPos) * TINYCHAR_HEIGHT;

		if ( cgs.clientinfo[cg.clientNum].team == TEAM_RED  &&  cgs.clientinfo[cg.clientNum].infoValid ) {
			hcolor[0] = 1.0f;
			hcolor[1] = 0.0f;
			hcolor[2] = 0.0f;
			hcolor[3] = 0.33f;
		} else if ( cgs.clientinfo[cg.clientNum].team == TEAM_BLUE  &&  cgs.clientinfo[cg.clientNum].infoValid ) {
			hcolor[0] = 0.0f;
			hcolor[1] = 0.0f;
			hcolor[2] = 1.0f;
			hcolor[3] = 0.33f;
		} else {
			hcolor[0] = 0.0f;
			hcolor[1] = 1.0f;
			hcolor[2] = 0.0f;
			hcolor[3] = 0.33f;
		}

		trap_R_SetColor( hcolor );
		CG_DrawPic( CHATLOC_X, CHATLOC_Y - h, 640, h, cgs.media.teamStatusBar );
		trap_R_SetColor( NULL );

		hcolor[0] = hcolor[1] = hcolor[2] = 1.0f;
		hcolor[3] = 1.0f;

		for (i = cgs.teamChatPos - 1; i >= cgs.teamLastChatPos; i--) {
			CG_DrawStringExt( CHATLOC_X + TINYCHAR_WIDTH,
				CHATLOC_Y - (cgs.teamChatPos - i)*TINYCHAR_HEIGHT,
				cgs.teamChatMsgs[i % chatHeight], hcolor, qfalse, qfalse,
							  TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0, &cgs.media.tinychar );
		}
	}
}
#endif // MISSIONPACK

/*
===================
CG_DrawHoldableItem
===================
*/
#ifndef MISSIONPACK
static void CG_DrawHoldableItem( void ) {
	int		value;

	if (cg_qlhud.integer) {
		return;
	}

	// wolfcam_following check done before this function is called


	QLWideScreen = WIDESCREEN_RIGHT;

	value = cg.snap->ps.stats[STAT_HOLDABLE_ITEM];
	if ( value ) {
		CG_RegisterItemVisuals( value );
		CG_DrawPic( 640-ICON_SIZE, (SCREEN_HEIGHT-ICON_SIZE)/2, ICON_SIZE, ICON_SIZE, cg_items[ value ].icon );
	}

}
#endif // MISSIONPACK

#ifdef MISSIONPACK
/*
===================
CG_DrawPersistantPowerup
===================
*/
#if 0 // sos001208 - DEAD
static void CG_DrawPersistantPowerup( void ) {
	int		value;

	QLWideScreen = WIDESCREEN_RIGHT;

	value = cg.snap->ps.stats[STAT_PERSISTANT_POWERUP];
	if ( value ) {
		CG_RegisterItemVisuals( value );
		CG_DrawPic( 640-ICON_SIZE, (SCREEN_HEIGHT-ICON_SIZE)/2 - ICON_SIZE, ICON_SIZE, ICON_SIZE, cg_items[ value ].icon );
	}
}
#endif
#endif // MISSIONPACK

static void CG_RewardAnnouncements (void)
{
	int i;

	if (cg.time - cg.rewardTime >= cg_drawRewardsTime.integer) {
		if (cg.rewardStack > 0) {
			for(i = 0; i < cg.rewardStack; i++) {
				cg.rewardSound[i] = cg.rewardSound[i+1];
				cg.rewardShader[i] = cg.rewardShader[i+1];
				cg.rewardCount[i] = cg.rewardCount[i+1];
			}
			cg.rewardTime = cg.time;
			cg.rewardStack--;
			if (cg_audioAnnouncerRewards.integer) {
				CG_StartLocalSound(cg.rewardSound[0], CHAN_ANNOUNCER);
			}
		} else {
			return;
		}
	}

}

/*
===================
CG_DrawReward
===================
*/
static void CG_DrawReward (void)
{
	//FIXME imageWidth, maxWidth as float
	vec4_t	color;
	int		i, count;
	float x, y;
	char	buf[32];
	const fontInfo_t *font;
	int imageWidth;
	float tw, th;
	int maxWidth;
	float textScale;
	float imageScale;

	if ( !cg_drawRewards.integer ) {
		return;
	}

#if 0
	//FIXME testing
	{
		int testCount = 13;
		cg.rewardTime = cg.time;
		cg.rewardStack = 1;
		cg.rewardSound[0] = cgs.media.excellentSound;
		cg.rewardShader[0] = cgs.media.medalExcellent;
		if (SC_Cvar_Get_Int("debug_rewards")) {
			testCount = SC_Cvar_Get_Int("debug_rewards");
		}
		cg.rewardCount[0] = testCount;
	}
#endif

	CG_FadeColorVec4(color, cg.rewardTime, cg_drawRewardsTime.integer, cg_drawRewardsFadeTime.integer);

	if (!color[3]) {
		return;
	}

	if (!cg_drawRewardsFade.integer) {
		Vector4Set(color, 1.0, 1.0, 1.0, 1.0);
	}

#if 0
	if ( !color[3] ) {
		if (cg.rewardStack > 0) {
			for(i = 0; i < cg.rewardStack; i++) {
				cg.rewardSound[i] = cg.rewardSound[i+1];
				cg.rewardShader[i] = cg.rewardShader[i+1];
				cg.rewardCount[i] = cg.rewardCount[i+1];
			}
			cg.rewardTime = cg.time;
			cg.rewardStack--;
			//color = CG_FadeColor( cg.rewardTime, REWARD_TIME );
			//CG_FadeColor(color, cg.rewardTime, cg_drawRewardsFadeTime.integer);
			CG_FadeColorVec4(color, cg.rewardTime, cg_drawRewardsTime.integer, cg_drawRewardsFadeTime.integer);
			CG_StartLocalSound(cg.rewardSound[0], CHAN_ANNOUNCER);
		} else {
			return;
		}
	}
#endif

	if (*cg_drawRewardsColor.string) {
		SC_Vec3ColorFromCvar(color, &cg_drawRewardsColor);
	}
	if (*cg_drawRewardsAlpha.string) {
		color[3] = (float)cg_drawRewardsAlpha.integer / 255.0 - (1.0 - color[3]);
	}

	trap_R_SetColor( color );

	if (*cg_drawRewardsFont.string) {
		font = &cgs.media.rewardsFont;
	} else {
		font = &cgDC.Assets.textFont;
	}
	QLWideScreen = cg_drawRewardsWideScreen.integer;

	imageScale = cg_drawRewardsImageScale.value;
	imageWidth = (float)ICON_SIZE * imageScale;

	if (cg_drawRewards.integer == 3) {  // cpma style multiple icons
		int mimp, mexc, mgau, mdef, mass, mcap;
		int numMedals;
		int count;
		qboolean skip[MAX_REWARDSTACK];

		memset(skip, 0, sizeof(skip));
		mimp = mexc = mgau = mdef = mass = mcap = 0;
		numMedals = 0;
		for (i = 0;  i < (cg.rewardStack + 1);  i++) {
			qhandle_t s;

			s = cg.rewardShader[i];
			skip[i] = qfalse;

			if (s == cgs.media.medalImpressive) {
				if (mimp) {
					skip[i] = qtrue;
					continue;
				}
				mimp++;
				numMedals++;
			} else if (s == cgs.media.medalExcellent) {
				if (mexc) {
					skip[i] = qtrue;
					continue;
				}
				mexc++;
				numMedals++;
			} else if (s == cgs.media.medalGauntlet) {
				if (mgau) {
					skip[i] = qtrue;
					continue;
				}
				mgau++;
				numMedals++;
			} else if (s == cgs.media.medalDefend) {
				if (mdef) {
					skip[i] = qtrue;
					continue;
				}
				mdef++;
				numMedals++;
			} else if (s == cgs.media.medalAssist) {
				if (mass) {
					skip[i] = qtrue;
					continue;
				}
				mass++;
				numMedals++;
			} else if (s == cgs.media.medalCapture) {
				if (mcap) {
					skip[i] = qtrue;
					continue;
				}
				mcap++;
				numMedals++;
			}
		}

		count = 0;
		for (i = 0;  i < (cg.rewardStack + 1);  i++) {
			if (skip[i]) {
				continue;
			}

			Com_sprintf(buf, sizeof(buf), "%d", cg.rewardCount[i]);
			textScale = cg_drawRewardsScale.value;
			tw = CG_Text_Width(buf, textScale, 0, font);
			th = CG_Text_Height(buf, textScale, 0, font);

			maxWidth = imageWidth;
			if (tw > maxWidth) {
				maxWidth = tw;
			}

			y = 56;
			x = 320;
			if (*cg_drawRewardsY.string) {
				y = cg_drawRewardsY.value;
			}
			if (*cg_drawRewardsX.string) {
				x = cg_drawRewardsX.value;
			}

			if (cg_drawRewardsAlign.integer == 1) {
				x -= (imageWidth * (numMedals)) / 2;
			} else if (cg_drawRewardsAlign.integer == 2) {
				x -= (imageWidth * (numMedals));
			}

			CG_DrawPic(x + ((float)ICON_SIZE * imageScale * count), y, (float)ICON_SIZE * imageScale, (float)ICON_SIZE * imageScale, cg.rewardShader[i] );
			CG_Text_Paint(x + imageWidth / 2 - tw / 2 + ((float)ICON_SIZE * imageScale * count), y + imageWidth + (float)th * 1.5, textScale, color, buf, 0, 0, cg_drawRewardsStyle.integer, font);

			count++;
			if (count >= numMedals) {
				break;
			}
		}

		return;
	}

	if ( cg.rewardCount[0] >= cg_drawRewardsMax.integer ) {
		Com_sprintf(buf, sizeof(buf), "%d", cg.rewardCount[0]);
		textScale = cg_drawRewardsScale.value;
		tw = CG_Text_Width(buf, textScale, 0, font);
        th = CG_Text_Height(buf, textScale, 0, font);

		maxWidth = imageWidth;
		if (tw > maxWidth) {
			maxWidth = tw;
		}

		y = 56;
		x = 320;  //  - ICON_SIZE/2;
		if (*cg_drawRewardsY.string) {
			y = cg_drawRewardsY.value;
		}
		if (*cg_drawRewardsX.string) {
			x = cg_drawRewardsX.value;
		}
		if (cg_drawRewards.integer == 2) {
			// like q3
			if (cg_drawRewardsAlign.integer == 1) {
				x -= imageWidth / 2;
			} else if (cg_drawRewardsAlign.integer == 2) {
				x -= imageWidth;
			}

			CG_DrawPic( x, y, (float)ICON_SIZE * imageScale, (float)ICON_SIZE * imageScale, cg.rewardShader[0] );
			CG_Text_Paint(x + imageWidth / 2 - tw / 2, y + imageWidth + (float)th * 1.5, textScale, color, buf, 0, 0, cg_drawRewardsStyle.integer, font);
		} else if (cg_drawRewards.integer == 1) {
			// like quake live
			if (cg_drawRewardsAlign.integer == 1) {
				x -= (imageWidth * 1.2 + tw) / 2;
			} else if (cg_drawRewardsAlign.integer == 2) {
				x -= (imageWidth * 1.2 + tw);
			}

			CG_DrawPic( x, y, (float)ICON_SIZE * imageScale, (float)ICON_SIZE * imageScale, cg.rewardShader[0] );
			CG_Text_Paint(x + imageWidth * 1.2, y + imageWidth / 2 + th / 2, textScale, color, buf, 0, 0, cg_drawRewardsStyle.integer, font);
		}
	} else {
		count = cg.rewardCount[0];

		y = 56;
		x = 320;  // - count * ICON_SIZE/2;
		if (*cg_drawRewardsY.string) {
			y = cg_drawRewardsY.value;
		}
		if (*cg_drawRewardsX.string) {
			x = cg_drawRewardsX.value;
		}
		if (cg_drawRewardsAlign.integer == 1) {
			x -= (count * (((float)(ICON_SIZE + 4)) * imageScale + 1)) / 2;
		} else if (cg_drawRewardsAlign.integer == 2) {
			x -= (count * (((float)(ICON_SIZE + 4)) * imageScale + 1));
		}

		for ( i = 0 ; i < count ; i++ ) {
			CG_DrawPic( x, y, (float)ICON_SIZE * imageScale, (float)ICON_SIZE * imageScale, cg.rewardShader[0] );
			//FIXME why?  to match ql?
			x += (float)(ICON_SIZE + 4) * imageScale + 1;
		}
	}
	trap_R_SetColor( NULL );
}


/*
===============================================================================

LAGOMETER

===============================================================================
*/


lagometer_t		lagometer;

void CG_ResetLagometer (void)
{
#if 1
	lagometer.snapshotCount = 1;
	lagometer.frameCount = 1;
#endif

#if 0
	lagometer.snapshotCount = 0;
	lagometer.frameCount = 0;
	memset(&lagometer, 0, sizeof(lagometer));
#endif
}

/*
==============
CG_AddLagometerFrameInfo

Adds the current interpolate / extrapolate bar for this frame
==============
*/
void CG_AddLagometerFrameInfo( void ) {
	int			offset;

	if (cg.snap->ps.pm_type == PM_INTERMISSION) {
		return;
	}

	offset = cg.time - cg.latestSnapshotTime;
	lagometer.frameSamples[ lagometer.frameCount & ( LAG_SAMPLES - 1) ] = offset;
	lagometer.frameCount++;
	//Com_Printf("lagometerframeinfo\n");
}

/*
==============
CG_AddLagometerSnapshotInfo

Each time a snapshot is received, log its ping time and
the number of snapshots that were dropped before it.

Pass NULL for a dropped packet.
==============
*/
void CG_AddLagometerSnapshotInfo( snapshot_t *snap ) {
	//Com_Printf("lagometer snap\n");
	// dropped packet
	if ( !snap ) {
		lagometer.snapshotSamples[ lagometer.snapshotCount & ( LAG_SAMPLES - 1) ] = -999;
		lagometer.snapshotCount++;
		//CG_Printf ("[skipnotify]wolcam: missed snapshot %d\n", cg.time);
		return;
	}

	if (snap->ps.pm_type == PM_INTERMISSION) {
		return;
	}

#if 0
	// CHRUKER: b093 - Lagometer ping not correct during demo playback
    if (cg.demoPlayback  &&  snap->ps.pm_type != PM_INTERMISSION) {  //  && snap->ps.clientNum == cg.clientNum) {
		//FIXME checking
		if (1) {  //(cg.snap  &&  !cg.demoSeeking) {
			//int serverFrameTime;

			//serverFrameTime = snap->serverTime - cg.snap->serverTime;
			// - 15   dependent on fps and maxpackets
			//snap->ping = (snap->serverTime - snap->ps.commandTime) - serverFrameTime - 15;
			snap->ping = (snap->serverTime - snap->ps.commandTime) - cg.serverFrameTime - cg.serverFrameTime / 2.0;
			//Com_Printf("stime %d\n", serverFrameTime);
		} else {
			//snap->ping = (snap->serverTime - snap->ps.commandTime);
		}
	}
#endif

	// add this snapshot's info
	lagometer.snapshotSamples[ lagometer.snapshotCount & ( LAG_SAMPLES - 1) ] = snap->ping;
	lagometer.snapshotFlags[ lagometer.snapshotCount & ( LAG_SAMPLES - 1) ] = snap->snapFlags;
	lagometer.snapshotCount++;
}

void CG_LagometerMarkNoMove (void)
{
	if (lagometer.snapshotCount < 1) {
		return;
	}

	lagometer.snapshotSamples[ (lagometer.snapshotCount - 1) & ( LAG_SAMPLES - 1) ] = -1000;
}

/*
==============
CG_DrawDisconnect

Should we draw something differnet for long lag vs no packets?
==============
*/
static void CG_DrawDisconnect( void ) {
	float		x, y;
	int			cmdNum;
	usercmd_t	cmd;
	const char		*s;
	float		w;  // bk010215 - FIXME char message[1024];
	float scale;
	int align;
	int picSize;

	// draw the phone jack if we are completely past our buffers
	cmdNum = trap_GetCurrentCmdNumber() - CMD_BACKUP + 1;
	trap_GetUserCmd( cmdNum, &cmd );
	if ( cmd.serverTime <= cg.snap->ps.commandTime
		|| cmd.serverTime > cg.time ) {	// special check for map_restart // bk 0102165 - FIXME
		return;
	}

	if (cg.demoPlayback  &&  cg.demoStreaming  &&  cg.demoWaitingForStream) {
		// blink
		//if ( ( cg.time >> 9 ) & 1 ) {
		if ((cg.realTime >> 9) & 1) {
			return;
		}

		s = "^3Waiting For Demo Stream";
		QLWideScreen = WIDESCREEN_CENTER;
		w = CG_DrawStrlen( s, &cgs.media.bigchar );
		CG_DrawBigString( 320 - w/2, 50, s, 1.0F);
		return;
	}

    //CG_Printf ("cmd.serverTime:%d  cg.snap->ps.commandTime:%d  cg.time:%d\n", cmd.serverTime, cg.snap->ps.commandTime, cg.time);
    if (cg.demoPlayback  &&  cg_timescale.value != 1.0f) {
		//FIXME take timescale into account
        return;
	}

	//FIXME
	if (cg.demoPlayback) {  //  &&  cg.paused) {
	//if (cg.demoPlayback  &&  SC_Cvar_Get_Int("cl_freezeDemo")) {
		return;
	}

	if (cg.videoRecording) {  //FIXME what about real connections interrupted
		return;
	}

	// also add text in center of screen
	s = "Connection Interrupted"; // bk 010215 - FIXME
	QLWideScreen = WIDESCREEN_CENTER;
	w = CG_DrawStrlen( s, &cgs.media.bigchar );
	CG_DrawBigString( 320 - w/2, 100, s, 1.0F);

	// blink the icon
	if ( ( cg.time >> 9 ) & 1 ) {
		return;
	}

	x = cg_lagometerX.value;
	y = cg_lagometerY.value;

	QLWideScreen = cg_lagometerWideScreen.integer;

	align = cg_lagometerAlign.integer;
	scale = cg_lagometerScale.value;
	picSize = 48.0 * scale;
	if (align == 1) {
		x -= picSize / 2;
	} else if (align == 2) {
		x -= picSize;
	}

	CG_DrawPic(x, y, picSize, picSize, trap_R_RegisterShader("disconnected"));
}


#define	MAX_LAGOMETER_PING	900
#define	MAX_LAGOMETER_RANGE	300

/*
==============
CG_DrawLagometer
==============
*/
static void CG_DrawLagometer( void ) {
	int		a, i;
	float x, y;
	float	v;
	int pval;
	float	ax, ay, aw, ah, mid, range;
	int		color;
	float	vscale;
	int totalSum;
	int tcount;
	int ping;
	vec4_t fontColor;
	float w;
	int fontStyle, align;
	int fontAlign;
	float scale;
	float fontScale;
	const fontInfo_t *font;
	char *s;
	int tx;
	int picSize;
	vec4_t lagometerColor;
	int lagometerAlpha;
	vec4_t tmpColor;

	if ( !cg_lagometer.integer) {  // || cgs.localServer ) {
		CG_DrawDisconnect();
		return;
	}

#if 0
#ifdef MISSIONPACK
	x = 640 - 48;
	y = 480 - 144;
#else
	x = 640 - 48;
	y = 480 - 48;
#endif
#endif

	SC_Vec4ColorFromCvars(fontColor, &cg_lagometerFontColor, &cg_lagometerFontAlpha);
	align = cg_lagometerAlign.integer;
	lagometerAlpha = cg_lagometerAlpha.integer;
	fontAlign = cg_lagometerFontAlign.integer;
	fontScale = cg_lagometerFontScale.value;
	fontStyle = cg_lagometerFontStyle.integer;
	QLWideScreen = cg_lagometerWideScreen.integer;

	if (*cg_lagometerFont.string) {
		font = &cgs.media.lagometerFont;
	} else {
		font = &cgDC.Assets.textFont;
	}
	x = cg_lagometerX.value;
	y = cg_lagometerY.value;


#if 0
	if (cg_qlhud.integer) {  //  &&  !wolfcam_following) {
		y = 336;
	}

	if (cg_lagometerX.string[0] != '\0') {
		x = cg_lagometerX.value;
	}
	if (cg_lagometerY.string[0] != '\0') {
		y = cg_lagometerY.value;
	}
#endif

	scale = cg_lagometerScale.value;
	picSize = 48.0 * scale;
	if (align == 1) {
		x -= picSize / 2;
	} else if (align == 2) {
		x -= picSize;
	}

	//trap_R_SetColor( NULL );
	VectorSet(lagometerColor, 1, 0, 0);
	lagometerColor[3] = (float)lagometerAlpha / 255.0;
	trap_R_SetColor(lagometerColor);

	//CG_DrawPic( x, y, picSize, picSize, cgs.media.lagometerShader );
	CG_DrawPic( x, y, picSize, picSize, cgs.media.teamStatusBar);
	//CG_FillRect(x, y, picSize, picSize, lagometerColor);
	//Com_Printf("color[3] %f\n", lagometerColor[3]);
	//return;

	if (wolfcam_following) { //FIXME checking  ||  cg.clientNum != cg.snap->ps.clientNum)
		if (wclients[wcg.clientNum].serverPingSamples) {
			ping = wclients[wcg.clientNum].serverPingAccum / wclients[wcg.clientNum].serverPingSamples;
			if (cg_lagometerFlash.integer  &&  ping >= cg_lagometerFlashValue.integer) {
				vec4_t color;
				VectorCopy(colorYellow, color);
				color[3] = 0.3 * (float)lagometerAlpha / 255.0;
				CG_FillRect(x, y, picSize, picSize, color);
			}
			//CG_DrawBigString (x, y + 20, va("%d", ping), 1.0);
			s = va("%d", ping);
			w = CG_Text_Width(s, scale * fontScale, 0, font);
			tx = x;
			if (fontAlign == 1) {
				tx += picSize / 2;
				tx -= w / 2;
			} else if (fontAlign == 2) {
				tx += picSize;
				tx -= w;
			}

			if (cg_lagometerAveragePing.integer) {
				CG_Text_Paint_Bottom(tx, y, scale * fontScale, fontColor, s, 0, 0, fontStyle, font);
			}
		}
		return;
	}

	ping = cg.snap->ping;
	if (ping < 0)
		ping = 0;

	//CG_DrawSmallString(x, y, va("%d", ping), 1.0);

	for (totalSum = 0, i = 0, tcount = 0;  i < LAG_SAMPLES  &&  i < lagometer.snapshotCount;  i++) {
		int val;

		val =  lagometer.snapshotSamples[(lagometer.snapshotCount - i) & (LAG_SAMPLES - 1)];
		if (val >= 0) {
			totalSum += val;
			tcount++;
		}
	}

	if (tcount > 0  &&  cg_lagometerFlash.integer  &&  totalSum / tcount >= cg_lagometerFlashValue.integer) {
		vec4_t color;
		VectorCopy(colorYellow, color);
		color[3] = 0.3 * (float)lagometerAlpha / 255.0;

		CG_FillRect(x, y, picSize, picSize, color);
	}

	ax = x;
	ay = y;
	aw = picSize;
	ah = picSize;
	CG_AdjustFrom640( &ax, &ay, &aw, &ah );

	color = -1;
	range = ah / 3;
	mid = ay + range;

	vscale = range / MAX_LAGOMETER_RANGE;

	// draw the frame interpoalte / extrapolate graph
	for ( a = 0 ; a < aw ; a++ ) {
		//vec4_t tmpColor;

		i = ( lagometer.frameCount - 1 - a ) & (LAG_SAMPLES - 1);
		v = lagometer.frameSamples[i];
		v *= vscale;
		if ( v > 0 ) {
			if ( color != 1 ) {
				color = 1;
				Vector4Copy(g_color_table[ColorIndex(COLOR_YELLOW)], tmpColor);
				tmpColor[3] = (float)lagometerAlpha / 255.0;
				//trap_R_SetColor( g_color_table[ColorIndex(COLOR_YELLOW)] );
				trap_R_SetColor(tmpColor);
			}
			if ( v > range ) {
				v = range;
			}
			trap_R_DrawStretchPic ( ax + aw - a, mid - v, 1, v, 0, 0, 0, 0, cgs.media.whiteShader );
		} else if ( v < 0 ) {
			if ( color != 2 ) {
				color = 2;
				//trap_R_SetColor( g_color_table[ColorIndex(COLOR_BLUE)] );
				Vector4Copy(g_color_table[ColorIndex(COLOR_BLUE)], tmpColor);
				tmpColor[3] = (float)lagometerAlpha / 255.0;
				//trap_R_SetColor( g_color_table[ColorIndex(COLOR_YELLOW)] );
				trap_R_SetColor(tmpColor);
			}
			v = -v;
			if ( v > range ) {
				v = range;
			}
			trap_R_DrawStretchPic( ax + aw - a, mid, 1, v, 0, 0, 0, 0, cgs.media.whiteShader );
		}
	}

	// draw the snapshot latency / drop graph
	range = ah / 2;
	vscale = range / MAX_LAGOMETER_PING;

	for ( a = 0 ; a < aw ; a++ ) {
		i = ( lagometer.snapshotCount - 1 - a ) & (LAG_SAMPLES - 1);
		pval = lagometer.snapshotSamples[i];
		v = lagometer.snapshotSamples[i];
		if ( pval > 0 ) {
			if ( lagometer.snapshotFlags[i] & SNAPFLAG_RATE_DELAYED ) {
				if ( color != 5 ) {
					color = 5;	// YELLOW for rate delay
					//trap_R_SetColor( g_color_table[ColorIndex(COLOR_YELLOW)] );
					Vector4Copy(g_color_table[ColorIndex(COLOR_YELLOW)], tmpColor);
					tmpColor[3] = (float)lagometerAlpha / 255.0;
					//trap_R_SetColor( g_color_table[ColorIndex(COLOR_YELLOW)] );
					trap_R_SetColor(tmpColor);
				}
			} else {
				if ( color != 3 ) {
					color = 3;
					//trap_R_SetColor( g_color_table[ColorIndex(COLOR_GREEN)] );
					Vector4Copy(g_color_table[ColorIndex(COLOR_GREEN)], tmpColor);
					tmpColor[3] = (float)lagometerAlpha / 255.0;
					//trap_R_SetColor( g_color_table[ColorIndex(COLOR_YELLOW)] );
					trap_R_SetColor(tmpColor);
				}
			}
			v = v * vscale;
			if ( v > range ) {
				v = range;
			}
			trap_R_DrawStretchPic( ax + aw - a, ay + ah - v, 1, v, 0, 0, 0, 0, cgs.media.whiteShader );
		} else if ( pval == -999) {  //0 ) {
			if ( color != 4 ) {
				color = 4;		// RED for dropped snapshots
				//trap_R_SetColor( g_color_table[ColorIndex(COLOR_RED)] );
				Vector4Copy(g_color_table[ColorIndex(COLOR_RED)], tmpColor);
				tmpColor[3] = (float)lagometerAlpha / 255.0;
				//trap_R_SetColor( g_color_table[ColorIndex(COLOR_YELLOW)] );
				trap_R_SetColor(tmpColor);
			}
			trap_R_DrawStretchPic( ax + aw - a, ay + ah - range, 1, range, 0, 0, 0, 0, cgs.media.whiteShader );
		} else if ( pval == -1000) {  //FIXME dropped and nomove pref
			if ( color != 0 ) {
				color = 0;		// RED for dropped snapshots
				//trap_R_SetColor( g_color_table[ColorIndex(COLOR_RED)] );
				//Vector4Copy(g_color_table[ColorIndex(COLOR_RED)], tmpColor);
				VectorSet(tmpColor, 0, 1, 1);
				tmpColor[3] = (float)lagometerAlpha / 255.0;
				//trap_R_SetColor( g_color_table[ColorIndex(COLOR_YELLOW)] );
				trap_R_SetColor(tmpColor);
			}
			trap_R_DrawStretchPic( ax + aw - a, ay + ah - range, 1, range, 0, 0, 0, 0, cgs.media.whiteShader );
		} else if (pval < 0) {
			//Com_Printf("FIXME pval: %d\n", pval);
		}
	}

	trap_R_SetColor( NULL );

	if ( cg_nopredict.integer || cg_synchronousClients.integer ) {
		//CG_DrawBigString( x, y, "snc", 1.0 );
	}

	//Com_Printf("ping: %d\n", ping);
	tx = x;
	s = va("%d", ping);
	w = CG_Text_Width(s, scale * fontScale, 0, font);
	if (fontAlign == 1) {
		tx += picSize / 2;
		tx -= w / 2;
	} else if (fontAlign == 2) {
		tx += picSize;
		tx -= w;
	}

	//CG_DrawBigString(tx, y, va("%d", ping), 1.0);
	if (cg_lagometerSnapshotPing.integer) {
		CG_Text_Paint_Bottom(tx, y, scale * fontScale, fontColor, s, 0, 0, fontStyle, font);
	}

	tx = x;
	if (tcount > 0) {
		float p;

		p = (float)totalSum / (float)tcount;
		//Com_Printf("ping %f\n", p);
		s = va("%d", (int)floor(p + 0.5));
	} else {
		s = "-";
	}
	w = CG_Text_Width(s, scale * fontScale, 0, font);
	if (fontAlign == 1) {
		tx += picSize / 2;
		tx -= w / 2;
	} else if (fontAlign == 2) {
		tx += picSize;
		tx -= w;
	}

	//CG_DrawBigString (tx, y + 20, va("%d", totalSum / tcount), 1.0);
	if (cg_lagometerAveragePing.integer) {
		CG_Text_Paint_Bottom(tx, y + (20.0 * scale), scale * fontScale, fontColor, s, 0, 0, fontStyle, font);
	}

	CG_DrawDisconnect();
}



/*
===============================================================================

CENTER PRINTING

===============================================================================
*/


/*
==============
CG_CenterPrint

Called for important messages that should stay in the center of the screen
for a few moments
==============
*/
void CG_CenterPrint (const char *str, float y, int charWidth)
{
	char	*s;

	Q_strncpyz( cg.centerPrint, str, sizeof(cg.centerPrint) );

	QLWideScreen = cg_drawCenterPrintWideScreen.integer;

	if (cg_drawCenterPrintY.string[0] != '\0') {
		y = cg_drawCenterPrintY.value;
	}

	//Com_Printf("centerprint %s\n", str);

	cg.centerPrintTime = cg.time;
	cg.centerPrintY = y;
	cg.centerPrintCharWidth = charWidth;

	// count the number of lines for centering
	cg.centerPrintLines = 1;
	s = cg.centerPrint;
	while( *s ) {
		if (*s == '\n')
			cg.centerPrintLines++;
		s++;
	}

	cg.centerPrintIsTokenized = qfalse;
	cg.centerPrintIsFragMessage = qfalse;
}

void CG_CenterPrintFragMessage (const char *str, float y, int charWidth)
{
	CG_CenterPrint(str, y, charWidth);
	cg.centerPrintIsFragMessage = qtrue;
}

static void CG_RoundAnnouncements (void)
{
	int timeLeft;
	int clientNum;
	int ourTeam;

#if 0
	//FIXME cpma and others...
	if (cgs.protocolClass != PROTOCOL_QL) {
		return;
	}
#endif

	if (cg.time <= cgs.timeoutEndTime) {
		if (cg.time - cgs.timeoutBeginTime < 5) {
			//FIXME double check time
			// let centerprint stay a bit
			return;
		} else {
			return;
		}
	}

	if (cgs.gametype == GT_CA  ||  cgs.gametype == GT_FREEZETAG  ||  cgs.gametype == GT_CTFS  ||  cgs.gametype == GT_RED_ROVER) {
		int ival;

		ival = cg.time - atoi(CG_ConfigString(CS_ROUND_TIME));  //FIXME non ql

		if (!cgs.thirtySecondWarningPlayed  &&  cgs.roundStarted  &&  cgs.roundtimelimit  &&  cgs.roundtimelimit - (ival / 1000) <= 30) {
			if (cg_audioAnnouncerTimeLimit.integer) {
				CG_StartLocalSound(cgs.media.thirtySecondWarningSound, CHAN_ANNOUNCER);
			}
			cgs.thirtySecondWarningPlayed = qtrue;
		}
		if (cgs.countDownSoundPlayed == 1  &&  cgs.roundStarted) {
			if (cg_audioAnnouncerRound.integer) {
				if (cgs.gametype == GT_CTFS  &&  cg_attackDefendVoiceStyle.integer == 1) {
					if (wolfcam_following) {
						clientNum = wcg.clientNum;
					} else {
						clientNum = cg.snap->ps.clientNum;
					}
					ourTeam = cgs.clientinfo[clientNum].team;

					if (ourTeam == TEAM_RED  ||  ourTeam == TEAM_BLUE) {
						if ((cgs.roundTurn % 2 == 0  &&  ourTeam == TEAM_RED)  ||  (cgs.roundTurn % 2 != 0  &&  ourTeam == TEAM_BLUE)) {
							CG_StartLocalSound(cgs.media.attackFlagSound, CHAN_ANNOUNCER);
						} else {
							CG_StartLocalSound(cgs.media.defendFlagSound, CHAN_ANNOUNCER);
						}
					}
				} else {
					if (cgs.gametype == GT_RED_ROVER) {

						// 2021-08-06 thought this gong sound was only for
						// 'infected' but seems to be for regular red rover
						// as well
						if (cg_redRoverRoundStartSound.integer) {
							CG_StartLocalSound(cgs.media.kamikazeRespawnSound, CHAN_LOCAL);
						}

						if (cgs.customServerSettings & SERVER_SETTING_INFECTED  &&  cg_allowServerOverride.integer) {
							int ourClientNum;

							if (wolfcam_following) {
								ourClientNum = wcg.clientNum;
							} else {
								ourClientNum = cg.snap->ps.clientNum;
							}
							if (cgs.clientinfo[ourClientNum].team == TEAM_RED) {
								CG_StartLocalSound(cgs.media.countBiteSound, CHAN_ANNOUNCER);
							} else {
								CG_StartLocalSound(cgs.media.countFightSound, CHAN_ANNOUNCER);
							}
						} else {
							CG_StartLocalSound(cgs.media.countFightSound, CHAN_ANNOUNCER);
						}
					} else if (cgs.gametype == GT_RACE) {
						CG_StartLocalSound(cgs.media.countGoSound, CHAN_ANNOUNCER);
					} else {
						//CG_Printf("^2FIGHT sound cg_draw.c\n");
						CG_StartLocalSound(cgs.media.countFightSound, CHAN_ANNOUNCER);
					}
				}
			}
			cgs.countDownSoundPlayed = 0;
		} else  if (!cgs.roundStarted  &&  cgs.roundBeginTime > 0) {
			timeLeft = (cgs.roundBeginTime - cg.time) / 1000;
			timeLeft++;

			// talk at 5, count from 7
			if (timeLeft < 8  &&  timeLeft > 0) {

			}

			if (timeLeft == 5  &&  cgs.countDownSoundPlayed != 5) {
				if (cgs.gametype == GT_CA  ||  cgs.gametype == GT_CTFS  ||  cgs.gametype == GT_RED_ROVER) {
					if (cg_audioAnnouncerRound.integer) {
						CG_StartLocalSound(cgs.media.roundBeginsInSound, CHAN_ANNOUNCER);
					}
				} else if (cgs.gametype == GT_FREEZETAG) {
					if (cg_audioAnnouncerRound.integer) {
						CG_StartLocalSound(cgs.media.countPrepareTeamSound, CHAN_ANNOUNCER);
					}
				}
				cgs.countDownSoundPlayed = 5;
			} else if (timeLeft == 3  &&  cgs.countDownSoundPlayed != 3) {
				if (cg_audioAnnouncerRound.integer) {
					CG_StartLocalSound( cgs.media.count3Sound, CHAN_ANNOUNCER);
				}
				cgs.countDownSoundPlayed = 3;
			} else if (timeLeft == 2  &&  cgs.countDownSoundPlayed != 2) {
				if (cg_audioAnnouncerRound.integer) {
					CG_StartLocalSound(cgs.media.count2Sound, CHAN_ANNOUNCER);
				}
				cgs.countDownSoundPlayed = 2;
			} else if (timeLeft == 1  &&  cgs.countDownSoundPlayed != 1) {
				if (cg_audioAnnouncerRound.integer) {
					CG_StartLocalSound(cgs.media.count1Sound, CHAN_ANNOUNCER);
				}
				cgs.countDownSoundPlayed = 1;
			} else if (cgs.countDownSoundPlayed == 1  &&  cgs.roundStarted) {
				//Com_Printf("xx\n");
			}

			// fight sound done when the server indicates
		}
		return;
	}
}

static void CG_DrawCenter (void)
{
	int timeLeft;

	if (cg.time <= cgs.timeoutEndTime) {
		if (cg.time - cgs.timeoutBeginTime < 5) {
			//FIXME double check time
			// let centerprint stay a bit
			//Com_Printf("stay\n");
			return;
		} else {
			//FIXME dont use centerprint it will hover around after match has restarted
			if (cgs.protocolClass == PROTOCOL_QL) {
				CG_CenterPrint(va("Match resumes in ^5%d ^7seconds", (cgs.timeoutEndTime - cg.time) / 1000 + 1), SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH);
				//if (cgs.timeoutEndTime - cg.snap->serverTime == 0) {
				if (cgs.timeoutEndTime - cg.time == 0) {
					CG_CenterPrint("", SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH);
				}
			}
			cgs.timeoutCountingDown = qtrue;
			return;
		}
	}

	if (cgs.timeoutCountingDown) {
		if (cgs.protocolClass == PROTOCOL_QL) {
			// clear message
			CG_CenterPrint("", SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH);
		}
		cgs.timeoutCountingDown = qfalse;
		return;
	}

	if (cgs.gametype == GT_CA  ||  cgs.gametype == GT_FREEZETAG  ||  cgs.gametype == GT_CTFS  ||  cgs.gametype == GT_RED_ROVER) {
		//int ival;

		//ival = cg.time - atoi(CG_ConfigString(CS_ROUND_TIME));

		//Com_Printf("^5played %d  started %d  roundBeginTime %d\n", cgs.countDownSoundPlayed, cgs.roundStarted, cgs.roundBeginTime);
		if (cgs.countDownSoundPlayed == 0  &&  cgs.roundStarted) {
			//Com_Printf("one ..\n");
			if (cg_drawFightMessage.integer) {
				if (cgs.gametype == GT_RED_ROVER  &&  cgs.customServerSettings & SERVER_SETTING_INFECTED  &&  cg_allowServerOverride.integer) {
					//FIXME here??
					int clientNum;

					if (wolfcam_following) {
						clientNum = wcg.clientNum;
					} else {
						clientNum = cg.snap->ps.clientNum;
					}

					if (cgs.clientinfo[clientNum].team == TEAM_RED) {
						CG_CenterPrint("BITE!", 120, BIGCHAR_WIDTH);
					} else if (cgs.clientinfo[clientNum].team == TEAM_BLUE) {
						CG_CenterPrint("RUN!", 120, BIGCHAR_WIDTH);
					}
				} else if (cgs.gametype == GT_CTFS) {
					int ourTeam;

					if (wolfcam_following) {
						ourTeam = cgs.clientinfo[wcg.clientNum].team;
					} else {
						ourTeam = cgs.clientinfo[cg.snap->ps.clientNum].team;
					}
					if ((ourTeam == TEAM_RED  ||  ourTeam == TEAM_BLUE)  &&  cg_attackDefendVoiceStyle.integer == 1) {
						if ((cgs.roundTurn % 2 == 0  &&  ourTeam == TEAM_RED)  ||  (cgs.roundTurn % 2 != 0  &&  ourTeam == TEAM_BLUE)) {
							CG_CenterPrint("ATTACK THE FLAG!", 120, BIGCHAR_WIDTH);
						} else {
							CG_CenterPrint("DEFEND THE FLAG!", 120, BIGCHAR_WIDTH);
						}
					} else {
						CG_CenterPrint("FIGHT!", SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH);
					}
				} else {
					//CG_Printf("^6FIGHT cg_draw\n");
					CG_CenterPrint("FIGHT!", SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH);
				}
			} else {  // no fight screen message
				CG_CenterPrint("", SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH);
			}

			cgs.countDownSoundPlayed = -999;
		} else  if (!cgs.roundStarted  &&  cgs.roundBeginTime > 0) {
			timeLeft = (cgs.roundBeginTime - cg.time) / 1000;
			timeLeft++;

			// talk at 5, count from 7
			if (timeLeft < 8  &&  timeLeft > 0) {
				//CG_CenterPrint( "No item to use", SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH );
				if (cgs.gametype == GT_CA) {
					CG_CenterPrint(va("Round Begins in:\n%d", timeLeft), SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH);
				} else if (cgs.gametype == GT_FREEZETAG) {
					CG_CenterPrint(va("Get Ready\n%d", timeLeft), SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH);
				} else if (cgs.gametype == GT_CTFS) {
					if (cgs.roundTurn % 2 == 0) {
						CG_CenterPrint(va("Red is on offense\n\n%d", timeLeft), SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH);
					} else {
						CG_CenterPrint(va("Blue is on offense\n\n%d", timeLeft), SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH);
					}
				} else if (cgs.gametype == GT_RED_ROVER) {
					CG_CenterPrint(va("Round Begins in:\n%d", timeLeft), SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH);
				}
			}
		}
		return;
	}
}

// handles both frag and obituary
floatint_t *CG_CreateFragString (qboolean lastFrag, int indexNum, const char *tokenStringOverride)
{
	static floatint_t extString[MAX_STRING_CHARS];
	int i, j, k;
	const char *tokenString;
	int hits;
	int atts;
	float acc;
	const wweaponStats_t *ws;
	int ourClientNum;
	const obituary_t *obituary;

	if (wolfcam_following) {
		ourClientNum = wcg.clientNum;
	} else {
		ourClientNum = cg.snap->ps.clientNum;
	}

	if (lastFrag) {
		if (cg.lastFragMod == MOD_THAW) {
			tokenString = cg_drawFragMessageThawTokens.string;
		} else {
			if (cgs.gametype == GT_FREEZETAG) {
				if (lastFrag  &&  (CG_IsTeamGame(cgs.gametype)  &&  cg.lastFragKillerTeam == cg.lastFragVictimTeam)) {
					tokenString = cg_drawFragMessageFreezeTeamTokens.string;
				} else {
					tokenString = cg_drawFragMessageFreezeTokens.string;
				}
			} else {
				//FIXME ctfs with team damage???
				if (lastFrag  &&  (CG_IsTeamGame(cgs.gametype)  &&  cg.lastFragKillerTeam == cg.lastFragVictimTeam)  &&  (cgs.gametype != GT_CA  &&  cgs.gametype != GT_RED_ROVER  &&  cgs.gametype != GT_DOMINATION  &&  cgs.gametype != GT_CTFS)) {
					tokenString = cg_drawFragMessageTeamTokens.string;
				} else {
					tokenString = cg_drawFragMessageTokens.string;
				}
			}
		}
	} else {
		tokenString = cg_obituaryTokens.string;
	}

	i = (cg.obituaryIndex + indexNum - 1) % MAX_OBITUARIES;
	obituary = &cg.obituaries[i];

	if (tokenStringOverride) {
		tokenString = tokenStringOverride;
	}

	i = 0;
	j = 0;

	while (1) {
		char c;
		const char *s;

		if (i >= strlen(tokenString)) {
			extString[j].i = 0;
			//Com_Printf("ent past  %d > %d\n", i, strlen(tokenString));
			break;
		}

		c = tokenString[i];
		if (!lastFrag) {
			//Com_Printf("tttt   %c\n", c);
		}

		if (c == '\0') {
			extString[j].i = 0;
			break;
		}

		if (c != '%') {
			extString[j].i = (unsigned char)tokenString[i];
			i++;
			j++;
			continue;
		}

		// else it's a token
		i++;
		c = tokenString[i];
		if (!lastFrag) {
			//Com_Printf("token %c\n", c);
		}
		if (c == 'v') {  // name, unedited
			//Com_Printf("got v...\n");
			if (lastFrag) {
				s = cg.lastFragVictimName;
				//m_Printf("last victim name: '%s'\n", s);
			} else {
				s = obituary->victim;
			}
			while (*s) {
				extString[j].i = (unsigned char)s[0];
				j++;
				s++;
			}
		} else if (c == 'V') {
			if (lastFrag) {
				s = cg.lastFragVictimWhiteName;
			} else {
				s = obituary->victimWhiteName;
			}
			while (*s) {
				extString[j].i = (unsigned char)s[0];
				j++;
				s++;
			}
		} else if (c == 'k') {
			if (lastFrag) {
				s = cgs.clientinfo[ourClientNum].name;
			} else {
				if (obituary->killerClientNum == obituary->victimClientNum) {
					s = "";
				} else {
					s = obituary->killer;
				}
			}
			while (*s) {
				extString[j].i = (unsigned char)s[0];
				j++;
				s++;
			}
		} else if (c == 'K') {
			if (lastFrag) {
				s = cgs.clientinfo[ourClientNum].whiteName;
			} else {
				if (obituary->killerClientNum == obituary->victimClientNum) {
					s = "";
				} else {
					s = obituary->killerWhiteName;
				}
			}
			while (*s) {
				extString[j].i = (unsigned char)s[0];
				j++;
				s++;
			}
		} else if (c == 'q') {
			if (lastFrag) {
				s = cg.lastFragq3obitString;
			} else {
				s = obituary->q3obitString;
			}
			while (*s) {
				extString[j].i = (unsigned char)s[0];
				j++;
				s++;
			}
		} else if (c == 'w') {
			//FIXME obits non weapons are going to print "none"
			//s = cg.weaponNamesCvars[cg.lastFragWeapon].string;
			if (lastFrag) {
				if (cg.lastFragWeapon >= 0  &&  cg.lastFragWeapon <= MAX_WEAPONS) {
				s = weapNamesCasual[cg.lastFragWeapon];
				} else {
					Com_Printf("^1CG_CreateFragString invalid last frag weapon %d\n", cg.lastFragWeapon);
					s = "unknown weapon";
				}
			} else {
				if (obituary->weapon >= 0  &&  obituary->weapon <= MAX_WEAPONS) {
					s = weapNamesCasual[obituary->weapon];
				} else {
					Com_Printf("^1CG_CreateFragString invalid obituary weapon %d\n", obituary->weapon);
					s = "unknown weapon";
				}
			}
			while (*s) {
				extString[j].i = (unsigned char)s[0];
				j++;
				s++;
			}
		} else if (c == 'i') {
			qhandle_t icon;

			if (lastFrag) {
				icon = cg_weapons[cg.lastFragWeapon].weaponIcon;
			} else {
				icon = obituary->icon;
			}
			extString[j].i = TEXT_PIC_PAINT_ICON;
			j++;
			extString[j].i = icon;
			j++;
		} else if (c == 'A') {
			// total accuracy
			if (lastFrag) {
				ws = wclients[ourClientNum].lastKillwstats;
			} else {
				ws = wclients[obituary->killerClientNum].lastKillwstats;
			}
			hits = 0;
			atts = 0;
			for (k = 0;  k < WP_NUM_WEAPONS;  k++) {
				if (k != WP_MACHINEGUN  &&  k != WP_LIGHTNING  &&  k != WP_RAILGUN  &&  k != WP_NAILGUN) {
					continue;
				}

				hits += ws[k].hits;
				atts += ws[k].atts;
			}
			if (atts) {
				acc = (float)hits / (float)atts;
			} else {
				acc = 0;
			}

			switch (cg_fragTokenAccuracyStyle.integer) {
			case 1:
				s = va("%d", (int)(acc * 100.0));
				break;
			case 2:
				s = va("%.2f", acc);
				break;
			case 0:
			default:
				s = va("%d%%", (int)(acc * 100.0));
				break;
			}

			//Com_Printf("acc: %s\n", s);
			while (*s) {
				extString[j].i = (unsigned char)s[0];
				j++;
				s++;
			}
		} else if (c == 'a') {
			// kill weapon accuracy
			int weapon;

			if (lastFrag) {
				ws = wclients[ourClientNum].lastKillwstats;
				weapon = wclients[ourClientNum].lastKillWeapon;
			} else {
				ws = wclients[obituary->killerClientNum].lastKillwstats;
				weapon = obituary->weapon;
			}
			hits = 0;
			atts = 0;
			hits = ws[weapon].hits;
			atts = ws[weapon].atts;
			if (atts) {
				acc = (float)hits / (float)atts;
			} else {
				acc = 0;
			}
			//FIXME until acc gets sorted out
			if (weapon != WP_MACHINEGUN  &&  weapon != WP_LIGHTNING  &&  weapon != WP_RAILGUN  &&  weapon != WP_NAILGUN) {
				s = "";
			} else {
				switch (cg_fragTokenAccuracyStyle.integer) {
				case 1:
					s = va("%d", (int)(acc * 100.0));
					break;
				case 2:
					s = va("%.2f", acc);
					break;
				case 0:
				default:
					s = va("%d%%", (int)(acc * 100.0));
					break;
				}
			}
			while (*s) {
				extString[j].i = (unsigned char)s[0];
				j++;
				s++;
			}
		} else if (c == 'D') {  // victim accuracy
			// total accuracy
			if (lastFrag) {
				ws = wclients[cg.lastFragVictim].lastKillwstats;
			} else {
				ws = wclients[obituary->victimClientNum].lastKillwstats;
			}
			hits = 0;
			atts = 0;
			for (k = 0;  k < WP_NUM_WEAPONS;  k++) {
				if (k != WP_MACHINEGUN  &&  k != WP_LIGHTNING  &&  k != WP_RAILGUN  &&  k != WP_NAILGUN) {
					continue;
				}

				hits += ws[k].hits;
				atts += ws[k].atts;
			}
			if (atts) {
				acc = (float)hits / (float)atts;
			} else {
				acc = 0;
			}
			switch (cg_fragTokenAccuracyStyle.integer) {
			case 1:
				s = va("%d", (int)(acc * 100.0));
				break;
			case 2:
				s = va("%.2f", acc);
				break;
			case 0:
			default:
				s = va("%d%%", (int)(acc * 100.0));
				break;
			}
			//Com_Printf("acc: %s\n", s);
			while (*s) {
				extString[j].i = (unsigned char)s[0];
				j++;
				s++;
			}
		} else if (c == 'd') {
			// kill weapon accuracy
			int weapon;

			if (lastFrag) {
				ws = wclients[cg.lastFragVictim].lastKillwstats;
				weapon = wclients[cg.lastFragVictim].lastKillWeapon;
			} else {
				ws = wclients[obituary->victimClientNum].lastKillwstats;
				//weapon = obituary->weapon;
				weapon = wclients[obituary->victimClientNum].lastKillWeapon;  //FIXME don't think this works
			}
			hits = 0;
			atts = 0;
			hits = ws[weapon].hits;
			atts = ws[weapon].atts;
			if (atts) {
				acc = (float)hits / (float)atts;
			} else {
				acc = 0;
			}
			//FIXME until acc gets sorted out
			if (weapon != WP_MACHINEGUN  &&  weapon != WP_LIGHTNING  &&  weapon != WP_RAILGUN  &&  weapon != WP_NAILGUN) {
				s = "";
			} else {
				switch (cg_fragTokenAccuracyStyle.integer) {
				case 1:
					s = va("%d", (int)(acc * 100.0));
					break;
				case 2:
					s = va("%.2f", acc);
					break;
				case 0:
				default:
					s = va("%d%%", (int)(acc * 100.0));
					break;
				}
			}
			while (*s) {
				extString[j].i = (unsigned char)s[0];
				j++;
				s++;
			}
		} else if (c == 'c') {
			char tmpString[9];
			extString[j].i = TEXT_PIC_PAINT_COLOR;
			j++;
			Q_strncpyz(tmpString, tokenString + i + 1, sizeof(tmpString));
			i++;
			i += 7;  // hex color is 8 (0xff00ff) and 'i' will be incremented before fetching next token
			extString[j].i = Com_HexStrToInt(tmpString);
			j++;
		} else if (c == 't') {
			int team;

			if (lastFrag) {
				team = cg.lastFragVictimTeam;
			} else {
				team = obituary->victimTeam;
			}

			extString[j].i = TEXT_PIC_PAINT_COLOR;
			j++;
			if (team == TEAM_RED) {
				extString[j].i = Com_HexStrToInt(cg_textRedTeamColor.string);
			} else if (team == TEAM_BLUE) {
				extString[j].i = Com_HexStrToInt(cg_textBlueTeamColor.string);
			} else {
				extString[j].i = Com_HexStrToInt("0xffffff");
			}
			j++;
		} else if (c == 'T') {
			int team;
			int ourClientNum;

			if (wolfcam_following) {
				ourClientNum = wcg.clientNum;
			} else {
				ourClientNum = cg.snap->ps.clientNum;
			}

			if (lastFrag) {
				team = cgs.clientinfo[ourClientNum].team;
			} else {
				team = obituary->killerTeam;
			}

			extString[j].i = TEXT_PIC_PAINT_COLOR;
			j++;
			if (team == TEAM_RED) {
				extString[j].i = Com_HexStrToInt(cg_textRedTeamColor.string);
			} else if (team == TEAM_BLUE) {
				extString[j].i = Com_HexStrToInt(cg_textBlueTeamColor.string);
			} else {
				extString[j].i = Com_HexStrToInt("0xffffff");
			}
			j++;
		} else if (c == 'f') {
			int team;
			//qboolean suicide = qfalse;  //FIXME suicide doesn't go here, it's for killer
			int ourClientNum;

			if (wolfcam_following) {
				ourClientNum = wcg.clientNum;
			} else {
				ourClientNum = cg.snap->ps.clientNum;
			}
			if (lastFrag) {
				team = cg.lastFragVictimTeam;
				if (cg.lastFragVictim == ourClientNum) {
					//suicide = qtrue;
				}
			} else {
				team = obituary->victimTeam;
				if (obituary->victimClientNum == obituary->killerClientNum) {
					//suicide = qtrue;
				}
			}

			if (CG_IsTeamGame(cgs.gametype)) {
				extString[j].i = TEXT_PIC_PAINT_ICON;
				j++;
				if (0) {  //(suicide) {
					extString[j - 1].i = -1;
					extString[j].i = 0;
				} else if (team == TEAM_RED) {
					extString[j].i = cgs.media.redFlagAtBaseShader;
				} else if (team == TEAM_BLUE) {
					extString[j].i = cgs.media.blueFlagAtBaseShader;
				} else {
					extString[j].i = 0;
				}
			} else {
				extString[j].i = -1;
				j++;
				extString[j].i = 0;
			}
			j++;
		} else if (c == 'F') {
			int team;
			int ourClientNum;
			qboolean suicide = qfalse;

			if (wolfcam_following) {
				ourClientNum = wcg.clientNum;
			} else {
				ourClientNum = cg.snap->ps.clientNum;
			}

			if (lastFrag) {
				team = cgs.clientinfo[ourClientNum].team;
				if (cg.lastFragVictim == ourClientNum) {
					suicide = qtrue;
				}
			} else {
				team = obituary->killerTeam;
				if (obituary->victimClientNum == obituary->killerClientNum) {
					suicide = qtrue;
				}
			}

			if (CG_IsTeamGame(cgs.gametype)) {
				extString[j].i = TEXT_PIC_PAINT_ICON;
				j++;
				if (suicide) {
					extString[j - 1].i = -1;
					extString[j].i = 0;
				} else if (team == TEAM_RED) {
					extString[j].i = cgs.media.redFlagAtBaseShader;
				} else if (team == TEAM_BLUE) {
					extString[j].i = cgs.media.blueFlagAtBaseShader;
				} else {
					extString[j].i = 0;
				}
			} else {
				extString[j].i = -1;
				j++;
				extString[j].i = 0;
			}
			j++;
		} else if (c == 'n') {  // victim's country flag
			qhandle_t icon;
			int clientNum;

			if (lastFrag) {
				clientNum = cg.lastFragVictim;
			} else {
				clientNum = obituary->victimClientNum;
			}

			if (clientNum >= 0  ||  clientNum < MAX_CLIENTS) {
				icon = cgs.clientinfo[clientNum].countryFlag;
				if (icon) {
					extString[j].i = TEXT_PIC_PAINT_ICON;
					j++;
					extString[j].i = icon;
					j++;
				}
			}
		} else if (c == 'N') {  // killer's country flag
			qhandle_t icon;
			int clientNum;

			if (lastFrag) {
				clientNum = cg.lastFragKiller;
			} else {
				clientNum = obituary->killerClientNum;
			}

			if (clientNum >= 0  ||  clientNum < MAX_CLIENTS) {
				icon = cgs.clientinfo[clientNum].countryFlag;
				if (icon) {
					extString[j].i = TEXT_PIC_PAINT_ICON;
					j++;
					extString[j].i = icon;
					j++;
				}
			}
		} else if (c == 'l') {  // victim's clan tag
			if (lastFrag) {
				if (*cgs.clientinfo[cg.lastFragVictim].clanTag) {
					s = va("%s ", cgs.clientinfo[cg.lastFragVictim].clanTag);
				} else {
					s = "";
				}
			} else {
				if (*cgs.clientinfo[obituary->victimClientNum].clanTag) {
					s = va("%s ", cgs.clientinfo[obituary->victimClientNum].clanTag);
				} else {
					s = "";
				}
			}
			while (*s) {
				extString[j].i = (unsigned char)s[0];
				j++;
				s++;
			}
		} else if (c == 'L') {  // victim's clan tag with no color codes
			if (lastFrag) {
				if (*cgs.clientinfo[cg.lastFragVictim].whiteClanTag) {
					s = va("%s ", cgs.clientinfo[cg.lastFragVictim].whiteClanTag);
				} else {
					s = "";
				}
			} else {
				if (*cgs.clientinfo[obituary->victimClientNum].whiteClanTag) {
					s = va("%s ", cgs.clientinfo[obituary->victimClientNum].whiteClanTag);
				} else {
					s = "";
				}
			}
			while (*s) {
				extString[j].i = (unsigned char)s[0];
				j++;
				s++;
			}
		} else if (c == 'x') {  // killer's clan tag
			if (lastFrag) {
				if (*cgs.clientinfo[ourClientNum].clanTag) {
					s = va("%s ", cgs.clientinfo[ourClientNum].clanTag);
				} else {
					s = "";
				}
			} else {
				if (obituary->killerClientNum == obituary->victimClientNum) {
					s = "";
				} else {
					if (*cgs.clientinfo[obituary->killerClientNum].clanTag) {
						s = va("%s ", cgs.clientinfo[obituary->killerClientNum].clanTag);
					} else {
						s = "";
					}
				}
			}
			while (*s) {
				extString[j].i = (unsigned char)s[0];
				j++;
				s++;
			}
		} else if (c == 'X') {  // killer's clan tag with no color codes
			if (lastFrag) {
				if (*cgs.clientinfo[ourClientNum].whiteClanTag) {
					s = va("%s ", cgs.clientinfo[ourClientNum].whiteClanTag);
				} else {
					s = "";
				}
			} else {
				if (obituary->killerClientNum == obituary->victimClientNum) {
					s = "";
				} else {
					if (*cgs.clientinfo[obituary->killerClientNum].whiteClanTag) {
						s = va("%s ", cgs.clientinfo[obituary->killerClientNum].whiteClanTag);
					} else {
						s = "";
					}
				}
			}
			while (*s) {
				extString[j].i = (unsigned char)s[0];
				j++;
				s++;
			}
		} else if (c == '{') {  // long form commands
			qboolean foundEndBrace;
			int rets;

			rets = 0;

			if (!Q_stricmpn(&tokenString[i + 1], "newline ", strlen("newline "))) {
				extString[j].i = TEXT_PIC_PAINT_NEWLINE;
				j++;
				rets = sscanf(&tokenString[i + 1 + strlen("newline ")], "%f", &extString[j].f);
				if (rets != 1) {
					Com_Printf("^3CG_CreateFragString:  invalid newline long token\n");
					extString[j].f = 16.0f;
				}
				//Com_Printf("^3got newline: %f\n", extString[j].f);
				j++;
			} else if (!Q_stricmpn(&tokenString[i + 1], "x ", strlen("x "))) {
				extString[j].i = TEXT_PIC_PAINT_XOFFSET;
				j++;
				rets = sscanf(&tokenString[i + 1 + strlen("x ")], "%f", &extString[j].f);
				if (rets != 1) {
					Com_Printf("^3CG_CreateFragString:  invalid x long token\n");
					extString[j].f = 0;
				}
				j++;
			} else if (!Q_stricmpn(&tokenString[i + 1], "y ", strlen("y "))) {
				extString[j].i = TEXT_PIC_PAINT_YOFFSET;
				j++;
				rets = sscanf(&tokenString[i + 1 + strlen("y ")], "%f", &extString[j].f);
				if (rets != 1) {
					Com_Printf("^3CG_CreateFragString:  invalid y long token\n");
					extString[j].f = 0;
				}
				j++;
			} else if (!Q_stricmpn(&tokenString[i + 1], "font ", strlen("font "))) {
				fontInfo_t fi;
				int pointSize;
				char fontName[MAX_STRING_CHARS];
				char *sf;

				extString[j].i = TEXT_PIC_PAINT_FONT;
				j++;

				fontName[0] = '\0';
				rets = sscanf(&tokenString[i + 1 + strlen("font ")], "%d %s}", &pointSize, fontName);
				//Com_Printf("font: %d '%s'\n", pointSize, fontName);

				// strip final '}'
				sf = fontName;
				while (*sf) {
					if (*sf == '}') {
						*sf = '\0';
						break;
					}

					sf++;
				}
				//Com_Printf("^5font: %d '%s'\n", pointSize, fontName);
				if (rets != 2) {
					Com_Printf("^3CG_CreateFragString:  invalid font long token\n");
					pointSize = 24;
					fontName[0] = '\0';
				}
				trap_R_RegisterFont(fontName, pointSize, &fi);
				extString[j].i = fi.fontId;
				j++;
			} else if (!Q_stricmpn(&tokenString[i + 1], "scale ", strlen("scale "))) {
				extString[j].i = TEXT_PIC_PAINT_SCALE;
				j++;
				rets = sscanf(&tokenString[i + 1 + strlen("scale ")], "%f", &extString[j].f);
				if (rets != 1) {
					Com_Printf("^3CG_CreateFragString:  invalid scale long token\n");
					extString[j].f = 1;
				}
				j++;
			} else if (!Q_stricmpn(&tokenString[i + 1], "iconscale ", strlen("iconscale "))) {
				extString[j].i = TEXT_PIC_PAINT_ICONSCALE;
				j++;
				rets = sscanf(&tokenString[i + 1 + strlen("iconscale ")], "%f", &extString[j].f);
				if (rets != 1) {
					Com_Printf("^3CG_CreateFragString:  invalid iconscale long token\n");
					extString[j].f = 1;
				}
				j++;
			} else if (!Q_stricmpn(&tokenString[i + 1], "style ", strlen("style "))) {
				extString[j].i = TEXT_PIC_PAINT_STYLE;
				j++;
				rets = sscanf(&tokenString[i + 1 + strlen("style ")], "%d", &extString[j].i);
				//extString[j].i = 0;

				if (rets != 1) {
					Com_Printf("^3CG_CreateFragString:  invalid style long token\n");
					extString[j].i = 0;
				}
				j++;

				//FIXME testing
				//Com_Printf("^2xxxx: ");
				//extString[j].i = 0;
				//printPicString(extString);
			} else {
				Com_Printf("^1CG_CreateFragString() unknown long command: '%s'\n", &tokenString[i + 1]);
			}

			i++;
			foundEndBrace = qfalse;
			while (tokenString[i]) {
				if (tokenString[i] == '}') {
					foundEndBrace = qtrue;
					break;
				}
				//Com_Printf("skipping '%c'\n", tokenString[i]);
				i++;
			}
			if (!foundEndBrace) {
				Com_Printf("^1CG_CreateFragString() couldn't find ending brace\n");
			}
		} else if (c == '%') {
			extString[j].i = '%';
			j++;
		} else {
			Com_Printf("^1CG_CreateFragString() unknown token '%c'\n", c);
		}

		i++;
		//Com_Printf("i %d\n", i);
	}

	//return fragString;
	return extString;
}

/*
===================
CG_DrawCenterString
===================
*/

// same size as CG_CreateFragString
floatint_t tmpExtString[MAX_STRING_CHARS];

static void CG_DrawCenterString( void ) {
	const char	*start;
	int		l;
	float x, y, w;
	float h;
	const fontInfo_t *font;
	int align;
	float scale;
	vec4_t color;
	const floatint_t *extString = NULL;
	int numIcons = 0;
	int startLen;

	if (!cg_drawCenterPrint.integer) {
		return;
	}

	if ( !cg.centerPrintTime ) {
		return;
	}

	QLWideScreen = cg_drawCenterPrintWideScreen.integer;

	if (cg_drawCenterPrintFade.integer) {
		CG_FadeColorVec4(color, cg.centerPrintTime, cg_drawCenterPrintTime.integer, cg_drawCenterPrintFadeTime.integer);
	} else {
		Vector4Set(color, 1.0, 1.0, 1.0, 1.0);
	}

	if (color[0] == 0.0  &&  color[1] == 0.0  &&  color[2] == 0.0  &&  color[3] == 0.0) {
		return;
	}

	if (*cg_drawCenterPrintColor.string) {
		SC_Vec3ColorFromCvar(color, &cg_drawCenterPrintColor);
	}

	if (*cg_drawCenterPrintAlpha.string) {
		color[3] = (float)cg_drawCenterPrintAlpha.integer / 255.0 - (1.0 - color[3]);
		if (color[3] <= 0.0) {
			return;
		}
	}

	trap_R_SetColor( color );

	start = cg.centerPrint;

	//FIXME draw top down??
	if (cg_drawCenterPrintOld.integer == 0) {

		//FIXME 2021-07-25 this wont center correctly a '/centerprint' 'token' option line that uses %{newline ...}

		if (cg_qlhud.integer == 0  &&  !*cg_drawCenterPrintFont.string) {
			y = cg.centerPrintY - cg.centerPrintLines * BIGCHAR_HEIGHT / 2;
		} else {
			//FIXME hack, should be storing this from font info
			y = cg.centerPrintY - cg.centerPrintLines * CG_Text_Height("IP", cg_drawCenterPrintScale.value, 0, &cgs.media.centerPrintFont) / 2;
		}

	} else {  // old q3 hud code that uses default bigchar font
		y = cg.centerPrintY - cg.centerPrintLines * BIGCHAR_HEIGHT / 2;
	}

	//Com_Printf("y: %d\n", y);

	while ( 1 ) {
		char linebuffer[1024];

		//Com_Printf("%d\n", sizeof(linebuffer));

		startLen = strlen(start);
		if (startLen >= sizeof(linebuffer)) {
			startLen = sizeof(linebuffer);
		}

		//for ( l = 0; l < 50; l++ ) {
		for (l = 0;  l < startLen;  l++) {
			if ( !start[l] || start[l] == '\n' ) {
				break;
			}
			linebuffer[l] = start[l];
		}
		linebuffer[l] = 0;

		//Com_Printf("linebuffer: '%s'\n", linebuffer);

		if (cg_drawCenterPrintOld.integer == 0) {

			if (*cg_drawCenterPrintFont.string) {
				font = &cgs.media.centerPrintFont;
			} else {
				if (cg_qlhud.integer == 0) {
					font = &cgs.media.giantchar;
				} else {
					font = &cgDC.Assets.textFont;
				}
			}

			// check that 'start' hasn't advanced  since tokenized message parses the entire thing
			if ((cg.centerPrintIsFragMessage  ||  cg.centerPrintIsTokenized)  &&  start == cg.centerPrint) {
				char *lb;
				const floatint_t *es;

				if (cg.centerPrintIsTokenized) {
					extString = CG_CreateFragString(qtrue, 0, cg.centerPrint);
				} else {
					extString = CG_CreateFragString(qtrue, 0, NULL);
				}

				//Com_Printf("^3fullstring: ");
				//printPicString(extString);

				while (1) {
					floatint_t *ts;
					qboolean haveNewLineAmount = qfalse;
					float newLineAmount = 0;

					if (extString[0].i == 0) {
						break;
					}

					// get a line
					ts = tmpExtString;

					while (extString[0].i) {
						if (extString[0].i >= 0  &&  extString[0].i <= 255) {
							*ts = *extString;
							//Com_Printf("'%c'\n", (char)*extString);
							if (extString[0].i == 0) {
								break;
							}
							extString++;
							ts++;
							continue;
						}

						if (extString[0].i == TEXT_PIC_PAINT_NEWLINE) {
							extString++;
							newLineAmount = extString[0].f;
							extString++;

							haveNewLineAmount = qtrue;
							ts[0].i = 0;
							break;
						}
						// command (2 ints)
						*ts = *extString;
						extString++;
						ts++;
						*ts = *extString;
						extString++;
						ts++;
					}
					ts[0].i = 0;

					//Com_Printf("^6beforenewline: ");
					//printPicString(tmpExtString);

					lb = linebuffer;
					es = tmpExtString;
					numIcons = 0;
					while (es[0].i) {

						if (es[0].i >= 0  &&  es[0].i <= 255) {
							*lb = (char)es[0].i;

							if (es[0].i == 0) {
								break;
							}
							lb++;
							es++;
							continue;
						}
						if (es[0].i == TEXT_PIC_PAINT_ICON) {
							numIcons++;
						}
						es += 2;
					}
					*lb = '\0';

					//Com_Printf("linebuffer:  '%s'\n", linebuffer);
					scale = cg_drawCenterPrintScale.value;
					if (cg_qlhud.integer == 0  &&  !*cg_drawCenterPrintFont.string) {
						// 0.35 center
						// 0.25 frag
						// match default giantchar scaling
						//   maybe default center value should have been 0.355
						//scale *= 0.5;
						scale *= 0.5071428571428571;
					}

					//w = CG_Text_Width(linebuffer, scale, 0, font);
					h = CG_Text_Height(linebuffer, scale, 0, font);

					//Com_Printf("old w %d  icons %d\n", w, numIcons);
					//w += ((float)h * cg_drawFragMessageIconScale.value) * (float)numIcons;
					w = CG_Text_Pic_Width(tmpExtString, scale, cg_drawFragMessageIconScale.value, /*limit*/ 0, h, font);
					//Com_Printf("width: %f\n", w);
					align = cg_drawCenterPrintAlign.integer;
					x = (SCREEN_WIDTH - w) / 2;
					if (*cg_drawCenterPrintX.string) {
						x = cg_drawCenterPrintX.value;
						if (align == 1) {
							x -= w / 2;
						} else if (align == 2) {
							x -= w;
						}
					}

					CG_Text_Pic_Paint(x, y + h, scale, color, tmpExtString, 0, 0, cg_drawCenterPrintStyle.integer, font, h, cg_drawFragMessageIconScale.value);

					if (haveNewLineAmount) {
						y += newLineAmount;
					} else {
						y += h + cg_drawCenterPrintLineSpacing.value;
						if (cg_qlhud.integer == 0  &&  !*cg_drawCenterPrintFont.string) {
							// team arena and ql add a bit of spacing
							y -= 6.0;
						}
					}
				}  // while (1)  getting new line
			} else { // (centerprintisfrag or istoken)  and start == cg.centerprint

				//Com_Printf("centerprint %s font %d\n", linebuffer, *font);
				scale = cg_drawCenterPrintScale.value;
				if (cg_qlhud.integer == 0  &&  !*cg_drawCenterPrintFont.string) {
					// 0.35 center
					// 0.25 frag
					// match default giantchar scaling
					//   maybe default center value should have been 0.355
					//scale *= 0.5;
					scale *= 0.5071428571428571;
				}

				w = CG_Text_Width(linebuffer, scale, 0, font);
				h = CG_Text_Height(linebuffer, scale, 0, font);

				align = cg_drawCenterPrintAlign.integer;
				x = (SCREEN_WIDTH - w) / 2;
				if (*cg_drawCenterPrintX.string) {
					x = cg_drawCenterPrintX.value;
					if (align == 1) {
						x -= w / 2;
					} else if (align == 2) {
						x -= w;
					}
				}

				CG_Text_Paint(x, y + h, scale, color, linebuffer, 0, 0, cg_drawCenterPrintStyle.integer, font);

				y += h + cg_drawCenterPrintLineSpacing.value;
				if (cg_qlhud.integer == 0  &&  !*cg_drawCenterPrintFont.string) {
					// team arena and ql add a bit of spacing
					y -= 6.0;
				}
			}  // end (centerprintisfrag or istoken)  and start == cg.centerprint

		} else {  // else old q3 hud code that uses bigchar font
			//FIXME width
			//FIXME 2021-07-25 why does this get divided by BIGCHAR_WIDTH ?
			w = cg.centerPrintCharWidth * CG_DrawStrlen( linebuffer, &cgs.media.bigchar ) / BIGCHAR_WIDTH;

			x = ( SCREEN_WIDTH - w ) / 2;

			CG_DrawStringExt( x, y, linebuffer, color, qfalse, qtrue,
							  cg.centerPrintCharWidth, (int)(cg.centerPrintCharWidth * 1.5), 0, &cgs.media.bigchar );

			y += cg.centerPrintCharWidth * 1.5;
		}

		while (*start  &&  (*start != '\n')) {
			start++;
		}
		if (!*start) {
			break;
		}
		if (*start == '\n') {
			start++;
		}
		if (!*start) {
			break;
		}
	}

	trap_R_SetColor( NULL );
}

static void CG_DrawFragMessage (void)
{
	float x, y, tw, th;
	const fontInfo_t *font;
	float scale;
	int align;
	vec4_t color;
	const char *text;
	const floatint_t *extString;
	int numIcons;
	char linebuffer[1024];
	char *lb;
	const floatint_t *es;

	if (!cg_drawFragMessageSeparate.integer) {
		return;
	}

	if (cg.time - cg.lastFragTime > cg_drawFragMessageTime.integer) {
		return;
	}

	if (cg.lastFragString[0] == '\0') {
		return;
	}

#if 0
	if (wolfcam_following) {
		ourClientNum = wcg.clientNum;
	} else {
		ourClientNum = cg.snap->ps.clientNum;
	}
#endif

	if (*cg_drawFragMessageFont.string) {
		font = &cgs.media.fragMessageFont;
	} else {
		if (cg_qlhud.integer == 0) {
			font = &cgs.media.giantchar;
		} else {
			font = &cgDC.Assets.textFont;
		}
	}
	QLWideScreen = cg_drawFragMessageWideScreen.integer;


	y = cg_drawFragMessageY.value;
	scale = cg_drawFragMessageScale.value;

	if (cg_qlhud.integer == 0  &&  !*cg_drawFragMessageFont.string) {
		// match default giantchar scaling
		scale *= 0.75;
	}

	//FIXME duplicate code in draw center print
	extString = CG_CreateFragString(qtrue, 0, NULL);

	while (1) {
		floatint_t *ts;
		qboolean haveNewLineAmount = qfalse;
		float newLineAmount = 0;

		if (extString[0].i == 0) {
			break;
		}

		// get a line
		ts = tmpExtString;

		while (extString[0].i) {
			if (extString[0].i >= 0  &&  extString[0].i <= 255) {
				*ts = *extString;
				if (extString[0].i == 0) {
					break;
				}
				extString++;
				ts++;
				continue;
			}

			if (extString[0].i == TEXT_PIC_PAINT_NEWLINE) {
				extString++;
				newLineAmount = extString[0].f;
				extString++;
				haveNewLineAmount = qtrue;
				ts[0].i = 0;
				break;
			}

			// command (2 ints)
			*ts = *extString;
			extString++;
			ts++;
			*ts = *extString;
			extString++;
			ts++;
		}  // end while (*extString)
		ts[0].i = 0;

		lb = linebuffer;
		es = tmpExtString;  //extString;
		numIcons = 0;
		while (es[0].i) {
			if (es[0].i >= 0  &&  es[0].i <= 255) {
				*lb = (char)es[0].i;
				if (es[0].i == 0) {
					break;
				}
				lb++;
				es++;
				continue;
			}
			if (es[0].i == TEXT_PIC_PAINT_ICON) {
				numIcons++;
			}

			es += 2;
		}
		*lb = '\0';
		text = linebuffer;

		//tw = CG_Text_Width(text, scale, 0, font);
		th = CG_Text_Height(text, scale, 0, font);
		//Com_Printf("old w %d  icons %d\n", tw, numIcons);
		//Com_Printf("linebuffer: %s\n", linebuffer);

		//tw += ((float)th * cg_drawFragMessageIconScale.value) * (float)numIcons;
		tw = CG_Text_Pic_Width(tmpExtString, scale, cg_drawFragMessageIconScale.value, /*limit*/ 0, th, font);

		//Com_Printf("new w %d\n", tw);

		x = cg_drawFragMessageX.value;
		align = cg_drawFragMessageAlign.integer;
		if (align == 1) {
			x -= (tw / 2);
		} else if (align == 2) {
			x -= tw;
		}

		if (cg_drawFragMessageFade.integer) {
			CG_FadeColorVec4(color, cg.lastFragTime, cg_drawFragMessageTime.integer, cg_drawFragMessageFadeTime.integer);
		} else {
			Vector4Set(color, 1.0, 1.0, 1.0, 1.0);
		}

		if (color[3] <= 0.0) {
			return;
		}

		if (*cg_drawFragMessageColor.string) {
			SC_Vec3ColorFromCvar(color, &cg_drawFragMessageColor);
		}

		if (*cg_drawFragMessageAlpha.string) {
			color[3] = (float)cg_drawFragMessageAlpha.integer / 255.0 - (1.0 - color[3]);
			if (color[3] <= 0.0) {
				return;
			}
		}

		CG_Text_Pic_Paint(x, y, scale, color, tmpExtString, 0, 0, cg_drawFragMessageStyle.integer, font, th, cg_drawFragMessageIconScale.value);

		if (haveNewLineAmount) {
			y += (float)newLineAmount;
		}

	}  // while (1)  getting new line
}



/*
================================================================================

CROSSHAIR

================================================================================
*/


static void Wolfcam_DrawCrosshair (void)
{
    float w, h;
    qhandle_t hShader;
    float x, y;
    int ca;
	vec4_t color;
	float aspect;

    if (cg_entities[wcg.clientNum].currentState.eFlags & EF_DEAD)
		return;

	w = h = cg_crosshairSize.value;

    x = cg_crosshairX.value;
    y = cg_crosshairY.value;
	//QLWideScreen = cg_crosshairWideScreen.integer;
	//FIXME change?
	QLWideScreen = WIDESCREEN_STRETCH;

	CG_AdjustFrom640( &x, &y, &w, &h );

	if (cg_wideScreen.integer == 3) {
		//FIXME this is broken
		aspect = (float)cgs.glconfig.vidHeight / (float)cgs.glconfig.vidWidth;
		w -= ((480.0 / 640.0) - aspect) * w;
	} else {  // use ql widescreen
		h = cg_crosshairSize.value * cgs.screenXScale;
	}

    ca = cg_drawCrosshair.integer;
    if (ca <= 0) {
		return;
    }
	ca = ca % NUM_CROSSHAIRS;
	if (ca == 0) {
		return;
	}
	// indexed at 1
    hShader = cgs.media.crosshairShader[ca];

#if 0
	if (cg_crosshairColor.string[0] != '\0') {
		trap_R_SetColor(cg.crosshairColor);
	} else {
		trap_R_SetColor(NULL);
	}
#endif
	SC_Vec3ColorFromCvar(color, &cg_crosshairColor);
	color[3] = cg_crosshairAlpha.value / 255.0;  //1.0;
	trap_R_SetColor(color);

    trap_R_DrawStretchPic( x + cg.refdef.x + 0.5 * (cg.refdef.width - w),
                           y + cg.refdef.y + 0.5 * (cg.refdef.height - h),
                           w, h, 0, 0, 1, 1, hShader );

	trap_R_SetColor( NULL );
}

void CG_CreateNewCrosshairs (void)
{
	int i;
	int w, h;
	int j;
	//imgBuff
	float b;
	int alpha;
	//int alphaThreshold;


#if 0
	if (SC_Cvar_Get_Int("r_texturebits") == 32) {
		Com_Printf("FIXME crosshair tweaking with r_texturebits 32\n");
		return;
	}
#endif

	Com_Printf("new crosshairs\n");

	b = cg_crosshairBrightness.value;
	alpha = cg_crosshairAlphaAdjust.integer;
	//alphaThreshold = cg_crosshairAlphaAdjustThreshold;
	//return;

	if (b == 1.0  &&  alpha == 0) {
		Com_Printf("default crosshairs\n");
		for (i = 1;  i < NUM_CROSSHAIRS;  i++) {
			trap_GetShaderImageDimensions(cgs.media.crosshairShader[i], &w, &h);
			if (w > 64  ||  h > 64) {
				Com_Printf("FIXME dim %d %d\n", w, h);
				continue;  //FIXME
			}
			trap_ReplaceShaderImage(cgs.media.crosshairShader[i], cgs.media.crosshairOrigImage[i], w, h);
		}
		return;
	}

	for (i = 1;  i < NUM_CROSSHAIRS;  i++) {
		trap_GetShaderImageDimensions(cgs.media.crosshairShader[i], &w, &h);
		if (w > 64  ||  h > 64) {
			Com_Printf("FIXME dim %d %d\n", w, h);
			continue;  //FIXME
		}
		memcpy(imgBuff, cgs.media.crosshairOrigImage[i], w * h * 4);
		for (j = 0;  j < w * h * 4;  j += 4) {
			float v;
			int a;

			if (imgBuff[j + 3] == 0) {
				continue;
			}

			if (alpha > 0) {
				a = (int)imgBuff[j + 3] + alpha;
				if (a > 255) {
					a = 255;
				} else if (a < 0) {
					a = 0;
				}
			} else {
				if (imgBuff[j + 3] < 125) {  //FIXME 200
					a = (int)imgBuff[j + 3] + alpha;
					if (a > 255) {
						a = 255;
					} else if (a < 0) {
						a = 0;
					}
					//Com_Printf("lowered\n");
				} else {
					a = imgBuff[j + 3];
				}
			}
			imgBuff[j + 3] = a;
			//Com_Printf("%d  %d\n", j, a);

			if (b < 1.0) {
				v = (float)imgBuff[j + 0] * b;
			} else {
				v = (float)imgBuff[j + 0] + b;
			}

			if (v > 255.0) {
				v = 255.0;
			} else if (v < 0.0) {
				v = 0.0;
			}
			imgBuff[j + 0] = (byte)v;


			if (b < 1.0) {
				v = (float)imgBuff[j + 1] * b;
			} else {
				v = (float)imgBuff[j + 1] + b;
			}

			if (v > 255.0) {
				v = 255.0;
			} else if (v < 0.0) {
				v = 0.0;
			}
			imgBuff[j + 1] = (byte)v;


			if (b < 1.0) {
				v = (float)imgBuff[j + 2] * b;
			} else {
				v = (float)imgBuff[j + 2] + b;
			}

			if (v > 255.0) {
				v = 255.0;
			} else if (v < 0.0) {
				v = 0.0;
			}
			imgBuff[j + 2] = (byte)v;


		}
		trap_ReplaceShaderImage(cgs.media.crosshairShader[i], imgBuff, w, h);
	}
}

static void CG_CrosshairSetHitColor (void)
{
	vec4_t color;

		if (cg.time - cg.damageDoneTime < cg_crosshairHitTime.integer) {
			if (cg.hitSound == 0) {
				//vec4_t tmpColor;
				// light blue  #38B0DE

				//trap_R_DrawStretchPic( x + cg.refdef.x + 0.5 * (cg.refdef.width - w), y + cg.refdef.y + 0.5 * (cg.refdef.height - h), w, h, 0, 0, 1, 1, hShader );
				VectorSet(color, 0.22, 0.69, 0.87);
				//trap_R_SetColor(tmpColor);
				//trap_R_DrawStretchPic( x + cg.refdef.x + 0.5 * (cg.refdef.width - w), y + cg.refdef.y + 0.5 * (cg.refdef.height - h), w, h, 0, 0, 1, 1, hShader );
				//color[3] = 0;
				//trap_R_SetColor(color);
				//trap_R_DrawStretchPic( x + cg.refdef.x + 0.5 * (cg.refdef.width - w/3), y + cg.refdef.y + 0.5 * (cg.refdef.height - h/3), w/3, h/3, 0, 0, 1, 1, hShader );
				// fuck this
				//return;
			} else if (cg.hitSound == 1) {
				// yellow
				VectorSet(color, 1, 1, 0);
			} else if (cg.hitSound == 2) {
				// orange  ff7f00
				VectorSet(color, 1, 0.5, 0);
				// rail hack -- this is fucked up
				//Com_Printf("damage done: %d\n", cg.damageDone);
				//if (cg.snap->ps.persistant[PERS_ATTACKEE_ARMOR] 0xff > 80) {
				if (cg.damageDone > 80) {  //FIXME rail hack
					VectorSet(color, 1, 0, 0);
				}
			} else if (cg.hitSound == 3) {
				// red
				VectorSet(color, 1, 0, 0);
			} else {
				// light blue
				VectorSet(color, 0.22, 0.69, 0.87);
			}
			color[3] = cg_crosshairAlpha.value / 255.0;
			trap_R_SetColor(color);
		}
}

/*
=================
CG_DrawCrosshair
=================
*/
static void CG_DrawCrosshair(void) {
	float		w, h;
	qhandle_t	hShader;
	float		f;
	float		x, y;
	int			ca;
	vec4_t color;
	float aspect;

	if ( !cg_drawCrosshair.integer ) {
		return;
	}

	if (cg_crosshairBrightness.value != cg.crosshairBrightness  ||  cg_crosshairAlphaAdjust.integer != cg.crosshairAlphaAdjust) {
		CG_CreateNewCrosshairs();
		cg.crosshairBrightness = cg_crosshairBrightness.value;
		cg.crosshairAlphaAdjust = cg_crosshairAlphaAdjust.integer;
	}

    if (wolfcam_following) {
        Wolfcam_DrawCrosshair ();
        return;
    }

	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR) {
		return;
	}

	if (  cg.renderingThirdPerson ) {
		if (cg.freecam  &&  cg_freecam_crosshair.integer) {
			goto want_crosshair;
		} else {
			return;
		}
		return;
	}

 want_crosshair:
	// set color based on health
	if ( cg_crosshairHealth.integer ) {
		vec4_t		hcolor;

		CG_ColorForHealth( hcolor );
		trap_R_SetColor( hcolor );
	} else if (cg_crosshairColor.string[0] != '\0') {
		//trap_R_SetColor(cg.crosshairColor);
		SC_Vec3ColorFromCvar(color, &cg_crosshairColor);
		color[3] = cg_crosshairAlpha.value / 255.0;  //1.0;
		trap_R_SetColor(color);
	} else {
		trap_R_SetColor( NULL );
	}

	w = h = cg_crosshairSize.value;

	if (cg_crosshairPulse.integer) {
		// pulse the size of the crosshair when picking up items
		f = cg.time - cg.itemPickupBlendTime;
		if ( f > 0 && f < ITEM_BLOB_TIME ) {
			f /= ITEM_BLOB_TIME;
			w *= ( 1 + f );
			h *= ( 1 + f );
		}
	}

	x = cg_crosshairX.value;
	y = cg_crosshairY.value;
	//QLWideScreen = cg_crosshairWideScreen.integer;
	//FIXME change?
	QLWideScreen = WIDESCREEN_STRETCH;

	CG_AdjustFrom640( &x, &y, &w, &h );

	if (cg_wideScreen.integer == 3) {
		//FIXME this is broken
		aspect = (float)cgs.glconfig.vidHeight / (float)cgs.glconfig.vidWidth;
		w -= ((480.0 / 640.0) - aspect) * w;
	} else {  // use ql widescreen
		h = cg_crosshairSize.value * cgs.screenXScale;
	}

	ca = cg_drawCrosshair.integer;
	if (ca <= 0) {
		trap_R_SetColor(NULL);
		return;
	}
	ca = ca % NUM_CROSSHAIRS;
	if (ca == 0) {
		trap_R_SetColor(NULL);
		return;
	}
	// indexed at 1
	hShader = cgs.media.crosshairShader[ca];

	if (cg_crosshairHitStyle.integer == 1) {
		if (cg.time - cg.damageDoneTime < cg_crosshairHitTime.integer) {
			if (cg.hitSound == 0) {
				vec4_t tmpColor;
				// light blue  #38B0DE

				//trap_R_DrawStretchPic( x + cg.refdef.x + 0.5 * (cg.refdef.width - w), y + cg.refdef.y + 0.5 * (cg.refdef.height - h), w, h, 0, 0, 1, 1, hShader );
				VectorSet(tmpColor, 0.22, 0.69, 0.87);
				trap_R_SetColor(tmpColor);
				trap_R_DrawStretchPic( x + cg.refdef.x + 0.5 * (cg.refdef.width - w), y + cg.refdef.y + 0.5 * (cg.refdef.height - h), w, h, 0, 0, 1, 1, hShader );
				color[3] = 0;
				//trap_R_SetColor(color);
				//trap_R_DrawStretchPic( x + cg.refdef.x + 0.5 * (cg.refdef.width - w/3), y + cg.refdef.y + 0.5 * (cg.refdef.height - h/3), w/3, h/3, 0, 0, 1, 1, hShader );
				// fuck this

				trap_R_SetColor(NULL);
				return;
			} else if (cg.hitSound == 1) {
				// yellow
				VectorSet(color, 1, 1, 0);
			} else if (cg.hitSound == 2) {
				// orange  ff7f00
				VectorSet(color, 1, 0.5, 0);
				// rail hack -- this is fucked up
				//Com_Printf("damage done: %d\n", cg.damageDone);
				//if (cg.snap->ps.persistant[PERS_ATTACKEE_ARMOR] 0xff > 80) {
				if (cg.damageDone > 80) {  //FIXME rail hack
					VectorSet(color, 1, 0, 0);
				}
			} else if (cg.hitSound == 3) {
				// red
				VectorSet(color, 1, 0, 0);
			} else {
				// light blue
				VectorSet(color, 0.22, 0.69, 0.87);
			}
			color[3] = cg_crosshairAlpha.value / 255.0;
			trap_R_SetColor(color);
		}
	} else if (cg_crosshairHitStyle.integer == 2) {
		if (cg.time - cg.damageDoneTime < cg_crosshairHitTime.integer) {
			SC_Vec3ColorFromCvar(color, &cg_crosshairHitColor);
			color[3] = cg_crosshairAlpha.value / 255.0;  //1.0;
			trap_R_SetColor(color);
		}
	} else if (cg_crosshairHitStyle.integer == 3) {
		f = cg.time - cg.damageDoneTime;
		if ( f > 0 && f < cg_crosshairHitTime.integer ) {
			f /= cg_crosshairHitTime.integer;
			w *= ( 1 + f );
			h *= ( 1 + f );
		}
	} else if (cg_crosshairHitStyle.integer == 4) {
		f = cg.time - cg.damageDoneTime;
		if ( f > 0 && f < cg_crosshairHitTime.integer ) {
			f /= cg_crosshairHitTime.integer;
			w *= ( 1 + f );
			h *= ( 1 + f );
		}
		CG_CrosshairSetHitColor();
	} else if (cg_crosshairHitStyle.integer == 5) {
		f = cg.time - cg.damageDoneTime;
		if ( f > 0 && f < cg_crosshairHitTime.integer ) {
			f /= cg_crosshairHitTime.integer;
			w *= ( 1 + f );
			h *= ( 1 + f );
		}
		if (cg.time - cg.damageDoneTime < cg_crosshairHitTime.integer) {
			SC_Vec3ColorFromCvar(color, &cg_crosshairHitColor);
			color[3] = cg_crosshairAlpha.value / 255.0;  //1.0;
			trap_R_SetColor(color);
		}
	} else if (cg_crosshairHitStyle.integer == 6) {
		f = cg.time - cg.damageDoneTime;
		if ( f > 0 && f < ITEM_BLOB_TIME ) {
			f /= ITEM_BLOB_TIME;
			w *= ( 1 + f );
			h *= ( 1 + f );
		}
	} else if (cg_crosshairHitStyle.integer == 7) {
		f = cg.time - cg.damageDoneTime;
		if ( f > 0 && f < ITEM_BLOB_TIME ) {
			f /= ITEM_BLOB_TIME;
			w *= ( 1 + f );
			h *= ( 1 + f );
		}
		CG_CrosshairSetHitColor();
	} else if (cg_crosshairHitStyle.integer == 8) {
		f = cg.time - cg.damageDoneTime;
		if ( f > 0 && f < ITEM_BLOB_TIME ) {
			f /= ITEM_BLOB_TIME;
			w *= ( 1 + f );
			h *= ( 1 + f );
		}
		if (cg.time - cg.damageDoneTime < cg_crosshairHitTime.integer) {
			SC_Vec3ColorFromCvar(color, &cg_crosshairHitColor);
			color[3] = cg_crosshairAlpha.value / 255.0;  //1.0;
			trap_R_SetColor(color);
		}
	}


	trap_R_DrawStretchPic( x + cg.refdef.x + 0.5 * (cg.refdef.width - w),
		y + cg.refdef.y + 0.5 * (cg.refdef.height - h),
		w, h, 0, 0, 1, 1, hShader );

	trap_R_SetColor(NULL);
}

/*
=================
CG_ScanForCrosshairEntity
=================
*/
static void CG_ScanForCrosshairEntity( void ) {
	trace_t		trace;
	vec3_t		start, end;
	//int			content;
	int skipNum;

	//CG_Printf("scanforcrosshairent\n");

	VectorCopy( cg.refdef.vieworg, start );
	VectorMA( start, 131072, cg.refdef.viewaxis[0], end );

	if (wolfcam_following) {
		skipNum = wcg.clientNum;
	} else if (cg.freecam) {
		skipNum = -1;
	} else {
		skipNum = cg.snap->ps.clientNum;
	}

	//Com_Printf("^3skip %d   following %d\n", skipNum, wolfcam_following);

	//FIXME wolfcam freecam  --  2018-06-05 switching to freecam while /following currently disables following
	//CG_Trace( &trace, start, vec3_origin, vec3_origin, end, skipNum, CONTENTS_SOLID|CONTENTS_BODY );
	Wolfcam_WeaponTrace( &trace, start, vec3_origin, vec3_origin, end, skipNum, CONTENTS_SOLID|CONTENTS_BODY );
	if ( trace.entityNum >= MAX_CLIENTS ) {
		return;
	}

    //CG_Printf ("^2crosshairEnt: %d\n", trace.entityNum);  // wolfcam testing

	// if the player is in fog, don't show it
	//FIXME wtf, ca and ctf trace.endpos always has CONTENTS_FOG
#if 0
	//content = CG_PointContents( trace.endpos, 0 );
	if (trace.entityNum < 0  ||  trace.entityNum >= MAX_CLIENTS) {
		return;
	}
	content = CG_PointContents(cg_entities[trace.entityNum].lerpOrigin, 0);
	if ( content & CONTENTS_FOG ) {
		//cg.crosshairClientNum = CROSSHAIR_CLIENT_INVALID;
		return;
	}
	//CG_Printf("not in fog\n");
#endif

	if (trace.entityNum < 0  ||  trace.entityNum >= MAX_CLIENTS) {
		return;
	}

	// if the player is invisible, don't show it
	if ( cg_entities[ trace.entityNum ].currentState.powerups & ( 1 << PW_INVIS ) ) {
		//cg.crosshairClientNum = CROSSHAIR_CLIENT_INVALID;
		return;
	}

	// update the fade timer
	cg.crosshairClientNum = trace.entityNum;
	cg.crosshairClientTime = cg.realTime;  //cg.time;
	//CG_Printf("crosshairname hit %d\n", cg.crosshairClientNum);
}


/*
=====================
CG_DrawCrosshairNames
=====================
*/
static void CG_DrawCrosshairNames( void ) {
	const float		*color = NULL;
	const char		*name;
	float		w;
	vec4_t selectedColor;
	int align;
	float x, y;
	const fontInfo_t *font;
	const char *clanTag;

	if ( !cg_drawCrosshair.integer ) {
		//return;
	}
	if ( !cg_drawCrosshairNames.integer ) {
		return;
	}
	if ( cg.renderingThirdPerson ) {
		//return;
	}
	if (cg.crosshairClientNum < 0  ||  cg.crosshairClientNum >= MAX_CLIENTS) {
		return;
	}

	if (cg_entities[cg.crosshairClientNum].currentState.eFlags & EF_DEAD) {
		return;
	}

	if (*cg_drawCrosshairNamesFont.string) {
		font = &cgs.media.crosshairNamesFont;
	} else {
		font = &cgDC.Assets.textFont;
	}
	QLWideScreen = cg_drawCrosshairNamesWideScreen.integer;

	align = cg_drawCrosshairNamesAlign.integer;

	// scan the known entities to see if the crosshair is sighted on one
	//CG_ScanForCrosshairEntity();

	//if (cg.time - cg.crosshairClientTime > cg_drawCrosshairNamesTime.integer) {
	if (cg.realTime - cg.crosshairClientTime > cg_drawCrosshairNamesTime.integer) {
		return;
	}

	if (cg_drawCrosshairNames.integer == 2  &&  !cg.freecam) {
		if (CG_IsEnemy(&cgs.clientinfo[cg.crosshairClientNum])) {
			return;
		}
	}

	SC_Vec4ColorFromCvars(selectedColor, &cg_drawCrosshairNamesColor, &cg_drawCrosshairNamesAlpha);
	// draw the name of the player being looked at
	if (cg_drawCrosshairNamesFade.integer) {
		color = CG_FadeColorRealTime( cg.crosshairClientTime, cg_drawCrosshairNamesFadeTime.integer );
		if (!color) {
			return;
		}
		selectedColor[3] -= (1.0 - color[3]);
	}
	if (selectedColor[3] <= 0.0) {
		return;
	}
	//if ( !color ) {
		//trap_R_SetColor( NULL );
		//return;
	//}

	clanTag = cgs.clientinfo[cg.crosshairClientNum].clanTag;
	if (*clanTag) {
		name = va("^7%s ^7%s", clanTag, cgs.clientinfo[cg.crosshairClientNum].name);
	} else {
		name = cgs.clientinfo[cg.crosshairClientNum].name;
	}

#ifdef MISSIONPACK
	//color[3] *= 0.5f;
	w = CG_Text_Width(name, 0.3f, 0, &cgDC.Assets.textFont);
	CG_Text_Paint( 320 - w / 2, 190, 0.3f, color, name, 0, 0, ITEM_TEXTSTYLE_SHADOWED, font);
#else
	//w = CG_DrawStrlen( name, &cgs.media.bigchar );
	//CG_DrawBigString( 320 - w / 2, 170, name, color[3] * 0.5f );
#endif
	//trap_R_SetColor( NULL );

	w = CG_Text_Width(name, cg_drawCrosshairNamesScale.value, 0, font);
	x = cg_drawCrosshairNamesX.value;
	y = cg_drawCrosshairNamesY.value;
	if (align == 1) {
		x -= w / 2;
	} else if (align == 2) {
		x -= w;
	}
	//CG_Text_Paint_Bottom(x, y, cg_drawCrosshairNamesScale.value, selectedColor, name, 0, 0, cg_drawCrosshairNamesStyle.integer, font);
	CG_Text_Paint(x, y, cg_drawCrosshairNamesScale.value, selectedColor, name, 0, 0, cg_drawCrosshairNamesStyle.integer, font);
}

static void CG_DrawCrosshairTeammateHealth (void)
{
	vec4_t color;
	float x, y, w;
	int style, align;
	float scale;
	const fontInfo_t *font;
	const float *fcolor;
	const char *s;
	vec4_t hcolor;
	float alpha;
	int health, armor;

	if (!cg_drawCrosshairTeammateHealth.integer) {
		return;
	}
	if (cg.crosshairClientNum < 0  ||  cg.crosshairClientNum >= MAX_CLIENTS) {
		return;
	}

	if (cg_entities[cg.crosshairClientNum].currentState.eFlags & EF_DEAD) {
		return;
	}

	// server didn't send team info
	if (numSortedTeamPlayers <= 0) {
		//Com_Printf("no team info\n");
		return;
	}

	if (cg.realTime - cg.crosshairClientTime > cg_drawCrosshairTeammateHealthTime.integer) {
		return;
	}

	//FIXME freecam
	if (!CG_IsTeammate(&cgs.clientinfo[cg.crosshairClientNum])) {
		//Com_Printf("not teammate\n");
		return;
	}

	//Com_Printf("yes.... %d\n", cg.crosshairClientNum);

	QLWideScreen = cg_drawCrosshairTeammateHealthWideScreen.integer;

	alpha = (float)cg_drawCrosshairTeammateHealthAlpha.integer / 255.0;

	SC_Vec4ColorFromCvars(color, &cg_drawCrosshairTeammateHealthColor, &cg_drawCrosshairTeammateHealthAlpha);
	if (cg_drawCrosshairTeammateHealthFade.integer) {
		fcolor = CG_FadeColorRealTime(cg.crosshairClientTime, cg_drawCrosshairTeammateHealthFadeTime.integer);

		if (!fcolor) {
			return;
		}
		color[3] -= (1.0 - fcolor[3]);
    }

	if (color[3] <= 0.0) {
		return;
	}

	// 2019-04-19 could use painEvent health
	health = Wolfcam_PlayerHealth(cg.crosshairClientNum, qfalse);
	armor = Wolfcam_PlayerArmor(cg.crosshairClientNum);

	align = cg_drawCrosshairTeammateHealthAlign.integer;
	scale = cg_drawCrosshairTeammateHealthScale.value;
	style = cg_drawCrosshairTeammateHealthStyle.integer;
	if (*cg_drawCrosshairTeammateHealthFont.string) {
		font = &cgs.media.crosshairTeammateHealthFont;
	} else {
		font = &cgDC.Assets.textFont;
	}

	x = cg_drawCrosshairTeammateHealthX.value;
	y = cg_drawCrosshairTeammateHealthY.value;

	//s = "Waiting for players";
	//FIXME check color
	s = va("%3d / %3d", health, armor);

	w = CG_Text_Width(s, scale, 0, font);
	if (align == 1) {
		x -= w / 2;
	} else if (align == 2) {
		x -= w;
	}
	if (*cg_drawCrosshairTeammateHealthColor.string) {
		//CG_Text_Paint_Bottom(x, y, scale, color, s, 0, 0, style, font);
		CG_Text_Paint(x, y, scale, color, s, 0, 0, style, font);
	} else {
		//CG_GetColorForHealth(health, armor, hcolor);
		CG_GetColorForHealth(health, 0, hcolor);
		//hcolor[0] = 1;
		//hcolor[2] = 0;
		//hcolor[2] = 1;
		hcolor[3] = alpha;
		//if (health < 66) {
		//	hcolor[2] = 0;
		//}
		//hcolor[1] = hcolor[1] / 255.0 * 227.0;
		// 0xffe334

		s = va("%3d ", health);
		w = CG_Text_Width(s, scale, 0, font);
		//CG_Text_Paint_Bottom(x, y, scale, hcolor, s, 0, 0, style, font);
		CG_Text_Paint(x, y, scale, hcolor, s, 0, 0, style, font);
		x += w;

		Vector4Set(hcolor, 1, 1, 1, alpha);
		s = "/ ";
		w = CG_Text_Width(s, scale, 0, font);
		//CG_Text_Paint_Bottom(x, y, scale, hcolor, s, 0, 0, style, font);
		CG_Text_Paint(x, y, scale, hcolor, s, 0, 0, style, font);
		x += w;

		CG_GetColorForHealth(armor, 0, hcolor);
		hcolor[3] = alpha;
		s = va("%3d", armor);
		//CG_Text_Paint_Bottom(x, y, scale, hcolor, s, 0, 0, style, font);
		CG_Text_Paint(x, y, scale, hcolor, s, 0, 0, style, font);
	}
}

static void CG_DrawKeyPress (void)
{
	float w, h;

	if (cg_drawKeyPress.integer == 0) {
		return;
	}

	//FIXME widescreen
	QLWideScreen = WIDESCREEN_CENTER;

	w = 16;
	h = 16;

	if (cg.playerKeyPressForward) {
		if (cgs.cpma  &&  CG_CheckCpmaVersion(1, 50, "")) {
			// this is accurate
			trap_R_SetColor(colorGreen);
		} else {
			// this is a guess so color it differently
			trap_R_SetColor(colorYellow);
		}
		CG_DrawPic(640/2 - w/2, 480/2 - h * 3 - h, w, h, cgs.media.playerKeyPressForwardShader);
	}

	if (cg.playerKeyPressBack) {
		trap_R_SetColor(colorGreen);
		CG_DrawPic(640/2 - w/2, 480/2 + h * 3, w, h, cgs.media.playerKeyPressBackShader);
	}

	if (cg.playerKeyPressRight) {
		trap_R_SetColor(colorGreen);
		CG_DrawPic(640/2 + w * 3, 480/2 - h/2, w, h, cgs.media.playerKeyPressRightShader);
	}

	if (cg.playerKeyPressLeft) {
		trap_R_SetColor(colorGreen);
		CG_DrawPic(640/2 - w * 3 - w, 480/2 - h/2, w, h, cgs.media.playerKeyPressLeftShader);
	}

	if (cg.playerKeyPressFire) {
		trap_R_SetColor(colorRed);
		CG_DrawPic(640/2 - w * 3 - w, 480/2 - h * 3, w, h, cgs.media.playerKeyPressMiscShader);
	}

	if (cg.playerKeyPressCrouch) {
		trap_R_SetColor(colorBlue);
		CG_DrawPic(640/2 + w * 3, 480/2 + h * 3, w, h, cgs.media.playerKeyPressMiscShader);

	}

	if (cg.playerKeyPressJump) {
		trap_R_SetColor(colorCyan);
		CG_DrawPic(640/2 + w * 3, 480/2 - h * 3, w, h, cgs.media.playerKeyPressMiscShader);
	}
}

//==============================================================================

/*
=================
CG_DrawSpectator
=================
*/
static void CG_DrawSpectator(void) {
	if (cg.demoPlayback) {
		//FIXME
		return;
	}

	QLWideScreen = WIDESCREEN_CENTER;

	CG_DrawBigString(320 - 9 * 8, 440, "SPECTATOR", 1.0F);
	if (CG_IsDuelGame(cgs.gametype)) {
		CG_DrawBigString(320 - 15 * 8, 460, "waiting to play", 1.0F);
	}
	else if ( CG_IsTeamGame(cgs.gametype) ) {
		CG_DrawBigString(320 - 39 * 8, 460, "press ESC and use the JOIN menu to play", 1.0F);
	}
}

static void CG_VoteAnnouncements (void)
{
	if ( !cgs.voteTime ) {
		return;
	}

	// play a talk beep whenever it is modified
	if ( cgs.voteModified ) {
		cgs.voteModified = qfalse;
		if (cg_audioAnnouncerVote.integer) {
			CG_StartLocalSound( cgs.media.talkSound, CHAN_LOCAL_SOUND );
		}
	}
}

/*
=================
CG_DrawVote
=================
*/
static void CG_DrawVote (void)
{
	const char	*s;
	int		sec;
	vec4_t color;
	float x, y, w;
	int style, align;
	float scale;
	const fontInfo_t *font;

	if (!cg_drawVote.integer) {
		return;
	}

	if ( !cgs.voteTime ) {
		return;
	}

	sec = ( VOTE_TIME - ( cg.time - cgs.voteTime ) ) / 1000;
	if ( sec < 0 ) {
		sec = 0;
	}

	SC_Vec4ColorFromCvars(color, &cg_drawVoteColor, &cg_drawVoteAlpha);
	align = cg_drawVoteAlign.integer;
	scale = cg_drawVoteScale.value;
	style = cg_drawVoteStyle.integer;
	QLWideScreen = cg_drawVoteWideScreen.integer;

	if (*cg_drawVoteFont.string) {
		font = &cgs.media.voteFont;
	} else {
		font = &cgDC.Assets.textFont;
	}
	x = cg_drawVoteX.value;
	y = cg_drawVoteY.value;

	s = va("VOTE(%is):%s  yes:%i no:%i", sec, cgs.voteString, cgs.voteYes, cgs.voteNo);

	w = CG_Text_Width(s, scale, 0, font);
	if (align == 1) {
		x -= w / 2;
	} else if (align == 2) {
		x -= w;
	}
	CG_Text_Paint_Bottom(x, y, scale, color, s, 0, 0, style, font);
}

static void CG_TeamVoteAnnouncements (void)
{
	int cs_offset;
	int ourClientNum;

	if (!cg_drawTeamVote.integer) {
		return;
	}

	if (wolfcam_following) {
		ourClientNum = wcg.clientNum;
	} else {
		ourClientNum = cg.snap->ps.clientNum;
	}

	if ( cgs.clientinfo[ourClientNum].team == TEAM_RED )
		cs_offset = 0;
	else if ( cgs.clientinfo[ourClientNum].team == TEAM_BLUE )
		cs_offset = 1;
	else
		return;

	if ( !cgs.teamVoteTime[cs_offset] ) {
		return;
	}

	// play a talk beep whenever it is modified
	if ( cgs.teamVoteModified[cs_offset] ) {
		cgs.teamVoteModified[cs_offset] = qfalse;
		if (cg_audioAnnouncerTeamVote.integer) {
			CG_StartLocalSound( cgs.media.talkSound, CHAN_LOCAL_SOUND );
		}
	}

}

/*
=================
CG_DrawTeamVote
=================
*/
#if 0
static void CG_DrawTeamVote(void) {
	const char	*s;
	int		sec, cs_offset;
	float x;
	float y;
	int ourClientNum;

	if (!cg_drawTeamVote.integer) {
		return;
	}

	if (wolfcam_following) {
		ourClientNum = wcg.clientNum;
	} else {
		ourClientNum = cg.snap->ps.clientNum;
	}

	if ( cgs.clientinfo[ourClientNum].team == TEAM_RED )
		cs_offset = 0;
	else if ( cgs.clientinfo[ourClientNum].team == TEAM_BLUE )
		cs_offset = 1;
	else
		return;

	if ( !cgs.teamVoteTime[cs_offset] ) {
		return;
	}

	sec = ( VOTE_TIME - ( cg.time - cgs.teamVoteTime[cs_offset] ) ) / 1000;
	if ( sec < 0 ) {
		sec = 0;
	}
	s = va("TEAMVOTE(%is):%s  yes:%i no:%i", sec, cgs.teamVoteString[cs_offset],
							cgs.teamVoteYes[cs_offset], cgs.teamVoteNo[cs_offset] );

	if (cg_drawTeamVoteX.string[0] != '\0') {
		x = cg_drawTeamVoteX.value;
	} else {
		x = 0;
	}
	if (cg_drawTeamVoteY.string[0] != '\0') {
		y = cg_drawTeamVoteY.value;
	} else {
		y = 90;
	}
	QLWideScreen = cg_drawTeamVoteWideScreen.integer;

	CG_DrawSmallString(x, y, s, 1.0);
}
#endif


static void CG_DrawTeamVote (void)
{
	const char *s;
	int		sec, cs_offset;
	float x, y, w;
	int style, align;
	float scale;
	const fontInfo_t *font;
	vec4_t color;
	int ourClientNum;

	if (!cg_drawTeamVote.integer) {
		return;
	}

	if (wolfcam_following) {
		ourClientNum = wcg.clientNum;
	} else {
		ourClientNum = cg.snap->ps.clientNum;
	}

	if (cgs.clientinfo[ourClientNum].team == TEAM_RED) {
		cs_offset = 0;
	} else if (cgs.clientinfo[ourClientNum].team == TEAM_BLUE) {
		cs_offset = 1;
	} else {
		return;
	}

	if (!cgs.teamVoteTime[cs_offset]) {
		return;
	}

	sec = (VOTE_TIME - (cg.time - cgs.teamVoteTime[cs_offset])) / 1000;
	if (sec < 0) {
		sec = 0;
	}

	SC_Vec4ColorFromCvars(color, &cg_drawTeamVoteColor, &cg_drawTeamVoteAlpha);
	align = cg_drawTeamVoteAlign.integer;
	scale = cg_drawTeamVoteScale.value;
	style = cg_drawTeamVoteStyle.integer;
	QLWideScreen = cg_drawTeamVoteWideScreen.integer;

	if (*cg_drawTeamVoteFont.string) {
		font = &cgs.media.teamVoteFont;
	} else {
		font = &cgDC.Assets.textFont;
	}
	x = cg_drawTeamVoteX.value;
	y = cg_drawTeamVoteY.value;

	s = va("TEAMVOTE(%is):%s  yes:%i no:%i", sec, cgs.teamVoteString[cs_offset], cgs.teamVoteYes[cs_offset], cgs.teamVoteNo[cs_offset]);

	w = CG_Text_Width(s, scale, 0, font);
	if (align == 1) {
		x -= w / 2;
	} else if (align == 2) {
		x -= w;
	}
	CG_Text_Paint_Bottom(x, y, scale, color, s, 0, 0, style, font);
}

////////////////////////////////////////////////////////////

static qboolean CG_DrawScoreboard (void)
{
	static qboolean firstTime = qtrue;
	qboolean dead;

	if (cg.freecam  &&  !cg.showScores) {
		return qfalse;
	}

	if (cg.snap->ps.pm_type == PM_INTERMISSION  &&  !cg_scoreBoardAtIntermission.integer) {
		return qfalse;
	}

	// don't draw scoreboard during death while warmup up
	if ((cg.warmup  &&  !cg_scoreBoardWarmup.integer)  &&  !cg.showScores) {
		return qfalse;
	}

	dead = qfalse;
	if (cg.snap->ps.stats[STAT_HEALTH] <= 0  &&  cgs.gametype != GT_FREEZETAG) {
		dead = qtrue;

		if (cgs.gametype == GT_CA  ||  cgs.gametype == GT_CTFS) {
			if (cg.snap->ps.clientNum == cg.clientNum  &&  cgs.clientinfo[cg.clientNum].team == TEAM_SPECTATOR) {
				dead = qfalse;
			} else if (cgs.clientinfo[cg.clientNum].team != TEAM_SPECTATOR  &&  cg.snap->ps.pm_type == PM_SPECTATOR) {
				dead = qfalse;
			}
		} else {
			if (cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_SPECTATOR) {
				dead = qfalse;
			}
		}
	}

	if (cg.showScores || (cg_scoreBoardWhenDead.integer  &&  dead) || (cg_scoreBoardAtIntermission.integer  &&  cg.predictedPlayerState.pm_type == PM_INTERMISSION)) {
		//Com_Printf("%f yes\n", cg.ftime);
	} else {
		if ( !CG_FadeColor( cg.scoreFadeTime, FADE_TIME ) ) {
			// next time scoreboard comes up, don't print killer
			cg.deferredPlayerLoading = 0;
			// 2018-07-15 ql doesn't reset the name and it can be used in the other parts of the hud with CG_KILLER so cg.killerNameHud not reset
			cg.killerName[0] = 0;
			firstTime = qtrue;
			//Com_Printf("%f nope\n", cg.ftime);
			return qfalse;
		}
	}

	if (cg_qlhud.integer  &&  !cg_scoreBoardOld.integer) {
		if (cg.menuScoreboard) {
			cg.menuScoreboard->window.flags &= ~WINDOW_FORCED;
		}
		if (cg_paused.integer) {
			cg.deferredPlayerLoading = 0;
			firstTime = qtrue;
			return qfalse;
		}

		// should never happen in Team Arena
		if (cgs.gametype == GT_SINGLE_PLAYER && cg.predictedPlayerState.pm_type == PM_INTERMISSION ) {
			cg.deferredPlayerLoading = 0;
			firstTime = qtrue;
			return qfalse;
		}


		if (cg.snap->ps.pm_type == PM_INTERMISSION) {
			if (cgs.gametype == GT_FFA  &&  !cg_scoreBoardOld.integer) {
				cg.menuScoreboard = Menus_FindByName("endscore_menu_ffa");
			} else if (CG_IsDuelGame(cgs.gametype)  &&  !cg_scoreBoardOld.integer) {
				cg.menuScoreboard = Menus_FindByName("endscore_menu_duel");
			} else if (cgs.gametype == GT_TEAM  &&  !cg_scoreBoardOld.integer) {
				cg.menuScoreboard = Menus_FindByName("endteamscore_menu_tdm");
			} else if (cgs.gametype == GT_CA  &&  !cg_scoreBoardOld.integer) {
				cg.menuScoreboard = Menus_FindByName("endteamscore_menu_ca");
			} else if ((cgs.gametype == GT_CTF  ||  cgs.gametype == GT_CTFS  ||  cgs.gametype == GT_NTF)  &&  !cg_scoreBoardOld.integer) {
				cg.menuScoreboard = Menus_FindByName("endteamscore_menu_ctf");
			} else if (cgs.gametype == GT_FREEZETAG  &&  !cg_scoreBoardOld.integer) {
				cg.menuScoreboard = Menus_FindByName("endteamscore_menu_ft");
			} else if (cgs.gametype == GT_1FCTF  &&  !cg_scoreBoardOld.integer) {
				cg.menuScoreboard = Menus_FindByName("endteamscore_menu_1fctf");
			} else if (cgs.gametype == GT_HARVESTER  &&  !cg_scoreBoardOld.integer) {
				cg.menuScoreboard = Menus_FindByName("endteamscore_menu_har");
			} else if (cgs.gametype == GT_DOMINATION  &&  !cg_scoreBoardOld.integer) {
				cg.menuScoreboard = Menus_FindByName("endteamscore_menu_dom");
			} else if (cgs.gametype == GT_CTFS  &&  !cg_scoreBoardOld.integer) {
				cg.menuScoreboard = Menus_FindByName("endteamscore_menu_ad");
			} else if (cgs.gametype == GT_RED_ROVER  &&  !cg_scoreBoardOld.integer) {
				cg.menuScoreboard = Menus_FindByName("endscore_menu_ffa");
			} else if (cgs.gametype == GT_RACE  &&  !cg_scoreBoardOld.integer) {
				cg.menuScoreboard = Menus_FindByName("endscore_menu_race");
			} else {
				if (CG_IsTeamGame(cgs.gametype)  &&  cgs.gametype != GT_RED_ROVER ) {
					cg.menuScoreboard = Menus_FindByName("endteamscore_menu");
					if (!cg.menuScoreboard) {
						Com_Printf("couldn't find teamscore_menu\n");
					}
				} else {
					cg.menuScoreboard = Menus_FindByName("endscore_menu");
					if (!cg.menuScoreboard) {
						Com_Printf("couldn't find score_menu\n");
					}
				}
			}
		} else {
			if (cgs.gametype == GT_FFA  &&  !cg_scoreBoardOld.integer) {
				cg.menuScoreboard = Menus_FindByName("score_menu_ffa");
			} else if (CG_IsDuelGame(cgs.gametype)  &&  !cg_scoreBoardOld.integer) {
				cg.menuScoreboard = Menus_FindByName("score_menu_duel");
			} else if (cgs.gametype == GT_TEAM  &&  !cg_scoreBoardOld.integer) {
				cg.menuScoreboard = Menus_FindByName("teamscore_menu_tdm");
			} else if (cgs.gametype == GT_CA  &&  !cg_scoreBoardOld.integer) {
				cg.menuScoreboard = Menus_FindByName("teamscore_menu_ca");
			} else if ((cgs.gametype == GT_CTF  ||  cgs.gametype == GT_CTFS  ||  cgs.gametype == GT_NTF)  &&  !cg_scoreBoardOld.integer) {
				cg.menuScoreboard = Menus_FindByName("teamscore_menu_ctf");
			} else if (cgs.gametype == GT_FREEZETAG  &&  !cg_scoreBoardOld.integer) {
				cg.menuScoreboard = Menus_FindByName("teamscore_menu_ft");
			} else if (cgs.gametype == GT_1FCTF  &&  !cg_scoreBoardOld.integer) {
				cg.menuScoreboard = Menus_FindByName("teamscore_menu_1fctf");
			} else if (cgs.gametype == GT_HARVESTER  &&  !cg_scoreBoardOld.integer) {
				cg.menuScoreboard = Menus_FindByName("teamscore_menu_har");
			} else if (cgs.gametype == GT_DOMINATION  &&  !cg_scoreBoardOld.integer) {
				cg.menuScoreboard = Menus_FindByName("teamscore_menu_dom");
			} else if (cgs.gametype == GT_CTFS  &&  !cg_scoreBoardOld.integer) {
				cg.menuScoreboard = Menus_FindByName("teamscore_menu_ad");
			} else if (cgs.gametype == GT_RED_ROVER  &&  !cg_scoreBoardOld.integer) {
				cg.menuScoreboard = Menus_FindByName("score_menu_ffa");
			} else if (cgs.gametype == GT_RACE  &&  !cg_scoreBoardOld.integer) {
				cg.menuScoreboard = Menus_FindByName("score_menu_race");
			} else {
				if (CG_IsTeamGame(cgs.gametype)  &&  cgs.gametype != GT_RED_ROVER) {
					cg.menuScoreboard = Menus_FindByName("teamscore_menu");
					if (!cg.menuScoreboard) {
						Com_Printf("couldn't find teamscore_menu\n");
					}
				} else {
					cg.menuScoreboard = Menus_FindByName("score_menu");
					if (!cg.menuScoreboard) {
						Com_Printf("couldn't find score_menu\n");
					}
				}
			}
		}

		if (cg.menuScoreboard) {
			if (firstTime) {
				CG_SetScoreSelection(cg.menuScoreboard);
				firstTime = qfalse;
			}
			Menu_HandleCapture();
			Menu_Paint(cg.menuScoreboard, qtrue);
		}

		// load any models that have been deferred
		if ( ++cg.deferredPlayerLoading > 10 ) {
			CG_LoadDeferredPlayers();
		}

		return qtrue;
	} else {
		QLWideScreen = cg_scoreBoardOldWideScreen.integer;
		return CG_DrawOldScoreboard();
	}
}

static void CG_DrawScoreboardMenuCursor (void)
{
	float w = 32;
	float h = 32;

	float x = cgs.cursorX - 16;
	float y = cgs.cursorY - 16;

	if (!cgs.glconfig.isFullscreen  &&  SC_Cvar_Get_Int("in_nograb")) {
		return;
	}

	QLWideScreen = WIDESCREEN_STRETCH;  //cg_scoreBoardCursorAreaWideScreen.integer;
	// don't stretch the cursor in widescreen mode
	if (cg_wideScreen.integer == 5  ||  cg_wideScreen.integer == 6  ||  cg_wideScreen.integer == 7) {
		float aspect;
		float width43;
		float newXScale;

		//FIXME duplicate code
		//FIXME store calculations
		aspect = (float)cgs.glconfig.vidWidth / (float)cgs.glconfig.vidHeight;
		if (aspect > 1.25f) {
			width43 = 4.0f * (cgs.glconfig.vidHeight / 3.0);
			newXScale = width43 / (float)cgs.glconfig.vidWidth;
			w *= newXScale;
		}
	}

	CG_DrawPic(x, y, w, h, cgs.media.selectCursor);

	// testing
	//QLWideScreen = WIDESCREEN_CENTER;
	//CG_DrawPic(x, y, w, h, cgs.media.plasmaBallShader);
}

/*
=================
CG_DrawIntermission
=================
*/
static void CG_DrawIntermission( void ) {
//	int key;
#ifdef MISSIONPACK
	//if (cg_singlePlayer.integer) {
	//	CG_DrawCenterString();
	//	return;
	//}
#else
	if ( cgs.gametype == GT_SINGLE_PLAYER ) {
		CG_DrawCenterString();
		return;
	}
#endif
	cg.scoreFadeTime = cg.time;
	cg.scoreBoardShowing = CG_DrawScoreboard();
	CG_DrawAccStats();
	CG_DrawEchoPopup();
	CG_DrawErrorPopup();
	if (cg.scoreBoardShowing) {
		CG_DrawScoreboardMenuCursor();
	}
}

/*
=================
CG_DrawFollow
=================
*/
static qboolean CG_DrawFollow( void ) {
	//float		x;
	vec4_t		color;
	int style, align;
	float scale;
	const fontInfo_t *font;
	//const char	*name;
	float x, y, w;
	//int h;
	const char *s;
	const char *clanTag;

	if (wolfcam_following) {
		return qfalse;  //FIXME wolfcam  -- 2018-06-05 duplicate code Wolfcam_DrawFollowing()
	}

	if (!cg_drawFollowing.integer) {
		return qtrue;
	}

	if ( !(cg.snap->ps.pm_flags & PMF_FOLLOW)  &&  cg_drawFollowing.integer != 2) {
		return qfalse;
		//return qtrue;
	}

	SC_Vec4ColorFromCvars(color, &cg_drawFollowingColor, &cg_drawFollowingAlpha);
	align = cg_drawFollowingAlign.integer;
	scale = cg_drawFollowingScale.value;
	style = cg_drawFollowingStyle.integer;
	QLWideScreen = cg_drawFollowingWideScreen.integer;

	if (*cg_drawFollowingFont.string) {
		font = &cgs.media.followingFont;
	} else {
		font = &cgDC.Assets.textFont;
	}
	x = cg_drawFollowingX.value;
	y = cg_drawFollowingY.value;

	clanTag = cgs.clientinfo[cg.snap->ps.clientNum].clanTag;
	if (*clanTag) {
		s = va("^7%s ^7%s", clanTag, cgs.clientinfo[cg.snap->ps.clientNum].name);
	} else {
		s = cgs.clientinfo[cg.snap->ps.clientNum].name;
	}

	// testing UTF-8
	//s = "Τη γλώσσα μου έδωσαν ελληνική";
	//s = "Τη γλώσσα";  // μου έδωσαν ελληνική";
	//s = "ABCD₪ Τη γλώσσαEFGHI";

	w = CG_Text_Width(s, scale, 0, font);
	//h = CG_Text_Height(s, scale, 0, font);
	if (align == 1) {
		x -= w / 2;
	} else if (align == 2) {
		x -= w;
	}

	CG_Text_Paint_Bottom(x, y, scale, color, s, 0, 0, style, font);

	return qtrue;
}



/*
=================
CG_DrawAmmoWarning
=================
*/
static void CG_DrawAmmoWarning( void ) {
	const char	*s;
	float x, y, w;
	float scale;
	int align;
	vec4_t color;
	const fontInfo_t *font;
	int weapon;
	int clientNum;

	if (cg_drawAmmoWarning.integer == 0) {
		return;
	}

	if (cg.lowAmmoWarning == AMMO_WARNING_OK) {
		return;
	}

	if (cg.freecam) {
		return;
	}

	weapon = cg.snap->ps.weapon;
	clientNum = cg.snap->ps.clientNum;

	if (wolfcam_following) {
		weapon = cg_entities[wcg.clientNum].currentState.weapon;
		clientNum = wcg.clientNum;

		if (CG_IsCpmaMvd()) {
			if (cgs.clientinfo[wcg.clientNum].team == TEAM_SPECTATOR) {
				return;
			}
		} else {
			if (wcg.clientNum != cg.snap->ps.clientNum) {
				return;
			}
		}
	}

	if (cg.snap->ps.pm_type == PM_SPECTATOR  &&  cg.snap->ps.clientNum == cg.clientNum) {
		return;
	}

	if (CG_IsCpmaMvd()) {
		//FIXME need check for cpma frozen player
		if (cgs.gametype == GT_FREEZETAG) {
			//return;
		}
	} else {
		if (cgs.gametype == GT_FREEZETAG  &&  CG_FreezeTagFrozen(clientNum)) {
			return;
		}
	}

	//FIXME is this check needed, should be done in CG_CheckAmmo() ?
	if (weapon == WP_GAUNTLET) {
		return;
	}

	if (*cg_drawAmmoWarningFont.string) {
		font = &cgs.media.ammoWarningFont;
	} else {
		font = &cgDC.Assets.textFont;
	}

	if (cg_drawAmmoWarning.integer == 2  &&  cg.lowAmmoWarning != AMMO_WARNING_EMPTY) {
		return;
	}

	QLWideScreen = cg_drawAmmoWarningWideScreen.integer;

	scale = cg_drawAmmoWarningScale.value;
	SC_Vec4ColorFromCvars(color, &cg_drawAmmoWarningColor, &cg_drawAmmoWarningAlpha);
	align = cg_drawAmmoWarningAlign.integer;

	if (cg.lowAmmoWarning == AMMO_WARNING_EMPTY) {
		s = "OUT OF AMMO";
	} else {
		s = "LOW AMMO WARNING";
	}

	//w = CG_DrawStrlen( s, &cgs.media.bigchar );
	w = CG_Text_Width(s, scale, 0, font);

	x = cg_drawAmmoWarningX.value;
	if (align == 1) {
		x -= w / 2;
	} else if (align == 2) {
		x -= w;
	}
	y = cg_drawAmmoWarningY.value;

	//CG_DrawBigString(x, y, s, 1.0F);
	CG_Text_Paint_Bottom(x, y, scale, color, s, 0, 0, cg_drawAmmoWarningStyle.integer, font);
}


#if 1  //def MPACK
/*
=================
CG_DrawProxWarning
=================
*/
static void CG_DrawProxWarning (void)
{
	vec4_t color;
	int style, align;
	float scale;
	const fontInfo_t *font;
	float x, y, w;
	const char *s;

	if (!(cg.snap->ps.eFlags & EF_TICKING)) {
		return;
	}

	if (wolfcam_following) {
		return;  //FIXME wolfcam
	}

	if (!cg_drawProxWarning.integer) {
		return;
	}

	SC_Vec4ColorFromCvars(color, &cg_drawProxWarningColor, &cg_drawProxWarningAlpha);
	align = cg_drawProxWarningAlign.integer;
	scale = cg_drawProxWarningScale.value;
	style = cg_drawProxWarningStyle.integer;
	QLWideScreen = cg_drawProxWarningWideScreen.integer;

	if (*cg_drawProxWarningFont.string) {
		font = &cgs.media.proxWarningFont;
	} else {
		font = &cgDC.Assets.textFont;
	}

	x = cg_drawProxWarningX.value;
	y = cg_drawProxWarningY.value;


	if (cg.proxTick != 0) {
		s = va("INTERNAL COMBUSTION IN: %i", cg.proxTick);
	} else {
		s = "YOU HAVE BEEN MINED";
	}

	w = CG_Text_Width(s, scale, 0, font);
	if (align == 1) {
		x -= w / 2;
	} else if (align == 2) {
		x -= w;
	}
	CG_Text_Paint_Bottom(x, y, scale, color, s, 0, 0, style, font);
}
#endif


/*
=================
CG_DrawWarmup
=================
*/
static void CG_WarmupAnnouncements (void)
{
	int sec;
	int remainder;

	sec = cg.warmup;

	if (!sec) {
		return;
	}

	if (sec < 0) {
		return;
	}

	sec = ( sec - cg.time ) / 1000;
	if (cgs.cpma   &&  sec < -1) {  //FIXME hack for cpma ca demos .. FIX
		cg.warmup = 0;
		sec = 0;
		return;
	}

	if ( sec < 0 ) {
		cg.warmup = 0;
		sec = 0;
	}

	//Com_Printf("warmupcount %d\n", cg.warmupCount);

	if (cg.warmupCount == -1) {
		if (sec > 0) {
			remainder = (cg.warmup - cg.time) % (sec * 1000);
		} else {
			remainder = cg.warmup - cg.time;
		}
		if (remainder > 50) {
			cg.warmupCount = sec;
			return;
		}
	}

	if ( sec != cg.warmupCount ) {
		cg.warmupCount = sec;
		//Com_Printf("^5%d  %d!!!\n", sec + 1, remainder);
		if (cg_audioAnnouncerWarmup.integer) {
			switch ( sec ) {
			case 8:
				if (CG_IsTeamGame(cgs.gametype)  &&  cgs.gametype != GT_RED_ROVER) {
					CG_StartLocalSound(cgs.media.countPrepareTeamSound, CHAN_ANNOUNCER);
				} else {
					CG_StartLocalSound(cgs.media.countPrepareSound, CHAN_ANNOUNCER);
				}
				break;
			case 0:
				CG_StartLocalSound( cgs.media.count1Sound, CHAN_ANNOUNCER );
				break;
			case 1:
				CG_StartLocalSound( cgs.media.count2Sound, CHAN_ANNOUNCER );
				break;
			case 2:
				CG_StartLocalSound( cgs.media.count3Sound, CHAN_ANNOUNCER );
				break;
			default:
				break;
			}
		}
	}

}

static void CG_DrawWarmup( void ) {
	float w;  //, h;
	int			sec;
	int			i;
	float scale;
	const clientInfo_t	*ci1, *ci2;
	//int			cw;
	const char	*s;
	float x;
	float y;
	int align;
	vec4_t color;
	int style;
	const fontInfo_t *font;
	int readyUpTime;

	//Com_Printf("drawwarmup()  cg.warmup %d   cg.time %d\n", cg.warmup, cg.time);
	sec = cg.warmup;
	if ( !sec ) {
		return;
	}

	if ( sec < 0 ) {
		if (!cg_drawWaitingForPlayers.integer) {
			return;
		}
		SC_Vec4ColorFromCvars(color, &cg_drawWaitingForPlayersColor, &cg_drawWaitingForPlayersAlpha);
		align = cg_drawWaitingForPlayersAlign.integer;
		scale = cg_drawWaitingForPlayersScale.value;
		style = cg_drawWaitingForPlayersStyle.integer;
		QLWideScreen = cg_drawWaitingForPlayersWideScreen.integer;

		if (*cg_drawWaitingForPlayersFont.string) {
			font = &cgs.media.waitingForPlayersFont;
		} else {
			font = &cgDC.Assets.textFont;
		}
		x = cg_drawWaitingForPlayersX.value;
		y = cg_drawWaitingForPlayersY.value;

		if (cgs.protocolClass == PROTOCOL_QL  &&  *CG_ConfigString(CS_READY_UP_TIME)) {
			readyUpTime = atoi(CG_ConfigString(CS_READY_UP_TIME));
			if (readyUpTime > cg.time) {
				int nx;

				nx = x;
				s = "Players must ready";
				w = CG_Text_Width(s, scale, 0, font);
				if (align == 1) {
					nx -= w / 2;
				} else if (align == 2) {
					nx -= w;
				}
				CG_Text_Paint_Bottom(nx, y, scale, color, s, 0, 0, style, font);
				y += CG_Text_Height(s, scale, 0, font) * 1.75;
				//FIXME periods..
				s = va("within %d seconds", (readyUpTime - cg.time) / 1000 + 1);
			} else {
				s = "Waiting for players";
			}
		} else {
			s = "Waiting for players";
		}

		w = CG_Text_Width(s, scale, 0, font);
		if (align == 1) {
			x -= w / 2;
		} else if (align == 2) {
			x -= w;
		}
		CG_Text_Paint_Bottom(x, y, scale, color, s, 0, 0, style, font);
		cg.warmupCount = 0;
		return;
	}

	if (!cg_drawWarmupString.integer) {
		return;
	}

	if (*cg_drawWarmupStringFont.string) {
		font = &cgs.media.warmupStringFont;
	} else {
		font = &cgDC.Assets.textFont;
	}
	QLWideScreen = cg_drawWarmupStringWideScreen.integer;

	align = cg_drawWarmupStringAlign.integer;
	SC_Vec4ColorFromCvars(color, &cg_drawWarmupStringColor, &cg_drawWarmupStringAlpha);

	if (CG_IsDuelGame(cgs.gametype)) {
		// find the two active players
		ci1 = NULL;
		ci2 = NULL;
		for ( i = 0 ; i < cgs.maxclients ; i++ ) {
			if ( cgs.clientinfo[i].infoValid && cgs.clientinfo[i].team == TEAM_FREE ) {
				if ( !ci1 ) {
					ci1 = &cgs.clientinfo[i];
				} else {
					ci2 = &cgs.clientinfo[i];
				}
			}
		}

		if ( ci1 && ci2 ) {
			s = va( "%s ^7vs %s", ci1->name, ci2->name );
			//w = CG_Text_Width(s, 0.6f, 0, &cgDC.Assets.textFont);
			w = CG_Text_Width(s, cg_drawWarmupStringScale.value, 0, font);
			x = cg_drawWarmupStringX.value;
			y = cg_drawWarmupStringY.value;
			if (align == 1) {
				x -= w / 2;
			} else if (align == 2) {
				x -= w;
			}

			CG_Text_Paint(x, y, cg_drawWarmupStringScale.value, color, s, 0, 0, cg_drawWarmupStringStyle.integer, font);
		}
	} else {
		if ( cgs.gametype == GT_FFA ) {
			s = "Free For All";
		} else if ( cgs.gametype == GT_TEAM ) {
			s = "Team Deathmatch";
		} else if ( cgs.gametype == GT_CTF ) {
			s = "Capture the Flag";
		} else if ( cgs.gametype == GT_CA ) {
			s = "Clan Arena";
#if 1  //def MPACK
		} else if ( cgs.gametype == GT_1FCTF ) {
			s = "One Flag CTF";
		} else if ( cgs.gametype == GT_OBELISK ) {
			s = "Overload";
		} else if ( cgs.gametype == GT_HARVESTER ) {
			s = "Harvester";
#endif
		} else if (cgs.gametype == GT_FREEZETAG) {
			s = "Freeze Tag";
		} else if (cgs.gametype == GT_CTFS) {
			if (cgs.cpma) {
				s = "Capture Strike";
			} else {
				s = "Attack and Defend";
			}
		} else if (cgs.gametype == GT_DOMINATION) {
			s = "Domination";
		} else if (cgs.gametype == GT_RED_ROVER) {
			s = "Red Rover";
		} else if (cgs.gametype == GT_NTF) {
			s = "Not Team Fortress";
		} else if (cgs.gametype == GT_2V2) {
			s = "Two vs. Two";
		} else if (cgs.gametype == GT_HM) {
			s = "Hoonymode";
		} else if (cgs.gametype == GT_RACE) {
			s = "Race";
		} else if (cgs.gametype == GT_SINGLE_PLAYER) {
			s = "Single Player";
		} else {
			s = va("unknown: %d", cgs.gametype);
		}
#if 1  //def MPACK
		//w = CG_Text_Width(s, 0.6f, 0, &cgDC.Assets.textFont);
		w = CG_Text_Width(s, cg_drawWarmupStringScale.value, 0, font);
		x = cg_drawWarmupStringX.value;
		y = cg_drawWarmupStringY.value + CG_Text_Height(s, cg_drawWarmupStringScale.value, 0, font);  //FIXME 30 ?  not text height?
		if (align == 1) {
			x -= w / 2;
		} else if (align == 2) {
			x -= w;
		}

		CG_Text_Paint(x, y, cg_drawWarmupStringScale.value, color, s, 0, 0, cg_drawWarmupStringStyle.integer, font);
#else
		//FIXME width
		w = CG_DrawStrlen( s, &cgs.media.giantchar ) / GIANTCHAR_WIDTH;
		if ( w > 640 / GIANT_WIDTH ) {
			cw = 640 / w;
		} else {
			cw = GIANT_WIDTH;
		}
		if (cg_drawWarmupStringX.string[0] != '\0') {
			x = cg_drawWarmupStringX.value;
		} else {
			x = 320 - w * cw/2;
		}
		if (cg_drawWarmupStringY.string[0] != '\0') {
			y = cg_drawWarmupStringY.value + 5.0f;
		} else {
			y = 25;
		}
		CG_DrawStringExt( 320 - w * cw/2, 25,s, colorWhite,
						  qfalse, qtrue, cw, (int)(cw * 1.1f), 0, &cgs.media.giantchar );
#endif
	}

	sec = ( sec - cg.time ) / 1000;
	if ( sec < 0 ) {
		cg.warmup = 0;
		sec = 0;
	}
	s = va( "Starts in: %i", sec + 1 );

	switch ( cg.warmupCount ) {
	case 0:
		//cw = 28;
		scale = 0.54f;
		break;
	case 1:
		//cw = 24;
		scale = 0.51f;
		break;
	case 2:
		//cw = 20;
		scale = 0.48f;
		break;
	default:
		//cw = 16;
		scale = 0.45f;
		break;
	}


#if 1  //def MPACK
	// no thanks
	scale = 0.45;

	//w = CG_Text_Width(s, scale, 0, &cgDC.Assets.textFont);
	w = CG_Text_Width(s, cg_drawWarmupStringScale.value, 0, font);
	//Com_Printf("height: %d\n", CG_Text_Height(s, cg_drawWarmupStringScale.value, 0, &cgs.media.warmupStringFont));
	x = cg_drawWarmupStringX.value;
	y = cg_drawWarmupStringY.value + 3.0f * CG_Text_Height(s, cg_drawWarmupStringScale.value, 0, font);  //FIXME 65 ?  not text height?
	if (align == 1) {
		x -= w / 2;
	} else if (align == 2) {
		x -= w;
	}

	CG_Text_Paint(x, y, cg_drawWarmupStringScale.value, color, s, 0, 0, cg_drawWarmupStringStyle.integer, font);

#else
	//cw = 16;
	scale = 0.45f;
	//FIXME width
	w = CG_DrawStrlen( s, &cgs.media.giantchar ) / GIANTCHAR_WIDTH;
	if (cg_drawWarmupStringX.string[0] != '\0') {
		x = cg_drawWarmupStringX.value;
	} else {
		x = 320 - w * cw/2;
	}
	if (cg_drawWarmupStringY.string[0] != '\0') {
		y = cg_drawWarmupStringY.value + 50.0f;
	} else {
		y = 70;
	}
	CG_DrawStringExt(x, y + CG_Text_Height(s, scale, 0, &cgDC.Assets.textFont)+ 8, s, colorWhite,
					  qfalse, qtrue, cw, (int)(cw * 1.5), 0, &cgs.media.giantchar );
#endif
}

//==================================================================================
#ifdef MISSIONPACK
/*
=================
CG_DrawTimedMenus
=================
*/
static void CG_DrawTimedMenus( void ) {
	if (cg.voiceTime) {
		int t = cg.time - cg.voiceTime;
		if ( t > 2500 ) {
			Menus_CloseByName("voiceMenu");
			trap_Cvar_Set("cl_conXOffset", "0");
			cg.voiceTime = 0;
		}
	}
}
#endif

//FIXME scaling
static void CG_DrawAccStats (void)
{
	vec4_t color;
	float x, y;
	int i;
	const wclient_t *wc;
	const wweaponStats_t *ws;
	int clientNum;
	int yoffset;
	int acc;
	int windowWidth;
	int windowHeight;

	if (!cg.drawAccStats) {
		return;
	}
	yoffset = 15;

	if (wolfcam_following) {
		clientNum = wcg.clientNum;
	} else {
		clientNum = cg.snap->ps.clientNum;
	}

	wc = &wclients[clientNum];
	ws = wc->wstats;

	VectorSet(color, 255, 0, 0);
	color[3] = 0.3;  //cg_accAlpha.value / 255.0;  //0.3;

	//CG_FillRect(100, 100, 48, 48, colorRed);
	x = cg_accX.value;  //450;  //20;
	y = cg_accY.value;  //100;
	QLWideScreen = cg_accWideScreen.integer;

	windowWidth = 80;
	windowHeight = WP_NUM_WEAPONS * yoffset + 10;

	CG_FillRect(x - 10, y - 10, windowWidth, windowHeight, color);
	//y += yoffset;

	//if (clientNum == cg.serverAccuracyStatsClientNum) {
	if (1) {
		if (cg.serverAccuracyStatsTime == 0) {
			CG_Text_Paint(x + 10, y, 0.2, colorWhite, "no stats", 0, 0, 0, &cgDC.Assets.textFont);
		} else {
			int mins, secs, t;

			t = cg.time - cg.serverAccuracyStatsTime;
			t /= 1000;
			mins = t / 60;
			secs = t - (mins * 60);
			CG_Text_Paint(x + 10, y, 0.2, colorWhite, va("%d:%02d", mins, secs), 0, 0, 0, &cgDC.Assets.textFont);
		}
	}
	y += yoffset;

	for (i = WP_GAUNTLET;  i < WP_NUM_WEAPONS;  i++) {
		//Com_Printf("%d %f\n", i, (float)ws[i].hits / (float)ws[i].atts);
		if (!cg_weapons[i].registered) {
			continue;
		}

		if (1) {  //(ws[i].atts) {
			//CG_Text_Paint(x, y, 0.2, colorWhite, weapNamesCasual[i], 0, 0, 0, &cgDC.Assets.textFont);
			CG_DrawPic(x, y - 9, 10, 10, cg_weapons[i].weaponIcon);
			if (cg.serverAccuracyStatsTime  &&  clientNum == cg.serverAccuracyStatsClientNum) {
				acc = cg.serverAccuracyStats[i];
				CG_Text_Paint(x + windowWidth - 55, y, 0.2, colorWhite, va("%d", acc), 0, 0, 0, &cgDC.Assets.textFont);
			}
			if (ws[i].atts) {
				acc = (int)(((float)ws[i].hits / (float)ws[i].atts) * 100.0);
			} else {
				acc = 0;
			}
			CG_Text_Paint(x + windowWidth - 30, y, 0.2, colorWhite, va("^3%d", acc), 0, 0, 0, &cgDC.Assets.textFont);
			y += yoffset;
		}
	}

}

static void CG_DrawErrorPopup (void)
{
	//int w;
	float h, x, y;
	float scale;
	vec4_t color;

	if (cg.realTime - cg.errorPopupStartTime > cg.errorPopupTime) {
		return;
	}

	//FIXME cvars to control

	QLWideScreen = WIDESCREEN_LEFT;

	scale = 0.3;  //cg.echoPopupScale;
	x = 0;  //cg.echoPopupX;
	y = 5;  //cg.echoPopupY;

	//w = CG_Text_Width(cg.errorPopupString, scale, 0, &cgDC.Assets.textFont);
	h = CG_Text_Height(cg.errorPopupString, scale, 0, &cgDC.Assets.textFont);

	x = 5;
	y = 5 + h;
	color[0] = 0.0;
	color[1] = 0.0;
	color[2] = 0.0;
	color[3] = 1.0;

	//CG_FillRect(x - 5, y  - h, w + 20, h + 10, color);
	//CG_FillRect(x , y - h , w , h , color);
	//CG_FillRect(x - w/10, y - h - h/10 , w + w/5 , h + h/5 , color);
	CG_FillRect(0, y - h - h / 2, 640, h + h / 1, color);
	CG_Text_Paint(x, y, scale, colorRed, cg.errorPopupString, 0, 0, 0, &cgDC.Assets.textFont);
}

static void CG_DrawEchoPopup (void)
{
	float w, h, x, y;
	float scale;
	vec4_t color;

	if (cg.realTime - cg.echoPopupStartTime > cg.echoPopupTime) {
		return;
	}

	scale = cg.echoPopupScale;
	x = cg.echoPopupX;
	y = cg.echoPopupY;
	QLWideScreen = cg.echoPopupWideScreen;

	w = CG_Text_Width(cg.echoPopupString, scale, 0, &cgDC.Assets.textFont);
	h = CG_Text_Height(cg.echoPopupString, scale, 0, &cgDC.Assets.textFont);

	color[0] = 1.0;
	color[1] = 0.0;
	color[2] = 0.0;
	color[3] = 0.2;

	//CG_FillRect(x - 5, y  - h, w + 20, h + 10, color);
	//CG_FillRect(x , y - h , w , h , color);
	CG_FillRect(x - w/10, y - h - h/10 , w + w/5 , h + h/5 , color);
	CG_Text_Paint(x, y, scale, colorWhite, cg.echoPopupString, 0, 0, 0, &cgDC.Assets.textFont);
}

static void CG_DrawDominationPointStatus (void)
{
	int i;
	const centity_t *cent;
	const char *s;
	float textScale;
	int style;
	const fontInfo_t *font;
	float x, y;
	int team;
	char pointName;
	rectDef_t rect;
	int align;
	float scale;
	vec4_t barColor;
	vec4_t textColor;
	vec4_t backgroundColor;
	vec4_t colorRedr;
	vec4_t colorBluer;

	if (!cg_drawDominationPointStatus.integer) {
		return;
	}

	if (wolfcam_following) {
		team = cgs.clientinfo[wcg.clientNum].team;
	} else {
		team = cg.snap->ps.persistant[PERS_TEAM];
	}

	x = cg_drawDominationPointStatusX.value;  //258;
	y = cg_drawDominationPointStatusY.value;  //365;
	scale = cg_drawDominationPointStatusScale.value;
	QLWideScreen = cg_drawDominationPointStatusWideScreen.integer;

	textScale = 0.25 * scale;
	if (*cg_drawDominationPointStatusFont.string) {
		font = &cgs.media.dominationPointStatusFont;
	} else {
		font = &cgDC.Assets.textFont;
	}

	//&cgs.media.qlfont16;

	style = cg_drawDominationPointStatusTextStyle.integer;  //ITEM_TEXTSTYLE_SHADOWED;
	align = 1;

	if (*cg_drawDominationPointStatusEnemyColor.string) {
		SC_Vec3ColorFromCvar(barColor, &cg_drawDominationPointStatusEnemyColor);
		barColor[3] = cg_drawDominationPointStatusAlpha.value / 255.0;
		if (team == TEAM_RED) {
			Vector4Copy(barColor, colorBluer);
		} else {
			Vector4Copy(barColor, colorRedr);
		}
	} else {
		if (team == TEAM_RED) {
			Vector4Set(colorBluer, 0, 0.5, 1, cg_drawDominationPointStatusAlpha.value / 255.0);
		} else {
			Vector4Set(colorRedr, 1, 0, 0, cg_drawDominationPointStatusAlpha.value / 255.0);
		}
	}

	if (*cg_drawDominationPointStatusTeamColor.string) {
		SC_Vec3ColorFromCvar(barColor, &cg_drawDominationPointStatusTeamColor);
		barColor[3] = cg_drawDominationPointStatusAlpha.value / 255.0;
		if (team == TEAM_RED) {
			Vector4Copy(barColor, colorRedr);
		} else {
			Vector4Copy(barColor, colorBluer);
		}
	} else {
		if (team == TEAM_RED) {
			Vector4Set(colorRedr, 1, 0, 0, cg_drawDominationPointStatusAlpha.value / 255.0);
		} else {
			Vector4Set(colorBluer, 0, 0.5, 1, cg_drawDominationPointStatusAlpha.value / 255.0);
		}
	}

	if (*cg_drawDominationPointStatusTextColor.string) {
		SC_Vec3ColorFromCvar(textColor, &cg_drawDominationPointStatusTextColor);
		textColor[3] = cg_drawDominationPointStatusTextAlpha.value / 255.0;
	} else {
		Vector4Set(textColor, 1, 1, 1, cg_drawDominationPointStatusTextAlpha.value / 255.0);
	}

	for (i = 0;  i < 5;  i++) {
		cent = cgs.dominationControlPointEnts[i];
		if (!cent) {
			break;
		}

		if (Distance(cent->lerpOrigin, cg.snap->ps.origin) > DOMINATION_POINT_DISTANCE) {
			continue;
		}

		pointName = 'A' + (cent->currentState.powerups - 1);

		SC_Vec3ColorFromCvar(backgroundColor, &cg_drawDominationPointStatusBackgroundColor);
		backgroundColor[3] = cg_drawDominationPointStatusAlpha.value / 255.0;

		CG_FillRect(x, y, 124.0 * scale, 10.0 * scale, backgroundColor);
		if (team == cent->currentState.modelindex2) {  // defending
			if (team == TEAM_RED) {
				CG_FillRect(x, y, scale * 124.0 * ((float)cent->currentState.generic1 / 100.0), scale * 10.0, colorRedr);
			} else if (team == TEAM_BLUE) {
				CG_FillRect(x, y, scale * 124.0 * ((float)cent->currentState.generic1 / 100.0), 10 * scale, colorBluer);
			}
			rect.x = x + scale * (124 / 2);
			rect.y = y - CG_Text_Height("T", textScale, 0, font) * 0.5;

			s = va("Defending %c", pointName);
			CG_Text_Paint_Align(&rect, textScale, textColor, s, 0, 0, style, font, align);

		} else {  //if (cent->currentState.modelindex2 == 0) {
			// capturing
			if (cent->currentState.modelindex2 == TEAM_RED) {
				CG_FillRect(x, y, scale * 124.0 * (( (float)cent->currentState.generic1) / 100.0), scale * 10.0, colorRedr);
			} else if (cent->currentState.modelindex2 == TEAM_BLUE) {
				CG_FillRect(x, y, scale * 124.0 * (( (float)cent->currentState.generic1) / 100.0), scale * 10.0, colorBluer);
			} else {
				if (team == TEAM_RED) {
					CG_FillRect(x, y, scale * 124.0 * ((float)cent->currentState.generic1 / 100.0), scale * 10.0, colorRedr);
				} else if (team == TEAM_BLUE) {
					CG_FillRect(x, y, scale * 124.0 * ((float)cent->currentState.generic1 / 100.0), scale * 10.0, colorBluer);
				}
			}
			if (cent->currentState.otherEntityNum2 > 1) {
				s = va("Capturing %c (%dx)", pointName, cent->currentState.otherEntityNum2);
			} else {
				s = va("Capturing %c", pointName);
			}
			rect.x = x + scale * (124 / 2);
			rect.y = y - CG_Text_Height("T", textScale, 0, font) * 0.5;

			CG_Text_Paint_Align(&rect, textScale, textColor, s, 0, 0, style, font, align);
		}

		break;
	}
}

static void CG_DrawCtfsRoundScoreboard (void)
{
	rectDef_t rect;
	float x, y;
	vec4_t color;
	int style;
	//int align;
	float scale;
	int i;
	//float h;
	const fontInfo_t *font;

	if (!cg_roundScoreBoard.integer) {
		return;
	}

	if (cgs.gametype != GT_CTFS) {
		return;
	}

	if (cgs.roundStarted  ||  cgs.roundBeginTime <= 0) {
		return;
	}

	font = &cgDC.Assets.textFont;
	scale = 0.18;
	//align = ITEM_ALIGN_CENTER;
	style = ITEM_TEXTSTYLE_SHADOWED;

	QLWideScreen = WIDESCREEN_CENTER;

	x = 200;
	y = 152 + 20;

	Vector4Set(color, 0, 0, 0, 0.55);
	CG_FillRect(x, y, 240, 36, color);

	if (cgs.roundTurn % 2 == 0) {  // red on offense
		Vector4Set(color, 0.5, 0, 0, 0.5);
		CG_FillRect(x, y + 12, 204, 12, color);
	} else {
		Vector4Set(color, 0, 0.25, 0.5, 0.5);
		CG_FillRect(x, y + 12 * 2, 204, 12, color);
	}

	rect.x = x + 2;
	rect.y = y + 12 - 4;
	// 0xefeab4
	Vector4Set(color, 1, 1, 0.7, 1);
	//h = CG_Text_Height("Round", scale, 0, font);

	CG_Text_Paint_Align(&rect, scale, color, "Round", 0, 0, style, font, ITEM_ALIGN_LEFT);

	for (i = 1;  i < 11;  i++) {
		rect.x = (240 - 16) + (16 * i);
		rect.y = y + 12 - 4;

		CG_Text_Paint_Align(&rect, scale, color, va("%d", i), 0, 0, style, font, ITEM_ALIGN_CENTER);
	}

	rect.x = 404 + 4;
	rect.y = y + 12 - 4;
	CG_Text_Paint_Align(&rect, scale, color, "Score", 0, 0, style, font, ITEM_ALIGN_LEFT);

	// red
	rect.x = x + 2;
	rect.y = y + 24 - 4;
	Vector4Set(color, 1, 0, 0, 1);
	CG_Text_Paint_Align(&rect, scale, color, "Red", 0, 0, style, font, ITEM_ALIGN_LEFT);

	Vector4Set(color, 1, 1, 1, 1);
	rect.x = 240;

	//Com_Printf("num %d  turn %d\n", cgs.roundNum, cgs.roundTurn);

	for (i = 0;  i < 10  &&  cg.ctfsRoundScoreValid;  i++) {
		if (cg.ctfsRedRoundScores[i] < 0) {
			break;
		}
		CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.ctfsRedRoundScores[i]), 0, 0, style, font, ITEM_ALIGN_CENTER);
		rect.x += 16;
	}

	rect.x = 404 + 18;
	Vector4Set(color, 1, 0, 0, 1);
	CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.ctfsRedScore), 0, 0, style, font, ITEM_ALIGN_CENTER);

	// blue
	rect.x = x + 2;
	rect.y = y + 36 - 4;
	Vector4Set(color, 0, 0.5, 1, 1);
	CG_Text_Paint_Align(&rect, scale, color, "Blue", 0, 0, style, font, ITEM_ALIGN_LEFT);

	Vector4Set(color, 1, 1, 1, 1);
	rect.x = 240;

	for (i = 0;  i < 10  &&  cg.ctfsRoundScoreValid;  i++) {
		if (cg.ctfsBlueRoundScores[i] < 0) {
			break;
		}
		CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.ctfsBlueRoundScores[i]), 0, 0, style, font, ITEM_ALIGN_CENTER);
		rect.x += 16;
	}

	rect.x = 404 + 18;
	Vector4Set(color, 0, 0.5, 1, 1);
	CG_Text_Paint_Align(&rect, scale, color, va("%d", cg.ctfsBlueScore), 0, 0, style, font, ITEM_ALIGN_CENTER);

	if (cgs.roundTurn % 2 == 0) {  // red attacking
		rect.y = y + 12;
		//rect.x = 240 + 16 * (cgs.roundTurn / 2) - 8;
		rect.x = 240 + 16 * (i) - 8;
		//Com_Printf("x %f\n", rect.x);
		Vector4Set(color, 0.65, 0, 0, 1);
	} else {
		rect.y = y + 24;
		//rect.x = 240 + 16 * (cgs.roundTurn / 2) - 8;
		rect.x = 240 + 16 * (i) - 8;
		Vector4Set(color, 0, 0.30, 0.65, 1);
	}
	CG_FillRect(rect.x, rect.y, 16, 12, color);
}


/*
=================
CG_Draw2D
=================
*/
static void CG_Draw2D( void ) {
#ifdef MISSIONPACK
	if (cgs.orderPending && cg.time > cgs.orderTime) {
		CG_CheckOrderPending();
	}
#endif
	// if we are taking a levelshot for the menu, don't draw anything
	if ( cg.levelShot ) {
		return;
	}

	if (cg_fadeStyle.integer == 0) {
		CG_DrawFade();
	}

	if (cg.testMenu) {
		//QLWideScreen = WIDESCREEN_CENTER;
		Menu_HandleCapture();
		//Menu_PaintAll();
		Menu_Paint(cg.testMenu, qtrue);
		//CG_DrawPic(cgs.cursorX - 16, cgs.cursorY - 16, 32, 32, cgs.media.selectCursor);
		CG_DrawScoreboardMenuCursor();
		return;
	}

	if ( cg_draw2D.integer == 0 ) {
		CG_DrawEchoPopup();
		CG_DrawErrorPopup();
		CG_WarmupAnnouncements();
		CG_RoundAnnouncements();
		CG_RewardAnnouncements();
		CG_VoteAnnouncements();
		CG_TeamVoteAnnouncements();
		if (cg_fadeStyle.integer == 1) {
			CG_DrawFade();
		}
		return;
	}

	if (cg_draw2D.integer == 2) {
		const fontInfo_t *font;

		if (*cg_drawCameraPointInfoFont.string) {
			font = &cgs.media.cameraPointInfoFont;
		} else {
			font = &cgDC.Assets.textFont;
		}

		QLWideScreen = WIDESCREEN_LEFT;
		CG_Text_Paint(2, 16, 0.2, colorRed, "Camera Edit Hud (set cg_draw2d 1  to disable)", 0, 0, 1, font);


		if (cg.numCameraPoints > 0) {
			if (cg.cameraPlaying) {
				CG_Text_Paint(2, 32, 0.2, colorWhite, "^5Camera view is ^3locked ^5(camera is playing, use /stopcamera  to allow changing origin and angles)", 0, 0, 1, font);
			} else {
				if (cg_cameraQue.integer) {
					CG_Text_Paint(2, 32, 0.2, colorWhite, "^5Camera view is ^3locked ^5(set cg_cameraQue 0  to allow changing origin and angles)", 0, 0, 1, font);
				} else {
					CG_Text_Paint(2, 32, 0.2, colorWhite, "^5Camera view is ^7unlocked ^5(set cg_cameraQue 1  to lock and follow camera view)", 0, 0, 1, font);
				}
			}
		} else if (demo.camera.points) {
			// q3mme camera points present
			if (cg.cameraQ3mmePlaying) {
				CG_Text_Paint(2, 32, 0.2, colorWhite, "^6Q3mme Camera view is ^3locked ^6(camera is playing, use /stopq3mmecamera  to allow changing origin and angles)", 0, 0, 1, font);
			} else {
				if (cg_cameraQue.integer) {
					CG_Text_Paint(2, 32, 0.2, colorWhite, "^6Q3mme Camera view is ^3locked ^6(set cg_cameraQue 0  to allow changing origin and angles)", 0, 0, 1, font);
				} else {
					CG_Text_Paint(2, 32, 0.2, colorWhite, "^6Q3mme Camera view is ^7unlocked ^6(set cg_cameraQue 1  to lock and follow camera view)", 0, 0, 1, font);
				}
			}
		}

		CG_DrawFPS(10.0);
		Wolfcam_DrawSpeed(30.0);

		CG_ScanForCrosshairEntity();
		CG_DrawCrosshairNames();
		CG_DrawCrosshair();

		CG_DrawCameraPointInfo();

		CG_DrawEchoPopup();
		CG_DrawErrorPopup();
		CG_WarmupAnnouncements();
		CG_RoundAnnouncements();
		CG_RewardAnnouncements();
		CG_VoteAnnouncements();
		CG_TeamVoteAnnouncements();
		if (cg_fadeStyle.integer == 1) {
			CG_DrawFade();
		}
		return;
	}

	if ( cg.snap->ps.pm_type == PM_INTERMISSION ) {
		CG_DrawIntermission();
		if (cg_fadeStyle.integer == 1) {
			CG_DrawFade();
		}
		return;
	}

/*
	if (cg.cameraMode) {
		return;
	}
*/

// camera script
	if (cg.cameraMode) {
		CG_DrawFlashFade();
		return;
	}
	// end camera script

	//CG_Printf("pm_type: %d, pm_flags: %d\n", cg.snap->ps.pm_type, cg.snap->ps.pm_flags);
	//if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR  ||  (cgs.gametype == GT_CA  &&  cg.snap->ps.pm_type == PM_SPECTATOR) ) {
	//if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR ) {

	if (wolfcam_following) {
		if (cg_drawStatus.integer) {
			if (cg_qlhud.integer) {
				if (!cg.showDemoScores) {
					Menu_PaintAll();
				}
				//CG_DrawTimedMenus();  //FIXME ac
			} else {
				CG_DrawStatusBar();
			}
		}
		CG_DrawWeaponBar();
		Wolfcam_DrawCrosshair();
		CG_ScanForCrosshairEntity();
		CG_DrawCrosshairNames();
		CG_DrawCrosshairTeammateHealth();
		CG_DrawKeyPress();
		// draw this last so it's not obscured
		CG_DrawAmmoWarning();
	} else if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR  ||  (cgs.gametype == GT_CA  &&  cg.snap->ps.pm_type == PM_SPECTATOR  &&  cg.clientNum == cg.snap->ps.clientNum) ) {

		//FIXME ql hud?

		if (cgs.gametype == GT_CA  &&  cgs.clientinfo[cg.snap->ps.clientNum].team != TEAM_SPECTATOR) {
			//Com_Printf("ca\n");
			if (cg_drawStatus.integer) {
				if (cg_qlhud.integer) {
					if (!cg.showDemoScores) {
						Menu_PaintAll();
					}
					//CG_DrawTimedMenus();  //FIXME ac
				} else {
					CG_DrawStatusBar();
				}
			}
		}
		CG_DrawSpectator();
		CG_DrawCrosshair();
		CG_ScanForCrosshairEntity();
		CG_DrawCrosshairNames();
		CG_DrawCrosshairTeammateHealth();
		CG_DrawKeyPress();
    } else {  // !wolfcam_following
		// don't draw any status if dead or the scoreboard is being explicitly shown
		if ( (!cg.showScores && (cg.snap->ps.stats[STAT_HEALTH] > 0  ||  (cgs.gametype == GT_FREEZETAG  ||  cgs.gametype == GT_CA)))  ||  cg.freecam) {

#if 1  //def MPACK
			//if (cg_drawStatus.integer  &&  (cgs.gametype != GT_CA  ||  (cgs.gametype == GT_CA  &&  cg.snap->ps.pm_type == PM_DEAD  &&  !cg_scoreBoardWhenDead.integer)  ||  (cgs.gametype == GT_CA  &&  cg.snap->ps.pm_type == PM_INTERMISSION  &&  !cg_scoreBoardAtIntermission.integer))) {
			if (cgs.gametype == GT_CA) {
				if ((cg.snap->ps.pm_type == PM_DEAD  &&  cg_scoreBoardWhenDead.integer)  ||
					((cg.snap->ps.pm_type == PM_SPECTATOR  &&  cgs.clientinfo[cg.snap->ps.clientNum].team != TEAM_SPECTATOR  &&  cg.snap->ps.stats[STAT_HEALTH] <= 0)  &&  cg_scoreBoardWhenDead.integer)  ||
					(cg.snap->ps.pm_type == PM_INTERMISSION  &&  cg_scoreBoardAtIntermission.integer)) {
					// don't draw hud
				} else {
					if (cg_qlhud.integer  &&  cg_drawStatus.integer) {
						if (!cg.showDemoScores) {
							Menu_PaintAll();
						}
						//CG_DrawTimedMenus();  //FIXME ac
					} else {
						CG_DrawStatusBar();
					}
				}
			} else {
				if (cg_qlhud.integer  &&  cg_drawStatus.integer) {
					//Com_Printf("xx  %d\n", cg.showScores);
					if (!cg.showDemoScores) {
						Menu_PaintAll();
					}
					//CG_DrawTimedMenus();  //FIXME ac
				} else {
					CG_DrawStatusBar();
				}
			}
#else
			CG_DrawStatusBar();
#endif

#if 1  //def MPACK
			CG_DrawProxWarning();
#endif
			CG_DrawCrosshair();
			CG_ScanForCrosshairEntity();
			CG_DrawCrosshairNames();
			CG_DrawCrosshairTeammateHealth();
			CG_DrawKeyPress();
			CG_DrawDominationPointStatus();
			//CG_DrawWeaponSelect();
			CG_DrawWeaponBar();
			CG_DrawCtfsRoundScoreboard();

#ifndef MISSIONPACK   // definately not
			CG_DrawHoldableItem();
#else
			//CG_DrawPersistantPowerup();
#endif
			CG_RewardAnnouncements();
			CG_DrawReward();
			// draw this last so it's not obscured
			CG_DrawAmmoWarning();
		}
	}

	if ( CG_IsTeamGame(cgs.gametype) ) {
#ifndef MISSIONPACK  //FIXME check
		CG_DrawTeamInfo();
#endif
	}

	CG_VoteAnnouncements();
	CG_DrawVote();
	CG_TeamVoteAnnouncements();
	CG_DrawTeamVote();

	CG_DrawLagometer();

#ifdef MISSIONPACK
	if (!cg_paused.integer) {
		CG_DrawUpperRight();
		CG_DrawUpperLeft();
	}
#else
	CG_DrawUpperRight();
	CG_DrawUpperLeft();
	if (cg_drawClientItemTimer.integer) {
		CG_DrawClientItemTimer();
	}
	if (cg_fxDebugEntities.integer > 2  ||  cg_fxDebugEntities.integer < 0) {
		CG_DrawFxDebugEntities();
	}
#endif

	if (cg_drawJumpSpeeds.integer) {
		CG_DrawJumpSpeeds();
	}
	if (cg_drawJumpSpeedsTime.integer) {
		CG_DrawJumpSpeedsTime();
	}

	if (cg_drawRaceTime.integer) {
		CG_DrawRaceTime();
	}

#ifndef MISSIONPACK
	CG_DrawLowerRight();
	CG_DrawLowerLeft();
#endif

	// keep different draw follow together so behavior is consistent (previously wolfcam follow would draw over scoreboard)
	//FIXME wolfcam draw follow should probably just be handled in CG_DrawFollow()
	if ( !CG_DrawFollow() ) {
		//CG_DrawWarmup();
	}
	Wolfcam_DrawFollowing();
	CG_DrawCpmaMvdIndicator();

	CG_WarmupAnnouncements();
	CG_DrawWarmup();

    if (!wolfcam_following) {
        // don't draw center string if scoreboard is up
        cg.scoreBoardShowing = CG_DrawScoreboard();
    } else {
		if (cg.showDemoScores)
			cg.scoreBoardShowing = CG_DrawScoreboard();
		else
			cg.scoreBoardShowing = qfalse;
    }

#if 0  // testing
	if (cg.scoreBoardShowing  &&  cg.demoPlayback) {
		Menu_HandleCapture();
		CG_DrawScoreboardMenuCursor();
	}
#endif

	CG_RoundAnnouncements();
	CG_DrawCenter();

	if ( !cg.scoreBoardShowing) {
		CG_DrawCenterString();
		CG_DrawFragMessage();
	}

#if 0
	if (wolfcam_following  ||  cg_qlhud.integer == 0) {
		//trap_R_DrawConsoleLines();
	}
#endif
	//if (cg_qlhud.integer == 0  &&  SC_Cvar_Get_Int("cl_noprint") == 0) {
	if (cg_qlhud.integer == 0) {
		trap_R_DrawConsoleLines();
	}
	CG_DrawAccStats();
	CG_DrawCameraPointInfo();
	CG_DrawEchoPopup();
	// camera script
	CG_DrawFlashFade();
	// end camera script
	if (cg_fadeStyle.integer == 1) {
		CG_DrawFade();
	}
	CG_DrawErrorPopup();
	//FIXME testing fonts
	if (SC_Cvar_Get_Int("debug_fonts")) {
		char buf[1024];

		Q_strncpyz(buf, "01234567890abcdefghijklmnop\nqrstuvwxyzABCDEFGHIJKLMN\nOPQRSTUVWXYZ\n~!@#$%^&*()_-+=\\]}[{'\";:/?.>,<\n", sizeof(buf));
		QLWideScreen = WIDESCREEN_CENTER;
	//FIXME testing
		CG_CenterPrint( buf, SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH );
		//CG_CenterPrint("qrstuvwxyzABCDEFGHIJK", SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH);
		//CG_CenterPrint("LMNOPQRSTUVWXYZ", SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH);
	}

	if (cg.scoreBoardShowing) {
		CG_DrawScoreboardMenuCursor();
	}
}


/*
=====================
CG_DrawActive

Perform all drawing needed to completely fill the screen
=====================
*/
void CG_DrawActive( stereoFrame_t stereoView ) {
	float		separation;
	vec3_t		baseOrg;
	//static vec3_t lastOrigin;

	// optionally draw the info screen instead
	if ( !cg.snap ) {
		trap_R_BeginHud();
		CG_DrawInformation(qtrue);
		return;
	}

	// optionally draw the tournement scoreboard instead
	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR &&
		( cg.snap->ps.pm_flags & PMF_SCOREBOARD ) ) {
		trap_R_BeginHud();
		CG_DrawTourneyScoreboard();
		return;
	}

	switch ( stereoView ) {
	case STEREO_CENTER:
		separation = 0;
		break;
	case STEREO_LEFT:
		separation = -cg_stereoSeparation.value / 2;
		break;
	case STEREO_RIGHT:
		separation = cg_stereoSeparation.value / 2;
		break;
	default:
		separation = 0;
		CG_Error( "CG_DrawActive: Undefined stereoView" );
	}


	// clear around the rendered view if sized down
	CG_TileClear();

	// offset vieworg appropriately if we're doing stereo separation
	VectorCopy( cg.refdef.vieworg, baseOrg );
	if ( separation != 0 ) {
		VectorMA( cg.refdef.vieworg, -separation, cg.refdef.viewaxis[1], cg.refdef.vieworg );
	}

	// draw 3D view
	//Com_Printf("angles %f %f %f render\n", cg.refdefViewAngles[0], cg.refdefViewAngles[1], cg.refdefViewAngles[2]);
	//Com_Printf("viewaxis %f %f %f render\n", cg.refdef.viewaxis[0], cg.refdef.viewaxis[1], cg.refdef.viewaxis[2]);
	if (cg.viewEnt > -1) {  //  &&  !cg.freecam) {
		//FIXME check for proper view mode, ex: not intermission
		vec3_t dir;
		vec3_t angles;

		VectorSubtract(cg_entities[cg.viewEnt].lerpOrigin, cg.refdef.vieworg, dir);
		vectoangles(dir, angles);
		VectorCopy(dir, cg.viewEntAngles);

		if (!cg.freecam) {
			VectorCopy(angles, cg.refdefViewAngles);
			AnglesToAxis (cg.refdefViewAngles, cg.refdef.viewaxis );
		}
	}

	CG_FloatEntNumbers();

	if (cg_animationsIgnoreTimescale.integer) {
		cg.refdef.time = cg.realTime;
	}
	//cg.refdef.time *= cg_animationsRate.value;
	// agkr234: Fixed stuttering in cg_animationsRate at low timescales
	cg.refdef.time = cg.ftime * cg_animationsRate.value;

	if (0) {  //(cg.demoSeeking) {
		trap_R_ClearScene();
	} else {
		trap_R_RenderScene( &cg.refdef );
#if 0
		if (Distance(lastOrigin, cg.refdef.vieworg)) {
			Com_Printf("from %f %f %f  to  %f %f %f\n", lastOrigin[0], lastOrigin[1], lastOrigin[2], cg.refdef.vieworg[0], cg.refdef.vieworg[1], cg.refdef.vieworg[2]);
		}
		VectorCopy(cg.refdef.vieworg, lastOrigin);
#endif
	}

	// restore original viewpoint if running stereo
	if ( separation != 0 ) {
		VectorCopy( baseOrg, cg.refdef.vieworg );
	}

	trap_R_BeginHud();

	if (cg.drawInfo) {
		CG_DrawInformation(qfalse);
		CG_DrawEchoPopup();
		CG_DrawErrorPopup();
		CG_WarmupAnnouncements();
		CG_RoundAnnouncements();
		CG_RewardAnnouncements();
		CG_VoteAnnouncements();
		CG_TeamVoteAnnouncements();
	} else {
		// draw status bar and other floating elements
		CG_Draw2D();
	}
}

// camera script
/*
=================
CG_Fade
=================
*/
void CG_Fade( int a, int time, int duration ) {
	cgs.scrFadeAlpha = (float)a / 255.0f;
	cgs.scrFadeStartTime = time;
	cgs.scrFadeDuration = duration;
	if (cgs.scrFadeStartTime + cgs.scrFadeDuration <= cg.time) {
		cgs.scrFadeAlphaCurrent = cgs.scrFadeAlpha;
	}
	return;
}

void CG_DrawFlashFade( void ) {
	static int lastTime;
	int elapsed, time;
	vec4_t col;
	if (cgs.scrFadeStartTime + cgs.scrFadeDuration < cg.time) {
		cgs.scrFadeAlphaCurrent = cgs.scrFadeAlpha;
	} else if (cgs.scrFadeAlphaCurrent != cgs.scrFadeAlpha) {
		elapsed = (time = trap_Milliseconds()) - lastTime;
		lastTime = time;
		if (elapsed < 500 && elapsed > 0) {
			if (cgs.scrFadeAlphaCurrent > cgs.scrFadeAlpha) {
				cgs.scrFadeAlphaCurrent -= ((float)elapsed/(float)cgs.scrFadeDuration);
				if (cgs.scrFadeAlphaCurrent < cgs.scrFadeAlpha)
					cgs.scrFadeAlphaCurrent = cgs.scrFadeAlpha;
			} else {
				cgs.scrFadeAlphaCurrent += ((float)elapsed/(float)cgs.scrFadeDuration);
				if (cgs.scrFadeAlphaCurrent > cgs.scrFadeAlpha)
					cgs.scrFadeAlphaCurrent = cgs.scrFadeAlpha;
			}
		}
	}
	// now draw the fade
	if (cgs.scrFadeAlphaCurrent > 0.0) {
		VectorClear( col );
		col[3] = cgs.scrFadeAlphaCurrent;
		CG_FillRect( 0, 0, 640, 480, col );
	}
}
// end camera script

// newer version using cvars
static void CG_DrawFade (void)
{
	vec4_t color;

	if (cg_fadeAlpha.integer <= 0) {
		return;
	}

	SC_Vec4ColorFromCvars(color, &cg_fadeColor, &cg_fadeAlpha);
	CG_FillRect(0, 0, 640, 480, color);
}
