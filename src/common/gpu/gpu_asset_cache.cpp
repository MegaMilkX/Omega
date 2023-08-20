#include "gpu_asset_cache.hpp"

#include "embedded_files/decal.png.h"
#include "embedded_files/nimbusmono_bold.otf.h"

static const char* decal_vertex_shader = R"(#version 450 
layout (location = 0) in vec3 inPosition;
layout(std140) uniform bufCamera3d {
	mat4 matProjection;
	mat4 matView;
	vec2 screenSize;
};
layout(std140) uniform bufModel {
	mat4 matModel;
};
out mat4 fragProjection;
out mat4 fragView;
out mat4 fragModel;
void main() {
	fragProjection = matProjection;
	fragView = matView;
	fragModel = matModel;         

	gl_Position = matProjection * matView * matModel * vec4(inPosition, 1.0);
}
)";

static const char* decal_fragment_shader = R"(#version 450
layout(std140) uniform bufCamera3d {
	mat4 matProjection;
	mat4 matView;
	vec2 screenSize;
};
layout(std140) uniform bufDecal {
	uniform vec3 boxSize;
	uniform vec4 RGBA;
};            
uniform sampler2D tex;
uniform sampler2D Depth;
in mat4 fragProjection;
in mat4 fragView;
in mat4 fragModel;
out vec4 outAlbedo;

float contains(vec3 pos, vec3 bottom_left, vec3 top_right) {
	vec3 s = step(bottom_left, pos) - step(top_right, pos);
	return s.x * s.y * s.z;
}

vec3 worldPosFromDepth(float depth, vec2 uv, mat4 proj, mat4 view) {
	float z = depth * 2.0 - 1.0;

	vec4 clipSpacePosition = vec4(uv * 2.0 - 1.0, z, 1.0);
	vec4 viewSpacePosition = inverse(proj) * clipSpacePosition;

	viewSpacePosition /= viewSpacePosition.w;
	
	vec4 worldSpacePosition = inverse(view) * viewSpacePosition;

	return worldSpacePosition.xyz;
}

void main(){
	vec4 frag_coord = gl_FragCoord;
	float frag_u = frag_coord.x / screenSize.x;
	float frag_v = frag_coord.y / screenSize.y;
	vec4 depth_sample = texture(Depth, vec2(frag_u, frag_v));
	
	vec3 world_pos = worldPosFromDepth(depth_sample.x, vec2(frag_u, frag_v), fragProjection, fragView);
	vec4 decal_pos = inverse(fragModel) * vec4(world_pos, 1);
	if(contains(decal_pos.xyz, -boxSize * .5, boxSize * .5) < 1.0) {
		discard;
	}
	vec2 decal_uv = vec2(1.0 - decal_pos.x / boxSize.x + .5, decal_pos.z / boxSize.z + .5);
	vec4 decal_sample = texture(tex, decal_uv);
	float alpha = 1.0 - abs(decal_pos.y / boxSize.y * 2.0);
	outAlbedo = vec4(decal_sample.xyz, decal_sample.a * alpha) * RGBA;
}
)";

gpuAssetCache::gpuAssetCache() {
    default_decal_shader.reset_acquire();
    default_decal_shader->init(decal_vertex_shader, decal_fragment_shader);

    default_decal_texture.reset_acquire();
    ktImage img;
    loadImage(&img, decal_png, sizeof(decal_png));
    default_decal_texture->setData(&img);

    default_decal_material.reset_acquire();
    auto tech = default_decal_material->addTechnique("Decals");
    auto pass = tech->addPass();
    pass->setShader(default_decal_shader);
    pass->depth_write = 0;
    //pass->depth_test = 0;
    //pass->cull_faces = 0;
    pass->blend_mode = GPU_BLEND_MODE::ADD;
    default_decal_material->addPassOutputSampler("Depth");
    default_decal_material->addSampler("tex", default_decal_texture);
    default_decal_material->compile();


    default_typeface.reset_acquire();
    typefaceLoad(default_typeface.get(), (void*)nimbusmono_bold_otf, sizeof(nimbusmono_bold_otf));

    default_font.reset_acquire();
    default_font->init(default_typeface.get(), 16, 72);
}
gpuAssetCache::~gpuAssetCache() {

}

RHSHARED<gpuMaterial> gpuAssetCache::getDefaultDecalMaterial() {
    return default_decal_material;
}
RHSHARED<Typeface> gpuAssetCache::getDefaultTypeface() {
    return default_typeface;
}
RHSHARED<Font> gpuAssetCache::getDefaultFont() {
    return default_font;
}
