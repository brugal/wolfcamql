#ifndef camera_h_included
#define camera_h_included

#define WOLFCAM_CAMERA_VERSION 8
#define MAX_CAMERAPOINTS 256
#define MAX_SPLINEPOINTS (1024 * 10)  //(1024 * 1024 * 3)
#define DEFAULT_NUM_SPLINES 40


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
	CAMERA_ANGLES_ENUM_END,
};

enum {
	CAMERA_ROLL_INTERP,
	CAMERA_ROLL_FIXED,
	CAMERA_ROLL_PASS,
	CAMERA_ROLL_ENUM_END,
};

enum {
	CAMERA_FOV_USE_CURRENT,
	CAMERA_FOV_INTERP,
	CAMERA_FOV_FIXED,
	CAMERA_FOV_PASS,
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

typedef struct {
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
	double timescale;
	qboolean timescaleInterp;

	qboolean useOriginVelocity;
	double originInitialVelocity;
	double originFinalVelocity;
	double originImmediateInitialVelocity;
	double originImmediateFinalVelocity;

	qboolean useAnglesVelocity;
	double anglesInitialVelocity;
	double anglesFinalVelocity;

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

	double originDistance;
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

} cameraPoint_t;

#endif  // camera_h_included
