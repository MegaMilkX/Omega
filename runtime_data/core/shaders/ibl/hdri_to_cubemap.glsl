#vertex
#version 460

layout(location = 0) in vec3 inPosition;

out vec3 fragPosition;

uniform mat4 matProjection;
uniform mat4 matView;

void main() {
    fragPosition = inPosition;
    gl_Position = matProjection * matView * vec4(inPosition, 1.0);
}


#fragment
#version 460

out vec4 outAlbedo;
in vec3 fragPosition;

uniform sampler2D texHdri;

const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 SampleSphericalMap(vec3 v) {
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

void main() {
    vec2 uv = SampleSphericalMap(normalize(fragPosition));
    vec3 color = texture(texHdri, uv).xyz;

    outAlbedo = vec4(color, 1.0);
}
