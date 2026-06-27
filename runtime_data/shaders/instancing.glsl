#vertex
#version 450 
in vec3 inPosition;
in vec3 inColorRGB;
in vec2 inUV;
in vec3 inNormal;
in vec4 inParticlePosition;
in vec4 inParticleQuat;
out vec3 pos_frag;
out vec3 col_frag;
out vec2 uv_frag;
out vec3 normal_frag;


#include "uniform_blocks/common.glsl"
#include "uniform_blocks/model.glsl"

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

mat4 buildTranslation(vec3 t, vec4 quat, float scale) {
	mat3 rot = quatToMat3(quat);
	rot = rot * scale;
	return mat4(
		vec4(rot[0].x, rot[0].y, rot[0].z, 0.0),
		vec4(rot[1].x, rot[1].y, rot[1].z, 0.0),
		vec4(rot[2].x, rot[2].y, rot[2].z, 0.0),
		vec4(t, 1.0)
	);
}

void main(){
	mat4 mdl = buildTranslation(inParticlePosition.xyz, inParticleQuat.xyzw, inParticlePosition.w);
	
	uv_frag = inUV;
	normal_frag = (mdl * vec4(inNormal, 0)).xyz;
	pos_frag = (mdl * vec4(inPosition, 1)).xyz;
	col_frag = inColorRGB;
	vec4 pos = matProjection * matView * mdl * matModel * vec4(inPosition, 1);
	gl_Position = pos;
}

#fragment
#version 450
in vec3 pos_frag;
in vec3 col_frag;
in vec2 uv_frag;
in vec3 normal_frag;

out vec4 outAlbedo;
out vec4 outPosition;
out vec4 outNormal;
out vec4 outRoughness;
out vec4 outMetallic;

uniform sampler2D texAlbedo;

#include "uniform_blocks/common.glsl"
#include "functions/tonemapping.glsl"

void main(){
	vec4 pix = texture(texAlbedo, uv_frag);
	float a = pix.a;
	vec3 N = normalize(normal_frag);
	if(!gl_FrontFacing) {
		N *= -1;
	}
	vec3 color = col_frag * (pix.rgb);
	
	color.xyz = inverseGammaCorrect(color.xyz, gamma);
	
	if(a < .1) {
		discard;
	}
	
	outAlbedo = vec4(color, 1);
	outPosition = vec4(pos_frag, 1);
	outNormal = vec4((N + 1.0) / 2.0, 1);
	outRoughness = vec4(1, 1, 1, 1);
	outMetallic = vec4(0, 0, 0, 0);
}
