#geometry
#version 450
layout (lines) in;
layout (triangle_strip, max_vertices = 4) out;

#include "interface_blocks/in_vertex_array.glsl"

in LINE_DATA {
	float thickness;
} in_line[];

#include "interface_blocks/out_vertex.glsl"


#include "uniform_blocks/common.glsl"
float clipNear(vec4 p0, vec4 p1, out vec4 outPos) {
    float d0 = p0.z + p0.w;
    float d1 = p1.z + p1.w;
    float t = d0 / (d0 - d1);
    outPos = mix(p0, p1, t);
    return t;
}
float clipLine(vec3 v0, vec3 v1, float near, out vec3 outPos) {	
	float t = (near - v0.z) / (v1.z - v0.z);
	outPos = mix(v0, v1, t);
	return t;
}
void main() {
	vec4 C0 = vec4(in_vertex[0].col, in_vertex[0].alpha);
	vec4 C1 = vec4(in_vertex[1].col, in_vertex[1].alpha);
	
	vec4 p1 = gl_in[0].gl_Position;
    vec4 p2 = gl_in[1].gl_Position;
    float d1 = p1.z + p1.w;
    float d2 = p2.z + p2.w;
	
	if (d1 < 0.0 && d2 < 0.0) {
		// Whole line behind near plane
        return;
    }
	
	if(d1 < .0) {
		float t = clipNear(p1, p2, p1);
		C0 = mix(C0, C1, t);
	}
	if(d2 < .0) {
		float t = clipNear(p1, p2, p2);
		C1 = mix(C0, C1, t);
	}
	
	vec4 ndc_uv1 = p1;
	vec4 ndc_uv2 = p2;

	vec2 vpsz = max(vec2(1, 1), viewportSize);
	vec2 dir  = normalize((p2.xy / p2.w - p1.xy / p1.w) * vpsz);
	float thickness1 = in_line[0].thickness;
	float thickness2 = in_line[1].thickness;
    vec2 offset1 = vec2(-dir.y, dir.x) * thickness1 * p1.w / vpsz;
    vec2 offset2 = vec2(-dir.y, dir.x) * thickness2 * p2.w / vpsz;

    gl_Position = p1 + vec4(offset1.xy, 0.0, 0.0);
	out_vertex.col = C0.rgb;
	out_vertex.alpha = C0.w;
	out_vertex.uv = ndc_uv1.xy;
    EmitVertex();
    gl_Position = p1 - vec4(offset1.xy, 0.0, 0.0);
	out_vertex.col = C0.rgb;
	out_vertex.alpha = C0.w;
	out_vertex.uv = ndc_uv1.xy;
    EmitVertex();
    gl_Position = p2 + vec4(offset2.xy, 0.0, 0.0);
	out_vertex.col = C1.rgb;
	out_vertex.alpha = C1.w;
	out_vertex.uv = ndc_uv2.xy;
    EmitVertex();
    gl_Position = p2 - vec4(offset2.xy, 0.0, 0.0);
	out_vertex.col = C1.rgb;
	out_vertex.alpha = C1.w;
	out_vertex.uv = ndc_uv2.xy;
    EmitVertex();

    EndPrimitive();
}