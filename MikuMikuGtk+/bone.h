#ifndef _INCLUDED_BONE_H_
#define _INCLUDED_BONE_H_

#include "types.h"
#include "memory_stream.h"
#include "model.h"
#include "animation.h"
#include "ght_hash_table.h"

typedef struct _BONE_INTERFACE
{
	int index;
	char *name;
	char *english_name;
	void *local_transform;
	void (*get_local_transform)(void*, void*);
	void (*set_local_transform)(void*, void*);
	void (*get_world_transform)(void*, void*);
	void (*set_local_rotation)(void*, float*);
	void (*get_local_rotation)(void*, float*);
	void (*get_local_translation)(void*, float*);
	void (*set_local_translation)(void*, float*);
	void (*set_inverse_kinematics_enable)(void*, int);
	void (*update_local_transform)(void*);
	void (*get_local_axes)(void*, float*);
	void (*get_fixed_axis)(void*, float*);
	void (*get_destination_origin)(void*, float*);
	void (*get_effector_bones)(void*, POINTER_ARRAY*);
	int (*is_movable)(void*);
	int (*is_rotatable)(void*);
	int (*is_interactive)(void*);
	int (*is_visible)(void*);
	int (*has_fixed_axis)(void*);
	int (*has_local_axis)(void*);
	int (*has_inverse_kinematics)(void*);
	struct _BONE_INTERFACE *parent_bone;
	struct _BONE_INTERFACE *effector_bone;
	struct _BASE_RIGID_BODY *body;
	struct _MODEL_INTERFACE *model;
} BONE_INTERFACE;

#define BONE_NAME_SIZE 20
#define BONE_CATEGORY_NAME_SIZE 50

#define PMX_BUFFER_SIZE 32

typedef enum _eBONE_TYPE
{
	BONE_TYPE_PMD,
	BONE_TYPE_PMX
} eBONE_TYPE;

typedef enum _eBONE_MOVE_TYPE
{
	BONE_TYPE_ROTATE,
	BONE_TYPE_ROTATE_AND_MOVE,
	BONE_TYPE_IK_DESTINATION,
	BONE_TYPE_UNKOWN,
	BONE_TYPE_UNDER_IK,
	BONE_TYPE_UNDER_ROTATE,
	BONE_TYPE_IK_TARGET,
	BONE_TYPE_INVISIBLE,
	BONE_TYPE_TWIST,
	BONE_TYPE_FOLLOW_ROTATE
} eBONE_MOVE_TYPE;

typedef enum _eBONE_FLAGS
{
	BONE_FLAG_HAS_PARENT = 0x01,
	BONE_FLAG_PARENT_IS_ROOT = 0x02,
	BONE_FLAG_CONSTRAINTED_X_COORDINATE_FOR_IK = 0x04,
	BONE_FLAG_SIMULATED = 0x08,
	BONE_FLAG_MOTION_INDEPENDENT = 0x10
} eBONE_FLAGS;

typedef enum _ePMX_BONE_FLAGS
{
	PMX_BONE_FLAG_HAS_DESTINATION_ORIGIN = 0x0001,
	PMX_BONE_FLAG_ROTATABLE = 0x0002,
	PMX_BONE_FLAG_MOVABLE = 0x0004,
	PMX_BONE_FLAG_VISIBLE = 0x0008,
	PMX_BONE_FLAG_INTERACTIVE = 0x0010,
	PMX_BONE_FLAG_HAS_INVERSE_KINEMATICS = 0x0020,
	PMX_BONE_FLAG_HAS_INHERENT_ROTATION = 0x0100,
	PMX_BONE_FLAG_HAS_INHERENT_TRANSLATION = 0x0200,
	PMX_BONE_FLAG_HAS_FIXED_AXIS = 0x0400,
	PMX_BONE_FLAG_HAS_LOCAL_AXIS = 0x0800,
	PMX_BONE_FLAG_TRANSFORM_AFTER_PHYSICS = 0x1000,
	PMX_BONE_FLAG_TRANSFORM_BY_EXTERNAL_PARENT = 0x2000,
	PMX_BONE_FLAG_ENABLE_INVERSE_KINEMATICS = 0x4000,
	PMX_BONE_FLAG_HAS_ANGLE_LIMIT = 0x8000
} ePMX_BONE_FLAGS;

