#fragment
#version 460

#include "types/vertex.glsl"
#include "types/fragment.glsl"
#include "interface_blocks/in_vertex.glsl"
#include "util.glsl"

uniform sampler2D texAlbedo;
uniform sampler2D texAlbedo2;

vec4 boxmap( in sampler2D s, in sampler2D s2, in vec3 p, in vec3 n, in float k ) {
    // project+fetch
	vec4 x = texture( s2, p.yz );
	vec4 y = texture( s, p.zx );
	vec4 z = texture( s2, p.xy );
    
    // and blend
    vec3 m = pow( abs(n), vec3(k) );
	return (x*m.x + y*m.y + z*m.z) / (m.x + m.y + m.z);
}

void evalFragment(in VERTEX vert, inout FRAGMENT frag) {
	vec3 N = vert.TBN[2];
	frag.normal = N;
	
	vec4 pix = boxmap(texAlbedo, texAlbedo2, vert.pos * .1, N, 4);
	frag.albedo = pix.rgb;// * vert.col.rgb;
	frag.alpha *= pix.a;
	frag.roughness = 1;
	frag.metallic = 0;
	frag.emission = vec3(0);
	frag.ao = 1;
}

