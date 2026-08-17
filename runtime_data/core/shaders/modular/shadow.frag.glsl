#fragment
#version 460

#include "interface_blocks/in_vertex.glsl"
#include "types/vertex.glsl"
#include "types/fragment.glsl"
void evalAttribFragment(inout VERTEX vert, inout FRAGMENT frag);
void evalFragment(in VERTEX vert, inout FRAGMENT frag);

void main() {
	VERTEX vert;
	FRAGMENT frag;
	{
		vert.pos = in_vertex.pos;
		vert.col = in_vertex.col;
		vert.uv = in_vertex.uv;
		
		frag.alpha = 1.0;
		
#ifdef ENABLE_FRAG_ATTRIB		
		evalAttribFragment(vert, frag);
#endif
#ifdef ENABLE_FRAG_EXTENSION
		evalFragment(vert, frag);
#endif
	}
	
	if(frag.alpha <= .5) {
		discard;
	}
}
