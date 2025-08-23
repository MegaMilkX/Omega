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
out vec4 outLightness;

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

// Define your 6 cubemap face colors (RGB in linear space)
// Order: +X, -X, +Y, -Y, +Z, -Z
const vec3 faceColors[6] = vec3[6](
    vec3(1.0, 0.0, 0.0) * 10., // +X red
    vec3(1.0, 0.0, 0.0) * 10., // -X green
    vec3(0.0, 1.0, 0.0) * 10., // +Y blue
    vec3(0.0, 1.0, 0.0) * 10., // -Y yellow
    vec3(0.0, 0.0, 1.0) * 10., // +Z cyan
    vec3(0.0, 0.0, 1.0) * 10.  // -Z magenta
);
// Face directions
const vec3 faceDirs[6] = vec3[6](
    vec3( 1.0,  0.0,  0.0), // +X
    vec3(-1.0,  0.0,  0.0), // -X
    vec3( 0.0,  1.0,  0.0), // +Y
    vec3( 0.0, -1.0,  0.0), // -Y
    vec3( 0.0,  0.0,  1.0), // +Z
    vec3( 0.0,  0.0, -1.0)  // -Z
);
// Evaluate the 9 SH basis functions for a normalized dir
void shBasis9(vec3 dir, out float sh[9]) {
    float x = dir.x, y = dir.y, z = dir.z;
    sh[0] = 0.282095;               // Y00
    sh[1] = 0.488603 * y;           // Y1-1
    sh[2] = 0.488603 * z;           // Y10
    sh[3] = 0.488603 * x;           // Y11
    sh[4] = 1.092548 * x * y;       // Y2-2
    sh[5] = 1.092548 * y * z;       // Y2-1
    sh[6] = 0.315392 * (3.0*z*z - 1.0); // Y20
    sh[7] = 1.092548 * x * z;       // Y21
    sh[8] = 0.546274 * (x*x - y*y); // Y22
}
// Compute SH coefficients from our 6 colors (simple equal-weight sampling)
void computeSHCoefficients_O2(out vec3 coeffs[9]) {
    // Init to zero
    for (int i = 0; i < 9; ++i)
        coeffs[i] = vec3(0.0);

    // Accumulate from each face
    for (int f = 0; f < 6; ++f) {
        float sh[9];
        vec3 n = normalize(faceDirs[f]);
        shBasis9(n, sh);
        for (int b = 0; b < 9; ++b) {
            coeffs[b] += faceColors[f] * sh[b];
        }
    }

    // Normalize by number of samples
    for (int i = 0; i < 9; ++i)
        coeffs[i] /= 6.0;
}

// Each face's solid angle (steradians)
const float faceSolidAngle = 2.0 * 3.14159265359 / 3.0; // 2pi/3
// Precompute SH coefficients
void computeSHCoefficients2_O2(out vec3 coeffs[9]) {
    for (int i = 0; i < 9; ++i)
        coeffs[i] = vec3(0.0);

    for (int f = 0; f < 6; ++f) {
        float sh[9];
        vec3 n = normalize(faceDirs[f]);
        shBasis9(n, sh);
        for (int b = 0; b < 9; ++b) {
            coeffs[b] += faceColors[f] * sh[b] * faceSolidAngle;
        }
    }

    // Normalize so Y00 matches average radiance
    float normFactor = 1.0 / (4.0 * 3.14159265359); // 1 / 4pi
    for (int i = 0; i < 9; ++i)
        coeffs[i] *= normFactor;
}

