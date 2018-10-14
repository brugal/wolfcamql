#ifndef cg_q3mme_demos_h_included
#define cg_q3mme_demos_h_included

#include "../qcommon/q_shared.h"
#include "../game/bg_xmlparser.h"
#include "cg_local.h"
#include "cg_q3mme_demos_math.h"

typedef struct demoCameraPoint_s {
	struct			demoCameraPoint_s *next, *prev;
	vec3_t			origin, angles;
	float			fov;
	int				time, flags;
	float			len;
} demoCameraPoint_t;

typedef struct demoDofPoint_s {
        struct                  demoDofPoint_s *next, *prev;
        float                   focus, radius;
        int                             time;
} demoDofPoint_t;

typedef struct demoMain_s {
	int serverTime;

    struct {
		// selects start and end points for /q3mmecamera shift
        int                     start, end;
        int                     target, flags;
        int                     shiftWarn;
        float           timeShift;
        float           fov;
		// wolfcamql this is lke cg_cameraQue
        // qboolean        locked;
        vec3_t          angles, origin, velocity;
        demoCameraPoint_t *points;
        posInterpolate_t        smoothPos;
        angleInterpolate_t      smoothAngles;
    } camera;

	struct {
        int                     start, end;
		int                     target;
		int                     shiftWarn;
		float           timeShift;
		float           focus, radius;
		qboolean        locked;
		demoDofPoint_t *points;
    } dof;

	struct {
		int time;
		float fraction;
	} play;

	vec3_t viewOrigin;
	vec3_t viewAngles;
	//vec3_t viewFov;
	float viewFov;

	int viewTarget;
	float viewFocus, viewFocusOld, viewRadius;

} demoMain_t;

extern demoMain_t demo;

void demoCommandValue( const char *cmd, float * oldVal );
void CG_DemosAddLog (const char *fmt, ...);

#endif  // cg_q3mme_demos_h_included
