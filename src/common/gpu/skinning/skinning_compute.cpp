#include "skinning_compute.hpp"

#include <vector>
#include "log/log.hpp"


static int s_init_count = 0;
GLuint s_prog_skinning = 0;

void initSkinning() {
    ++s_init_count;
    if (s_init_count > 1) {
        return;
    }

    
    GLuint cs = glCreateShader(GL_COMPUTE_SHADER);
    const char* cs_source = R"(
        #version 450
        layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

        struct VEC3 {
            float x, y, z;
        };
        layout(std430, binding = 0) buffer Position {
            VEC3 Positions[];
        };
        layout(std430, binding = 1) buffer Normal {
            VEC3 Normals[];
        };
        layout(std430, binding = 2) buffer Tangent {
            VEC3 Tangents[];
        };
        layout(std430, binding = 3) buffer Bitangent {
            VEC3 Bitangents[];
        };
        layout(std430, binding = 4) buffer BoneIndex4Buf {
            ivec4 BoneIndex4[];
        };
        layout(std430, binding = 5) buffer BoneWeight4Buf {
            vec4 BoneWeight4[];
        };
        layout(std430, binding = 6) buffer Pose {
            mat4 Poses[];
        };

        layout(std430, binding = 7) buffer out_Position {
            VEC3 out_Positions[];
        };
        layout(std430, binding = 8) buffer out_Normal {
            VEC3 out_Normals[];
        };
        layout(std430, binding = 9) buffer out_Tangent {
            VEC3 out_Tangents[];
        };
        layout(std430, binding = 10) buffer out_Bitangent {
            VEC3 out_Bitangents[];
        };
        
        void main() {
            uint gid = gl_GlobalInvocationID.x;

            vec3 P = vec3(Positions[gid].x, Positions[gid].y, Positions[gid].z);
            vec3 N = vec3(Normals[gid].x, Normals[gid].y, Normals[gid].z);
            vec3 T = vec3(Tangents[gid].x, Tangents[gid].y, Tangents[gid].z);
            vec3 B = vec3(Bitangents[gid].x, Bitangents[gid].y, Bitangents[gid].z);

            ivec4 bi = ivec4(
                int(BoneIndex4[gid].x), int(BoneIndex4[gid].y),
                int(BoneIndex4[gid].z), int(BoneIndex4[gid].w)
            );
            vec4 w = BoneWeight4[gid];

            mat4 m0 = Poses[bi.x];
            mat4 m1 = Poses[bi.y];
            mat4 m2 = Poses[bi.z];
            mat4 m3 = Poses[bi.w];

            vec3 NS = (
                m0 * vec4(N, 0.0) * w.x +
                m1 * vec4(N, 0.0) * w.y +
                m2 * vec4(N, 0.0) * w.z +
                m3 * vec4(N, 0.0) * w.w
            ).xyz;
            vec3 TS = (
                m0 * vec4(T, 0.0) * w.x +
                m1 * vec4(T, 0.0) * w.y +
                m2 * vec4(T, 0.0) * w.z +
                m3 * vec4(T, 0.0) * w.w
            ).xyz;
            vec3 BS = (
                m0 * vec4(B, 0.0) * w.x +
                m1 * vec4(B, 0.0) * w.y +
                m2 * vec4(B, 0.0) * w.z +
                m3 * vec4(B, 0.0) * w.w
            ).xyz;

            //NS = normalize(NS);
            //TS = normalize(TS);
            //BS = normalize(BS);

            vec4 PS = (
                m0 * vec4(P, 1.0) * w.x +
                m1 * vec4(P, 1.0) * w.y +
                m2 * vec4(P, 1.0) * w.z +
                m3 * vec4(P, 1.0) * w.w
            );

            out_Positions[gid].x = PS.x;
            out_Positions[gid].y = PS.y;
            out_Positions[gid].z = PS.z;
            out_Normals[gid].x = NS.x;
            out_Normals[gid].y = NS.y;
            out_Normals[gid].z = NS.z;
            out_Tangents[gid].x = TS.x;
            out_Tangents[gid].y = TS.y;
            out_Tangents[gid].z = TS.z;
            out_Bitangents[gid].x = BS.x;
            out_Bitangents[gid].y = BS.y;
            out_Bitangents[gid].z = BS.z;
        }
    )";

    glShaderSource(cs, 1, &cs_source, 0);
    glCompileShader(cs);
    GLint res = GL_FALSE;
    int infoLogLen;
    glGetShaderiv(cs, GL_COMPILE_STATUS, &res);
    glGetShaderiv(cs, GL_INFO_LOG_LENGTH, &infoLogLen);
    if (infoLogLen > 1) {
        std::vector<char> errMsg(infoLogLen + 1);
        glGetShaderInfoLog(cs, infoLogLen, NULL, &errMsg[0]);
        LOG_ERR("GLSL skin compute compile: " << &errMsg[0]);
    }

    s_prog_skinning = glCreateProgram();
    glAttachShader(s_prog_skinning, cs);
    glLinkProgram(s_prog_skinning);
    glGetProgramiv(s_prog_skinning, GL_LINK_STATUS, &res);
    glGetProgramiv(s_prog_skinning, GL_INFO_LOG_LENGTH, &infoLogLen);
    if (infoLogLen > 1) {
        std::vector<char> errMsg(infoLogLen + 1);
        glGetProgramInfoLog(s_prog_skinning, infoLogLen, NULL, &errMsg[0]);
        LOG_ERR("GLSL skin compute link: " << &errMsg[0]);
    }

    glDeleteShader(cs);
}
void cleanupSkinning() {
    --s_init_count;
    if (s_init_count > 0) {
        return;
    }

    glDeleteProgram(s_prog_skinning);
}

