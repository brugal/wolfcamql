#ifndef cg_predict_h_included
#define cg_predict_h_included

extern int cg_numSolidEntities;
extern centity_t *cg_solidEntities[MAX_ENTITIES_IN_SNAPSHOT];

void CG_BuildSolidList( void );
void CG_Trace( trace_t *result, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int skipNumber, int mask );
int	CG_PointContents( const vec3_t point, int passEntityNum );
void CG_PredictPlayerState( void );

#endif  // cg_predict_h_included
