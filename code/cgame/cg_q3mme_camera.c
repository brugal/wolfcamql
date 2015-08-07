#include "cg_local.h"

#include "cg_main.h"
#include "cg_mem.h"
#include "cg_q3mme_camera.h"
#include "cg_syscalls.h"  // trap_Print, trap_FS_Write
#include "../game/bg_xmlparser.h"

demoMain_t demo;

// mov_smoothCamPos renamed to cg_q3mmeCameraSmoothPos

static void CG_DemosAddLog (const char *fmt, ...)
{
	va_list args;
	char text[MAX_PRINT_MSG];

	va_start(args, fmt);
	Q_vsnprintf(text, sizeof(text), fmt, args);
	va_end(args);

	trap_Print(va("^6q3mme camera: ^7%s\n", text));
}

static void demoSaveLine (fileHandle_t fileHandle, const char *fmt, ...)
{
	va_list args;
	char buf[1024];
	int len;

	va_start(args, fmt);
	len = Q_vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	trap_FS_Write(buf, len, fileHandle);
}

static void cameraPointMatch( const demoCameraPoint_t *point, int mask, const demoCameraPoint_t *match[4] ) {
	const demoCameraPoint_t *p;

	p = point;
	while (p && !(p->flags & mask))
		p = p->prev;

	if (!p) {
		match[0] = 0;
		match[1] = 0;
		p = point;
		while (p && !(p->flags & mask))
			p = p->next;
		if (!p) {
			match[2] = 0;
			match[3] = 0;
			return;
		}
	} else {
		match[1] = p;
		p = p->prev;
		while (p && !(p->flags & mask))
			p = p->prev;
		match[0] = p;
	}
	p = point->next;
	while (p && !(p->flags & mask))
		p = p->next;
	match[2] = p;
	if (p)
		p = p->next;
	while (p && !(p->flags & mask))
		p = p->next;
	match[3] = p;
}

static void cameraMatchAt( int time, int mask, const demoCameraPoint_t *match[4] ) {
	const demoCameraPoint_t *p;

	p = demo.camera.points;
	match[0] = match[1] = 0;
	while( p ) {
		if (p->time > time)
			break;
		if (p->flags & mask) {
			match[0] = match[1];
			match[1] = p;
		}
		p = p->next;
	}
	while (p && !(p->flags & mask))
		p = p->next;
	match[2] = p;
	if (p)
		p = p->next;
	while (p && !(p->flags & mask))
		p = p->next;
	match[3] = p;
}

static qboolean cameraMatchOrigin( const demoCameraPoint_t *match[4], vec3_t origin[4] ) {
	if ( !match[1] )
		return qfalse;

	VectorCopy( match[1]->origin, origin[1] );

	if (match[0])
		VectorCopy( match[0]->origin, origin[0] );
	else if (match[2])
		VectorSubDelta( match[1]->origin, match[2]->origin, origin[0] );
	else 
		VectorCopy( match[1]->origin, origin[0] );

	if (match[2]) {
		VectorCopy( match[2]->origin, origin[2] );
		if (match[3])
			VectorCopy( match[3]->origin, origin[3] );
		else 
			VectorAddDelta( match[1]->origin, match[2]->origin, origin[3] );
	} else if ( match[0] ) {
		VectorAddDelta( match[0]->origin, match[1]->origin, origin[2] );
		VectorAddDelta( match[0]->origin, match[1]->origin, origin[3] );
	} else {
		VectorCopy( match[1]->origin, origin[2] );
		VectorCopy( match[1]->origin, origin[3] );
	}
	return qtrue;
}

static float cameraPointLength( demoCameraPoint_t *point ) {
	int i;
	demoCameraPoint_t *match[4];
	vec3_t control[4];
	vec3_t lastOrigin, nextOrigin; 
	float len = 0, step = 0, addStep, distance, deltaDist = 0.01f;

	if (point->len >= 0)
		return point->len;

	cameraPointMatch( point, CAM_ORIGIN, (const demoCameraPoint_t **)match );

	if (!cameraMatchOrigin( (const demoCameraPoint_t **)match, control )) {
		point->len = 0;
		return 0;
	}
	if ( demo.camera.smoothPos == posLinear ) {
		point->len = VectorDistance( control[1], control[2] );
		return point->len;
	}
	posGet( 0, demo.camera.smoothPos, (const vec3_t *)control, lastOrigin );
	if (cg_q3mmeCameraSmoothPos.value >= 1)
		deltaDist = 0.01f / cg_q3mmeCameraSmoothPos.value;
	while (step < 1) {
		addStep = 1 - step;
		if (addStep > 0.01f)
			addStep = 0.01f;
		for (i = 0; i < 10; i++) {
			posGet( step+addStep, demo.camera.smoothPos, (const vec3_t *)control, nextOrigin );
			distance = VectorDistanceSquared( lastOrigin, nextOrigin);
			if ( distance <= deltaDist)
				break;
			addStep *= 0.7f;
		}
		step += addStep;
		len += sqrt( distance );
		VectorCopy( nextOrigin, lastOrigin );
		if (!distance)
			break;
	}
	point->len = len;
	return point->len;
}

