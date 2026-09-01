struct VERTEX {
	vec3 pos;
	vec3 world_pos;
	vec3 col;
	float alpha;
	vec2 uv;
	vec2 uv2;
	vec3 normal;
	vec3 tangent;
	vec3 bitangent;
	mat3 TBN;
	mat3 invTBN;
};