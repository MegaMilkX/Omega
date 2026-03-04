#include "shared_resources.hpp"


gpuMesh* gpuSharedResources::getUnitCube() {
    return nullptr;
}

gpuMesh* gpuSharedResources::getInvertedUnitCube() {
    return nullptr;
}

gpuMesh* gpuSharedResources::getDecalUnitCube() {
    if (!mesh_decal_cube) {
        const gfxm::vec3 boxSize = gfxm::vec3(1.0f, 1.0f, 1.0f);
        const float width = boxSize.x;
        const float height = boxSize.y;
        const float depth = boxSize.z;
        const float w = width * .5f;
        const float h = height * .5f;
        const float d = depth * .5f;
        float vertices[] = {
            -w, -h,  d,     w,  h,  d,      w, -h,  d,
             w,  h,  d,    -w, -h,  d,     -w,  h,  d,

             w, -h,  d,     w,  h, -d,      w, -h, -d,
             w,  h, -d,     w, -h,  d,      w,  h,  d,

             w, -h, -d,    -w,  h, -d,     -w, -h, -d,
            -w,  h, -d,     w, -h, -d,      w,  h, -d,

            -w, -h, -d,    -w,  h,  d,     -w, -h,  d,
            -w,  h,  d,    -w, -h, -d,     -w,  h, -d,

            -w,  h,  d,     w,  h, -d,      w,  h,  d,
             w,  h, -d,    -w,  h,  d,     -w,  h, -d,

            -w, -h, -d,     w, -h,  d,      w, -h, -d,
             w, -h,  d,    -w, -h, -d,     -w, -h,  d
        };

        Mesh3d m3d;
        m3d.setAttribArray(VFMT::Position_GUID, vertices, sizeof(vertices));
        
        mesh_decal_cube.reset(new gpuMesh);
        mesh_decal_cube->setData(&m3d);
        mesh_decal_cube->setDrawMode(MESH_DRAW_MODE::MESH_DRAW_TRIANGLES);
    }
    return mesh_decal_cube.get();
}

