#ifndef camera_h_included
#define camera_h_included

#include "../qcommon/q_shared.h"
#include "cg_q3mme_math.h"  // posInterpolate_t

#define WOLFCAM_CAMERA_VERSION 10
#define MAX_CAMERAPOINTS 512
#define MAX_SPLINEPOINTS (MAX_CAMERAPOINTS * 40) //FIXME define default 40 spline points per camera point
#define DEFAULT_NUM_SPLINES 40

// q3mme: cg_demos.h
#define CAM_ORIGIN      0x001
#define CAM_ANGLES      0x002
#define CAM_FOV         0x004
#define CAM_TIME        0x100

// camera edit fields
enum {
	CEF_NUMBER = 0,
	CEF_CURRENT_TIME,
	CEF_TIME,
	CEF_ORIGIN,
	CEF_ANGLES,
	CEF_CAMERA_TYPE,
	CEF_VIEW_TYPE,
	CEF_ROLL_TYPE,
	CEF_FLAGS,
	CEF_NUMBER_OF_SPLINES,
	CEF_VIEWPOINT_ORIGIN,
	CEF_VIEW_ENT,
	CEF_VIEW_ENT_OFFSETS,
	CEF_OFFSET_TYPE,
	CEF_FOV,
	CEF_FOV_TYPE,
	CEF_INITIAL_VELOCITY,
	CEF_FINAL_VELOCITY,
	CEF_PREV_VELOCITY,
	CEF_ANGLES_INITIAL_VELOCITY,
	CEF_ANGLES_FINAL_VELOCITY,
	CEF_ANGLES_PREV_VELOCITY,
	CEF_XOFFSET_INITIAL_VELOCITY,
	CEF_XOFFSET_FINAL_VELOCITY,
	CEF_XOFFSET_PREV_VELOCITY,
	CEF_YOFFSET_INITIAL_VELOCITY,
	CEF_YOFFSET_FINAL_VELOCITY,
	CEF_YOFFSET_PREV_VELOCITY,
	CEF_ZOFFSET_INITIAL_VELOCITY,
	CEF_ZOFFSET_FINAL_VELOCITY,
	CEF_ZOFFSET_PREV_VELOCITY,
	CEF_FOV_INITIAL_VELOCITY,
	CEF_FOV_FINAL_VELOCITY,
	CEF_FOV_PREV_VELOCITY,
	CEF_ROLL_INITIAL_VELOCITY,
	CEF_ROLL_FINAL_VELOCITY,
	CEF_ROLL_PREV_VELOCITY,
	CEF_COMMAND,
	CEF_DISTANCE,
};

enum {
	CAMERA_SPLINE,
	CAMERA_INTERP,
	CAMERA_JUMP,
	CAMERA_CURVE,
	CAMERA_SPLINE_BEZIER,
	CAMERA_SPLINE_CATMULLROM,
	CAMERA_ENUM_END,
};

enum {
	CAMERA_ANGLES_INTERP,
	CAMERA_ANGLES_INTERP_USE_PREVIOUS,
	CAMERA_ANGLES_FIXED,
	CAMERA_ANGLES_FIXED_USE_PREVIOUS,
	CAMERA_ANGLES_ENT,
	CAMERA_ANGLES_VIEWPOINT_INTERP,
	CAMERA_ANGLES_VIEWPOINT_PASS,
	CAMERA_ANGLES_VIEWPOINT_FIXED,
	CAMERA_ANGLES_SPLINE,
	CAMERA_ANGLES_ENUM_END,
};

enum {
	CAMERA_ROLL_INTERP,
	CAMERA_ROLL_FIXED,
	CAMERA_ROLL_PASS,
	CAMERA_ROLL_AS_ANGLES,
	CAMERA_ROLL_ENUM_END,
};

enum {
	CAMERA_FOV_USE_CURRENT,
	CAMERA_FOV_INTERP,
	CAMERA_FOV_FIXED,
	CAMERA_FOV_PASS,
	CAMERA_FOV_SPLINE,
	CAMERA_FOV_ENUM_END,
};

