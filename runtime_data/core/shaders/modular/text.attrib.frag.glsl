#fragment
#version 460

uniform sampler2D texFontAtlas;

#include "types/vertex.glsl"
#include "types/fragment.glsl"
void evalAttribFragment(inout VERTEX vert, inout FRAGMENT frag) {
	float c = texture(texFontAtlas, vert.uv).x;
	frag.alpha = c;
}