gpuShaderProgram* gpuSharedResources::getPresentProgram(RT_OUTPUT type) {
    switch (type) {
    case RT_OUTPUT_RGB: {
        if (!prog_present_rgb) {
            const char* vs = R"(
            #version 450 
            in vec3 inPosition;
            out vec2 uv_frag;
        
            void main(){
                uv_frag = vec2((inPosition.x + 1.0) / 2.0, (inPosition.y + 1.0) / 2.0);
                vec4 pos = vec4(inPosition, 1);
                gl_Position = pos;
            })";
            const char* fs = R"(
            #version 450
            in vec2 uv_frag;
            out vec4 outAlbedo;
            uniform sampler2D texAlbedo;
            uniform sampler2D Depth;
            void main(){
                vec4 pix = texture(texAlbedo, uv_frag);
                float a = pix.a;
                outAlbedo = vec4(pix.rgb, 1);
	            gl_FragDepth = texture(Depth, uv_frag).x;
            })";
            prog_present_rgb.reset(new gpuShaderProgram(vs, fs));
        }
        return prog_present_rgb.get();
    }
    case RT_OUTPUT_RRR: {
        if (!prog_present_rrr) {
            const char* vs = R"(
            #version 450 
            in vec3 inPosition;
            out vec2 uv_frag;
        
            void main(){
                uv_frag = vec2((inPosition.x + 1.0) / 2.0, (inPosition.y + 1.0) / 2.0);
                vec4 pos = vec4(inPosition, 1);
                gl_Position = pos;
            })";
            const char* fs = R"(
            #version 450
            in vec2 uv_frag;
            out vec4 outAlbedo;
            uniform sampler2D texAlbedo;
            uniform sampler2D Depth;
            void main(){
                vec4 pix = texture(texAlbedo, uv_frag);
                float a = pix.a;
                outAlbedo = vec4(pix.rrr, 1);
	            gl_FragDepth = texture(Depth, uv_frag).x;
            })";
            prog_present_rrr.reset(new gpuShaderProgram(vs, fs));
        }
        return prog_present_rrr.get();
    }
    case RT_OUTPUT_GGG: {
        if (!prog_present_ggg) {
            const char* vs = R"(
            #version 450 
            in vec3 inPosition;
            out vec2 uv_frag;
        
            void main(){
                uv_frag = vec2((inPosition.x + 1.0) / 2.0, (inPosition.y + 1.0) / 2.0);
                vec4 pos = vec4(inPosition, 1);
                gl_Position = pos;
            })";
            const char* fs = R"(
            #version 450
            in vec2 uv_frag;
            out vec4 outAlbedo;
            uniform sampler2D texAlbedo;
            uniform sampler2D Depth;
            void main(){
                vec4 pix = texture(texAlbedo, uv_frag);
                float a = pix.a;
                outAlbedo = vec4(pix.ggg, 1);
	            gl_FragDepth = texture(Depth, uv_frag).x;
            })";
            prog_present_ggg.reset(new gpuShaderProgram(vs, fs));
        }
        return prog_present_ggg.get();
    }
    case RT_OUTPUT_BBB: {
        if (!prog_present_bbb) {
            const char* vs = R"(
            #version 450 
            in vec3 inPosition;
            out vec2 uv_frag;
        
            void main(){
                uv_frag = vec2((inPosition.x + 1.0) / 2.0, (inPosition.y + 1.0) / 2.0);
                vec4 pos = vec4(inPosition, 1);
                gl_Position = pos;
            })";
            const char* fs = R"(
            #version 450
            in vec2 uv_frag;
            out vec4 outAlbedo;
            uniform sampler2D texAlbedo;
            uniform sampler2D Depth;
            void main(){
                vec4 pix = texture(texAlbedo, uv_frag);
                float a = pix.a;
                outAlbedo = vec4(pix.bbb, 1);
	            gl_FragDepth = texture(Depth, uv_frag).x;
            })";
            prog_present_bbb.reset(new gpuShaderProgram(vs, fs));
        }
        return prog_present_bbb.get();
    }
    case RT_OUTPUT_AAA: {
        if (!prog_present_aaa) {
            const char* vs = R"(
            #version 450 
            in vec3 inPosition;
            out vec2 uv_frag;
        
            void main(){
                uv_frag = vec2((inPosition.x + 1.0) / 2.0, (inPosition.y + 1.0) / 2.0);
                vec4 pos = vec4(inPosition, 1);
                gl_Position = pos;
            })";
            const char* fs = R"(
            #version 450
            in vec2 uv_frag;
            out vec4 outAlbedo;
            uniform sampler2D texAlbedo;
            uniform sampler2D Depth;
            void main(){
                vec4 pix = texture(texAlbedo, uv_frag);
                float a = pix.a;
                outAlbedo = vec4(pix.aaa, 1);
	            gl_FragDepth = texture(Depth, uv_frag).x;
            })";
            prog_present_aaa.reset(new gpuShaderProgram(vs, fs));
        }
        return prog_present_aaa.get();
    }
    case RT_OUTPUT_DEPTH: {
        if (!prog_present_depth) {
            const char* vs = R"(
            #version 450 
            in vec3 inPosition;
            out vec2 uv_frag;
        
            void main(){
                uv_frag = vec2((inPosition.x + 1.0) / 2.0, (inPosition.y + 1.0) / 2.0);
                vec4 pos = vec4(inPosition, 1);
                gl_Position = pos;
            })";
            const char* fs = R"(
            #version 450
            in vec2 uv_frag;
            out vec4 outAlbedo;
            uniform sampler2D texAlbedo;
            uniform sampler2D Depth;

            float LinearizeDepth(float depth, float near, float far)
            {
                float z = depth * 2.0 - 1.0; // Back to NDC
                return (2.0 * near * far) / (far + near - z * (far - near));
            }

            void main(){
                vec4 pix = texture(texAlbedo, uv_frag);
                float a = pix.a;
                float zNear = .01;
                float zFar = 1000.;
                float dist = LinearizeDepth(pix.r, zNear, zFar);
                float depth = dist / (zFar - zNear);
                depth = sqrt(depth);
                outAlbedo = vec4(vec3(1) - vec3(depth), 1);
	            gl_FragDepth = texture(Depth, uv_frag).x;
            })";
            // TODO: zNear and zFar are hardcoded here, needs update
            prog_present_depth.reset(new gpuShaderProgram(vs, fs));
        }
        return prog_present_depth.get();
    }
    default:
        assert(false);
        return nullptr;
    }
}

gpuShaderProgram* gpuSharedResources::getCubemapSampleProgram() {    
    if (!prog_sample_cubemap) {
        const char* vs = R"(
            #version 450 
            in vec3 inPosition;
            out vec3 local_pos;
            uniform mat4 matView;

            mat4 makeProjectionMatrix(float fov, float aspect, float znear, float zfar) {
                float tanHalfFovy = tan(fov / 2.0f);

                mat4 m = mat4(0.0);
                m[0][0] = 1.0f / (aspect * tanHalfFovy);
                m[1][1] = 1.0f / (tanHalfFovy);
                m[2][2] = -(zfar + znear) / (zfar - znear);
                m[2][3] = -1.0;
                m[3][2] = -(2.0 * zfar * znear) / (zfar - znear);
                return m;
            }

            const float PI = 3.14159265359;
        
            void main(){
                const mat4 matProjection = makeProjectionMatrix(
                    PI * .5, 1.0, 0.1, 10.0
                );
            
                local_pos = inPosition;
                gl_Position = matProjection * matView * vec4(inPosition, 1.0);
            })";
        const char* fs = R"(
            #version 450
            out vec4 outAlbedo;
            in vec3 local_pos;
            uniform sampler2D texAlbedo;
            const vec2 invAtan = vec2(0.1591, 0.3183);
            vec2 sampleSphericalMap(vec3 v) {
                vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
                uv *= invAtan;
                uv += 0.5;
                return uv;
            }
            void main() {
                vec2 uv = sampleSphericalMap(normalize(local_pos));
                vec3 color = texture(texAlbedo, uv).rgb;
                outAlbedo = vec4(color, 1.0);
            })";
        prog_sample_cubemap.reset(new gpuShaderProgram(vs, fs));
    }
    return prog_sample_cubemap.get();
}