// Sample the SH at a given normal
vec3 sampleSH2(vec3 normal, vec3 coeffs[9]) {
    float sh[9];
    shBasis9(normalize(normal), sh);
    vec3 result = vec3(0.0);
    for (int i = 0; i < 9; ++i)
        result += coeffs[i] * sh[i];
    return result;
}
/*
// Each cube face solid angle (steradians)
const float FACE_OMEGA = 2.0 * 3.14159265359 / 3.0; // 2pi/3
const float INV_4PI    = 1.0 / (4.0 * 3.14159265359);

// ==== Order-3 (l=0..3) real SH basis: 16 terms ====
// Constants (fully normalized, common in graphics)
const float c0  = 0.2820947918;  // l=0
const float c1  = 0.4886025119;  // l=1
const float c2  = 1.0925484306;  // l=2
const float c3  = 0.3153915653;  // l=2
const float c4  = 0.5462742153;  // l=2
const float c5  = 0.5900435899;  // l=3
const float c6  = 2.8906114426;  // l=3
const float c7  = 0.4570457995;  // l=3
const float c8  = 0.3731763326;  // l=3
const float c9  = 1.4453057213;  // l=3
const float c10 = 0.7463526653;  // l=3

// Layout (16): [Y00, Y1-1, Y10, Y11, Y2-2, Y2-1, Y20, Y21, Y22, Y3-3, Y3-2, Y3-1, Y30, Y31, Y32, Y33]
void shBasis16(vec3 dir, out float sh[16]) {
    vec3 d = normalize(dir);
    float x = d.x, y = d.y, z = d.z;

    // l = 0
    sh[0]  = c0;                 // Y00

    // l = 1
    sh[1]  = c1 * y;             // Y1-1
    sh[2]  = c1 * z;             // Y10
    sh[3]  = c1 * x;             // Y11

    // l = 2
    sh[4]  = c2 * x*y;                         // Y2-2
    sh[5]  = c2 * y*z;                         // Y2-1
    sh[6]  = c3 * (3.0*z*z - 1.0);             // Y20
    sh[7]  = c2 * x*z;                         // Y21
    sh[8]  = c4 * (x*x - y*y);                 // Y22

    // l = 3
    sh[9]  = c5  * y * (3.0*x*x - y*y);        // Y3-3
    sh[10] = c6  * x * y * z;                  // Y3-2
    sh[11] = c7  * y * (5.0*z*z - 1.0);        // Y3-1
    sh[12] = c8  * z * (5.0*z*z - 3.0);        // Y30
    sh[13] = c7  * x * (5.0*z*z - 1.0);        // Y31
    sh[14] = c9  * z * (x*x - y*y);            // Y32
    sh[15] = c10 * x * (x*x - 3.0*y*y);        // Y33
}
void computeSHCoefficients3(out vec3 coeffs[16]) {
    for (int i = 0; i < 16; ++i) coeffs[i] = vec3(0.0);

    for (int f = 0; f < 6; ++f) {
        float sh[16];
        shBasis16(faceDirs[f], sh);
        for (int b = 0; b < 16; ++b) {
            coeffs[b] += faceColors[f] * (sh[b] * FACE_OMEGA);
        }
    }

    // Normalize to match the SH definition over the sphere
    for (int i = 0; i < 16; ++i) coeffs[i] *= INV_4PI;
}
vec3 sampleSH3(vec3 dir, vec3 coeffs[16]) {
    float sh[16];
    shBasis16(dir, sh);
    vec3 c = vec3(0.0);
    for (int i = 0; i < 16; ++i) c += coeffs[i] * sh[i];
    return c;
}*/

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
	
	vec3 coeffs[9];
	computeSHCoefficients2_O2(coeffs);
	vec3 SH_color = sampleSH2(normal, coeffs);
	
	outAlbedo = vec4(pix.rgb * in_data.col.rgb, pix.a);
	outPosition = vec4(in_data.pos, 1);
	outNormal = vec4((normal + 1.0) / 2.0, 1);
	outMetalness = vec4(metallic, 0, 0, 1);
	outRoughness = vec4(roughness, 0, 0, 1);
	outEmission = vec4(emission.xyz * pix.rgb, 1);
	outAmbientOcclusion = vec4(ao.xyz, 1);
	outLightness = vec4(SH_color, 1);
}
