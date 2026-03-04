
vec3 normalSampleToWorld(vec3 s, mat3 tbn, bool is_front_facing) {
	vec3 normal = s * 2.0 - 1.0;/*
	tbn[0] = normalize(tbn[0]);
	tbn[1] = normalize(tbn[1]);
	tbn[2] = normalize(tbn[2]);*/
	normal = normalize(tbn * normal);
	if(!is_front_facing) {
		normal *= -1;
	}
	return normal;
}