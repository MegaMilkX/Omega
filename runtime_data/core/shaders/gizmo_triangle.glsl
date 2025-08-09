#vertex
#version 450 
in vec3 inPosition;
in vec4 inColorRGBA;

out VERTEX_DATA {
	vec4 color;
	vec4 uv;
} out_vert;

#include "uniform_blocks/common.glsl"
#include "uniform_blocks/model.glsl"

void main(){
	out_vert.color = inColorRGBA;
	vec4 pos = matProjection * matView * matModel * vec4(inPosition, 1);
	out_vert.uv = pos;
	gl_Position = pos;
}


#fragment
#version 450

in VERTEX_DATA {
	vec4 color;
	vec4 uv;
} in_vert;

uniform sampler2D Depth;
out vec4 outColor;

#include "uniform_blocks/common.glsl"

void main(){
	vec4 uv_frag = in_vert.uv;
	vec4 col_frag = in_vert.color;
	
	vec2 uv = vec2((uv_frag.x / uv_frag.w + 1.0) * .5, (uv_frag.y / uv_frag.w + 1.0) * .5);
	float depth = texture(Depth, uv).x;
	float cur_depth = gl_FragCoord.z;
	float mul = 1.0;
	if(cur_depth > depth) {
		mul = .70;
	}
	outColor = vec4(col_frag.xyz, col_frag.a * mul);
}
