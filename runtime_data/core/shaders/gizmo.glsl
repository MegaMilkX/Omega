#vertex
#version 450 
in vec3 inPosition;
in vec4 inColorRGBA;
out vec4 col_geom;
out vec4 uv_geom;

#include "uniform_blocks/common.glsl"
#include "uniform_blocks/model.glsl"

void main(){
	col_geom = inColorRGBA;
	vec4 pos = matProjection * matView * matModel * vec4(inPosition, 1);
	uv_geom = pos;
	gl_Position = pos;
}

#geometry
#version 450
layout (lines) in;
layout (triangle_strip, max_vertices = 4) out;

in vec4 col_geom[];
in vec4 uv_geom[];
out vec4 uv_frag;
out vec4 col_frag;

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
	vec2 half_thickness = 2.0 / viewportSize * THICKNESS_PX * .5;
	vec3 V0 = vec3(V.xy * half_thickness * P0.w, V.z);
	vec3 V1 = vec3(V.xy * half_thickness * P1.w, V.z);
	V0.z = 0;
	V1.z = 0;
	
	vec4 C0 = col_geom[0];
	vec4 C1 = col_geom[1];
	
    gl_Position = gl_in[0].gl_Position - vec4(V0, 0);
	col_frag = C0;
	uv_frag = uv_geom[0];
    EmitVertex();
	
    gl_Position = gl_in[0].gl_Position + vec4(V0, 0);
	col_frag = C0;
	uv_frag = uv_geom[0];
    EmitVertex();
	
    gl_Position = gl_in[1].gl_Position - vec4(V1, 0);
	col_frag = C1;
	uv_frag = uv_geom[1];
    EmitVertex();
	
    gl_Position = gl_in[1].gl_Position + vec4(V1, 0);
	col_frag = C1;
	uv_frag = uv_geom[1];
    EmitVertex();
	
    EndPrimitive();
}

#fragment
#version 450
in vec4 uv_frag;
in vec4 col_frag;
uniform sampler2D Depth;
out vec4 outColor;

#include "uniform_blocks/common.glsl"

void main(){
	vec2 uv = vec2((uv_frag.x / uv_frag.w + 1.0) * .5, (uv_frag.y / uv_frag.w + 1.0) * .5);
	float depth = texture(Depth, uv).x;
	float cur_depth = gl_FragCoord.z;
	float mul = 1.0;
	if(cur_depth > depth) {
		mul = .70;
	}
	outColor = vec4(col_frag.xyz, col_frag.a * mul);
}
