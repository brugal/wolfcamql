#ifndef cg_q3mme_demos_camera_h_included
#define cg_q3mme_demos_camera_h_included

#if 0
#include "../qcommon/q_shared.h"
#include "../game/bg_xmlparser.h"
#include "cg_local.h"
#include "cg_q3mme_demos_math.h"
#endif

#include "cg_q3mme_demos.h"

demoCameraPoint_t *CG_Q3mmeCameraPointAdd (int time, int flags);
qboolean CG_Q3mmeCameraOriginAt (int time, float timeFraction, vec3_t origin);
qboolean CG_Q3mmeCameraAnglesAt (int time, float timeFraction, vec3_t angles);
qboolean CG_Q3mmeCameraFovAt (int time, float timeFraction, float *fov);
void CG_Q3mmeCameraDrawPath (demoCameraPoint_t *point, const vec4_t color);

void CG_Q3mmeDemoCameraCommand_f (void);
void CG_Q3mmeCameraSave (fileHandle_t fileHandle);
qboolean CG_Q3mmeCameraParse (BG_XMLParse_t *parse, const struct BG_XMLParseBlock_s *fromBlock, void *data);
void CG_Q3mmeCameraResetInternalLengths (void);

#endif  // cg_q3mme_demos_camera_h_included

