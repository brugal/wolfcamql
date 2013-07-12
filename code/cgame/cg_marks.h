#ifndef cg_marks_h_included
#define cg_marks_h_included

void CG_InitMarkPolys( void );

void CG_ImpactMark( qhandle_t markShader,
				    const vec3_t origin, const vec3_t dir,
					float orientation,
				    float r, float g, float b, float a,
					qboolean alphaFade,
					   float radius, qboolean temporary, qboolean energy, qboolean debug );

void CG_AddMarks( void );

void	CG_ClearParticles (void);
void	CG_AddParticles (void);

void CG_ParticleExplosion (const char *animStr, const vec3_t origin, const vec3_t vel, int duration, int sizeStart, int sizeEnd);

int CG_NewParticleArea ( int num );

#endif  // cg_marks_h_included
