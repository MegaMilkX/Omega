#vertex
#version 450 
layout (location = 0) in vec3 vertexPosition;
layout (location = 1) in vec2 UV;
uniform mat4 matView;
uniform mat4 matProjection;
uniform mat4 matModel;
out vec2 fragUV;
void main() {
	fragUV = UV;

	mat4 billboardViewModel = matView * matModel;
	billboardViewModel[0] = vec4(1,0,0,0);
	billboardViewModel[1] = vec4(0,1,0,0);
	billboardViewModel[2] = vec4(0,0,1,0);  
	
	gl_Position = matProjection * billboardViewModel * vec4(vertexPosition, 1.0);
}

#fragment
#version 450
uniform sampler2D tex;
in vec2 fragUV;
out vec4 outAlbedo;
void main(){
	vec4 s = texture(tex, fragUV.xy);
	outAlbedo = s;
}