void updateSkinVertexDataComputeSingle(
    const gfxm::mat4* pose_transforms, int pose_count,
    gpuBuffer* bufVerticesSource, gpuBuffer* bufNormalsSource,
    gpuBuffer* bufTangentsSource, gpuBuffer* bufBitangentsSource,
    gpuBuffer* bufBoneIndices, gpuBuffer* bufBoneWeights,
    gpuBuffer* bufVerticesOut, gpuBuffer* bufNormalsOut,
    gpuBuffer* bufTangentsOut, gpuBuffer* bufBitangentsOut,
    int vertex_count
) {
    GLuint bufPose;
    glGenBuffers(1, &bufPose);

    glBindBuffer(GL_ARRAY_BUFFER, bufPose);
    glBufferData(GL_ARRAY_BUFFER, pose_count * sizeof(pose_transforms[0]), pose_transforms, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);

    // Bind buffers
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, bufVerticesSource->getId());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, bufNormalsSource->getId());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, bufTangentsSource->getId());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, bufBitangentsSource->getId());

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, bufBoneIndices->getId());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, bufBoneWeights->getId());

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, bufPose);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, bufVerticesOut->getId());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, bufNormalsOut->getId());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 9, bufTangentsOut->getId());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 10, bufBitangentsOut->getId());

    glDispatchCompute(vertex_count, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);

    // Cleanup
    glDeleteBuffers(1, &bufPose);
}

void updateSkinVertexDataCompute(SkinUpdateData* skin_instances, int count) {
    glBindVertexArray(0);

    glUseProgram(s_prog_skinning);
    for (int i = 0; i < count; ++i) {
        auto& data = skin_instances[i];
        if (!data.is_valid) {
            continue;
        }
        updateSkinVertexDataComputeSingle(
            data.pose_transforms, data.pose_count,
            data.bufVerticesSource, data.bufNormalsSource,
            data.bufTangentsSource, data.bufBitangentsSource,
            data.bufBoneIndices, data.bufBoneWeights,
            data.bufVerticesOut, data.bufNormalsOut,
            data.bufTangentsOut, data.bufBitangentsOut,
            data.vertex_count
        );
    }

    // Cleanup
    glUseProgram(0);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 9, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 10, 0);
}