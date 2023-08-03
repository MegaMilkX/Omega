#vertex
#version 450 

in vec3 inPosition;
in vec2 inUV;
in vec3 inNormal;
out vec3 pos_frag;
out vec2 uv_frag;
out vec3 normal_frag;

layout(std140) uniform bufCamera3d {
	mat4 matProjection;
	mat4 matView;
	vec2 screenSize;
};
layout(std140) uniform bufModel {
	mat4 matModel;
};

void main(){
	uv_frag = inUV;
	normal_frag = normalize((matModel * vec4(inNormal, 0)).xyz);
	pos_frag = (matModel * vec4(inPosition, 1)).xyz;
	vec4 pos = matProjection * matView * matModel * vec4(inPosition, 1);
	gl_Position = pos;
}

#fragment
#version 450
in vec3 pos_frag;
in vec2 uv_frag;
in vec3 normal_frag;
out vec4 outAlbedo;
uniform sampler2D texAlbedo;

vec3 calcPointLightness(vec3 frag_pos, vec3 normal, vec3 light_pos, float radius, vec3 L_col) {
	vec3 light_vec = light_pos - frag_pos;
	float dist = length(light_vec);
	vec3 light_dir = normalize(light_vec);
	float lightness = max(dot(normal, light_dir), .0);
	float attenuation = clamp(1.0 - dist*dist/(radius*radius), 0.0, 1.0);
	attenuation *= attenuation;
	return L_col * lightness * attenuation;
}

vec3 calcDirLight(vec3 N, vec3 L_dir, vec3 L_col) {
	float L = max(dot(N, -L_dir), .0f);
	return L * L_col;
}

void main(){
	vec3 N = normalize(normal_frag);
	if(!gl_FrontFacing) {
		N *= -1;
	}
	vec4 pix = texture(texAlbedo, uv_frag);
	float a = pix.a;
	vec3 L = calcPointLightness(pos_frag, N, vec3(0, 2, -10), 10, vec3(1,1,1))
		+ calcPointLightness(pos_frag, N, vec3(0, 2, 10), 10, vec3(1,1,1))
		+ calcPointLightness(pos_frag, N, vec3(4, 3, 1), 10, vec3(0.2,0.5,1))
		+ calcPointLightness(pos_frag, N, vec3(-4, 3, 1), 10, vec3(1,0.5,0.2))
		+ calcDirLight(N, vec3(0, -1, 0), vec3(.3,.3,.3));
	vec3 color = (pix.rgb) * L;
	//outAlbedo = vec4(N, 1);
	outAlbedo = vec4(color, a);
}