typedef struct _DEFAULT_BONE
{
	BONE_INTERFACE interface_data;
	struct _APPLICATION *application_context;
} DEFAULT_BONE;

typedef struct _PMX_IK_CONSTRAINT
{
	struct _PMX_BONE *joint_bone;
	int joint_bone_index;
	uint8 has_angle_limit;
	float lower_limit[3];
	float upper_limit[3];
} PMX_IK_CONSTRAINT;

typedef struct _PMX_BONE
{
	BONE_INTERFACE interface_data;
	STRUCT_ARRAY *constraints;
	struct _PMX_BONE *parent_inherent_bone;
	struct _PMX_BONE *destination_origin_bone;
	QUATERNION local_rotation;
	QUATERNION local_inherent_rotation;
	QUATERNION local_morph_rotation;
	QUATERNION joint_rotation;
	void *world_transform;
	float origin[3];
	VECTOR3 offset_from_parent;
	VECTOR3 local_translation;
	VECTOR3 local_inherent_translation;
	VECTOR3 local_morph_translation;
	VECTOR3 destination_origin;
	VECTOR3 fixed_axis;
	VECTOR3 axis_x;
	VECTOR3 axis_z;
	float angle_limit;
	float coefficient;
	int parent_bone_index;
	int layer_index;
	int destination_origin_bone_index;
	int effector_bone_index;
	int num_iteration;
	int parent_inherent_bone_index;
	int global_id;
	uint16 flags;
} PMX_BONE;

typedef enum _ePMD2_BONE_TYPE
{
	PMD2_BONE_TYPE_ROTATE,
	PMD2_BONE_TYPE_ROTATE_AND_MOVE,
	PMD2_BONE_TYPE_IK_ROOT,
	PMD2_BONE_TYPE_UNKOWN,
	PMD2_BONE_TYPE_IK_JOINT,
	PMD2_BONE_TYPE_UNDER_ROTATE,
	PMD2_BONE_TYPE_IK_EFFECTOR,
	PMD2_BONE_TYPE_INVISIBLE,
	PMD2_BONE_TYPE_TWIST,
	PMD2_BONE_TYPE_FOLLOW_ROTATE,
	MAX_PMD2_BONE_TYPE
} ePMD2_BONE_TYPE;

typedef enum _ePMD2_BONE_FLAGS
{
	PMD2_BONE_FLAG_ENABLE_INVERSE_KINEMATICS = 0x01
} ePMD2_BONE_FLAGS;

typedef struct _PMD2_BONE
{
	BONE_INTERFACE interface_data;
	ePMD2_BONE_TYPE type;
	struct _APPLICATION *application;
	struct _PMD2_BONE *parent_bone;
	struct _PMD2_BONE *target_bone;
	struct _PMD2_BONE *child_bone;
	VECTOR3 fixed_axis;
	VECTOR3 origin;
	VECTOR3 offset;
	VECTOR3 local_translation;
	QUATERNION rotation;
	void *world_transform;
	int parent_bone_index;
	int target_bone_index;
	int child_bone_index;
	unsigned int flags;
} PMD2_BONE;

typedef struct _ASSET_ROOT_BONE
{
	BONE_INTERFACE interface_data;
	void *world_transform;
} ASSET_ROOT_BONE;

#define ASSET_SCALE_BONE_MAX_VALUE 0.01f

typedef struct _ASSET_SCALE_BONE
{
	BONE_INTERFACE interface_data;
	struct _APPLICATION *application;
	VECTOR3 position;
} ASSET_SCALE_BONE;

