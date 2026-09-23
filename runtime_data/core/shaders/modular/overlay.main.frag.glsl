#fragment
#version 460

uniform sampler2D Depth;
out vec4 outFinal;

#include "interface_blocks/in_vertex.glsl"
#include "uniform_blocks/common.glsl"

#include "types/vertex.glsl"
#include "types/fragment.glsl"
void evalAttribFragment(inout VERTEX vert, inout FRAGMENT frag);
void evalFragment(in VERTEX vert, inout FRAGMENT frag);


void main(){
	vec3 N = normalize(in_vertex.TBN[2]);
	if(!gl_FrontFacing) {
		N *= -1;
	}
	
	VERTEX vert;
	FRAGMENT frag;
	{
		vert.pos = in_vertex.pos;
		vert.world_pos = in_vertex.pos;
		vert.col = in_vertex.col;
		vert.alpha = in_vertex.alpha;
		vert.uv = in_vertex.uv;
		vert.uv2 = in_vertex.uv2;
		vert.TBN = in_vertex.TBN;
		vert.invTBN = in_vertex.invTBN;
		
		frag.albedo = vert.col;
		frag.normal = N;
		frag.light_mask = 1.0;
		frag.roughness = 1.0;
		frag.metallic = 0.0;
		frag.emission = vec3(0, 0, 0);
		frag.ao = 0.0;
		frag.alpha = vert.alpha;
		frag.depth = gl_FragCoord.z;
		
#ifdef ENABLE_FRAG_ATTRIB		
		evalAttribFragment(vert, frag);
#endif
#ifdef ENABLE_FRAG_EXTENSION
		evalFragment(vert, frag);
#endif
	}
	
	float alpha = 1.0;
	float d = texture(Depth, gl_FragCoord.xy / viewportSize.xy).x;
	if(d < gl_FragCoord.z) {
		alpha = .5;
	}
	
	outFinal = vec4(vert.col * frag.albedo, vert.alpha * frag.alpha * alpha);
}

