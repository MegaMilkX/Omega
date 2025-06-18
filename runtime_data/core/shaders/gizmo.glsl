#vertex
#version 450 
in vec3 inPosition;
in vec4 inColorRGBA;
out vec4 col_frag;
out vec4 frag_uv;

#include "uniform_blocks/common.glsl"
#include "uniform_blocks/model.glsl"

void main(){
	col_frag = inColorRGBA;
	vec4 pos = matProjection * matView * matModel * vec4(inPosition, 1);
	frag_uv = pos;
	gl_Position = pos;
}

#fragment
#version 450
in vec4 frag_uv;
in vec4 col_frag;
uniform sampler2D Depth;
out vec4 outColor;

#include "uniform_blocks/common.glsl"

void main(){
	vec2 uv = vec2((frag_uv.x / frag_uv.w + 1.0) * .5, (frag_uv.y / frag_uv.w + 1.0) * .5);
	float depth = texture(Depth, uv).x;
	float cur_depth = gl_FragCoord.z;
	float mul = 1.0;
	if(cur_depth > depth) {
		mul = .70;
	}
	outColor = vec4(col_frag.xyz, col_frag.a * mul);
}
