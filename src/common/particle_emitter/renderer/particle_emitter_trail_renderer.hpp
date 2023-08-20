#pragma once

#include <random>

#include "particle_emitter_renderer.hpp"

#include "resource/resource.hpp"
#include "gpu/gpu_texture_2d.hpp"
#include "gpu/gpu_shader_program.hpp"
#include "gpu/gpu_material.hpp"
#include "gpu/gpu_renderable.hpp"
#include "gpu/gpu.hpp"


class ParticleTrailRendererInstance;
class ParticleTrailRendererMaster : public IParticleRendererMasterT<ParticleTrailRendererInstance> {
    TYPE_ENABLE();
public:
    void init() override {

    }
};

class ParticleTrailRendererInstance : public IParticleRendererInstanceT<ParticleTrailRendererMaster> {
    HSHARED<scnMeshObject> scn_mesh;

    RHSHARED<gpuTexture2d> texture;
    RHSHARED<gpuShaderProgram> prog;
    gpuBuffer vertexBuffer;
    gpuBuffer uvBuffer;
    gpuMeshDesc meshDesc;
    gpuMaterial* mat = 0;
    //std::unique_ptr<gpuRenderable> renderable;

    struct TrailNode {
        gfxm::vec3  position;
        float       scale;
        gfxm::vec4  color;
        gfxm::vec3  normal;
        float       distance_traveled;
    };
    static_assert(sizeof(TrailNode) == 48, "");
    std::vector<TrailNode> nodes;
    const int MAX_TRAIL_SEGMENTS = 50;
    struct TrailData {
        TrailNode prev_head_state;
        TrailNode tail_state;
        float age;
        float distance_traveled_step;
        float length_distance;
    };
    std::vector<TrailData> trails;
    int active_trail_count = 0;

    struct TrailInstanceData {
        float length_distance;
        float reserved_0;
        float reserved_1;
        float reserved_2;
    };
    std::vector<TrailInstanceData> instance_data;

    std::vector<gfxm::vec3> vertices;
    HSHARED<gpuBufferTexture1d> lut_;
    gpuBuffer trailInstanceBuffer;
    gpuInstancingDesc instDesc;

    std::random_device m_seed;
    std::mt19937_64 mt_gen;
    std::uniform_real_distribution<float> u01;

    inline void addTrail(ptclParticleData* pd, int particle_id) {
        if (active_trail_count == trails.size()) {
            return;
        }
        int trail_i = active_trail_count;
        trails[trail_i].age = .0f;
        TrailNode n = TrailNode{
            gfxm::vec3(pd->particlePositions[particle_id]),
            .0f,
            pd->particleColors[particle_id],
            gfxm::normalize(pd->particleStates[particle_id].velocity),
            .0f
        };
        trails[trail_i].prev_head_state = n;
        trails[trail_i].tail_state = n;
        trails[trail_i].distance_traveled_step = .0f;
        trails[trail_i].length_distance = .0f;
        for (int i = 0; i < MAX_TRAIL_SEGMENTS; ++i) {
            nodes[trail_i * MAX_TRAIL_SEGMENTS + i] = n;
        }
        instance_data[trail_i].length_distance = .0f;
        active_trail_count++;
    }
    inline void removeTrail(int i) {
        if (active_trail_count == 0) {
            assert(false);
            return;
        }
        if (active_trail_count == 1) {
            active_trail_count = 0;
            return;
        }
        int last_trail_id = active_trail_count - 1;
        trails[i] = trails[last_trail_id];
        memcpy(&nodes[i * MAX_TRAIL_SEGMENTS], &nodes[last_trail_id * MAX_TRAIL_SEGMENTS], MAX_TRAIL_SEGMENTS * sizeof(nodes[0]));
        instance_data[i] = instance_data[last_trail_id];
        active_trail_count--;
    }
public:
    ParticleTrailRendererInstance()
        : mt_gen(m_seed()), u01(-1.0f, 1.f) {}

    void onParticlesSpawned(ptclParticleData* pd, int begin, int end) override {
        for (int i = begin; i < end; ++i) {
            addTrail(pd, i);
        }
    }
    void onParticleDespawn(ptclParticleData* pd, int i) override {
        removeTrail(i);
    }

