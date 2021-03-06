#ifndef _INCLUDED_SOFT_BODY_H_
#define _INCLUDED_SOFT_BODY_H_

#include "types.h"
#include "utils.h"

typedef enum _eSOFT_BODY_SHAPE_TYPE
{
	SOFT_BODY_SHAPE_TYPE_UNKNOWN = -1,
	SOFT_BODY_SHAPE_TYPE_TRI_MESH,
	SOFT_BODY_SHAPE_TYPE_ROPE,
	MAX_SOFT_BODY_SHAPE_TYPE
} eSOFT_BODY_SHAPE_TYPE;

typedef enum _eSOFT_BODY_AERO_MODEL_TYPE
{
	SOFT_BODY_AERO_MODEL_TYPE_UNKNOWN = -1,
	SOFT_BODY_AERO_MODEL_TYPE_V_POINT,
	SOFT_BODY_AERO_MODEL_TYPE_V_TWO_SIDED,
	SOFT_BODY_AERO_MODEL_TYPE_V_ONE_SIDED,
	SOFT_BODY_AERO_MODEL_TYPE_F_TWO_SIDED,
	SOFT_BODY_AERO_MODEL_TYPE_F_ONE_SIDED,
	MAX_SOFT_BODY_AERO_MODEL_TYPE
} eSOFT_BODY_AERO_MODEL_TYPE;

typedef struct _PMX_SOFT_BODY
{
	struct _PMX_MODEL *model;
	char *name;
	char *english_name;
	int index;
} PMX_SOFT_BODY;

#ifdef __cplusplus
extern "C" {
#endif

extern int LoadPmxSoftBodies(STRUCT_ARRAY* bodies);

extern void ReleasePmxSoftBody(PMX_SOFT_BODY* body);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_SOFT_BODY_H_