enum {
	CAMERA_OFFSET_INTERP,
	CAMERA_OFFSET_FIXED,
	CAMERA_OFFSET_PASS,
	CAMERA_OFFSET_ENUM_END,
};

enum {
	SPLINE_FIXED,
	SPLINE_DISTANCE,
	SPLINE_ENUM_END,
};

typedef struct cameraPoint_s {
	int version;

	vec3_t origin;
	vec3_t angles;
	int type;  // CAMERA_SPLINE ...
	int viewType;  // CAMERA_ANGLES_INTERP ...
	int rollType;
	double cgtime;
	int splineType;
	int numSplines;

	vec3_t viewPointOrigin;
	qboolean viewPointOriginSet;

	int viewEnt;
	vec3_t viewEntStartingOrigin;  // for transition towards from previous cam point
	qboolean viewEntStartingOriginSet;
	int offsetType;
	double xoffset;
	double yoffset;
	double zoffset;

	double fov;
	int fovType;

	// never implemented but saved in camera version files < 10
	// double timescale;
	// qboolean timescaleInterp;

	qboolean useOriginVelocity;
	double originInitialVelocity;
	double originFinalVelocity;
	double originImmediateInitialVelocity;
	double originImmediateFinalVelocity;

	qboolean useAnglesVelocity;
	double anglesInitialVelocity;
	double anglesFinalVelocity;
	double anglesImmediateInitialVelocity;
	double anglesImmediateFinalVelocity;

	qboolean useXoffsetVelocity;
	double xoffsetInitialVelocity;
	double xoffsetFinalVelocity;

	qboolean useYoffsetVelocity;
	double yoffsetInitialVelocity;
	double yoffsetFinalVelocity;

	qboolean useZoffsetVelocity;
	double zoffsetInitialVelocity;
	double zoffsetFinalVelocity;

	qboolean useFovVelocity;
	double fovInitialVelocity;
	double fovFinalVelocity;

	qboolean useRollVelocity;
	double rollInitialVelocity;
	double rollFinalVelocity;

	char command[MAX_STRING_CHARS];

	// dynamic vars, not saved to cam file
	int splineStart;
	vec3_t lastAngles;

	double originDistance;  // this is the total distance until the next camera point that has flag CAM_ORIGIN, this isn't valid for the points that don't have the flag
	double originAvgVelocity;
	double originAccel;
	double originThrust;

	double anglesDistance;
	double anglesAvgVelocity;
	double anglesAccel;
	double anglesThrust;

	double xoffsetDistance;
	double xoffsetAvgVelocity;
	double xoffsetAccel;
	double xoffsetThrust;

	double yoffsetDistance;
	double yoffsetAvgVelocity;
	double yoffsetAccel;
	double yoffsetThrust;

	double zoffsetDistance;
	double zoffsetAvgVelocity;
	double zoffsetAccel;
	double zoffsetThrust;

	double fovDistance;
	double fovAvgVelocity;
	double fovAccel;
	double fovThrust;

	double rollDistance;
	double rollAvgVelocity;
	double rollAccel;
	double rollThrust;

	int viewPointPassStart;
	int viewPointPassEnd;

	int rollPassStart;
	int rollPassEnd;

	int fovPassStart;
	int fovPassEnd;

	int offsetPassStart;
	int offsetPassEnd;

	// quadratic coefficient
	long double a[3];
	long double b[3];
	long double c[3];
	qboolean hasQuadratic;
	double quadraticStartTime;

	int curveCount;

	// for q3mme camera functions
	struct cameraPoint_s *next, *prev;  // not stored in cam file
	float len;  // dynamic variable, not stored in cam file
	int flags;
} cameraPoint_t;


qboolean CG_CameraSplineAnglesAt (double ftime, vec3_t angles);
qboolean CG_CameraSplineOriginAt (double ftime, posInterpolate_t posType, vec3_t origin);
qboolean CG_CameraSplineFovAt (double ftime, float *fov);

float CG_CameraAngleLongestDistanceNoRoll (const vec3_t a0, const vec3_t a1);
float CG_CameraAngleLongestDistanceWithRoll (const vec3_t a0, const vec3_t a1);
void CG_CameraResetInternalLengths (void);

#endif  // camera_h_included
