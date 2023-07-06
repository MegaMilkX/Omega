#vertex
#version 450 

in vec3 inPosition;
out vec3 frag_cube_vec;

uniform mat4 matProjection;
uniform mat4 matView;

void main(){
	frag_cube_vec = vec3(inPosition.x, inPosition.y, -inPosition.z);
	mat4 view = matView;
	view[3] = vec4(0,0,0,1);
	vec4 pos = matProjection * view * vec4(inPosition, 1.0);
	gl_Position = pos.xyww;
}

#fragment
#version 450

in vec3 frag_cube_vec;
uniform samplerCube cubeMap;
out vec4 outAlbedo;

void main(){
	vec4 color = texture(cubeMap, normalize(frag_cube_vec));
	
	outAlbedo = vec4(color.xyz, 1.0);
}
