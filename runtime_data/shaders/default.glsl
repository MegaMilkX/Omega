#vertex
#version 450 

in vec3 inPosition;
in vec3 inColorRGB;
in vec2 inUV;
in vec3 inNormal;
in vec3 inTangent;
in vec3 inBitangent;
/*
out vec3 pos_frag;
out vec3 col_frag;
out vec2 uv_frag;
out vec3 normal_frag;
out mat3 TBN_frag;
*/

out VERTEX_DATA {
	vec3 pos;
	vec3 col;
	vec2 uv;
	vec3 normal;
	mat3 TBN;
} out_data;


#include "uniform_blocks/common.glsl"
#include "uniform_blocks/model.glsl"

void main(){
	/*
	vec3 T = normalize(vec3(matModel * vec4(inTangent, 0.0)));
	vec3 B = normalize(vec3(matModel * vec4(inBitangent, 0.0)));
	vec3 N = normalize(vec3(matModel * vec4(inNormal, 0.0)));
	TBN_frag = mat3(T, B, N);
	
	uv_frag = inUV;
	normal_frag = normalize((matModel * vec4(inNormal, 0)).xyz);
	pos_frag = (matModel * vec4(inPosition, 1)).xyz;
	col_frag = inColorRGB;
	*/
	vec3 T = normalize(vec3(matModel * vec4(inTangent, 0.0)));
	vec3 B = normalize(vec3(matModel * vec4(inBitangent, 0.0)));
	vec3 N = normalize(vec3(matModel * vec4(inNormal, 0.0)));
	out_data.TBN = mat3(T, B, N);
	
	out_data.uv = inUV;
	out_data.normal = normalize((matModel * vec4(inNormal, 0)).xyz);
	out_data.pos = (matModel * vec4(inPosition, 1)).xyz;
	out_data.col = inColorRGB;
	
	vec4 pos = matProjection * matView * matModel * vec4(inPosition, 1);
	gl_Position = pos;
}

#skip geometry
#version 450
layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

in VERTEX_DATA {
	vec3 pos;
	vec3 col;
	vec2 uv;
	vec3 normal;
	mat3 TBN;
} in_data[];

out VERTEX_DATA {
	vec3 pos;
	vec3 col;
	vec2 uv;
	vec3 normal;
	mat3 TBN;
} out_data;

#include "uniform_blocks/common.glsl"

void main() {
	vec4 C
		= gl_in[0].gl_Position
		+ gl_in[1].gl_Position
		+ gl_in[2].gl_Position;
	C /= 3.0;
	C = inverse(matView) * inverse(matProjection) * C;
	vec3 N
		= in_data[0].normal
		+ in_data[1].normal
		+ in_data[2].normal;
	N /= 3.0;
	N = (matProjection * matView * vec4(N, 0)).xyz;
	
	float disp = (sin(time * 2.0 + C.y * 2.5) + 1.0) * .5;
	float max_disp = .15;
	
    gl_Position = gl_in[0].gl_Position + vec4(N * disp * max_disp, 0); 
	out_data.pos = in_data[0].pos;
	out_data.col = in_data[0].col;
	out_data.uv = in_data[0].uv;
	out_data.normal = in_data[0].normal;
	out_data.TBN = in_data[0].TBN;
    EmitVertex();

    gl_Position = gl_in[1].gl_Position + vec4(N * disp * max_disp, 0);
	out_data.pos = in_data[1].pos;
	out_data.col = in_data[1].col;
	out_data.uv = in_data[1].uv;
	out_data.normal = in_data[1].normal;
	out_data.TBN = in_data[1].TBN;
    EmitVertex();
	
    gl_Position = gl_in[2].gl_Position + vec4(N * disp * max_disp, 0);
	out_data.pos = in_data[2].pos;
	out_data.col = in_data[2].col;
	out_data.uv = in_data[2].uv;
	out_data.normal = in_data[2].normal;
	out_data.TBN = in_data[2].TBN;
    EmitVertex();
    
    EndPrimitive();
}

#fragment
#version 450

void frag(
	out vec4 outAlbedo,
	out vec4 outPosition,
	out vec4 outNormal,
	out vec4 outMetalness,
	out vec4 outRoughness,
	out vec4 outEmission,
	out vec4 outAmbientOcclusion
) {
	outAlbedo = vec4(1, 0, 0, 1);
	outPosition = vec4(1, 0, 0, 1);
	outNormal = vec4(1, 0, 0, 1);
	outMetalness = vec4(1, 0, 0, 1);
	outRoughness = vec4(1, 0, 0, 1);
	outEmission = vec4(1, 0, 0, 1);
	outAmbientOcclusion = vec4(1, 0, 0, 1);
}

#fragment
#version 450

in VERTEX_DATA {
	vec3 pos;
	vec3 col;
	vec2 uv;
	vec3 normal;
	mat3 TBN;
} in_data;

out vec4 outAlbedo;
out vec4 outPosition;
out vec4 outNormal;
out vec4 outMetalness;
out vec4 outRoughness;
out vec4 outEmission;
out vec4 outAmbientOcclusion;

uniform sampler2D texAlbedo;
uniform sampler2D texNormal;
uniform sampler2D texRoughness;
uniform sampler2D texMetallic;
uniform sampler2D texEmission;
uniform sampler2D texAmbientOcclusion;

#include "uniform_blocks/common.glsl"
#include "functions/tonemapping.glsl"

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
	vec3 N = normalize(in_data.normal);
	if(!gl_FrontFacing) {
		N *= -1;
	}
	//N = N * 2.0 - 1.0;
	//N = normalize(TBN_frag * N);
	vec3 normal = texture(texNormal, in_data.uv).xyz;
	normal = normal * 2.0 - 1.0;
	mat3 tbn = in_data.TBN;
	tbn[0] = normalize(tbn[0]);
	tbn[1] = normalize(tbn[1]);
	tbn[2] = normalize(tbn[2]);
	normal = normalize(tbn * normal);
	if(!gl_FrontFacing) {
		normal *= -1;
	}
	
	vec4 pix = texture(texAlbedo, in_data.uv);
	float roughness = texture(texRoughness, in_data.uv).x;
	float metallic = texture(texMetallic, in_data.uv).x;
	vec3 emission = texture(texEmission, in_data.uv).xyz;
	vec4 ao = texture(texAmbientOcclusion, in_data.uv);
	
	emission.xyz = pix.xyz * emission.xyz * 4.0;
	
	pix.xyz = inverseGammaCorrect(pix.xyz, gamma);
	emission.xyz = inverseGammaCorrect(emission.xyz, gamma);
	/*
	frag(
		outAlbedo,
		outPosition,
		outNormal,
		outMetalness,
		outRoughness,
		outEmission,
		outAmbientOcclusion
	);*/
	
	outAlbedo = vec4(pix.rgb * in_data.col.rgb, pix.a);
	outPosition = vec4(in_data.pos, 1);
	outNormal = vec4((normal + 1.0) / 2.0, 1);
	outMetalness = vec4(metallic, 0, 0, 1);
	outRoughness = vec4(roughness, 0, 0, 1);
	outEmission = vec4(emission, 1);
	outAmbientOcclusion = vec4(ao.xyz, 1);
}