#define BONE_KEYFRAME_NAME_SIZE 15
#define BONE_KEYFRAME_TABLE_SIZE 64

typedef enum _eBONE_KEYFRAME_INTERPOLATION
{
	BONE_KEYFRAME_INTERPOLATION_X,
	BONE_KEYFRAME_INTERPOLATION_Y,
	BONE_KEYFRAME_INTERPOLATION_Z,
	BONE_KEYFRAME_INTERPOLATION_ROTATION,
	MAX_BONE_KEYFRAME_INTERPOLATION
} eBONE_KEYFRAME_INTERPOLATION;

typedef struct _BONE_INTERPOLATION_PARAMETER
{
	QUAD_WORD x;
	QUAD_WORD y;
	QUAD_WORD z;
	QUAD_WORD rotation;
} BONE_INTERPOLATION_PARAMETER;

typedef enum _eBONE_KEYFRAME_FLAGS
{
	BONE_KEYFRAME_FLAG_LINEAR0 = 0x01,
	BONE_KEYFRAME_FLAG_LINEAR1 = 0x02,
	BONE_KEYFRAME_FLAG_LINEAR2 = 0x04,
	BONE_KEYFRAME_FLAG_LINEAR3 = 0x08,
	BONE_KEYFRAME_FLAG_ENABLE_IK = 0x10
} eBONE_KEYFRAME_FLAGS;

typedef struct _BONE_KEYFRAME
{
	KEY_FRAME base_data;
	uint8 name[BONE_KEYFRAME_NAME_SIZE+1];
	float position[3];
	QUATERNION rotation;
	uint16 flags;

	float *interpolation_table[4];
	int8 raw_interpolation_table[BONE_KEYFRAME_TABLE_SIZE];
	BONE_INTERPOLATION_PARAMETER parameter;
} BONE_KEYFRAME;

typedef struct _BONE_KEYFRAME_LIST
{
	BONE_INTERFACE *bone;
	STRUCT_ARRAY *keyframes;
	float position[3];
	QUATERNION rotation;
	int last_index;
} BONE_KEYFRAME_LIST;

typedef enum _eBONE_ANIMATION_FLAGS
{
	BONE_ANIMATION_FLAG_HAS_CEANENTER_BONE_ANIMATION = 0x01,
	BONE_ANIMATION_FLAG_ENABLE_NULL_FRAME = 0x02
} eBONE_ANIMATION_FLAGS;

typedef struct _BONE_ANIMATION
{
	ANIMATION base_data;
	ght_hash_table_t *name2keyframes;
	struct _MODEL_INTERFACE *model;
	unsigned int flags;
} BONE_ANIMATION;

#ifdef __cplusplus
extern "C" {
#endif

extern void UpdateBoneLocalTransform(void* bone, int index, void* dummy);

extern int LoadPmxBones(STRUCT_ARRAY* bones);

extern void ReleasePmxBone(PMX_BONE* bone);

extern void SortPmxBones(STRUCT_ARRAY* bones, POINTER_ARRAY* aps_bones, POINTER_ARRAY* bps_bones);

extern void PmxBoneSetInverseKinematicsEnable(PMX_BONE* bone, int is_enable);

extern void PmxBoneResetIkLink(PMX_BONE* bone);

extern void PmxBonePerformTransform(PMX_BONE* bone);

extern void PmxBoneSolveInverseKinematics(PMX_BONE* bone);

extern int LoadPmd2Bones(STRUCT_ARRAY* bones);

extern void Pmd2BoneGetLocalTransform(PMD2_BONE* bone, void* world2local_transform);

extern void Pmd2BoneSetLocalTransform(PMD2_BONE* bone, void* local_transform);

extern void Pmd2BonePerformTransform(PMD2_BONE* bone);

extern void Pmd2BoneReadEnglishName(PMD2_BONE* bone, MEMORY_STREAM_PTR stream, int index);

extern int Pmd2BoneIsAxisXAligned(PMD2_BONE* bone);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_BONE_H_