qboolean CG_Q3mmeCameraOriginAt (int time, float timeFraction, vec3_t origin)
{
	demoCameraPoint_t	*match[4];
	float				searchLen;
	vec3_t				dx, dy;
	vec3_t				control[4];
	vec3_t				nextOrigin; 
	float				len = 0, step = 0, addStep, distance, deltaDist = 0.01f;
	int					i;

	cameraMatchAt( time, CAM_ORIGIN, (const demoCameraPoint_t **)match );

	if (!match[1]) {
		if (!match[2])
			return qfalse;
		if (!match[3])
			return qfalse;
		cameraPointMatch( match[2], CAM_ORIGIN, (const demoCameraPoint_t **)match );
		searchLen = 0;
	} else if (!match[2]) {
		/* Only match[1] is valid, will be copied to all points */
		searchLen = 0;
	} else {
		dx[1] = match[2]->time - match[1]->time;
		dy[1] = cameraPointLength( match[1] );
		if (match[0]) {
			dx[0] = match[1]->time - match[0]->time;
			dy[0] = cameraPointLength( match[0] );
		} else {
			dx[0] = dx[1];
			dy[0] = dy[1];
		}
		if (match[3]) {
			dx[2] = match[3]->time - match[2]->time;
			dy[2] = cameraPointLength( match[2] );
		} else {
			dx[2] = dx[1];
			dy[2] = dy[1];
		}
		searchLen = dsplineCalc( (time - match[1]->time) + timeFraction, dx, dy, 0 );
	}

	if (!cameraMatchOrigin( (const demoCameraPoint_t **)match, control ))
		return qfalse;

	if ( demo.camera.smoothPos == posLinear ) {
		float t = searchLen / match[1]->len ;
		posGet( t, demo.camera.smoothPos, (const vec3_t *)control, origin );
		return qtrue;
	} 
	posGet( 0, demo.camera.smoothPos, (const vec3_t *)control, origin );
	if (cg_q3mmeCameraSmoothPos.value >= 1)
		deltaDist = 0.01f / cg_q3mmeCameraSmoothPos.value;
	while (step < 1) {
		addStep = 1 - step;
		if (addStep > 0.01f)
			addStep = 0.01f;
		for (i = 0; i < 10; i++) {
			posGet( step+addStep, demo.camera.smoothPos, (const vec3_t *)control, nextOrigin );
			distance = VectorDistanceSquared( origin, nextOrigin);
			if ( distance <= deltaDist)
				break;
			addStep *= 0.7f;
		}
		distance = sqrt( distance );
		if (len + distance > searchLen)
			break;
		len += distance;
		step += addStep;
		VectorCopy( nextOrigin, origin );
	}
	return qtrue;
}

qboolean CG_Q3mmeCameraAnglesAt (int time, float timeFraction, vec3_t angles)
{
	demoCameraPoint_t	*match[4];
	float				lerp;
	vec3_t				tempAngles;
	Quat_t				q0, q1, q2, q3, qr;

	cameraMatchAt( time, CAM_ANGLES, (const demoCameraPoint_t **)match );

	if (!match[1]) {
		if (match[2]) {
			VectorCopy( match[2]->angles, angles );
			return qtrue;
		}
		return qfalse;
	}
	if (!match[2]) {
		VectorCopy( match[1]->angles, angles );
		return qtrue;
	}

	lerp = (((int)time - match[1]->time) + timeFraction) / (match[2]->time - match[1]->time);

	switch ( demo.camera.smoothAngles ) {
	case angleLinear:
		LerpAngles( match[1]->angles, match[2]->angles, angles, lerp );
		break;
	default:
	case angleQuat:
		QuatFromAngles( match[1]->angles, q1 );
		if ( match[0] ) {
			QuatFromAnglesClosest( match[0]->angles, q1, q0 );
		} else {
			VectorSubDelta( match[1]->angles, match[2]->angles, tempAngles );
			QuatFromAnglesClosest( tempAngles, q1, q0 );
		}
		QuatFromAnglesClosest( match[2]->angles, q1, q2 );
		if (match[3]) {
			QuatFromAnglesClosest( match[3]->angles, q2, q3 );
		} else {
			VectorAddDelta( match[1]->angles, match[2]->angles, tempAngles );
			QuatFromAnglesClosest( tempAngles, q2, q3 );
		}
		QuatSquad( lerp, q0, q1, q2, q3, qr );
		QuatToAngles( qr, angles );
	}
	return qtrue;
}

qboolean CG_Q3mmeCameraFovAt (int time, float timeFraction, float *fov)
{
	demoCameraPoint_t	*match[4];
	float				lerp;
	float				f[4];
	int					t[4];

	cameraMatchAt( time, CAM_FOV, (const demoCameraPoint_t **)match );

	if (!match[1]) {
		if (match[2]) {
			*fov = match[2]->fov;
			return qtrue;
		}
		return qfalse;
	}
	if (!match[2]) {
		*fov = match[1]->fov;
		return qtrue;
	}

	f[1] = match[1]->fov;
	t[1] = match[1]->time;
	f[2] = match[2]->fov;
	t[2] = match[2]->time;

	if ( match[0] ) {
		f[0] = match[0]->fov;
		t[0] = match[0]->time;
	} else {
		f[0] = f[1] - (f[2] - f[1]);
		t[0] = t[1] - (t[2] - t[1]);
	}
	if (match[3]) {
		f[3] = match[3]->fov;
		t[3] = match[3]->time;
	} else {
		f[3] = f[2] + (f[2] - f[1]);
		t[3] = t[2] + (t[2] - t[1]);
	}
	lerp = ((time - t[1]) + timeFraction) / (t[2] - t[1]);
	VectorTimeSpline( lerp, t, (float*)f, fov, 1 );
	return qtrue;
}


static void cameraPointReset( demoCameraPoint_t *point ) {
	demoCameraPoint_t *p;
	int i;
	
	if (!point )
		return;
	point->len = -1;
	for( p = point->prev, i = 2;i > 0 && p; p = p->prev ) {
		if (!(p->flags & CAM_ORIGIN))
			continue;
		p->len = -1;
		i--;
	}
	for( p = point->next, i = 2;i > 0 && p; p = p->next ) {
		if (!(p->flags & CAM_ORIGIN))
			continue;
		p->len = -1;
		i--;
	}

}

