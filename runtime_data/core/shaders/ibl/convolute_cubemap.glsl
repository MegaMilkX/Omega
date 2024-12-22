#vertex
#version 460

layout(location = 0) in vec3 inPosition;
out vec3 fragPosition;

uniform mat4 matProjection;
uniform mat4 matView;

void main() {
    fragPosition = inPosition;

    mat4 rotView = mat4(mat3(matView)); // Removing translation
    vec4 clipPos = matProjection * rotView * vec4(inPosition, 1.0);

    gl_Position = clipPos.xyww;
}


#fragment
#version 460

out vec4 outAlbedo;
in vec3 fragPosition;

uniform samplerCube cubemapEnvironment;

const float PI = 3.14159265359;

void main() {
    vec3 normal = normalize(fragPosition);
    vec3 irradiance = vec3(0.0);

    vec3 up = vec3(0.0, 1.0, 0.0);
    vec3 right = normalize(cross(up, normal));
    up = normalize(cross(normal, right));

    float sampleDelta = 0.005;
    float nrSamples = 0.0;
    for(float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta) {
        for(float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta) {
            vec3 tangentSample = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
            vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * normal;

            irradiance += texture(cubemapEnvironment, sampleVec).rgb * cos(theta) * sin(theta);
            nrSamples++;
        }
    }
    irradiance = PI * irradiance * (1.0 / float(nrSamples));

    outAlbedo = vec4(irradiance, 1.0);
}