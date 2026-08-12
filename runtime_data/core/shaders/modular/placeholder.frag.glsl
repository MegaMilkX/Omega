#fragment
#version 460

#include "types/vertex.glsl"
#include "types/fragment.glsl"
#include "interface_blocks/in_vertex.glsl"
#include "util.glsl"

void evalFragment(in VERTEX vert, inout FRAGMENT frag) {
	frag.albedo = vec3(1, 1, 1);
	frag.alpha *= 1;
	frag.roughness = 1;
	frag.metallic = 0;
	frag.emission = vec3(0);
	frag.ao = 0;
}

