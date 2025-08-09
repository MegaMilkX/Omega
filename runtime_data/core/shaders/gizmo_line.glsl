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
layout (lines) in;
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

const float THICKNESS_PX = 3;

#include "uniform_blocks/common.glsl"

void main() {
	vec4 P0 = gl_in[0].gl_Position;
	vec4 P1 = gl_in[1].gl_Position;
	P0.xyz /= P0.w;
	P1.xyz /= P1.w;
	vec3 lineN = (P1.xyz - P0.xyz);
	lineN.z = 0;
	lineN = normalize(lineN);
	vec3 V = normalize(cross(lineN, vec3(0, 0, 1)));
	vec2 half_thickness_a = 2.0 / viewportSize * in_vert[0].thickness * .5;
	vec2 half_thickness_b = 2.0 / viewportSize * in_vert[1].thickness * .5;
	vec3 V0 = vec3(V.xy * half_thickness_a * P0.w, V.z);
	vec3 V1 = vec3(V.xy * half_thickness_b * P1.w, V.z);
	V0.z = 0;
	V1.z = 0;
	
	vec4 C0 = in_vert[0].color;
	vec4 C1 = in_vert[1].color;
	
    gl_Position = gl_in[0].gl_Position - vec4(V0, 0);
	out_vert.color = C0;
	out_vert.uv = in_vert[0].uv;
    EmitVertex();
	
    gl_Position = gl_in[0].gl_Position + vec4(V0, 0);
	out_vert.color = C0;
	out_vert.uv = in_vert[0].uv;
    EmitVertex();
	
    gl_Position = gl_in[1].gl_Position - vec4(V1, 0);
	out_vert.color = C1;
	out_vert.uv = in_vert[1].uv;
    EmitVertex();
	
    gl_Position = gl_in[1].gl_Position + vec4(V1, 0);
	out_vert.color = C1;
	out_vert.uv = in_vert[1].uv;
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
	outColor = vec4(col_frag.xyz, col_frag.a * mul);
}
