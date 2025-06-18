#vertex
#version 450 

in vec3 inPosition;
in vec3 inColorRGB;
in vec2 inUV;
in vec3 inNormal;
out vec3 pos_frag;
out vec3 col_frag;
out vec2 uv_frag;
out vec3 normal_frag;


#include "uniform_blocks/common.glsl"
#include "uniform_blocks/model.glsl"

void main(){
	uv_frag = inUV + (time * 0.25);
	normal_frag = (matModel * vec4(inNormal, 0)).xyz;
	pos_frag = (matModel * vec4(inPosition, 1)).xyz;
	col_frag = inColorRGB;
	vec4 pos = matProjection * matView * matModel * vec4(inPosition, 1);
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
uniform sampler2D texAlbedo;

#include "functions/tonemapping.glsl"

void main() {
	float GAMMA = 2.2;
	
	vec4 pix = texture(texAlbedo, uv_frag);
	float a = pix.a;
	pix.xyz = inverseGammaCorrect(inverseTonemapFilmicUncharted2(pix.xyz, .1), GAMMA);
	
	vec3 N = normalize(normal_frag);
	float lightness = dot(N, normalize(vec3(0, 0, 1))) + dot(N, normalize(vec3(0, 1, -1)));
	lightness = clamp(lightness, 0.2, 1.0) * 2.0;
	vec3 color = col_frag * (pix.rgb) * lightness;
	outAlbedo = vec4(color, a);
	
	outPosition = vec4(pos_frag, 1);
}