void CG_Q3mmeCameraDrawPath (demoCameraPoint_t *point, const vec4_t color)
{
	int i;
	polyVert_t verts[4];
	qboolean skipFirst = qtrue;
	vec3_t origin, lastOrigin; 
	demoCameraPoint_t *match[4];
	vec3_t control[4];
	float lerp, lerpFactor;
	int len, steps;

	cameraPointMatch( point, CAM_ORIGIN, (const demoCameraPoint_t **)match );
	if (match[1] != point)
		return;
	if (!match[2])
		return;
	cameraMatchOrigin( (const demoCameraPoint_t **)match, control );
	
	len = VectorDistance( control[1], control[2] );
	steps = len / 25;
	if (steps < 5) 
		steps = 5;
	demoDrawSetupVerts( verts, color );
	lerpFactor = 0.9999f / steps;
	for ( i = 0; i <= steps; i++) {
		lerp = i * lerpFactor;
		posGet( lerp, demo.camera.smoothPos, (const vec3_t *)control, origin );
		if (skipFirst) {
			skipFirst = qfalse;
			VectorCopy( origin, lastOrigin );
			continue;
		}
		demoDrawRawLine( origin, lastOrigin, 1, verts );
		VectorCopy( origin, lastOrigin);
	}
}

static demoCameraPoint_t *cameraPointSynch( int time ) {
	demoCameraPoint_t *point = demo.camera.points;
	if (!point)
		return 0;
	for( ;point->next && point->next->time <= time; point = point->next);
	return point;
}

static demoCameraPoint_t *cameraPointAlloc(void) {
	demoCameraPoint_t * point = (demoCameraPoint_t *)CG_MallocMem(sizeof (demoCameraPoint_t));
	if (!point)
		CG_Error("Out of memory");
	memset( point, 0, sizeof( demoCameraPoint_t ));
	return point;
}

static void cameraPointFree(demoCameraPoint_t *point) {
	if (point->prev) 
		point->prev->next = point->next;
	else 
		demo.camera.points = point->next;
	if (point->next ) 
		point->next->prev = point->prev;
	CG_FreeMem(point);
}

demoCameraPoint_t *CG_Q3mmeCameraPointAdd (int time, int flags)
{
	demoCameraPoint_t *point = cameraPointSynch( time );
	demoCameraPoint_t *newPoint;
	if (!point || point->time > time) {
		newPoint = cameraPointAlloc();
		newPoint->next = demo.camera.points;
		if (demo.camera.points)
			demo.camera.points->prev = newPoint;
		demo.camera.points = newPoint;
		newPoint->prev = 0;
		cg.numQ3mmeCameraPoints++;
	} else if (point->time == time) {
		newPoint = point;
	} else {
		newPoint = cameraPointAlloc();
		newPoint->prev = point;
		newPoint->next = point->next;
		if (point->next)
			point->next->prev = newPoint;
		point->next = newPoint;
		cg.numQ3mmeCameraPoints++;
	}
	cameraPointReset( newPoint );
	newPoint->time = time;
	newPoint->flags = flags;
	return newPoint;
}

static qboolean cameraDel( int time ) {
	demoCameraPoint_t *point;
	point = cameraPointSynch( time );
	if (!point)
		return qfalse;
	if (point->time != time)
		return qfalse;

	if (point->prev)
		cameraPointReset( point->prev );
	if (point->next)
		cameraPointReset( point->next );

	cameraPointFree( point );
	cg.numQ3mmeCameraPoints--;
	return qtrue;
}

static void cameraClear( void ) {
	demoCameraPoint_t *next, *point = demo.camera.points;
	while ( point ) {
		next = point->next;
		cameraPointFree( point );
		point = next;
	}
	cg.numQ3mmeCameraPoints = 0;
}

static int cameraPointShift( int shift ) {
	demoCameraPoint_t *startPoint, *endPoint;
	if ( demo.camera.start != demo.camera.end ) {
		if (demo.camera.start > demo.camera.end )
			return 0;
		startPoint = cameraPointSynch( demo.camera.start );
		endPoint = cameraPointSynch( demo.camera.end );
		if (!startPoint || !endPoint)
			return 0;
		if (startPoint->time != demo.camera.start || endPoint->time != demo.camera.end) {
			if (demo.camera.shiftWarn < demo.serverTime) {
				CG_DemosAddLog( "Failed to do shift, selection doesn't match points");
				demo.camera.shiftWarn = demo.serverTime + 1000;
			}
			return 0;
		}
	} else {
		startPoint = endPoint = cameraPointSynch( demo.play.time );
		if (!startPoint)
			return 0;
		if (startPoint->time != demo.play.time) {
			if (demo.camera.shiftWarn < demo.serverTime) {
				CG_DemosAddLog( "Failed to do shift, no point at current time");
				demo.camera.shiftWarn = demo.serverTime + 1000;
			}
			return 0;
		}

	}

	if (!shift)
		return 0;

	if (shift < 0) {
		if (!startPoint->prev) {
			if (startPoint->time + shift < 0)
				shift = -startPoint->time;
		} else {
			int delta = (startPoint->prev->time - startPoint->time );
			if (shift <= delta)
				shift = delta + 1;
		}
	} else {
		if (endPoint->next) {
			int delta = (endPoint->next->time - endPoint->time );
			if (shift >= delta)
				shift = delta - 1;
		}
	}
	while (startPoint) {
		startPoint->time += shift;
		if (startPoint == endPoint)
			break;
		startPoint = startPoint->next;
	}
	if ( demo.camera.start != demo.camera.end ) {
		demo.camera.start += shift;
		demo.camera.end += shift;
	} else {
		demo.play.time += shift;
	}
	return shift;
}

