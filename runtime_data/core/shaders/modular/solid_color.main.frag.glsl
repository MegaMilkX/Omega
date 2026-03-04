#fragment
#version 460
out vec4 outAlbedo;

uniform vec4 color;

void main(){
	// vec4(.1, .3, 1, 1)
	outAlbedo = color;
}