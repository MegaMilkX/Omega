#vertex
#version 450 
in vec3 inPosition;

layout(std140) uniform bufCamera3d {
	mat4 matProjection;
	mat4 matView;
};
layout(std140) uniform bufModel {
	mat4 matModel;
};

void main(){
	gl_Position = matProjection * matView * vec4(inPosition, 1.0);
}

#fragment
#version 450
out vec4 outAlbedo;

void main(){
	outAlbedo = vec4(1, 1, 1, 1);
}
