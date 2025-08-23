#vertex
#version 450

#include "uniform_blocks/common.glsl"

in vec3 inPosition;
out vec2 frag_uv;

void main(){
	vec2 uv = vec2((inPosition.x + 1.0) * .5, (inPosition.y + 1.0) * .5);
	frag_uv = mix(vp_rect_ratio.xy, vp_rect_ratio.zw, uv.xy);
	
	gl_Position = vec4(inPosition, 1.0);
}

#fragment
#version 450

#include "uniform_blocks/common.glsl"

in vec2 frag_uv;
uniform sampler2D Position;
out vec4 outColor;

void main() {
	vec3 worldPos = texture(Position, frag_uv).xyz;
	
	vec4 scrTo = (matProjection * matView * vec4(worldPos, 1));
	vec4 scrFrom = (matProjection * matView_prev * vec4(worldPos, 1));
	scrTo.xyz /= scrTo.w;
	scrFrom.xyz /= scrFrom.w;
	
	vec3 delta = scrTo.xyz - scrFrom.xyz;
	
	outColor = vec4(delta, 1);
}
