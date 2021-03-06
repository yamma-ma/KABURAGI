#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include "bone.h"
#include "application.h"
#include "memory_stream.h"
#include "bullet.h"
#include "pmx_model.h"
#include "pmd_model.h"
#include "asset_model.h"
#include "memory.h"

#ifndef M_PI
# define M_PI 3.1415926535897932384626433832795
#endif

#ifdef __cplusplus
extern "C" {
#endif

void ReleaseBoneInterface(BONE_INTERFACE* bone)
{
	MEM_FREE_FUNC(bone->name);
	MEM_FREE_FUNC(bone->english_name);
	DeleteBtTransform(bone->local_transform);
}

void UpdateBoneLocalTransform(void* bone, int index, void* dummy)
{
	BONE_INTERFACE **inter = (BONE_INTERFACE**)bone;
	(*inter)->update_local_transform(*inter);
}

void DefaultBoneGetIdentityTransform(DEFAULT_BONE* bone, void* transform)
{
	BtTransformSet(transform, bone->application_context->default_data.identity_transform);
}

BASE_RIGID_BODY* GetBoneBody(BONE_INTERFACE* bone)
{
	for( ; ; )
	{
		if(bone->body != NULL)
		{
			return bone->body;
		}
		else if(bone->effector_bone == NULL)
		{
			break;
		}
		bone = bone->effector_bone;
	}

	return NULL;
}

typedef struct _READ_BONE_DATA
{
	char *name;
	VECTOR3 position;
	QUATERNION rotation;
} READ_BONE_DATA;

/***********************************************
* ReadBoneData関数                             *
* ボーンの位置を向きの情報を読み込む           *
* 引数                                         *
* stream	: メモリデータ読み込み用ストリーム *
* model		: ボーンを保持するモデル           *
* project	: プロジェクトを管理するデータ     *
***********************************************/
void ReadBoneData(
	MEMORY_STREAM* stream,
	MODEL_INTERFACE* model,
	void* project
)
{
	PROJECT *project_context = (PROJECT*)project;
	READ_BONE_DATA *bone_states;
	BONE_INTERFACE *bone;
	int num_bones;
	float compare;
	float maximum = 0;
	float float_values[4];
	int loop;
	uint32 data32;
	int i, j;

	(void)MemRead(&data32, sizeof(data32), 1, stream);
	num_bones = (int)data32;

	bone_states = (READ_BONE_DATA*)MEM_ALLOC_FUNC(sizeof(*bone_states)*num_bones);
	for(i=0; i<num_bones; i++)
	{
		(void)MemRead(&data32, sizeof(data32), 1, stream);
		bone_states[i].name = (char*)&stream->buff_ptr[stream->data_point];
		(void)MemSeek(stream, data32, SEEK_CUR);
		(void)MemRead(bone_states[i].position, sizeof(*bone_states[i].position), 3, stream);
		(void)MemRead(bone_states[i].rotation, sizeof(*bone_states[i].rotation), 4, stream);

		for(j=0; j<3; j++)
		{
			compare = fabsf(bone_states[i].position[j]);
			if(compare > maximum)
			{
				maximum = compare;
			}
		}

		for(j=0; j<4; j++)
		{
			compare = fabsf(bone_states[i].rotation[j]);
			compare = (compare * 180.0f) / (float)M_PI;
			if(compare > maximum)
			{
				maximum = compare;
			}
		}
	}

	loop = (int)maximum;
	for(i=1; i<loop; i++)
	{
		for(j=0; j<num_bones; j++)
		{
			bone = model->find_bone(model, bone_states[j].name);
			if(bone != NULL)
			{
				float_values[0] = (bone_states[j].position[0] / loop) * i;
				float_values[1] = (bone_states[j].position[1] / loop) * i;
				float_values[2] = (bone_states[j].position[2] / loop) * i;
				bone->set_local_translation(bone, float_values);

				float_values[0] = (bone_states[j].rotation[0] / loop) * i;
				float_values[1] = (bone_states[j].rotation[1] / loop) * i;
				float_values[2] = (bone_states[j].rotation[2] / loop) * i;
				float_values[3] = (bone_states[j].rotation[3] / loop) * i;
				bone->set_local_rotation(bone, float_values);
			}
		}

		WorldStepSimulation(&project_context->world, 1.0f/60.0f);
	}

	for(i=0; i<num_bones; i++)
	{
		bone = model->find_bone(model, bone_states[i].name);
		if(bone != NULL)
		{
			bone->set_local_translation(bone, bone_states[i].position);
			bone->set_local_rotation(bone, bone_states[i].rotation);
		}

		WorldStepSimulation(&project_context->world, 1.0f/60.0f);
	}

	MEM_FREE_FUNC(bone_states);
}

/*****************************************************
* WriteBoneData関数                                  *
* ボーンの位置と向きの情報を書き出す                 *
* 引数                                               *
* model			: ボーンの位置と向きを書き出すモデル *
* out_data_size	: 書き出したバイト数の格納先         *
* 返り値                                             *
*	書き出したデータ                                 *
*****************************************************/
uint8* WriteBoneData(
	MODEL_INTERFACE* model,
	size_t* out_data_size
)
{
	MEMORY_STREAM *stream = CreateMemoryStream(4096);
	BONE_INTERFACE **bones;
	uint8 *result;
	uint32 data32;
	float float_values[4];
	int num_bones;
	int i;

	bones = (BONE_INTERFACE**)model->get_bones(model, &num_bones);
	data32 = (uint32)num_bones;
	(void)MemWrite(&data32, sizeof(data32), 1, stream);
	for(i=0; i<num_bones; i++)
	{
		BONE_INTERFACE *bone = bones[i];
		data32 = (uint32)strlen(bone->name)+1;
		(void)MemWrite(&data32, sizeof(data32), 1, stream);
		(void)MemWrite(bone->name, 1, data32, stream);
		bone->get_local_translation(bone, float_values);
		(void)MemWrite(float_values, sizeof(*float_values), 3, stream);
		bone->get_local_rotation(bone, float_values);
		(void)MemWrite(float_values, sizeof(*float_values), 4, stream);
	}

	if(out_data_size != NULL)
	{
		*out_data_size = stream->data_point;
	}

	result = stream->buff_ptr;
	MEM_FREE_FUNC(bones);
	MEM_FREE_FUNC(stream);

	return result;
}

void InitializeDefaultBone(DEFAULT_BONE* bone, APPLICATION* application_context)
{
	const float basis[] = IDENTITY_MATRIX3x3;
	(void)memset(bone, 0, sizeof(*bone));

	bone->application_context = application_context;

	bone->interface_data.index = -1;
	bone->interface_data.local_transform = BtTransformNew(basis);
	bone->interface_data.get_local_transform =
		(void (*)(void*, void*))DefaultBoneGetIdentityTransform;
	bone->interface_data.set_local_transform =
		(void (*)(void*, void*))DummyFuncNoReturn2;
	bone->interface_data.get_world_transform =
		(void (*)(void*, void*))DefaultBoneGetIdentityTransform;
	bone->interface_data.set_inverse_kinematics_enable =
		(void (*)(void*, int))DummyFuncNoReturn2;
	bone->interface_data.update_local_transform =
		(void (*)(void*))DummyFuncNoReturn;
}

#define PMD_BONE_UNIT_SIZE 39

typedef struct _BONE_UNIT
{
	uint8 name[BONE_NAME_SIZE];
	int16 parent_bone_id;
	int16 child_bone_id;
	uint8 type;
	int16 target_bone_id;
	float position[3];
} BONE_UNIT;

#define CENTER_BONE_NAME {0x83, 0x5A, 0x83, 0x83, 0x83, 0x5E, 0x81, 0x5B, 0x0}
#define ROOT_BONE_NAME {0x91, 0x53, 0x82, 0xC4, 0x82, 0xCC, 0x90, 0x65, 0x0}

typedef struct _IK_UNIT
{
	int32 num_iterations;
	float angle_limit;
	int32 num_constraints;
} IK_UNIT;

#define IK_UNIT_SIZE 12

static void PmxBoneSetLocalOrientation(PMX_BONE* bone, const float* value);

static void PmxBoneGetLocalTransformMulti(PMX_BONE* bone, void* transform, void* output)
{
	void *mult_trans;
	static const float basis[9] = IDENTITY_MATRIX3x3;
	float origin[3];

	origin[0] = - bone->origin[0];
	origin[1] = - bone->origin[1];
	origin[2] = - bone->origin[2];
	mult_trans = BtTransformNewWithVector(basis, origin);

	BtTransformMulti(transform, mult_trans, output);
	DeleteBtTransform(mult_trans);
}

static void PmxBoneGetLocalTransform(PMX_BONE* bone, void* transform)
{
	PmxBoneGetLocalTransformMulti(bone, bone->world_transform, transform);
}

static void PmxBoneSetLocalTransform(PMX_BONE* bone, void* transform)
{
	BtTransformSet(bone->interface_data.local_transform, transform);
}

static void PmxBoneGetWorldTransform(PMX_BONE* bone, void* transform)
{
	BtTransformSet(transform, bone->world_transform);
}

static void PmxBoneGetLocalRotation(PMX_BONE* bone, float* rotation)
{
	COPY_VECTOR4(rotation, bone->local_rotation);
}

static void PmxBoneGetLocalTranslation(PMX_BONE* bone, float* translation)
{
	COPY_VECTOR3(translation, bone->local_translation);
}

static void PmxBoneSetLocalTranslation(PMX_BONE* bone, float* translation)
{
	if(CompareVector3(bone->local_translation, translation) == FALSE)
	{
		// ADD_QUEUE_EVENT
		COPY_VECTOR3(bone->local_translation, translation);
	}
}

void PmxBoneSetInverseKinematicsEnable(PMX_BONE* bone, int is_enable)
{
	if(!BOOL_COMPARE(bone->flags & PMX_BONE_FLAG_ENABLE_INVERSE_KINEMATICS, is_enable))
	{
		// ADD_QUEUE_EVENT
		if(is_enable == FALSE)
		{
			bone->flags &= ~(PMX_BONE_FLAG_ENABLE_INVERSE_KINEMATICS);
		}
		else
		{
			bone->flags |= PMX_BONE_FLAG_ENABLE_INVERSE_KINEMATICS;
		}
	}
}

static void PmxBoneUpdateLocalTransform(PMX_BONE* bone)
{
	PmxBoneGetLocalTransform(bone, bone->interface_data.local_transform);
}

static void PmxBoneGetLocalAxes(PMX_BONE* bone, float* axes)
{
	if((bone->flags & PMX_BONE_FLAG_HAS_LOCAL_AXIS) != 0)
	{
		float axis_y[3];
		float axis_z[3];

		Cross3DVector(axis_y, bone->axis_z, bone->axis_x);
		Cross3DVector(axis_z, bone->axis_x, axis_y);
		COPY_VECTOR3(&axes[0], bone->axis_x);
		COPY_VECTOR3(&axes[3], axis_y);
		COPY_VECTOR3(&axes[6], axis_z);
	}
	else
	{
		LOAD_IDENTITY_MATRIX3x3(axes);
	}
}

static void PmxBOneGetFixedAxis(PMX_BONE* bone, float* axis)
{
	COPY_VECTOR3(axis, bone->fixed_axis);
}

static void PmxBoneGetDestinationOrigin(PMX_BONE* bone, float* origin)
{
	PMX_BONE *parent_bone;

	if((parent_bone = bone->destination_origin_bone) != NULL)
	{
		BtTransformGetOrigin(parent_bone->world_transform, origin);
	}
	else
	{
		float basis[9];
		float destination[3];
		BtTransformGetOrigin(bone->world_transform, origin);
		BtTransformGetBasis(bone->world_transform, basis);
		COPY_VECTOR3(destination, bone->destination_origin);
		origin[0] += bone->destination_origin[0];
		origin[1] += bone->destination_origin[1];
		origin[2] += bone->destination_origin[2];
	}
}

static void PmxBoneGetEffectorBones(PMX_BONE* bone, POINTER_ARRAY* bones)
{
	PMX_IK_CONSTRAINT *constraints = (PMX_IK_CONSTRAINT*)bone->constraints->buffer;
	PMX_IK_CONSTRAINT *constraint;
	PMX_BONE *b;
	const int num_links = (int)bone->constraints->num_data;
	int i;

	for(i=0; i<num_links; i++)
	{
		constraint = &constraints[i];
		b = constraint->joint_bone;
		PointerArrayAppend(bones, b);
	}
}

static int PmxBoneIsMovable(PMX_BONE* bone)
{
	return bone->flags & PMX_BONE_FLAG_MOVABLE;
}

static int PmxBoneIsRotatable(PMX_BONE* bone)
{
	return bone->flags & PMX_BONE_FLAG_ROTATABLE;
}

static int PmxBoneIsInteractive(PMX_BONE* bone)
{
	return bone->flags & PMX_BONE_FLAG_INTERACTIVE;
}

static int PmxBoneIsVisible(PMX_BONE* bone)
{
	return bone->flags & PMX_BONE_FLAG_VISIBLE;
}

static int PmxBoneHasFixedAxis(PMX_BONE* bone)
{
	return bone->flags & PMX_BONE_FLAG_HAS_FIXED_AXIS;
}

static int PmxBoneHasLocalAxis(PMX_BONE* bone)
{
	return bone->flags & PMX_BONE_FLAG_HAS_LOCAL_AXIS;
}

static int PmxBoneHasInverseKinematics(PMX_BONE* bone)
{
	return bone->flags & PMX_BONE_FLAG_HAS_INVERSE_KINEMATICS;
}

void InitializePmxBone(PMX_BONE* bone, PMX_MODEL* model)
{
	const float identity_matrix[] = IDENTITY_MATRIX3x3;

	(void)memset(bone, 0, sizeof(*bone));

	bone->interface_data.model = (MODEL_INTERFACE*)model;
	bone->constraints = StructArrayNew(sizeof(PMX_IK_CONSTRAINT), 8);
	//bone->constraints = StructArrayNew(sizeof(PMX_IK_CONSTRAINT), PMX_BUFFER_SIZE);
	bone->local_rotation[3] = 1;
	bone->local_inherent_rotation[3] = 1;
	bone->local_morph_rotation[3] = 1;
	bone->joint_rotation[3] = 1;
	bone->world_transform = BtTransformNew(identity_matrix);
	bone->interface_data.local_transform = BtTransformNew(identity_matrix);
	bone->coefficient = 1;
	bone->parent_bone_index = -1;
	bone->destination_origin_bone_index = -1;
	bone->parent_inherent_bone_index = -1;
	bone->flags |= PMX_BONE_FLAG_ENABLE_INVERSE_KINEMATICS;

	bone->interface_data.index = -1;
	bone->interface_data.get_local_transform =
		(void (*)(void*, void*))PmxBoneGetLocalTransform;
	bone->interface_data.set_local_transform =
		(void (*)(void*, void*))PmxBoneSetLocalTransform;
	bone->interface_data.get_world_transform =
		(void (*)(void*, void*))PmxBoneGetWorldTransform;
	bone->interface_data.set_local_rotation =
		(void (*)(void*, float*))PmxBoneSetLocalOrientation;
	bone->interface_data.get_local_rotation =
		(void (*)(void*, float*))PmxBoneGetLocalRotation;
	bone->interface_data.get_local_translation =
		(void (*)(void*,float*))PmxBoneGetLocalTranslation;
	bone->interface_data.set_local_translation =
		(void (*)(void*, float*))PmxBoneSetLocalTranslation;
	bone->interface_data.set_inverse_kinematics_enable =
		(void (*)(void*, int))PmxBoneSetInverseKinematicsEnable;
	bone->interface_data.update_local_transform =
		(void (*)(void*))PmxBoneUpdateLocalTransform;
	bone->interface_data.get_local_axes =
		(void (*)(void*, float*))PmxBoneGetLocalAxes;
	bone->interface_data.get_fixed_axis =
		(void (*)(void*, float*))PmxBOneGetFixedAxis;
	bone->interface_data.get_destination_origin =
		(void (*)(void*, float*))PmxBoneGetDestinationOrigin;
	bone->interface_data.get_effector_bones =
		(void (*)(void*, POINTER_ARRAY*))PmxBoneGetEffectorBones;
	bone->interface_data.is_movable =
		(int (*)(void*))PmxBoneIsMovable;
	bone->interface_data.is_rotatable =
		(int (*)(void*))PmxBoneIsRotatable;
	bone->interface_data.is_interactive =
		(int (*)(void*))PmxBoneIsInteractive;
	bone->interface_data.is_visible =
		(int (*)(void*))PmxBoneIsVisible;
	bone->interface_data.has_fixed_axis =
		(int (*)(void*))PmxBoneHasFixedAxis;
	bone->interface_data.has_local_axis =
		(int (*)(void*))PmxBoneHasLocalAxis;
	bone->interface_data.has_inverse_kinematics =
		(int (*)(void*))PmxBoneHasInverseKinematics;
}

int PmxBonePreparse(
	uint8* data,
	size_t* data_size,
	size_t rest,
	PMX_DATA_INFO* info
)
{
	MEMORY_STREAM stream = {data, 0, rest, 1};
	int32 num_bones;
	int32 bone_index_size;
	size_t base_size;
	uint16 flags;
	int i;

	bone_index_size = (int32)info->bone_index_size;
	if(MemRead(&num_bones, sizeof(num_bones), 1, &stream) == 0)
	{
		return FALSE;
	}
	info->bones = &stream.buff_ptr[stream.data_point];
	// vector3 + bone_index_size + hierarcy + flags
	base_size = sizeof(float)*3 + bone_index_size + sizeof(int32) + sizeof(uint16);
	for(i=0; i<num_bones; i++)
	{
		uint8 *name;
		int length;

		// 日本語名
		if((length = GetTextFromStream((char*)&stream.buff_ptr[stream.data_point], &name)) < 0)
		{
			return FALSE;
		}
		stream.data_point += sizeof(int32) + length;
		if(stream.data_point > stream.data_size)
		{
			return FALSE;
		}
		// 英語名
		if((length = GetTextFromStream((char*)&stream.buff_ptr[stream.data_point], &name)) < 0)
		{
			return FALSE;
		}
		stream.data_point += sizeof(int32) + length;
		if(stream.data_point > stream.data_size)
		{
			return FALSE;
		}

		stream.data_point += base_size;
		if(stream.data_point > stream.data_size)
		{
			return FALSE;
		}

		flags = *((uint16*)&stream.buff_ptr[stream.data_point - 2]);
		// 次のボーンの有無
		if((flags & PMX_BONE_FLAG_HAS_DESTINATION_ORIGIN) != 0)
		{
			stream.data_point += bone_index_size;
			if(stream.data_point > stream.data_size)
			{
				return FALSE;
			}
		}
		else
		{
			stream.data_point += sizeof(float) * 3;
			if(stream.data_point > stream.data_size)
			{
				return FALSE;
			}
		}
		// バイアスの有無
		if((flags & (PMX_BONE_FLAG_HAS_INHERENT_ROTATION | PMX_BONE_FLAG_HAS_INHERENT_TRANSLATION)) != 0)
		{
			stream.data_point += bone_index_size + sizeof(float);
			if(stream.data_point > stream.data_size)
			{
				return FALSE;
			}
		}
		// 固定されているか
		if((flags & PMX_BONE_FLAG_HAS_FIXED_AXIS) != 0)
		{
			stream.data_point += sizeof(float) * 3;
			if(stream.data_point > stream.data_size)
			{
				return FALSE;
			}
		}
		// ローカル座標の軸を使うか
		if((flags & PMX_BONE_FLAG_HAS_LOCAL_AXIS) != 0)
		{
			stream.data_point += sizeof(float) * 3 * 2;
			if(stream.data_point > stream.data_size)
			{
				return FALSE;
			}
		}
		// 親ボーンと一緒に変形
		if((flags & PMX_BONE_FLAG_TRANSFORM_BY_EXTERNAL_PARENT) != 0)
		{
			stream.data_point += sizeof(int32);
			if(stream.data_point > stream.data_size)
			{
				return FALSE;
			}
		}
		// IK
		if((flags & PMX_BONE_FLAG_HAS_INVERSE_KINEMATICS) != 0)
		{
			IK_UNIT unit;
			// bone_index + IK loop count + IK constraint
			size_t extra_size = bone_index_size + IK_UNIT_SIZE;
			uint8 has_angle_limit;
			int j;

			stream.data_point += bone_index_size;
			(void)MemRead(&unit.num_iterations, sizeof(unit.num_iterations), 1, &stream);
			(void)MemRead(&unit.angle_limit, sizeof(unit.angle_limit), 1, &stream);
			(void)MemRead(&unit.num_constraints, sizeof(unit.num_constraints), 1, &stream);

			if(stream.data_point > stream.data_size)
			{
				return FALSE;
			}

			for(j=0; j<unit.num_constraints; j++)
			{
				stream.data_point += bone_index_size;
				if(stream.data_point > stream.data_size)
				{
					return FALSE;
				}
				(void)MemRead(&has_angle_limit, sizeof(has_angle_limit), 1, &stream);
				if(stream.data_point > stream.data_size)
				{
					return FALSE;
				}
				if(has_angle_limit != FALSE)
				{
					stream.data_point += sizeof(float) * 3 * 2;
					if(stream.data_point > stream.data_size)
					{
						return FALSE;
					}
				}
			}
		}
	}

	info->bones_count = num_bones;
	*data_size = stream.data_point;

	return TRUE;
}

int LoadPmxBones(STRUCT_ARRAY* bones)
{
	PMX_BONE *bone = (PMX_BONE*)bones->buffer;
	const int num_bones = (int)bones->num_data;
	int i;

	for(i=0; i<num_bones; i++)
	{
		const int parent_bone_index = bone[i].parent_bone_index;
		const int destination_origin_bone_index = bone[i].destination_origin_bone_index;
		const int target_bone_index = bone[i].effector_bone_index;

		if(parent_bone_index >= 0)
		{
			if(parent_bone_index < num_bones)
			{
				PMX_BONE *parent = &bone[parent_bone_index];
				bone[i].offset_from_parent[0] -= parent->origin[0];
				bone[i].offset_from_parent[1] -= parent->origin[1];
				bone[i].offset_from_parent[2] -= parent->origin[2];

				bone[i].interface_data.parent_bone = (BONE_INTERFACE*)parent;
			}
			else
			{
				return FALSE;
			}
		}

		if(destination_origin_bone_index >= 0)
		{
			if(destination_origin_bone_index < num_bones)
			{
				bone[i].destination_origin_bone = &bone[destination_origin_bone_index];
			}
			else
			{
				return FALSE;
			}
		}
		
		if(target_bone_index >= 0)
		{
			if(target_bone_index < num_bones)
			{
				bone[i].interface_data.effector_bone = (BONE_INTERFACE*)&bone[target_bone_index];
			}
			else
			{
				return FALSE;
			}
		}

		if(bone[i].parent_inherent_bone_index >= 0)
		{
			if(bone[i].parent_inherent_bone_index < num_bones)
			{
				bone[i].parent_inherent_bone = &bone[bone[i].parent_inherent_bone_index];
			}
			else
			{
				return FALSE;
			}
		}

		if((bone[i].flags & PMX_BONE_FLAG_HAS_INVERSE_KINEMATICS) != 0)
		{
			PMX_IK_CONSTRAINT *constraints = (PMX_IK_CONSTRAINT*)bone[i].constraints->buffer;
			const int num_constraints = (int)bone[i].constraints->num_data;
			PMX_IK_CONSTRAINT *constraint;
			int joint_bone_index;
			int j;

			for(j=0; j<num_constraints; j++)
			{
				constraint = &constraints[j];
				joint_bone_index = constraint->joint_bone_index;
				if(joint_bone_index >= 0)
				{
					if(joint_bone_index < num_bones)
					{
						constraint->joint_bone = &bone[joint_bone_index];
					}
					else
					{
						return FALSE;
					}
				}
			}
		}

		bone[i].interface_data.index = i;
	}

	return TRUE;
}

#define IS_TRANSFORMED_AFTER_PHYSICS_SIMULATION(PMX_BONE_PTR) ((PMX_BONE_PTR)->flags & PMX_BONE_FLAG_TRANSFORM_AFTER_PHYSICS)

int ComparePmxBone(const PMX_BONE** left, const PMX_BONE** right)
{
	if(IS_TRANSFORMED_AFTER_PHYSICS_SIMULATION(*left) != FALSE
		== IS_TRANSFORMED_AFTER_PHYSICS_SIMULATION(*right) != FALSE)
	{
		if((*left)->layer_index == (*right)->layer_index)
		{
			return (*left)->interface_data.index - (*right)->interface_data.index;
		}
		return (*left)->layer_index - (*right)->layer_index;
	}
	return (IS_TRANSFORMED_AFTER_PHYSICS_SIMULATION(*right)) ? 1 : -1;
}

void SortPmxBones(STRUCT_ARRAY* bones, POINTER_ARRAY* aps_bones, POINTER_ARRAY* bps_bones)
{
	PMX_BONE **ordered_bones = (PMX_BONE**)MEM_ALLOC_FUNC(
		sizeof(*ordered_bones) * bones->num_data);
	const int num_bones = (int)bones->num_data;
	PMX_BONE *bone;
	int i;

	for(i=0; i<num_bones; i++)
	{
		ordered_bones[i] = (PMX_BONE*)&bones->buffer[sizeof(PMX_BONE)*i];
	}
	qsort(ordered_bones, num_bones, sizeof(*ordered_bones),
		(int (*)(const void*, const void*))ComparePmxBone);

	aps_bones->num_data = 0;
	bps_bones->num_data = 0;
	for(i=0; i<num_bones; i++)
	{
		bone = ordered_bones[i];
		if(IS_TRANSFORMED_AFTER_PHYSICS_SIMULATION(bone))
		{
			PointerArrayAppend(aps_bones, bone);
		}
		else
		{
			PointerArrayAppend(bps_bones, bone);
		}
	}

	MEM_FREE_FUNC(ordered_bones);
}

static void PmxBoneGetPositionFromIkUnit(
	const float *input_lower,
	const float *input_upper,
	float *output_lower,
	float *output_upper
)
{
	output_lower[0] = - input_upper[0];
	output_lower[1] = - input_upper[1];
	output_lower[2] = input_lower[2];

	output_upper[0] = - input_lower[0];
	output_upper[1] = - input_lower[1];
	output_upper[2] = input_upper[2];
}

void ReadPmxBone(
	PMX_BONE* bone,
	uint8* data,
	PMX_DATA_INFO* info,
	size_t* data_size
)
{
	MEMORY_STREAM stream = {data, 0, (size_t)(info->end - data), 1};
	char *name_ptr;
	size_t bone_index_size = info->bone_index_size;
	int32 int32_value;
	int length;

	// 日本語名
	length = GetTextFromStream((char*)data, &name_ptr);
	stream.data_point = sizeof(int32) + length;
	bone->interface_data.name = EncodeText(info->encoding, name_ptr, length);

	// 英語名
	length = GetTextFromStream((char*)&data[stream.data_point], &name_ptr);
	stream.data_point += sizeof(int32) + length;
	bone->interface_data.english_name = EncodeText(info->encoding, name_ptr, length);

	// 位置
	(void)MemRead(&bone->origin, sizeof(bone->origin), 1, &stream);
	SET_POSITION(bone->origin, bone->origin);
	COPY_VECTOR3(bone->offset_from_parent, bone->origin);
	BtTransformSetOrigin(bone->world_transform, bone->origin);
	
	// 親ボーン
	bone->parent_bone_index = GetSignedValue(&data[stream.data_point], (int)bone_index_size);
	stream.data_point += bone_index_size;

	// レイヤー
	(void)MemRead(&int32_value, sizeof(int32), 1, &stream);
	bone->layer_index = int32_value;

	// フラグ
	(void)MemRead(&bone->flags, sizeof(bone->flags), 1, &stream);
	// 次のボーン有無
	if((bone->flags & PMX_BONE_FLAG_HAS_DESTINATION_ORIGIN) != 0)
	{
		bone->destination_origin_bone_index = GetSignedValue(&data[stream.data_point], (int)bone_index_size);
		stream.data_point += bone_index_size;
	}
	else
	{
		float offset[3];
		(void)MemRead(offset, sizeof(offset), 1, &stream);
		SET_POSITION(bone->destination_origin, offset);
	}
	// バイアスの有無
	if((bone->flags & PMX_BONE_FLAG_HAS_INHERENT_ROTATION) != 0
		|| (bone->flags & PMX_BONE_FLAG_HAS_INHERENT_TRANSLATION) != 0)
	{
		bone->parent_inherent_bone_index = GetSignedValue(&data[stream.data_point], (int)bone_index_size);
		stream.data_point += bone_index_size;
		(void)MemRead(&bone->coefficient, sizeof(bone->coefficient), 1, &stream);
	}
	// 固定軸の有無
	if((bone->flags & PMX_BONE_FLAG_HAS_FIXED_AXIS) != 0)
	{
		(void)MemRead(bone->fixed_axis, sizeof(bone->fixed_axis), 1, &stream);
	}

	// ローカルの軸
	if((bone->flags & PMX_BONE_FLAG_HAS_LOCAL_AXIS) != 0)
	{
		(void)MemRead(bone->axis_x, sizeof(bone->axis_x), 1, &stream);
		SET_POSITION(bone->axis_x, bone->axis_x);
		(void)MemRead(bone->axis_z, sizeof(bone->axis_z), 1, &stream);
		SET_POSITION(bone->axis_z, bone->axis_z);
	}
	// 親ボーンと一緒に変形
	if((bone->flags & PMX_BONE_FLAG_TRANSFORM_BY_EXTERNAL_PARENT) != 0)
	{
		(void)MemRead(&int32_value, sizeof(int32), 1, &stream);
		bone->global_id = int32_value;
	}
	// IK
	if((bone->flags & PMX_BONE_FLAG_HAS_INVERSE_KINEMATICS) != 0)
	{
		IK_UNIT unit;
		int num_links;
		int i;
		bone->effector_bone_index = GetSignedValue(&data[stream.data_point], (int)bone_index_size);
		stream.data_point += bone_index_size;
		(void)MemRead(&unit.num_iterations, sizeof(unit.num_iterations), 1, &stream);
		(void)MemRead(&unit.angle_limit, sizeof(unit.angle_limit), 1, &stream);
		(void)MemRead(&unit.num_constraints, sizeof(unit.num_constraints), 1, &stream);
		bone->num_iteration = unit.num_iterations;
		bone->angle_limit = unit.angle_limit;
		num_links = unit.num_constraints;
		for(i=0; i<num_links; i++)
		{
			PMX_IK_CONSTRAINT *constraint = (PMX_IK_CONSTRAINT*)StructArrayReserve(bone->constraints);
			constraint->joint_bone_index = GetSignedValue(&data[stream.data_point], (int)bone_index_size);
			stream.data_point += bone_index_size;
			constraint->has_angle_limit = data[stream.data_point];
			if(constraint->has_angle_limit != FALSE)
			{
				bone->flags |= PMX_BONE_FLAG_HAS_ANGLE_LIMIT;
			}
			stream.data_point++;
			if(constraint->has_angle_limit != FALSE)
			{
				float lower[3], upper[3];
				(void)MemRead(lower, sizeof(lower), 1, &stream);
				(void)MemRead(upper, sizeof(upper), 1, &stream);
				PmxBoneGetPositionFromIkUnit(lower, upper,
					constraint->lower_limit, constraint->upper_limit);
			}
		}
	}

	*data_size = stream.data_point;
}

void ReleasePmxBone(PMX_BONE* bone)
{
	ReleaseBoneInterface(&bone->interface_data);
	StructArrayDestroy(&bone->constraints, NULL);
	DeleteBtTransform(bone->world_transform);
}

void PmxBoneResetIkLink(PMX_BONE* bone)
{
	LOAD_IDENTITY_QUATERNION(bone->joint_rotation);
}

void PmxBoneMergeMorph(PMX_BONE* bone, MORPH_BONE* morph, const FLOAT_T weight)
{
	bone->local_morph_translation[0] = (float)(morph->position[0] * weight);
	bone->local_morph_translation[1] = (float)(morph->position[1] * weight);
	bone->local_morph_translation[2] = (float)(morph->position[2] * weight);

	LOAD_IDENTITY_QUATERNION(bone->local_morph_rotation);
	QuaternionSlerp(bone->local_morph_rotation, bone->local_morph_rotation, morph->rotation, (float)weight);
}

void PmxBoneUpdateWorldTransform(PMX_BONE* bone, const float* translation, const float* rotation)
{
	float origin[3];

	BtTransformSetRotation(bone->world_transform, rotation);
	origin[0] = bone->offset_from_parent[0] + translation[0];
	origin[1] = bone->offset_from_parent[1] + translation[1];
	origin[2] = bone->offset_from_parent[2] + translation[2];
	BtTransformSetOrigin(bone->world_transform, origin);
	if(bone->interface_data.parent_bone != NULL)
	{
		BtTransformMulti(((PMX_BONE*)bone->interface_data.parent_bone)->world_transform, bone->world_transform, bone->world_transform);
	}
}

void PmxBoneUpdateWorldTransformSimple(PMX_BONE* bone)
{
	PmxBoneUpdateWorldTransform(bone, bone->local_translation, bone->local_rotation);
}

void PmxBonePerformTransform(PMX_BONE* bone)
{
	float rotation[4] = IDENTITY_QUATERNION;
	float position[3] = {0};

	if((bone->flags & PMX_BONE_FLAG_HAS_INHERENT_ROTATION) != 0)
	{
		PMX_BONE *parent_bone = bone->parent_inherent_bone;
		if(parent_bone != NULL)
		{
			if((parent_bone->flags & PMX_BONE_FLAG_HAS_INHERENT_ROTATION) != 0)
			{
				MultiQuaternion(rotation, parent_bone->local_inherent_rotation);
			}
			else
			{
				float rotate_value[4];
				COPY_VECTOR4(rotate_value, parent_bone->local_rotation);
				MultiQuaternion(rotate_value, parent_bone->local_morph_rotation);
				MultiQuaternion(rotation, rotate_value);

				//MultiQuaternion(rotation, parent_bone->local_morph_rotation);
				//MultiQuaternion(rotation, parent_bone->local_rotation);
			}
		}

		if(FuzzyZero(bone->coefficient - 1.0f) == 0)
		{
			float set_value[4] = IDENTITY_QUATERNION;
			QuaternionSlerp(set_value, set_value, rotation, bone->coefficient);
			COPY_VECTOR4(rotation, set_value);
		}

		if(parent_bone != NULL && (parent_bone->flags & PMX_BONE_FLAG_HAS_INVERSE_KINEMATICS) != 0)
		{
			MultiQuaternion(rotation, parent_bone->joint_rotation);
		}

		COPY_VECTOR4(bone->local_inherent_rotation, rotation);
		MultiQuaternion(bone->local_inherent_rotation, bone->local_rotation);
		MultiQuaternion(bone->local_inherent_rotation, bone->local_morph_rotation);
		QuaternionNormalize(bone->local_inherent_rotation);
	}

	MultiQuaternion(rotation, bone->local_rotation);
	MultiQuaternion(rotation, bone->local_morph_rotation);
	MultiQuaternion(rotation, bone->joint_rotation);
	QuaternionNormalize(rotation);

	if((bone->flags & PMX_BONE_FLAG_HAS_INHERENT_TRANSLATION) != 0)
	{
		PMX_BONE *parent_bone = bone->parent_inherent_bone;
		if(parent_bone != NULL)
		{
			if((parent_bone->flags & PMX_BONE_FLAG_HAS_INHERENT_TRANSLATION) != 0)
			{
				position[0] += parent_bone->local_inherent_translation[0];
				position[1] += parent_bone->local_inherent_translation[1];
				position[2] += parent_bone->local_inherent_translation[2];
			}
			else
			{
				position[0] += parent_bone->local_translation[0] + parent_bone->local_morph_translation[0];
				position[1] += parent_bone->local_translation[1] + parent_bone->local_morph_translation[1];
				position[2] += parent_bone->local_translation[2] + parent_bone->local_morph_translation[2];
			}
		}

		if(FuzzyZero(bone->coefficient - 1) == 0)
		{
			position[0] *= bone->coefficient;
			position[1] *= bone->coefficient;
			position[2] *= bone->coefficient;
		}
		COPY_VECTOR3(bone->local_inherent_translation, position);
	}

	position[0] += bone->local_translation[0] + bone->local_morph_translation[0];
	position[1] += bone->local_translation[1] + bone->local_morph_translation[1];
	position[2] += bone->local_translation[2] + bone->local_morph_translation[2];
	PmxBoneUpdateWorldTransform(bone, position, rotation);
}

static void PmxBoneSetLocalOrientation(PMX_BONE* bone, const float* value)
{
	if(bone->local_rotation[0] != value[0]
		|| bone->local_rotation[1] != value[1]
		|| bone->local_rotation[2] != value[2]
		|| bone->local_rotation[3] != value[3]
	)
	{
		// ADD_QUEUE_EVENT
		COPY_VECTOR4(bone->local_rotation, value);
	}
}

static float ClampAngle(const float minimum, const float maximum, const float result, const float source)
{
	if(FuzzyZero(minimum) && FuzzyZero(maximum))
	{
		return 0;
	}
	else if(result < minimum)
	{
		return minimum;
	}
	else if(result > maximum)
	{
		return maximum;
	}

	return source;
}

void PmxBoneSolveInverseKinematics(PMX_BONE* bone)
{
	PMX_IK_CONSTRAINT *constraints = (PMX_IK_CONSTRAINT*)bone->constraints->buffer;
	float root_bone_position[3];
	const float angle_limit = bone->angle_limit;
	const int num_constraints = (int)bone->constraints->num_data;
	const int num_iteration = bone->num_iteration;
	const int num_half_of_iteration = num_iteration / 2;
	PMX_BONE *effector_bone = (PMX_BONE*)bone->interface_data.effector_bone;
	float original_target_rotation[4];
	float joint_rotation[4] = IDENTITY_QUATERNION;
	float new_joint_local_rotation[4];
	float matrix[9];
	float local_effector_position[3] = {0};
	float local_root_bone_position[3] = {0};
	float local_axis[3] = {0};
	int i, j, k;

	if((bone->flags & PMX_BONE_FLAG_HAS_INVERSE_KINEMATICS) == 0)
	{
		return;
	}

	BtTransformGetOrigin(bone->world_transform, root_bone_position);
	COPY_VECTOR4(original_target_rotation, effector_bone->local_rotation);

	for(i=0; i<num_iteration; i++)
	{
		int perform_constraint = i < num_half_of_iteration;
		for(j=0; j<num_constraints; j++)
		{
			PMX_IK_CONSTRAINT *constraint = &constraints[j];
			PMX_BONE *joint_bone = constraint->joint_bone;
			float current_effector_position[3];
			void *joint_bone_transform = BtTransformCopy(joint_bone->world_transform);
			void *inversed_joint_bone_transform = BtTransformCopy(joint_bone->world_transform);
			float dot;
			float new_angle_limit;
			float angle;
			float value;

			BtTransformGetOrigin(effector_bone->world_transform, current_effector_position);
			BtTransformInverse(inversed_joint_bone_transform);

			BtTransformMultiVector3(inversed_joint_bone_transform, root_bone_position, local_root_bone_position);
			Normalize3DVector(local_root_bone_position);
			BtTransformMultiVector3(inversed_joint_bone_transform, current_effector_position, local_effector_position);
			Normalize3DVector(local_effector_position);
			dot = Dot3DVector(local_root_bone_position, local_effector_position);
			if(FuzzyZero(dot) != 0)
			{
				break;
			}
			Cross3DVector(local_axis, local_effector_position, local_root_bone_position);
			SafeNormalize3DVector(local_axis);
			new_angle_limit = angle_limit * (j + 1) * 2;
			/*if(dot < -1)
			{
				angle = -1;
			}
			else if(dot > 1)
			{
				angle = 1;
			}
			else
			{
				angle = acosf(dot);
			}*/
			value = dot;
			value = ExtendedFuzzyZero(1.0f - value) ? 1.0f : ExtendedFuzzyZero(1.0f + value) ? -1.0f : value;
			angle = acosf(value);
			CLAMPED(angle, - new_angle_limit, new_angle_limit);
			QuaternionSetRotation(joint_rotation, local_axis, angle);
			if(constraint->has_angle_limit != FALSE && perform_constraint != FALSE)
			{
				float lower_limit[3];
				float upper_limit[3];

				COPY_VECTOR3(lower_limit, constraint->lower_limit);
				COPY_VECTOR3(upper_limit, constraint->upper_limit);

				if(i == 0)
				{
					if(FuzzyZero(lower_limit[1]) && FuzzyZero(upper_limit[1])
						&& FuzzyZero(lower_limit[2]) && FuzzyZero(upper_limit[2]))
					{
						local_axis[0] = 1;
						local_axis[1] = 0;
						local_axis[2] = 0;
					}
					else if(FuzzyZero(lower_limit[0]) && FuzzyZero(upper_limit[0])
						&& FuzzyZero(lower_limit[2]) && FuzzyZero(upper_limit[2]))
					{
						local_axis[0] = 0;
						local_axis[1] = 1;
						local_axis[2] = 0;
					}
					else if(FuzzyZero(lower_limit[0]) && FuzzyZero(upper_limit[0])
						&& FuzzyZero(lower_limit[1]) && FuzzyZero(upper_limit[1]))
					{
						local_axis[0] = 0;
						local_axis[1] = 0;
						local_axis[2] = 1;
					}
					QuaternionSetRotation(joint_rotation, local_axis, angle);
				}
				else
				{
					float x1, y1, z1, x2, y2, z2, x3, y3, z3;
					Matrix3x3SetRotation(matrix, joint_rotation);
					Matrix3x3GetEulerZYX(matrix, &z1, &y1, &x1);
					Matrix3x3SetRotation(matrix, joint_bone->local_rotation);
					Matrix3x3GetEulerZYX(matrix, &z2, &y2, &x2);
					x3 = x1 + x2,	y3 = y1 + y2,	z3 = z1 + z2;
					x1 = ClampAngle(lower_limit[0], upper_limit[0], x3, x1);
					y1 = ClampAngle(lower_limit[1], upper_limit[1], y3, y1);
					z1 = ClampAngle(lower_limit[2], upper_limit[2], z3, z1);
					QuaternionSetEulerZYX(joint_rotation, z1, y1, x1);
				}
				COPY_VECTOR4(new_joint_local_rotation, joint_rotation);
				MultiQuaternion(new_joint_local_rotation, joint_bone->local_rotation);
			}
			else if(i == 0)
			{
				//COPY_VECTOR4(new_joint_local_rotation, joint_bone->local_rotation);
				//MultiQuaternion(new_joint_local_rotation, joint_rotation);
				COPY_VECTOR4(new_joint_local_rotation, joint_rotation);
				MultiQuaternion(new_joint_local_rotation, joint_bone->local_rotation);
			}
			else
			{
				//COPY_VECTOR4(new_joint_local_rotation, joint_rotation);
				//MultiQuaternion(new_joint_local_rotation, joint_bone->local_rotation);
				COPY_VECTOR4(new_joint_local_rotation, joint_bone->local_rotation);
				MultiQuaternion(new_joint_local_rotation, joint_rotation);
			}
			PmxBoneSetLocalOrientation(joint_bone, new_joint_local_rotation);
			COPY_VECTOR4(joint_bone->joint_rotation, joint_rotation);

			for(k=j; k>=0; k--)
			{
				PMX_IK_CONSTRAINT *ik = &constraints[k];
				PMX_BONE *joint = ik->joint_bone;
				PmxBoneUpdateWorldTransformSimple(joint);
			}
			PmxBoneUpdateWorldTransformSimple(effector_bone);

			DeleteBtTransform(joint_bone_transform);
			DeleteBtTransform(inversed_joint_bone_transform);
		}
	}

	PmxBoneSetLocalOrientation(effector_bone, original_target_rotation);
}

static void Pmd2BoneGetWorldTransform(PMD2_BONE* bone, void* transform)
{
	BtTransformSet(transform, bone->world_transform);
}

static void Pmd2BoneGetLocalTranslation(PMD2_BONE* bone, float* translation)
{
	COPY_VECTOR3(translation, bone->local_translation);
}

static void Pmd2BoneSetLocalTranslation(PMD2_BONE* bone, float* translation)
{
	COPY_VECTOR3(bone->local_translation, translation);
}

static void Pmd2BoneGetLocalRotation(PMD2_BONE* bone, float* rotation)
{
	COPY_VECTOR4(rotation, bone->rotation);
}

static void Pmd2BoneSetLocalRotation(PMD2_BONE* bone, float* rotation)
{
	COPY_VECTOR4(bone->rotation, rotation);
}

static void Pmd2BoneGetFixedAxis(PMD2_BONE* bone, float* axis)
{
	COPY_VECTOR3(axis, bone->fixed_axis);
}

static void Pmd2BoneGetDestinationOrigin(PMD2_BONE* bone, float* origin)
{
	if(bone->parent_bone != NULL)
	{
		COPY_VECTOR3(origin, bone->parent_bone->origin);
	}
	else
	{
		origin[0] = origin[1] = origin[2] = 0;
	}
}

static int Pmd2BoneIsMovable(PMD2_BONE* bone)
{
	switch(bone->type)
	{
	case PMD2_BONE_TYPE_ROTATE_AND_MOVE:
	case PMD2_BONE_TYPE_IK_ROOT:
	case PMD2_BONE_TYPE_IK_JOINT:
		return TRUE;
	//case PMD2_BONE_TYPE_UNKOWN:
	//case PMD2_BONE_TYPE_UNDER_ROTATE:
	//case PMD2_BONE_TYPE_IK_EFFECTOR:
	//case PMD2_BONE_TYPE_INVISIBLE:
	//case PMD2_BONE_TYPE_TWIST:
	//case PMD2_BONE_TYPE_FOLLOW_ROTATE:
	//default:
	//	;
	}
	return FALSE;
}

static int Pmd2BoneIsRotateable(PMD2_BONE* bone)
{
	switch(bone->type)
	{
	case PMD2_BONE_TYPE_ROTATE:
	case PMD2_BONE_TYPE_ROTATE_AND_MOVE:
	case PMD2_BONE_TYPE_IK_ROOT:
	case PMD2_BONE_TYPE_IK_JOINT:
	case PMD2_BONE_TYPE_UNDER_ROTATE:
	case PMD2_BONE_TYPE_TWIST:
		return TRUE;
	}
	return FALSE;
}

static int Pmd2BoneHasFixedAxis(PMD2_BONE* bone)
{
	return bone->type == PMD2_BONE_TYPE_TWIST;
}

static int Pmd2BoneHasInverseKinematics(PMD2_BONE* bone)
{
	return bone->type == PMD2_BONE_TYPE_IK_ROOT;
}

static void Pmd2BoneSetInverseKinematicsEnable(PMD2_BONE* bone, int enable)
{
	int current_state = bone->flags & PMD2_BONE_FLAG_ENABLE_INVERSE_KINEMATICS;
	if(BOOL_COMPARE(current_state, enable) == 0)
	{
		if(enable != FALSE)
		{
			bone->flags &= ~(PMD2_BONE_FLAG_ENABLE_INVERSE_KINEMATICS);
		}
		else
		{
			bone->flags |= PMD2_BONE_FLAG_ENABLE_INVERSE_KINEMATICS;
		}
	}
}

void InitializePmd2Bone(PMD2_BONE* bone, MODEL_INTERFACE* model, void* application_context)
{
	const float basis[] = IDENTITY_MATRIX3x3;
	(void)memset(bone, 0, sizeof(*bone));
	bone->interface_data.model = model;
	bone->rotation[3] = 1;
	bone->interface_data.local_transform = BtTransformNew(basis);
	bone->interface_data.index = -1;
	bone->world_transform = BtTransformNew(basis);
	bone->application = (APPLICATION*)application_context;
	bone->flags |= PMD2_BONE_FLAG_ENABLE_INVERSE_KINEMATICS;

	bone->interface_data.get_local_transform = (void (*)(void*, void*))Pmd2BoneGetLocalTransform;
	bone->interface_data.set_local_transform = (void (*)(void*, void*))Pmd2BoneSetLocalTransform;
	bone->interface_data.get_world_transform = (void (*)(void*, void*))Pmd2BoneGetWorldTransform;
	bone->interface_data.get_local_translation = (void (*)(void*, float*))Pmd2BoneGetLocalTranslation;
	bone->interface_data.set_local_translation = (void (*)(void*, float*))Pmd2BoneSetLocalTranslation;
	bone->interface_data.get_local_rotation = (void (*)(void*, float*))Pmd2BoneGetLocalRotation;
	bone->interface_data.set_local_rotation = (void (*)(void*, float*))Pmd2BoneSetLocalRotation;
	bone->interface_data.get_fixed_axis = (void (*)(void*, float*))Pmd2BoneGetFixedAxis;
	bone->interface_data.get_destination_origin = (void (*)(void*, float*))Pmd2BoneGetDestinationOrigin;
	bone->interface_data.get_effector_bones = (void (*)(void*, POINTER_ARRAY*))DummyFuncNoReturn2;
	bone->interface_data.is_movable = (int (*)(void*))Pmd2BoneIsMovable;
	bone->interface_data.is_rotatable = (int (*)(void*))Pmd2BoneIsRotateable;
	bone->interface_data.is_interactive = (int (*)(void*))Pmd2BoneIsRotateable;
	bone->interface_data.is_visible = (int (*)(void*))Pmd2BoneIsRotateable;
	bone->interface_data.has_fixed_axis = (int (*)(void*))Pmd2BoneHasFixedAxis;
	bone->interface_data.has_inverse_kinematics = (int (*)(void*))Pmd2BoneHasInverseKinematics;
	bone->interface_data.set_inverse_kinematics_enable =
		(void (*)(void*, int))Pmd2BoneSetInverseKinematicsEnable;
}

int Pmd2BonePreparse(
	MEMORY_STREAM_PTR stream,
	PMD_DATA_INFO* info
)
{
	uint16 size;
	if(MemRead(&size, sizeof(size), 1, stream) == 0
		|| size * PMD_BONE_UNIT_SIZE + stream->data_point > stream->data_size)
	{
		return FALSE;
	}
	info->bones_count = size;
	info->bones = &stream->buff_ptr[stream->data_point];
	return MemSeek(stream, size * PMD_BONE_UNIT_SIZE, SEEK_CUR) == 0;
}

int LoadPmd2Bones(STRUCT_ARRAY* bones)
{
	PMD2_BONE *b = (PMD2_BONE*)bones->buffer;
	PMD2_BONE *bone;
	const int num_bones = (int)bones->num_data;
	int index;
	int i;

	for(i=0; i<num_bones; i++)
	{
		bone = &b[i];
		index = bone->parent_bone_index;
		if(index >= 0)
		{
			if(index >= num_bones)
			{
				return FALSE;
			}
			else
			{
				PMD2_BONE *parent = &b[index];
				bone->offset[0] -= parent->origin[0];
				bone->offset[1] -= parent->origin[1];
				bone->offset[2] -= parent->origin[2];
				bone->parent_bone = parent;
			}
		}
		index = bone->target_bone_index;
		if(index >= 0)
		{
			if(index >= num_bones)
			{
				return FALSE;
			}
			else
			{
				bone->target_bone = &b[index];
			}
		}
		index = bone->child_bone_index;
		if(index >= 0)
		{
			if(index >= num_bones)
			{
				return FALSE;
			}
			else
			{
				bone->child_bone = &b[index];
			}
		}
		bone->interface_data.index = i;
	}
	return TRUE;
}

void ReadPmd2Bone(
	PMD2_BONE* bone,
	MEMORY_STREAM_PTR stream,
	PMD_DATA_INFO* info,
	size_t* data_size
)
{
	BONE_UNIT unit;

	(void)MemRead(unit.name, 1, sizeof(unit.name), stream);
	(void)MemRead(&unit.parent_bone_id, sizeof(unit.parent_bone_id), 1, stream);
	(void)MemRead(&unit.child_bone_id, sizeof(unit.child_bone_id), 1, stream);
	(void)MemRead(&unit.type, sizeof(unit.type), 1, stream);
	(void)MemRead(&unit.target_bone_id, sizeof(unit.target_bone_id), 1, stream);
	(void)MemRead(&unit.position, sizeof(unit.position), 1, stream);
	bone->interface_data.name = EncodeText(&bone->application->encode,
		(char*)unit.name, sizeof(unit.name));
	bone->child_bone_index = unit.child_bone_id;
	bone->parent_bone_index = unit.parent_bone_id;
	bone->target_bone_index = unit.target_bone_id;
	bone->type = (ePMD2_BONE_TYPE)unit.type;
	SET_POSITION(bone->origin, unit.position);
	COPY_VECTOR3(bone->offset, bone->origin);
	*data_size = PMD_BONE_UNIT_SIZE;
}

void Pmd2BoneGetLocalTransformMulti(PMD2_BONE* bone, void* world_transform, void* output)
{
	uint8 transform[TRANSFORM_SIZE];
	float origin[3] = { - bone->origin[0], - bone->origin[1], - bone->origin[2]};
	BtTransformSetIdentity(transform);
	BtTransformSetOrigin(transform, origin);
	BtTransformMulti(world_transform, transform, output);
}

void Pmd2BoneGetLocalTransform(PMD2_BONE* bone, void* world2local_transform)
{
	Pmd2BoneGetLocalTransformMulti(bone, bone->world_transform, world2local_transform);
}

void Pmd2BoneSetLocalTransform(PMD2_BONE* bone, void* local_transform)
{
	BtTransformSet(bone->interface_data.local_transform, local_transform);
}

void Pmd2BonePerformTransform(PMD2_BONE* bone)
{
	float origin[3];
	if(bone->type == PMD2_BONE_TYPE_UNDER_ROTATE && bone->target_bone != NULL)
	{
		QUATERNION rotation;
		COPY_VECTOR4(rotation, bone->rotation);
		MultiQuaternion(rotation, bone->target_bone->rotation);
		BtTransformSetRotation(bone->world_transform, rotation);
	}
	else if(bone->type == PMD2_BONE_TYPE_FOLLOW_ROTATE && bone->child_bone != NULL)
	{
		float coef = bone->target_bone_index * 0.01f;
		QUATERNION rotation = IDENTITY_QUATERNION;
		QuaternionSlerp(rotation, rotation, bone->rotation, coef);
		BtTransformSetRotation(bone->world_transform, rotation);
	}	
	else
	{
		BtTransformSetRotation(bone->world_transform, bone->rotation);
	}
	origin[0] = bone->offset[0] + bone->local_translation[0];
	origin[1] = bone->offset[1] + bone->local_translation[1];
	origin[2] = bone->offset[2] + bone->local_translation[2];
	BtTransformSetOrigin(bone->world_transform, origin);
	if(bone->parent_bone != NULL)
	{
		BtTransformMulti(bone->parent_bone->world_transform, bone->world_transform, bone->world_transform);
	}
	Pmd2BoneGetLocalTransform(bone, bone->interface_data.local_transform);
}

void Pmd2BoneReadEnglishName(PMD2_BONE* bone, MEMORY_STREAM_PTR stream, int index)
{
	if(index >= 0)
	{
		char name[BONE_NAME_SIZE+1];
		(void)MemSeek(stream, BONE_NAME_SIZE*index, SEEK_SET);
		(void)MemRead(name, BONE_NAME_SIZE, 1, stream);
		bone->interface_data.english_name = EncodeText(
			&bone->application->encode, name, BONE_NAME_SIZE);
	}
}

int Pmd2BoneIsAxisXAligned(PMD2_BONE* bone)
{
	if(bone->interface_data.name != NULL)
	{
		if(StringCompareIgnoreCase(bone->interface_data.name, "rightknee") == 0)
		{
			return TRUE;
		}
		if(StringCompareIgnoreCase(bone->interface_data.name, "leftknee") == 0)
		{
			return TRUE;
		}
	}
	return FALSE;
}

void AssetRootBoneGetWorldTransform(ASSET_ROOT_BONE* bone, void* transform)
{
	BtTransformSet(transform, bone->world_transform);
}

void AssetRootBoneGetLocalTranslation(ASSET_ROOT_BONE* bone, float* translation)
{
	COPY_VECTOR3(translation, ((ASSET_MODEL*)bone->interface_data.model)->position);
}

void AssetRootBoneSetLocalTranslation(ASSET_ROOT_BONE* bone, const float* translation)
{
	AssetModelSetWorldPositionInternal((ASSET_MODEL*)bone->interface_data.model, translation);
}

void AssetRootBoneGetLocalRotation(ASSET_ROOT_BONE* bone, float* rotation)
{
	COPY_VECTOR4(rotation, ((ASSET_MODEL*)bone->interface_data.model)->rotation);
}

void AssetRootBoneGetLocalTransform(ASSET_ROOT_BONE* bone, void* transform)
{
	BtTransformSet(transform, bone->world_transform);
}

void AssetRootBoneSetLocalRotation(ASSET_ROOT_BONE* bone, const float* rotation)
{
	AssetModelSetWorldRotationInternal((ASSET_MODEL*)bone->interface_data.model, rotation);
}

void InitializeAssetRootBone(
	ASSET_ROOT_BONE* bone,
	ASSET_MODEL* model,
	void* application_context
)
{
	const float basis[] = IDENTITY_MATRIX3x3;
	(void)memset(bone, 0, sizeof(*bone));

	bone->interface_data.name = "RootBoneAsset";
	bone->interface_data.local_transform = BtTransformNew(basis);
	bone->interface_data.model = (MODEL_INTERFACE*)model;
	bone->world_transform = BtTransformNew(basis);
	BtTransformSetOrigin(bone->interface_data.local_transform, model->position);
	BtTransformSetRotation(bone->interface_data.local_transform, model->rotation);

	bone->interface_data.get_destination_origin =
		(void (*)(void*, float*))DummyFuncGetZeroVector3;
	bone->interface_data.get_world_transform =
		(void (*)(void*, void*))AssetRootBoneGetWorldTransform;
	bone->interface_data.get_local_transform =
		(void (*)(void*, void*))AssetRootBoneGetLocalTransform;
	bone->interface_data.get_local_translation =
		(void (*)(void*, float*))AssetRootBoneGetLocalTranslation;
	bone->interface_data.set_local_translation = 
		(void (*)(void*, float*))AssetRootBoneSetLocalTranslation;
	bone->interface_data.get_local_rotation =
		(void (*)(void*, float*))AssetRootBoneGetLocalRotation;
	bone->interface_data.set_local_rotation =
		(void (*)(void*, float*))AssetRootBoneSetLocalRotation;
	bone->interface_data.is_movable =
		(int (*)(void*))DummyFuncTrueReturn;
	bone->interface_data.is_rotatable =
		(int (*)(void*))DummyFuncZeroReturn;
	bone->interface_data.is_interactive =
		(int (*)(void*))DummyFuncTrueReturn;
	bone->interface_data.is_visible =
		(int (*)(void*))DummyFuncZeroReturn;
	bone->interface_data.has_fixed_axis =
		(int (*)(void*))DummyFuncZeroReturn;
	bone->interface_data.has_inverse_kinematics =
		(int (*)(void*))DummyFuncZeroReturn;
}

void AssetScaleBoneGetLocalTranslation(ASSET_SCALE_BONE* bone, float* translation)
{
	COPY_VECTOR3(translation, bone->position);
}

void AssetScaleBoneSetLocalTranslation(ASSET_SCALE_BONE* bone, const float* translation)
{
	float scale;
	COPY_VECTOR3(bone->position, translation);
	SET_MAX(bone->position[0], ASSET_SCALE_BONE_MAX_VALUE);
	SET_MAX(bone->position[1], ASSET_SCALE_BONE_MAX_VALUE);
	SET_MAX(bone->position[2], ASSET_SCALE_BONE_MAX_VALUE);
	scale = (bone->position[0] + bone->position[1] + bone->position[2]) / 3.0f;
	bone->interface_data.model->scale_factor = scale;
}

static void AssetScaleBoneGetWorldTransform(ASSET_SCALE_BONE* bone, void* transform)
{
	BtTransformSetIdentity(transform);
}

void InitializeAssetScaleBone(
	ASSET_SCALE_BONE* bone,
	ASSET_MODEL* model,
	void* application_context
)
{
	(void)memset(bone, 0, sizeof(*bone));

	bone->interface_data.model = (MODEL_INTERFACE*)model;
	bone->application = (APPLICATION*)application_context;
	bone->position[0] = model->interface_data.scale_factor;
	bone->position[1] = model->interface_data.scale_factor;
	bone->position[2] = model->interface_data.scale_factor;
	bone->interface_data.name = "ScaleBoneAsset";
	bone->interface_data.get_local_translation =
		(void (*)(void*, float*))AssetScaleBoneGetLocalTranslation;
	bone->interface_data.get_local_rotation =
		(void (*)(void*, float*))DummyFuncGetIdentityQuaternion;
	bone->interface_data.get_destination_origin =
		(void (*)(void*, float*))DummyFuncGetZeroVector3;
	bone->interface_data.get_world_transform =
		(void (*)(void*, void*))AssetScaleBoneGetWorldTransform;
	bone->interface_data.get_local_transform =
		(void (*)(void*, void*))DummyFuncGetIdentityTransform;
	bone->interface_data.is_movable =
		(int (*)(void*))DummyFuncTrueReturn;
	bone->interface_data.is_rotatable =
		(int (*)(void*))DummyFuncZeroReturn;
	bone->interface_data.is_interactive =
		(int (*)(void*))DummyFuncTrueReturn;
	bone->interface_data.is_visible =
		(int (*)(void*))DummyFuncZeroReturn;
	bone->interface_data.has_fixed_axis =
		(int (*)(void*))DummyFuncZeroReturn;
	bone->interface_data.has_inverse_kinematics =
		(int (*)(void*))DummyFuncZeroReturn;
}

#ifdef __cplusplus
}
#endif
