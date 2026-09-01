#fragment
#version 460

#include "uniform_blocks/common.glsl"
#include "functions/tonemapping.glsl"

out vec4 outFinal;

uniform sampler2D tex;

void main(){
	ivec2 texSize = textureSize(tex, 0);
	float texRatio = float(texSize.x) / float(texSize.y);
	float vpRatio = float(viewportSize.x) / float(viewportSize.y);
	float ratio = vpRatio / texRatio;
	float mul = 10.0;
	
	vec2 uv = vec2(gl_FragCoord.x * ratio, gl_FragCoord.y) * mul;
	uv /= vec2(viewportSize.x, viewportSize.y);
	
	vec3 color = texture(tex, uv).xyz;
	
	// Yes, this is an error shader, but I don't want my eyes to bleed
	color.xyz = tonemapFilmicUncharted2(color.xyz, exposure);
	color.xyz = gammaCorrect(color.xyz, gamma);
	outFinal = vec4(color, 1);
}