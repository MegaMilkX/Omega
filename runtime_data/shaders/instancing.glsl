#vertex
#version 450 
in vec3 inPosition;
in vec3 inColorRGB;
in vec2 inUV;
in vec3 inNormal;
in vec4 inParticlePosition;
out vec3 pos_frag;
out vec3 col_frag;
out vec2 uv_frag;
out vec3 normal_frag;


#include "uniform_blocks/common.glsl"
#include "uniform_blocks/model.glsl"

mat4 buildTranslation(vec3 t) {
	return mat4(
		vec4(1.0, 0.0, 0.0, 0.0),
		vec4(0.0, 1.0, 0.0, 0.0),
		vec4(0.0, 0.0, 1.0, 0.0),
		vec4(t, 1.0));
}

void main(){
	mat4 mdl = buildTranslation(inParticlePosition.xyz);
	
	uv_frag = inUV;
	normal_frag = (mdl * vec4(inNormal, 0)).xyz;
	pos_frag = (mdl * vec4(inPosition, 1)).xyz;
	col_frag = inColorRGB;
	vec4 pos = matProjection * matView * mdl * vec4(inPosition, 1);
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

uniform sampler2D texAlbedo;

void main(){
	vec4 pix = texture(texAlbedo, uv_frag);
	float a = pix.a;
	vec3 N = normalize(normal_frag);
	vec3 color = col_frag * (pix.rgb);
	
	outAlbedo = vec4(color, a);
	outPosition = vec4(pos_frag, 1);
	outNormal = vec4((N + 1.0) / 2.0, 1);
}
