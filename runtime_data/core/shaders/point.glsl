#vertex
#version 450 
in vec3 inPosition;
in float inLineThickness;
in vec4 inColorRGBA;

out VERTEX_DATA {
	vec4 color;
	vec4 uv;
	float thickness;
} out_vert;

#include "uniform_blocks/common.glsl"
#include "uniform_blocks/model.glsl"

void main(){
	out_vert.color = inColorRGBA;
	vec4 pos = matProjection * matView * matModel * vec4(inPosition, 1);
	out_vert.uv = pos;
	out_vert.thickness = inLineThickness;
	gl_Position = pos;
}

#geometry
#version 450
layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

in VERTEX_DATA {
	vec4 color;
	vec4 uv;
	float thickness;
} in_vert[];

out VERTEX_DATA {
	vec4 color;
	vec4 uv;
	float thickness;
} out_vert;

#include "uniform_blocks/common.glsl"

void main() {
	vec4 P0 = gl_in[0].gl_Position;
	P0.xyz /= P0.w;
	
	//float THICKNESS_PX = in_vert[0].thickness;
	float THICKNESS_PX = 30;
	
	vec2 half_thickness = 2.0 / viewportSize * THICKNESS_PX * .5;
	vec2 Vmin = vec2(-.5, -.5) * half_thickness * P0.w;
	vec2 Vmax = vec2(.5, .5) * half_thickness * P0.w;
	
	vec4 C0 = in_vert[0].color;
	
    gl_Position = gl_in[0].gl_Position + vec4(Vmin.x, Vmin.y, 0, 0);
	out_vert.color = C0;
	out_vert.uv = in_vert[0].uv;
    EmitVertex();
	
    gl_Position = gl_in[0].gl_Position + vec4(Vmax.x, Vmin.y, 0, 0);
	out_vert.color = C0;
	out_vert.uv = in_vert[0].uv;
    EmitVertex();
	
    gl_Position = gl_in[0].gl_Position + vec4(Vmin.x, Vmax.y, 0, 0);
	out_vert.color = C0;
	out_vert.uv = in_vert[0].uv;
    EmitVertex();
	
    gl_Position = gl_in[0].gl_Position + vec4(Vmax.x, Vmax.y, 0, 0);
	out_vert.color = C0;
	out_vert.uv = in_vert[0].uv;
    EmitVertex();
	
    EndPrimitive();
}

#fragment
#version 450

in VERTEX_DATA {
	vec4 color;
	vec4 uv;
	float thickness;
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
	outColor = vec4(1, 0, 0, 1 /*col_frag.xyz, col_frag.a * mul*/);
}
