#vertex
#version 450 
layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec2 inUV;
layout (location = 4) in vec4 inParticlePosition;
layout (location = 5) in vec4 inParticleData;
layout (location = 6) in vec4 inParticleColorRGBA;
layout (location = 7) in vec4 inParticleSpriteData;
layout (location = 8) in vec4 inParticleSpriteUV;
layout(std140) uniform bufCamera3d {
	mat4 matProjection;
	mat4 matView;
};
layout(std140) uniform bufModel {
	mat4 matModel;
};
out vec4 fragColor;
out vec2 fragUV;
void main() {
	vec2 uv_ = vec2(
		inParticleSpriteUV.x + (inParticleSpriteUV.z - inParticleSpriteUV.x) * inUV.x,
		inParticleSpriteUV.y + (inParticleSpriteUV.w - inParticleSpriteUV.y) * inUV.y
	);
	fragUV = uv_;
	vec4 positionViewSpace = matView * vec4(inParticlePosition.xyz, 1.0);
	vec2 vertex = (inPosition.xy * inParticleSpriteData.xy - inParticleSpriteData.zw) * inParticlePosition.w;
	positionViewSpace.xy += vertex;
	fragColor = inParticleColorRGBA;
	gl_Position = matProjection * positionViewSpace;
}

#fragment
#version 450
uniform sampler2D tex;
in vec2 fragUV;
in vec4 fragColor;
out vec4 outAlbedo;
void main(){
	vec4 s = texture(tex, fragUV.xy);
	outAlbedo = s * fragColor;
}
