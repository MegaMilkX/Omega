#version 450

out vec4 outAlbedo;
out vec4 outPosition;
out vec4 outNormal;
out vec4 outMetalness;
out vec4 outRoughness;
out vec4 outEmission;
out vec4 outAmbientOcclusion;

void frag(
	out vec4 outAlbedo,
	out vec4 outPosition,
	out vec4 outNormal,
	out vec4 outMetalness,
	out vec4 outRoughness,
	out vec4 outEmission,
	out vec4 outAmbientOcclusion
);

void main(){
	frag(
		outAlbedo,
		outPosition,
		outNormal,
		outMetalness,
		outRoughness,
		outEmission,
		outAmbientOcclusion
	);
}
