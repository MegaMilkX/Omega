#vertex
#version 450 

in vec3 inPosition;
in vec3 inColorRGB;
in vec2 inUV;
in vec3 inNormal;
out vec3 pos_frag;
out vec3 col_frag;
out vec2 uv_frag;
out vec3 normal_frag;

layout(std140) uniform bufCamera3d {
	mat4 matProjection;
	mat4 matView;
};
layout(std140) uniform bufModel {
	mat4 matModel;
};

void main(){
	uv_frag = inUV;
	normal_frag = (matModel * vec4(inNormal, 0)).xyz;
	pos_frag = inPosition;
	col_frag = inColorRGB;
	vec4 pos = matProjection * matView * matModel * vec4(inPosition, 1);
	gl_Position = pos;
}

#fragment
#version 450
in vec3 pos_frag;
in vec3 col_frag;
in vec2 uv_frag;
in vec3 normal_frag;
out vec4 outAlbedo;
uniform sampler2D texAlbedo;

float calcLightness(vec3 frag_pos, vec3 normal, vec3 light_pos) {
	vec3 light_dir = normalize(light_pos - frag_pos);
	float lightness = max(dot(normal, light_dir), .0);
	return lightness;
}

void main(){
	vec4 pix = texture(texAlbedo, uv_frag);
	float a = pix.a;
	float lightness = calcLightness(pos_frag, normal_frag, vec3(0, 2, -10))
		+ calcLightness(pos_frag, normal_frag, vec3(0, 2, 10));
	vec3 color = col_frag * (pix.rgb) * lightness;
	outAlbedo = vec4(color, a);
}