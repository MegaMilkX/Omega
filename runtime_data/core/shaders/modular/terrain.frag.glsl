#fragment
#version 460

#include "types/fragment.glsl"
#include "interface_blocks/in_vertex.glsl"
#include "util.glsl"

uniform sampler2D texAlbedo;
uniform sampler2D texAlbedo2;
uniform sampler2D texNormal;
uniform sampler2D texRoughness;
uniform sampler2D texMetallic;
uniform sampler2D texEmission;
uniform sampler2D texAmbientOcclusion;

vec4 boxmap( in sampler2D s, in sampler2D s2, in vec3 p, in vec3 n, in float k ) {
    // project+fetch
	vec4 x = texture( s2, p.yz );
	vec4 y = texture( s, p.zx );
	vec4 z = texture( s2, p.xy );
    
    // and blend
    vec3 m = pow( abs(n), vec3(k) );
	return (x*m.x + y*m.y + z*m.z) / (m.x + m.y + m.z);
}

void evalFragment(inout FRAGMENT frag) {
	vec3 normal = texture(texNormal, in_vertex.uv).xyz;
	frag.normal = normalSampleToWorld(normal, in_vertex.TBN, gl_FrontFacing);
	
	vec4 pix = boxmap(texAlbedo, texAlbedo2, in_vertex.pos * .1, in_vertex.normal, 4);
	//vec4 pix = texture(texAlbedo, in_vertex.uv);
	frag.albedo = pix.rgb;// * in_vertex.col.rgb;
	frag.alpha = pix.a;
	frag.roughness = texture(texRoughness, in_vertex.uv).x;
	frag.metallic = texture(texMetallic, in_vertex.uv).x;
	frag.emission = texture(texEmission, in_vertex.uv).xyz;
	frag.ao = texture(texAmbientOcclusion, in_vertex.uv).x;
}

