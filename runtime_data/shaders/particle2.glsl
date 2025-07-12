#vertex
#version 450 
layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec2 inUV;
layout (location = 4) in vec4 inParticlePosition;
layout (location = 5) in vec4 inParticleScale;
layout (location = 6) in vec4 inParticleColorRGBA;
layout (location = 7) in vec4 inParticleSpriteData;
layout (location = 8) in vec4 inParticleSpriteUV;
layout (location = 9) in vec4 inParticleRotation;

#include "uniform_blocks/common.glsl"
#include "uniform_blocks/model.glsl"

out vec4 fragColor;
out vec2 fragUV;

vec3 rotateVertex(vec3 v, vec4 quat) {
	return v + 2.0 * cross(quat.xyz, cross(quat.xyz, v) + quat.w * v);
}

void main() {
	vec2 uv_ = vec2(
		inParticleSpriteUV.x + (inParticleSpriteUV.z - inParticleSpriteUV.x) * inUV.x,
		inParticleSpriteUV.y + (inParticleSpriteUV.w - inParticleSpriteUV.y) * inUV.y
	);
	//fragUV = uv_;
	fragUV = inUV;
	vec4 positionViewSpace = matView * vec4(inParticlePosition.xyz, 1.0);
	vec3 vertex = (inPosition.xyz) * inParticlePosition.w;
	vertex = vertex * inParticleScale.xyz;
	vertex = rotateVertex(vertex, inParticleRotation);
	
	positionViewSpace.xy += vertex.xy;
	fragColor = inParticleColorRGBA;
	gl_Position = matProjection * positionViewSpace;
}

#fragment
#version 450
uniform sampler2D tex;
uniform sampler2D Depth;
in vec2 fragUV;
in vec4 fragColor;
out vec4 outAlbedo;

#include "functions/tonemapping.glsl"
#include "uniform_blocks/common.glsl"

float LinearizeDepth(float depth, float near, float far)
{
    float z = depth * 2.0 - 1.0; // Back to NDC
    return (2.0 * near * far) / (far + near - z * (far - near));
}

void main(){	
	vec4 s = texture(tex, fragUV.xy);
	//s.xyz = inverseGammaCorrect(s.xyz, gamma);
	
	vec2 screen_uv = gl_FragCoord.xy / viewportSize.xy;
	float depth = texture(Depth, screen_uv.xy).r;
	if(depth < gl_FragCoord.z) {
		discard;
	}
	
	const float ALPHA_TRANSITION_DIST = .5;
	float dist = LinearizeDepth(depth, zNear, zFar);
	float dist_this = LinearizeDepth(gl_FragCoord.z, zNear, zFar);
	float alpha = clamp(dist - dist_this, 0.0, 1.0);
	alpha = clamp(alpha / ALPHA_TRANSITION_DIST, 0.0, 1.0);
	alpha = smoothstep(0.0, 1.0, alpha);
	alpha = alpha * s.a * fragColor.a;
	
	outAlbedo = vec4(s.xyz * fragColor.xyz, alpha);
}