#if 0  // wolfcamql  unused
static void cameraUpdate( int time, float timeFraction ) {
	centity_t *targetCent;

	if (!cg_cameraQue.integer || !demo.camera.points ) {

	} else {
		CG_Q3mmeCameraOriginAt( time, timeFraction, demo.camera.origin );
		CG_Q3mmeCameraAnglesAt( time, timeFraction, demo.camera.angles );
		if (!CG_Q3mmeCameraFovAt( time, timeFraction, &demo.camera.fov )) {
			demo.camera.fov = 0;
		} else {
			cg_fov.value = demo.camera.fov;
			//FIXME trap_Cvar_Set?
			trap_Cvar_Set("cg_fov", va("%f", cg_fov.value));
		}

	}

	/* Optionally look at a target */
	targetCent = demoTargetEntity( demo.camera.target );
	if ( targetCent ) {
		vec3_t targetOrigin, targetAngles;
		chaseEntityOrigin( targetCent, targetOrigin );
		VectorSubtract( targetOrigin, demo.camera.origin, targetOrigin );
		vectoangles( targetOrigin, targetAngles );
		demo.camera.angles[YAW] = targetAngles[YAW];
		demo.camera.angles[PITCH] = targetAngles[PITCH];
	} else {
	}
}
#endif

#if 0  // wolfcamql  unused
static void cameraMoveDirect(void) {
	if (cg_cameraQue.integer)
		return;
	/* First clear some related values */
	if (!(demo.oldcmd.buttons & BUTTON_ATTACK)) {
		VectorClear( demo.camera.velocity );
	}
	VectorAdd( demo.camera.angles, demo.cmdDeltaAngles, demo.camera.angles );
	AnglesNormalize180( demo.camera.angles );
	demoMovePoint( demo.camera.origin, demo.camera.velocity, demo.camera.angles );
}

static void cameraMove(void) {
	vec3_t moveAngles;
	float *origin;
	float *angles;
	float *fov;

	demoCameraPoint_t *point;

	if (!cg_cameraQue.integer) {
		angles = demo.camera.angles;
		VectorCopy( angles, moveAngles );
		origin = demo.camera.origin;
		fov = &demo.camera.fov;
		point = 0;
	} else {
		point = cameraPointSynch( demo.play.time );
		if (!point || point->time != demo.play.time || demo.play.fraction )
			return;
		if (!CG_Q3mmeCameraAnglesAt( point->time, 0, moveAngles ))
			return;
		angles = point->angles;
		origin = point->origin;
		fov =  &point->fov;
	}
	

	if (demo.cmd.buttons & BUTTON_ATTACK) {
		/* First clear some related values */
		if (!(demo.oldcmd.buttons & BUTTON_ATTACK)) {
			VectorClear( demo.camera.velocity );
		}
		VectorAdd( angles, demo.cmdDeltaAngles, angles );
		AnglesNormalize180( angles );
		demoMovePoint( origin, demo.camera.velocity, moveAngles );
		if (point)
			cameraPointReset( point );
	} else if (demo.cmd.buttons & BUTTON_AFFIRMATIVE) {
		angles[ROLL] -= demo.cmdDeltaAngles[YAW];
		AnglesNormalize180( angles );
	} else if ( (demo.cmd.upmove < 0) || (demo.cmd.forwardmove < 0) || (demo.cmd.rightmove < 0)  ) {
		demoMoveDirection( origin, moveAngles );
		if (point)
			cameraPointReset( point );
	} else if (demo.cmd.rightmove > 0) {
		int shift;
		demo.camera.timeShift -= demo.serverDeltaTime * 1000 * demo.cmdDeltaAngles[YAW];
		shift = (int)demo.camera.timeShift;
		demo.camera.timeShift -= shift;
		cameraPointShift( shift );
	} else if ( demo.cmd.forwardmove > 0 ) {
		if ( (demo.camera.flags & CAM_FOV) && !(demo.editType == editDof)) {
			fov[0] -= demo.cmdDeltaAngles[YAW];
			if (fov[0] < -180)
				fov[0] = -180;
			else if (fov[0] > 180)
				fov[0] = 180;
		}
	} else {
		demo.camera.timeShift = 0;
		return;
	}
}
#endif


typedef struct  {
	int		time, target;
	vec3_t	origin, angles;
	float	fov;
	int		flags;
} parseCameraPoint_t;

static qboolean cameraParseTime( BG_XMLParse_t *parse,const char *line, void *data) {
	parseCameraPoint_t *point= (parseCameraPoint_t *)data;

	point->time = atoi( line );
	point->flags |= CAM_TIME;
	return qtrue;
}
static qboolean cameraParseTarget( BG_XMLParse_t *parse,const char *line, void *data) {
	//FIXME unused
	//parseCameraPoint_t *point= (parseCameraPoint_t *)data;

	demo.camera.target = atoi( line );
	return qtrue;
}
static qboolean cameraParseFov( BG_XMLParse_t *parse,const char *line, void *data) {
	parseCameraPoint_t *point= (parseCameraPoint_t *)data;

	point->fov = atof( line );
	point->flags |= CAM_FOV;
	return qtrue;
}
static qboolean cameraParseAngles( BG_XMLParse_t *parse,const char *line, void *data) {
	parseCameraPoint_t *point= (parseCameraPoint_t *)data;

	sscanf( line, "%f %f %f", &point->angles[0], &point->angles[1], &point->angles[2] );
	point->flags |= CAM_ANGLES;
	return qtrue;
}
static qboolean cameraParseOrigin( BG_XMLParse_t *parse,const char *line, void *data) {
	parseCameraPoint_t *point= (parseCameraPoint_t *)data;

	sscanf( line, "%f %f %f", &point->origin[0], &point->origin[1], &point->origin[2] );
	point->flags |= CAM_ORIGIN;
	return qtrue;
}
static qboolean cameraParsePoint( BG_XMLParse_t *parse,const struct BG_XMLParseBlock_s *fromBlock, void *data) {
	parseCameraPoint_t pointLoad;
	demoCameraPoint_t *point;
	static BG_XMLParseBlock_t cameraParseBlock[] = {
		{"origin", 0, cameraParseOrigin},
		{"angles", 0, cameraParseAngles},
		{"time", 0, cameraParseTime},
		{"fov", 0, cameraParseFov},
		{0, 0, 0}
	};
	memset( &pointLoad, 0, sizeof( pointLoad ));
	pointLoad.target = -1;
	if (!BG_XMLParse( parse, fromBlock, cameraParseBlock, &pointLoad )) {
		return qfalse;
	}
	if (! (pointLoad.flags & CAM_TIME) ) 
		return BG_XMLError(parse, "Camera point has no time");

	point = CG_Q3mmeCameraPointAdd( pointLoad.time, pointLoad.flags );
	VectorCopy( pointLoad.origin, point->origin );
	VectorCopy( pointLoad.angles, point->angles );
	point->fov = pointLoad.fov;
	return qtrue;
}
static qboolean cameraParseLocked( BG_XMLParse_t *parse,const char *line, void *data) {
	//demo.camera.locked = atoi( line );
	trap_Cvar_Set("cg_cameraQue", line);
	return qtrue;
}
static qboolean cameraParseSmoothPos(BG_XMLParse_t *parse,const char *line, void *data) {
	demo.camera.smoothPos = atoi( line );
	return qtrue;
}
static qboolean cameraParseSmoothAngles(BG_XMLParse_t *parse,const char *line, void *data) {
	demo.camera.smoothAngles = atoi( line );
	return qtrue;
}

