#ifndef cg_q3mme_camera_h_included
#define cg_q3mme_camera_h_included

#include "../qcommon/q_shared.h"
#include "cg_local.h"
#include "cg_q3mme_math.h"

// q3mme: cg_demos.h
#define CAM_ORIGIN      0x001
#define CAM_ANGLES      0x002
#define CAM_FOV         0x004
#define CAM_TIME        0x100


//FIXME wolfcamql
#define CG_DemosAddLog CG_Printf

typedef struct demoCameraPoint_s {
	struct			demoCameraPoint_s *next, *prev;
	vec3_t			origin, angles;
	float			fov;
	int				time, flags;
	float			len;
} demoCameraPoint_t;

typedef struct demoMain_s {
	int serverTime;

    struct {
        int                     start, end;
        int                     target, flags;
        int                     shiftWarn;
        float           timeShift;
        float           fov;
        qboolean        locked;
        vec3_t          angles, origin, velocity;
        demoCameraPoint_t *points;
        posInterpolate_t        smoothPos;
        angleInterpolate_t      smoothAngles;
    } camera;

	struct {
		int time;
		float fraction;
	} play;

	vec3_t viewOrigin;
	vec3_t viewAngles;
	//vec3_t viewFov;
	float viewFov;
	
} demoMain_t;

extern demoMain_t demo;

//qboolean cameraOriginAt (int time, float timeFraction, vec3_t origin);
//qboolean cameraAnglesAt (int time, float timeFraction, vec3_t angles);
//qboolean cameraFovAt (int time, float timeFraction, float *fov);

extern demoCameraPoint_t *cameraPointAdd( int time, int flags );
extern qboolean cameraOriginAt( int time, float timeFraction, vec3_t origin );
extern qboolean cameraAnglesAt( int time, float timeFraction, vec3_t angles);

extern void CG_Q3mmeDemoCameraCommand_f (void);

#endif  // cg_q3mme_camera_h_included
