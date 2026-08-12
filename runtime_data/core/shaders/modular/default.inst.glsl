#vertex
#version 460

#include "uniform_blocks/common.glsl"
#include "types/vertex.glsl"

in vec4 inInstancePosition;
in vec4 inInstanceQuat;

mat3 quatToMat3(vec4 q) {
    mat3 m;
    float x = q.x, y = q.y, z = q.z, w = q.w;
    float x2 = x + x;
    float y2 = y + y;
    float z2 = z + z;
    
    float xx = x * x2;
    float xy = x * y2;
    float xz = x * z2;
    float yy = y * y2;
    float yz = y * z2;
    float zz = z * z2;
    float wx = w * x2;
    float wy = w * y2;
    float wz = w * z2;

    m[0][0] = 1.0 - (yy + zz);
    m[0][1] = xy + wz;
    m[0][2] = xz - wy;

    m[1][0] = xy - wz;
    m[1][1] = 1.0 - (xx + zz);
    m[1][2] = yz + wx;

    m[2][0] = xz + wy;
    m[2][1] = yz - wx;
    m[2][2] = 1.0 - (xx + yy);

    return m;
}

mat4 buildTransform(vec3 t, vec4 quat, float scale) {
	mat3 rot = quatToMat3(quat);
	rot = rot * scale;
	return mat4(
		vec4(rot[0].x, rot[0].y, rot[0].z, 0.0),
		vec4(rot[1].x, rot[1].y, rot[1].z, 0.0),
		vec4(rot[2].x, rot[2].y, rot[2].z, 0.0),
		vec4(t, 1.0)
	);
}


void evalToWorld(inout VERTEX vert) {
	mat4 mdl = buildTransform(inInstancePosition.xyz, inInstanceQuat.xyzw, inInstancePosition.w);
	
	mat4 mat_Model = mdl;
	mat4 mat_ViewModel = matView * mdl;
#if TRANSFORM_MODE == TRANSFORM_BILLBOARD
	float scale = inParticlePosition.w;

	mat3 billboardRot = transpose(mat3(matView));
	mat3 linearPart
		= billboardRot
		* mat3(
			scale, 0, 0,
			0, scale, 0,
			0, 0, scale
		);
	mat_Model[0] = vec4(linearPart[0], 0);
	mat_Model[1] = vec4(linearPart[1], 0);
	mat_Model[2] = vec4(linearPart[2], 0);
	
	mat_ViewModel = matView * mat_Model;
	
	vert.TBN = mat3(mat_Model) * vert.TBN;
	vert.TBN[0] = normalize(vert.TBN[0]);
	vert.TBN[1] = normalize(vert.TBN[1]);
	vert.TBN[2] = normalize(vert.TBN[2]);
#elif TRANSFORM_MODE == TRANSFORM_BILLBOARD_Y
	float scale = inParticlePosition.w;
	vec3 pivotWorld = mdl[3].xyz;
	vec3 cameraWorldPos = inverse(matView)[3].xyz;

	vec3 up = vec3(0, 1, 0);
	vec3 fwd = cameraWorldPos - pivotWorld;
	fwd.y = 0.0;
	fwd = normalize(fwd);
	vec3 right = normalize(cross(up, fwd));

	mat3 billboardRot = mat3(right, up, fwd) * mat3(scale, 0, 0, 0, scale, 0, 0, 0, scale);

	mat_Model[0] = vec4(billboardRot[0], 0);
	mat_Model[1] = vec4(billboardRot[1], 0);
	mat_Model[2] = vec4(billboardRot[2], 0);
	mat_ViewModel = matView * mat_Model;

	vert.TBN = mat3(mat_Model) * vert.TBN;
	vert.TBN[0] = normalize(vert.TBN[0]);
	vert.TBN[1] = normalize(vert.TBN[1]);
	vert.TBN[2] = normalize(vert.TBN[2]);
#else
	vert.TBN = mat3(mdl * mat4(vert.TBN));
	vert.TBN[0] = normalize(vert.TBN[0]);
	vert.TBN[1] = normalize(vert.TBN[1]);
	vert.TBN[2] = normalize(vert.TBN[2]);
#endif

	vert.world_pos = vec3(mat_Model * vec4(vert.pos, 1));
	vert.pos = vec3(mat_ViewModel * vec4(vert.pos, 1));
}