qboolean CG_Q3mmeCameraParse (BG_XMLParse_t *parse, const struct BG_XMLParseBlock_s *fromBlock, void *data)
{
	static BG_XMLParseBlock_t cameraParseBlock[] = {
		{"point",	cameraParsePoint,	0 },
		{"locked",	0,					cameraParseLocked },
		{"target",	0,					cameraParseTarget },
		{"smoothPos",	0,				cameraParseSmoothPos },
		{"smoothAngles",	0,			cameraParseSmoothAngles },
		{0, 0, 0}
	};

	cameraClear();
	if (!BG_XMLParse( parse, fromBlock, cameraParseBlock, data ))
		return qfalse;

	return qtrue;
}

void CG_Q3mmeCameraSave (fileHandle_t fileHandle)
{
	demoCameraPoint_t *point;

	point = demo.camera.points;
	demoSaveLine( fileHandle, "<camera>\n" );
	demoSaveLine( fileHandle, "\t<smoothPos>%d</smoothPos>\n", demo.camera.smoothPos );
	demoSaveLine( fileHandle, "\t<smoothAngles>%d</smoothAngles>\n", demo.camera.smoothAngles );
	demoSaveLine( fileHandle, "\t<locked>%d</locked>\n", cg_cameraQue.integer );
	demoSaveLine( fileHandle, "\t<target>%d</target>\n", demo.camera.target );
	while (point) {
		demoSaveLine( fileHandle, "\t<point>\n");
		demoSaveLine( fileHandle, "\t\t<time>%10d</time>\n", point->time );
		if (point->flags & CAM_ORIGIN)
			demoSaveLine( fileHandle, "\t\t<origin>%9.4f %9.4f %9.4f</origin>\n", point->origin[0], point->origin[1], point->origin[2] );
		if (point->flags & CAM_ANGLES)
			demoSaveLine( fileHandle, "\t\t<angles>%9.4f %9.4f %9.4f</angles>\n", point->angles[0], point->angles[1], point->angles[2]);
		if (point->flags & CAM_FOV)
			demoSaveLine( fileHandle, "\t\t<fov>%9.4f</fov>\n", point->fov );
		demoSaveLine( fileHandle, "\t</point>\n");
		point = point->next;
	}
	demoSaveLine( fileHandle, "</camera>\n" );
}

#if 0  // wolfcamql unused
static void cameraDraw( int time, float timeFraction ) {
	demoCameraPoint_t *point;
	centity_t *targetCent;
	vec3_t origin, angles, forward;
	point = cameraPointSynch( time - 10000 );
	if (!point)
		return;
	while ( point && point->time < (time - 10000))
		point = point->next;
	while (point && point->time < (time + 10000)) {
		/* Draw the splines */
		CG_Q3mmeCameraDrawPath( point, colorRed );
		demoDrawCross( point->origin, colorRed );
		point = point->next;
	}
	/* Draw current line */
	if ( CG_Q3mmeCameraOriginAt( time, timeFraction, origin) && CG_Q3mmeCameraAnglesAt( time, timeFraction, angles )) {
		AngleVectors( angles, forward, 0, 0);
		VectorMA( origin, 80, forward, angles );
		demoDrawLine( origin, angles, colorWhite );
	}
	/* Draw a box around an optional target */
	targetCent = demoTargetEntity( demo.camera.target );
	if (targetCent) {
		vec3_t container;
		demoCentityBoxSize( targetCent, container );
		demoDrawBox( targetCent->lerpOrigin, container, colorWhite );
	}
}
#endif

