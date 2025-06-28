layout(std140) uniform ubCommon {
	mat4 matProjection;
	mat4 matView;
	vec3 cameraPosition;
	float time; 
	vec2 viewportSize;
	float zNear;
	float zFar;
	vec4 vp_rect_ratio;
	
	float gamma;
	float exposure;
};