#vertex
#version 460

#define BILLOBARD_KEEP_ROTATION

#include "uniform_blocks/common.glsl"
#include "uniform_blocks/model.glsl"

#include "types/vertex.glsl"
void evalToWorld(inout VERTEX vert) {
#if TRANSFORM_MODE == TRANSFORM_BILLBOARD
	mat3 billboardRot = transpose(mat3(matView));
#ifdef BILLOBARD_KEEP_ROTATION
	mat3 linearPart
		= billboardRot
		* mat3(matModel[0].xyz, matModel[1].xyz, matModel[2].xyz);
#else
	vec3 scale = vec3(length(matModel[0]), length(matModel[1]), length(matModel[2]));
	mat3 linearPart = billboardRot * mat3(scale.x, 0, 0,	0, scale.y, 0,	0, 0, scale.z);
#endif
	mat4 mat_Model = matModel;
	mat_Model[0] = vec4(linearPart[0], 0);
	mat_Model[1] = vec4(linearPart[1], 0);
	mat_Model[2] = vec4(linearPart[2], 0);
	
	mat4 mat_ViewModel = matView * mat_Model;
	
	vert.TBN = mat3(mat_Model) * vert.TBN;
	vert.TBN[0] = normalize(vert.TBN[0]);
	vert.TBN[1] = normalize(vert.TBN[1]);
	vert.TBN[2] = normalize(vert.TBN[2]);
	vert.world_pos = vec3(mat_Model * vec4(vert.pos, 1));
	vert.pos = vec3(mat_ViewModel * vec4(vert.pos, 1));
#elif TRANSFORM_MODE == TRANSFORM_BILLBOARD_Y
	vec3 pivotWorld = matModel[3].xyz;
	vec3 cameraWorldPos = inverse(matView)[3].xyz;

	vec3 up = vec3(0, 1, 0);
	vec3 fwd = cameraWorldPos - pivotWorld;
	fwd.y = 0.0;
	fwd = normalize(fwd);
	vec3 right = normalize(cross(up, fwd));

#ifdef BILLOBARD_KEEP_ROTATION
	mat3 billboardRot = mat3(right, up, fwd) * mat3(matModel[0].xyz, matModel[1].xyz, matModel[2].xyz);
#else
	vec3 scale = vec3(length(matModel[0]), length(matModel[1]), length(matModel[2]));
	mat3 billboardRot = mat3(right, up, fwd) * mat3(scale.x, 0, 0, 0, scale.y, 0, 0, 0, scale.z);
#endif

	mat4 mat_Model = matModel;
	mat_Model[0] = vec4(billboardRot[0], 0);
	mat_Model[1] = vec4(billboardRot[1], 0);
	mat_Model[2] = vec4(billboardRot[2], 0);
	mat4 mat_ViewModel = matView * mat_Model;

	vert.TBN = mat3(mat_Model) * vert.TBN;
	vert.TBN[0] = normalize(vert.TBN[0]);
	vert.TBN[1] = normalize(vert.TBN[1]);
	vert.TBN[2] = normalize(vert.TBN[2]);
	vert.world_pos = vec3(mat_Model * vec4(vert.pos, 1));
	vert.pos = vec3(mat_ViewModel * vec4(vert.pos, 1));
#else
	vert.TBN = mat3(mat3(matModel) * vert.TBN);
	vert.TBN[0] = normalize(vert.TBN[0]);
	vert.TBN[1] = normalize(vert.TBN[1]);
	vert.TBN[2] = normalize(vert.TBN[2]);
	vert.world_pos = vec3(matModel * vec4(vert.pos, 1));
	vert.pos = vec3(matView * matModel * vec4(vert.pos, 1));
#endif
}
