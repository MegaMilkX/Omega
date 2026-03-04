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

float clipLine(vec3 v0, vec3 v1, float near, out vec3 outPos) {	
	float t = (near - v0.z) / (v1.z - v0.z);
	outPos = mix(v0, v1, t);
	return t;
}

void main(){
	out_vert.color = inColorRGBA;
	vec4 pos = matView * matModel * vec4(inPosition, 1);
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

const float THICKNESS_PX = 2;

#include "uniform_blocks/common.glsl"
float clipLine(vec3 v0, vec3 v1, float near, out vec3 outPos) {	
	float t = (near - v0.z) / (v1.z - v0.z);
	outPos = mix(v0, v1, t);
	return t;
}
void main() {
	vec4 C0 = in_vert[0].color;
	vec4 C1 = in_vert[1].color;
	
	vec4 p1 = gl_in[0].gl_Position;
    vec4 p2 = gl_in[1].gl_Position;
	if(p1.z > -zNear) {
		//C0 = vec4(0, 1, 0, 1);
		float t = clipLine(p1.xyz, p2.xyz, -zNear, p1.xyz);
	}
	if(p2.z > -zNear) {
		//C1 = vec4(0, 1, 0, 1);
		float t = clipLine(p1.xyz, p2.xyz, -zNear, p2.xyz);
	}
	
	p1 = matProjection * p1;
	p2 = matProjection * p2;
	
	vec4 ndc_uv1 = p1;
	vec4 ndc_uv2 = p2;

	vec2 vpsz = max(vec2(1, 1), viewportSize);
    vec2 dir    = normalize((p2.xy / p2.w - p1.xy / p1.w) * vpsz);
    vec2 offset1 = vec2(-dir.y, dir.x) * in_vert[0].thickness / vpsz;
    vec2 offset2 = vec2(-dir.y, dir.x) * in_vert[1].thickness / vpsz;

    gl_Position = p1 + vec4(offset1.xy * p1.w, 0.0, 0.0);
	out_vert.color = C0;
	out_vert.uv = ndc_uv1;
    EmitVertex();
    gl_Position = p1 - vec4(offset1.xy * p1.w, 0.0, 0.0);
	out_vert.color = C0;
	out_vert.uv = ndc_uv1;
    EmitVertex();
    gl_Position = p2 + vec4(offset2.xy * p2.w, 0.0, 0.0);
	out_vert.color = C1;
	out_vert.uv = ndc_uv2;
    EmitVertex();
    gl_Position = p2 - vec4(offset2.xy * p2.w, 0.0, 0.0);
	out_vert.color = C1;
	out_vert.uv = ndc_uv2;
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
		mul = .35;
	}
	outColor = vec4(col_frag.xyz, col_frag.a * mul);
}