static void cameraSmooth( void ) {
	demoCameraPoint_t *startPoint, *endPoint;
	demoCameraPoint_t *point;

	float doneLen, totalLen;
	int	totalTime;

	if (demo.camera.start == demo.camera.end) {
		Com_Printf( "No valid selection start/end\n");
		return;
	}
	startPoint = cameraPointSynch( demo.camera.start );
	if (!startPoint || startPoint->time != demo.camera.start ) {
		Com_Printf( "Camera start doesn't match a camera point\n");
		return;
	}
	endPoint = cameraPointSynch( demo.camera.end );
	if (!endPoint || endPoint->time != demo.camera.end ) {
		Com_Printf( "Camera end doesn't match a camera point\n");
		return;
	}
	totalTime = endPoint->time - startPoint->time;
	totalLen = 0;
	point = startPoint;
	while ( point ) {
		totalLen += cameraPointLength( point );
		if (point == endPoint)
			break;
		point = point->next;
		if (!point)
			CG_Error("Point chain corrupt");
	}
	if (!totalLen) {
		Com_Printf( "Total length is 0, can't smoothen.\n");
		return;
	}
	point = startPoint;
	doneLen = 0;
	while ( point ) {
		if (point == endPoint)
			return;
		if (!point)
			CG_Error("Point chain corrupt");
		point->time = startPoint->time + (doneLen / totalLen ) * totalTime;
		doneLen += cameraPointLength( point );
		point = point->next;
	}
}

static void cameraMoveAll( const vec3_t move ) {
	demoCameraPoint_t *point;

	point = demo.camera.points;

	while (point) {
		VectorAdd( point->origin, move, point->origin );
		cameraPointReset( point );
		point = point->next;
	}
}

static void cameraRotateAll( const vec3_t origin, const vec3_t angles ) {
	demoCameraPoint_t *point;
	vec3_t matrix[3];

	demoCreateRotationMatrix( angles, matrix );
	point = demo.camera.points;
	while (point) {
		
		VectorSubtract( point->origin, origin, point->origin );
		demoRotatePoint( point->origin, (const vec3_t *)matrix );
		VectorAdd( point->origin, origin, point->origin );
		VectorSubtract( point->angles, angles, point->angles );
		AnglesNormalize360( point->angles );
		cameraPointReset( point );
		point = point->next;
	}
}

// from q3mme:  game/bg_demos.c

static void demoCommandValue( const char *cmd, float * oldVal ) {
	if (!cmd[0])
		return;
	if (cmd[0] == 'a') {
		*oldVal += atof(cmd + 1);
	} else {
		*oldVal = atof(cmd);
	}
}


