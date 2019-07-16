#ifndef cg_q3mme_demos_math_public_h_included
#define cg_q3mme_demos_math_public_h_included

typedef enum {
	posLinear,
	posCatmullRom,
	posBezier,
	posLast,
} posInterpolate_t;

typedef enum {
	angleLinear,
	angleQuat,
	angleLast,
} angleInterpolate_t;

#endif  // cg_q3mme_demos_math_public_h_included
