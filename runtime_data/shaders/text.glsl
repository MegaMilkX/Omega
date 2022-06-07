#vertex
#version 450 
in vec3 inPosition;
in vec2 inUV;
in float inTextUVLookup;
in vec3 inColorRGB;
out vec2 uv_frag;
out vec4 col_frag;

uniform sampler2D texTextUVLookupTable;

layout(std140) uniform bufCamera3d {
	mat4 matProjection;
	mat4 matView;
};
layout(std140) uniform bufModel {
	mat4 matModel;
};
layout(std140) uniform bufText {
	int lookupTextureWidth;
};

void main(){
	float lookup_x = (inTextUVLookup + 0.5) / float(lookupTextureWidth);
	vec2 uv_ = texture(texTextUVLookupTable, vec2(lookup_x, 0), 0).xy;
	uv_frag = uv_;
	col_frag = vec4(inColorRGB, 1);        

	vec3 pos3 = inPosition;
	pos3.x = round(pos3.x);
	pos3.y = round(pos3.y);

	vec3 scale = vec3(length(matModel[0]),
			length(matModel[1]),
			length(matModel[2]));

	mat4 billboardView = matView * matModel;
	billboardView[0] = vec4(scale.x,0,0,0);
	billboardView[1] = vec4(0,scale.y,0,0);
	billboardView[2] = vec4(0,0,scale.z,0);   

	vec4 pos = matProjection * billboardView * vec4(inPosition, 1);
	gl_Position = pos;
}

#fragment
#version 450
in vec2 uv_frag;
in vec4 col_frag;
out vec4 outAlbedo;

uniform sampler2D texAlbedo;
uniform vec4 color;
void main(){
	float c = texture(texAlbedo, uv_frag).x;
	outAlbedo = vec4(1, 1, 1, c);// * col_frag;// * color;
}