//void demoCameraCommand_f(void) {
void CG_Q3mmeDemoCameraCommand_f (void)
{
	const char *cmd = CG_Argv(1);

	VectorCopy(cg.refdef.vieworg, demo.viewOrigin);
	VectorCopy(cg.refdefViewAngles, demo.viewAngles);
	//FIXME wolfcamql
	demo.viewFov = cg_fov.value;

	//FIXME wolfcamql
	demo.play.time = cg.time;
	demo.play.fraction = (float)cg.foverf;

	demo.serverTime = cg.snap->serverTime;

	if (!Q_stricmp(cmd, "smooth")) {
		cameraSmooth( );
	} else if (!Q_stricmp(cmd, "add")) {
		demoCameraPoint_t *point;	
		point = CG_Q3mmeCameraPointAdd( demo.play.time, demo.camera.flags );
		if (point) {
			VectorCopy( demo.viewOrigin, point->origin );
			VectorCopy( demo.viewAngles, point->angles );
			point->fov = (float)demo.viewFov - cg_fov.value;
			CG_DemosAddLog( "Added a camera point");
		} else {
			CG_DemosAddLog( "Failed to add a camera point");
		}
	}  else if (!Q_stricmp(cmd, "del") || !Q_stricmp(cmd, "delete" )) { 
		if (cameraDel( demo.play.time )) {
			CG_DemosAddLog( "Deleted camera point");
		} else {
			CG_DemosAddLog( "Failed to delete camera point");
		}
	} else if (!Q_stricmp(cmd, "start")) { 
		demo.camera.start = demo.play.time;
		if (demo.camera.start > demo.camera.end)
			demo.camera.start = demo.camera.end;
		if (demo.camera.end == demo.camera.start)
			CG_DemosAddLog( "Cleared camera selection");
		else
			CG_DemosAddLog( "Camera selection start at %d.%03d", demo.camera.start / 1000, demo.camera.start % 1000 );
	} else if (!Q_stricmp(cmd, "end")) { 
		demo.camera.end = demo.play.time;
		if (demo.camera.end < demo.camera.start)
			demo.camera.end = demo.camera.start;
		if (demo.camera.end == demo.camera.start)
			CG_DemosAddLog( "Cleared camera selection");
		else
			CG_DemosAddLog( "Camera selection end at %d.%03d", demo.camera.end / 1000, demo.camera.end % 1000 );
	} else if (!Q_stricmp(cmd, "next")) { 
		demoCameraPoint_t *point = cameraPointSynch( demo.play.time );
		if (!point) {
			Com_Printf("^3q3mmecamera next:  no point found\n");
			return;
		}
		if (point->next)
			point = point->next;

		//demo.play.time = point->time;
		//demo.play.fraction = 0;
		cg.cameraQ3mmePlaying = qfalse;
		//cg.q3mmeCameraPlaying = qfalse;
		cg.cameraPlayedLastFrame = qfalse;
		VectorCopy(point->origin, cg.freecamPlayerState.origin);
		VectorCopy(point->origin, cg.fpos);
		VectorCopy(point->angles, cg.freecamPlayerState.viewangles);
		VectorCopy(point->angles, cg.fang);
		trap_Cvar_Set("cl_freezeDemo", "1");
		trap_SendConsoleCommandNow(va("seekservertime %f\n", (float)point->time));
		//FIXME bad hack
		cg.atCameraPoint = qtrue;

	} else if (!Q_stricmp(cmd, "prev")) {
		demoCameraPoint_t *point = cameraPointSynch( demo.play.time );
		if (!point) {
			Com_Printf("^3q3mmecamera prev:  no point found\n");
			return;
		}
		if (point->prev)
			point = point->prev;

		//demo.play.time = point->time;
		//demo.play.fraction = 0;

		cg.cameraQ3mmePlaying = qfalse;
		//cg.q3mmeCameraPlaying = qfalse;
		cg.cameraPlayedLastFrame = qfalse;
		VectorCopy(point->origin, cg.freecamPlayerState.origin);
		VectorCopy(point->origin, cg.fpos);
		VectorCopy(point->angles, cg.freecamPlayerState.viewangles);
		VectorCopy(point->angles, cg.fang);
		trap_Cvar_Set("cl_freezeDemo", "1");
		trap_SendConsoleCommandNow(va("seekservertime %f\n", (float)point->time));
		//FIXME bad hack
		cg.atCameraPoint = qtrue;

	} else if (!Q_stricmp(cmd, "lock")) {
		qboolean cameraLockedOld = cg_cameraQue.integer;
		qboolean cameraLockedNew = !cameraLockedOld;
		//demo.camera.locked = !demo.camera.locked;

		trap_Cvar_Set("cg_cameraQue", va("%d", cameraLockedNew));

		if (cameraLockedNew) {
			CG_DemosAddLog("Camera view locked");
		} else {
			CG_DemosAddLog("Camera view unlocked");
		}

		cg.cameraQ3mmePlaying = qfalse;
	} else if (!Q_stricmp(cmd, "target")) {
		//Com_Printf("^3FIXME target not implemented\n");
#if 0  // wolfcamql
		if ( cg_cameraQue.integer ) {
			CG_DemosAddLog("Can't target while locked" );
		}
		if ( demo.cmd.buttons & BUTTON_ATTACK ) {
			if (demo.camera.target>=0) {
				CG_DemosAddLog("Cleared target %d", demo.camera.target );
				demo.camera.target = -1;
			} else {
				vec3_t forward;
				AngleVectors( demo.viewAngles, forward, 0, 0 );
				demo.camera.target = demoHitEntities( demo.viewOrigin, forward );
				if (demo.camera.target >= 0) {
					CG_DemosAddLog("Target set to %d", demo.camera.target );
				} else {
					CG_DemosAddLog("Unable to hit any target" );
				}
			}
		} else {
			VectorCopy( demo.viewOrigin, demo.camera.origin );
			VectorCopy( demo.viewAngles, demo.camera.angles );
			CG_DemosAddLog("Camera view synched");
		}
#endif
		if (CG_Argc() < 3) {
			Com_Printf("current target: %d\n", demo.camera.target);
		} else {
			demo.camera.target = atoi(CG_Argv(2));
			Com_Printf("target %d selected\n", demo.camera.target);
		}
	} else if (!Q_stricmp(cmd, "shift")) {
		int shift = 1000 * atof(CG_Argv(2));
		demo.camera.shiftWarn = 0;
		if (cameraPointShift( shift )) {
			CG_DemosAddLog( "Shifted %d.%03d seconds", shift / 1000, shift % 1000);
		}
	} else if (!Q_stricmp(cmd, "clear")) {
		cameraClear( );
		CG_DemosAddLog( "Camera points cleared" );
	} else if (!Q_stricmp(cmd, "pos")) {
		float *oldOrigin;
		//if (demo.camera.locked) {
		if (cg_cameraQue.integer) {
			demoCameraPoint_t *point = cameraPointSynch( demo.play.time );
			//if (!point || point->time != demo.play.time || demo.play.fraction ) {
			if (!point || point->time != cg.time) {
				Com_Printf("Can't change position when not synched to a point\n");
				return;
			}
			oldOrigin = point->origin;
		} else {
			//oldOrigin = cg.fpos;  //demo.camera.origin;
			oldOrigin = cg.freecamPlayerState.origin;
		}
		//Com_Printf("old origin:  %f %f %f\n", oldOrigin[0], oldOrigin[1], oldOrigin[2]);
		demoCommandValue( CG_Argv(2), oldOrigin );
		demoCommandValue( CG_Argv(3), oldOrigin+1 );
		demoCommandValue( CG_Argv(4), oldOrigin+2 );

		//Com_Printf("new origin:  %f %f %f\n", oldOrigin[0], oldOrigin[1], oldOrigin[2]);
	} else if (!Q_stricmp(cmd, "angles")) {
		float *oldAngles;
		//if (demo.camera.locked) {
		if (cg_cameraQue.integer) {
			demoCameraPoint_t *point = cameraPointSynch( demo.play.time );
			//if (!point || point->time != demo.play.time || demo.play.fraction ) {
			if (!point || point->time != cg.ftime) {
				Com_Printf("Can't change angles when not synched to a point\n");
				return;
			}
			oldAngles = point->angles;
		} else {
			//oldAngles = demo.camera.angles;
			oldAngles = cg.freecamPlayerState.viewangles;
		}
		demoCommandValue( CG_Argv(2), oldAngles );
		demoCommandValue( CG_Argv(3), oldAngles+1 );
		demoCommandValue( CG_Argv(4), oldAngles+2 );
		AnglesNormalize180( oldAngles );

	} else if (!Q_stricmp(cmd, "fov")) {
		float *oldFov;
		//if (demo.camera.locked) {
		if (cg_cameraQue.integer) {
			demoCameraPoint_t *point = cameraPointSynch( demo.play.time );
			//if (!point || point->time != demo.play.time || demo.play.fraction ) {
			if (!point || point->time != cg.ftime) {
				Com_Printf("Can't change fov when not synched to a point\n");
				return;
			}
			oldFov = &point->fov;
		} else {
			//FIXME wolfcamql shouldn't really set cg_fov
#if 0
			//oldFov = &demo.camera.fov;
			oldFov = &cg_fov.value;
			// hack
			demoCommandValue( CG_Argv(2), oldFov );
			trap_Cvar_Set("cg_fov", va("%f", cg_fov.value));
#endif
			Com_Printf("Camera is unlocked, fov not set\n");
			return;
		}
		demoCommandValue( CG_Argv(2), oldFov );
	} else if (!Q_stricmp(cmd, "smoothPos")) {
		int index = atoi( CG_Argv(2));
		if ( index >= 0 && index < posLast) {
			demo.camera.smoothPos = index;
		} else {
			Com_Printf("Illegal value %d, 0 - %d\n", index, posLast -1 );
			return;
		}
		Com_Printf( "Position smoothed with " );
		switch ( demo.camera.smoothPos ) {
		case posLinear:
			Com_Printf( "linear" );
			break;
		case posCatmullRom:
			Com_Printf( "catmull-rom" );
			break;
		default:
		case posBezier:
			Com_Printf( "bezier" );
			break;
		}
		Com_Printf( " interpolation\n" );
	} else if (!Q_stricmp(cmd, "smoothAngles")) {
		int index = atoi( CG_Argv(2));
		if ( index >= 0 && index < angleLast) {
			demo.camera.smoothAngles = index;
		} else {
			Com_Printf("Illegal value %d, 0 - %d\n", index, angleLast -1 );
			return;
		}
		Com_Printf( "Angles smoothed with " );
		switch ( demo.camera.smoothAngles ) {
		case angleLinear:
			Com_Printf( "linear" );
			break;
		default:
		case angleQuat:
			Com_Printf( "quaternion" );
			break;
		}
		Com_Printf( " interpolation\n" );
	} else if (!Q_stricmp(cmd, "toggle")) {
		//if (demo.camera.locked) {
		if (cg_cameraQue.integer) {
			demoCameraPoint_t *point = cameraPointSynch( demo.play.time );
			if (!point || point->time != cg.ftime) {
				Com_Printf("Can't toggle flags when not synched to a point\n");
				return;
			}
			point->flags ^= atoi( CG_Argv(2) );
		} else {
			demo.camera.flags ^= atoi( CG_Argv(2) );
		}
	} else if (!Q_stricmp(cmd, "move")) {
		vec3_t v;

		if (!Q_stricmp(CG_Argv(2), "here" )) {
			CG_Q3mmeCameraOriginAt( demo.play.time, demo.play.fraction, v );
			VectorSubtract( demo.viewOrigin, v, v );
		} else {
			v[0] = atof( CG_Argv(2) );
			v[1] = atof( CG_Argv(3) );
			v[2] = atof( CG_Argv(4) );
		}
		cameraMoveAll( v );
		Com_Printf("moving all points %.2f %.2f %.2f\n", v[0], v[1], v[2] );
	} else if (!Q_stricmp(cmd, "rotate")) {
		vec3_t v;

		v[0] = atof( CG_Argv(2) );
		v[1] = atof( CG_Argv(3) );
		v[2] = atof( CG_Argv(4) );

		cameraRotateAll( demo.viewOrigin, v );
		Com_Printf("Rotate all points %.2f %.2f %.2f\n", v[0], v[1], v[2] );
	} else if (!Q_stricmp(cmd, "flags")) {
		if (cg_cameraQue.integer) {
			int i;
			demoCameraPoint_t *point = cameraPointSynch( demo.play.time );
			if (!point || point->time != cg.ftime) {
				Com_Printf("Can't change mask when not synched to a point\n");
				return;
			}
			for (i = 2;  i < CG_Argc();  i++) {
				if (!Q_stricmp(CG_Argv(i), "origin")) {
					point->flags ^= CAM_ORIGIN;
				} else if (!Q_stricmp(CG_Argv(i), "angles")) {
					point->flags ^= CAM_ANGLES;
				} else if (!Q_stricmp(CG_Argv(i), "fov")) {
					point->flags ^= CAM_FOV;
				} else if (!Q_stricmp(CG_Argv(i), "time")) {
					point->flags ^= CAM_TIME;
				} else {
					Com_Printf("unknown flag type: '%s'\n", CG_Argv(i));
				}
			}

			// print flag values
			Com_Printf("flags: ");
			if (point->flags & CAM_ORIGIN) {
				Com_Printf("origin ");
			}
			if (point->flags & CAM_ANGLES) {
				Com_Printf("angles ");
			}
			if (point->flags & CAM_FOV) {
				Com_Printf("fov ");
			}
			if (point->flags & CAM_TIME) {
				Com_Printf("time ");
			}
			Com_Printf("\n");

		} else {
			Com_Printf("Camera is unlocked, can't edit mask\n");
			return;
		}
	} else {
		Com_Printf("camera usage:\n" );
		Com_Printf("camera add/del, add/del a camera point at current viewpoint and time.\n" );
		Com_Printf("camera clear, clear all camera points. (Use with caution...)\n" );
		Com_Printf("camera lock, lock the view to camera point or free moving.\n" );
		Com_Printf("camera shift time, move time indexes of camera points by certain amount.\n" );
		Com_Printf("camera next/prev, go to time index of next or previous point.\n" );
		Com_Printf("camera start/end, current time with be set as start/end of selection.\n" );
		Com_Printf("camera pos/angles (a)0 (a)0 (a)0, Directly control position/angles, optional a before number to add to current value.\n" );
		Com_Printf("camera target, Clear/Set the target currently being aimed at.\n" );
		Com_Printf("camera targetNext/Prev, Go the next or previous player.\n" );
		Com_Printf("camera smoothPos 0-2, linear, catmullrom, bezier interpolation.\n" );
		Com_Printf("camera smoothAngles 0-1, linear, quaternion interpolation.\n" );
		Com_Printf("camera toggle value, xor the flags with this value.\n" );
		Com_Printf("camera smooth, smooth out the speed between select start/end.\n" );
		Com_Printf("camera move here/x y z, Move the camera track to view position or directly move it around.\n" );
		Com_Printf("camera rotate pitch yaw roll, Rotate entire camera track around current view position.\n" );
	}
}