    void init(ptclParticleData* pd) {
        scn_mesh.reset_acquire();

        trails.resize(pd->maxParticles);
        nodes.resize(pd->maxParticles * MAX_TRAIL_SEGMENTS);
        /*for (int i = 0; i < pd->maxParticles * MAX_TRAIL_SEGMENTS; ++i) {
            float x = cosf(i / 200.0f * gfxm::pi * 2.0f) * 10.0f;
            float z = sinf(i / 200.0f * gfxm::pi * 2.0f) * 10.0f;
            float y = i / 200.0f * 5.0f;
            points[i] = gfxm::vec4(x, y, z, 0.0f);
        }*/

        texture = resGet<gpuTexture2d>("trail.jpg");

        lut_.reset_acquire();
        lut_->setData((void*)nodes.data(), nodes.size() * sizeof(nodes[0]));

        vertices.resize(2 * MAX_TRAIL_SEGMENTS);
        for (int i = 0; i < MAX_TRAIL_SEGMENTS; ++i) {
            vertices[i * 2] = gfxm::vec3(i, 1.f, .0f);
            vertices[i * 2 + 1] = gfxm::vec3(i, -1.f, .0f);
        }
        vertexBuffer.setArrayData(vertices.data(), vertices.size() * sizeof(vertices[0]));

        prog = resGet<gpuShaderProgram>("shaders/trail_instanced.glsl");
        meshDesc.setAttribArray(VFMT::Position_GUID, &vertexBuffer);
        meshDesc.setVertexCount(vertices.size());
        meshDesc.setDrawMode(MESH_DRAW_TRIANGLE_STRIP);

        mat = gpuGetPipeline()->createMaterial();
        mat->addBufferSampler("lutPos", lut_);
        mat->addSampler("tex", texture);
        auto tech = mat->addTechnique("Normal");
        auto pass = tech->addPass();
        pass->setShader(prog);
        pass->blend_mode = GPU_BLEND_MODE::ADD;
        pass->depth_write = 0;
        pass->cull_faces = 0;
        mat->compile();

        instance_data.resize(pd->maxParticles);
        trailInstanceBuffer.setArrayData(instance_data.data(), instance_data.size() * sizeof(instance_data[0]));
        instDesc.setInstanceAttribArray(VFMT::TrailInstanceData0_GUID, &trailInstanceBuffer);
        instDesc.setInstanceCount(pd->maxParticles);

        scn_mesh->setMeshDesc(&meshDesc);
        scn_mesh->setMaterial(mat);
        scn_mesh->getRenderable(0)->setInstancingDesc(&pd->instDesc);
        //renderable.reset(new gpuRenderable(mat, &meshDesc, &instDesc));
    }
    void update(ptclParticleData* pd, float dt) override {
        const float max_segment_distance = .1f;
        for (int i = 0; i < active_trail_count; ++i) {
            auto& t = trails[i];

            gfxm::vec3 pt_pos = pd->particlePositions[i];
            gfxm::vec3 pt_norm = gfxm::normalize(pd->particleStates[i].velocity);
            float distance_traveled = gfxm::length(gfxm::vec3(pt_pos) - t.prev_head_state.position);
            t.distance_traveled_step += distance_traveled;

            instance_data[i].length_distance += distance_traveled;
            
            float scl = 1.0f;
            int head_id = i * MAX_TRAIL_SEGMENTS;
            int tail_id = i * MAX_TRAIL_SEGMENTS + MAX_TRAIL_SEGMENTS - 1;
            nodes[head_id].position = pt_pos;
            nodes[head_id].scale = scl;
            nodes[head_id].color = pd->particleColors[i];
            nodes[head_id].normal = pt_norm;
            //nodes[head_id].uv_offset = .0f;
            nodes[head_id].distance_traveled = instance_data[i].length_distance;
            if (t.distance_traveled_step >= max_segment_distance) {
                t.distance_traveled_step = .0f;

                t.tail_state = nodes[tail_id];
                for (int j = MAX_TRAIL_SEGMENTS - 1; j > 0; --j) {
                    int cur_id = i * MAX_TRAIL_SEGMENTS + j;
                    int next_id = i * MAX_TRAIL_SEGMENTS + j - 1;
                    nodes[cur_id].position = nodes[next_id].position;
                    nodes[cur_id].scale = nodes[next_id].scale;
                    nodes[cur_id].color = nodes[next_id].color;
                    nodes[cur_id].normal = nodes[next_id].normal;
                    //nodes[cur_id].uv_offset = j / (float)10;
                    //nodes[cur_id].distance_traveled = nodes[next_id].distance_traveled;
                    nodes[cur_id].distance_traveled = nodes[next_id].distance_traveled;
                }
            }
            for (int j = 1; j < MAX_TRAIL_SEGMENTS; ++j) {
                int id = i * MAX_TRAIL_SEGMENTS + j;
                nodes[id].scale = gfxm::_max(.0f, nodes[id].scale - dt);
            }
            t.prev_head_state = nodes[i * MAX_TRAIL_SEGMENTS];
            /*
            nodes[tail_id].position = gfxm::lerp(
                t.tail_state.position, nodes[i * MAX_TRAIL_SEGMENTS + MAX_TRAIL_SEGMENTS - 2].position, t.distance_traveled_step / max_segment_distance
            );
            nodes[tail_id].scale = gfxm::lerp(
                t.tail_state.scale, nodes[i * MAX_TRAIL_SEGMENTS + MAX_TRAIL_SEGMENTS - 2].scale, t.distance_traveled_step / max_segment_distance
            );
            nodes[tail_id].color = gfxm::lerp(
                t.tail_state.color, nodes[i * MAX_TRAIL_SEGMENTS + MAX_TRAIL_SEGMENTS - 2].color, t.distance_traveled_step / max_segment_distance
            );
            nodes[tail_id].distance_traveled = gfxm::lerp(
                t.tail_state.distance_traveled, nodes[i * MAX_TRAIL_SEGMENTS + MAX_TRAIL_SEGMENTS - 2].distance_traveled, t.distance_traveled_step / max_segment_distance
            );*/
        }
        lut_->setData((void*)nodes.data(), active_trail_count * MAX_TRAIL_SEGMENTS * sizeof(nodes[0]));

        trailInstanceBuffer.setArrayData(instance_data.data(), active_trail_count * sizeof(instance_data[0]));

        instDesc.setInstanceCount(active_trail_count);


        for (int i = 0; i < trails.size(); ++i) {
            auto& t = trails[i];
            t.age += dt;
        }
    }
    void onSpawn(scnRenderScene* scn) override {
        scn->addRenderObject(scn_mesh.get());
    }
    void onDespawn(scnRenderScene* scn) override {
        scn->removeRenderObject(scn_mesh.get());
    }
};