#fragment
#version 460

#include "types/fragment.glsl"
void evalFragment(inout FRAGMENT frag);

void main() {
	FRAGMENT frag;
	{
		frag.alpha = 1.0;
#ifdef ENABLE_FRAG_EXTENSION
		evalFragment(frag);
#endif
	}
	
	if(frag.alpha <= .5) {
		discard;
	}
}
