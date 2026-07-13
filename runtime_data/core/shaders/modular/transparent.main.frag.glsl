#fragment
#version 460

out vec4 outFinal;

#include "interface_blocks/in_vertex.glsl"
#include "uniform_blocks/common.glsl"
#include "functions/tonemapping.glsl"

#include "types/fragment.glsl"
void evalFragment(inout FRAGMENT frag);

void main(){
	vec3 N = normalize(in_vertex.normal);
	if(!gl_FrontFacing) {
		N *= -1;
	}
	
	FRAGMENT frag;
	{
		frag.albedo = vec3(1, 1, 1);
		frag.normal = N;
		frag.light_mask = 1.0;
		frag.roughness = 1.0;
		frag.metallic = 0.0;
		frag.emission = vec3(0, 0, 0);
		frag.ao = 0.0;
		frag.alpha = 1.0;
#ifdef ENABLE_FRAG_EXTENSION
		evalFragment(frag);
#endif
	}
	
	frag.albedo = inverseGammaCorrect(frag.albedo, gamma);
	
	outFinal = vec4(frag.albedo, frag.alpha);
}

