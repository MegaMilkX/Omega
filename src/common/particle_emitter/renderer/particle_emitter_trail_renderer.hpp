#pragma once

#include <random>

#include "particle_emitter_renderer.hpp"

#include "resource/resource.hpp"
#include "gpu/texture/texture2d.hpp"
#include "gpu/gpu_shader_program.hpp"
#include "gpu/gpu_material.hpp"
#include "gpu/gpu_renderable.hpp"
#include "gpu/gpu.hpp"
#include "gpu/trail_instancing_desc.hpp"


class ParticleTrailRendererInstance;
class ParticleTrailRendererMaster : public IParticleRendererMasterT<ParticleTrailRendererInstance> {
public:
    TYPE_ENABLE();
    void init() override {

    }
    void onInstanceCreated(ParticleTrailRendererInstance* inst) const override {
        // TODO:
    }
};

class ParticleTrailRendererInstance : public IParticleRendererInstanceT<ParticleTrailRendererMaster> {
    HSHARED<scnMeshObject> scn_mesh;

    ResourceRef<gpuTexture2d> texture;
    RHSHARED<gpuShaderProgram> prog;
    gpuBuffer vertexBuffer;
    gpuBuffer uvBuffer;
    gpuMeshDesc meshDesc;
    ResourceRef<gpuMaterial> mat;
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
    /*
    struct TrailInstanceData {
        float length_distance;
        float reserved_0;
        float reserved_1;
        float reserved_2;
    };
    std::vector<TrailInstanceData> instance_data;
    */
    std::vector<gfxm::vec3> vertices;
    //gpuBuffer trailInstanceBuffer;
    //gpuInstancingDesc instDesc;
    gpuTrailInstancingDesc instDesc;
    std::vector<gpuTrailInstancingDesc::Instance> trail_instances;


    std::random_device m_seed;
    std::mt19937_64 mt_gen;
    std::uniform_real_distribution<float> u01;

    void addTrail(ptclParticleData* pd, int particle_id) {
        const auto& inst = pd->instances[particle_id];

        if (active_trail_count == trails.size()) {
            return;
        }
        int trail_i = active_trail_count;
        trails[trail_i].age = .0f;
        TrailNode n = TrailNode{
            .position = gfxm::vec3(inst.pos),
            .scale = .0f,
            .color = inst.rgba,
            .normal = gfxm::normalize(pd->particleStates[particle_id].velocity),
            .distance_traveled = .0f
        };
        trails[trail_i].prev_head_state = n;
        trails[trail_i].tail_state = n;
        trails[trail_i].distance_traveled_step = .0f;
        trails[trail_i].length_distance = .0f;
        for (int i = 0; i < MAX_TRAIL_SEGMENTS; ++i) {
            nodes[trail_i * MAX_TRAIL_SEGMENTS + i] = n;
        }
        trail_instances[trail_i].length_distance = .0f;
        active_trail_count++;
    }
    void removeTrail(int i) {
        if (active_trail_count == 0) {
            assert(false);
            return;
        }
        if (active_trail_count == 1) {
            active_trail_count = 0;
            return;
        }
        int last_trail_id = active_trail_count - 1;
        copyTrail(last_trail_id, i);
        active_trail_count--;
    }
    void copyTrail(int source, int target) {
        trails[target] = trails[source];
        memcpy(
            &nodes[target * MAX_TRAIL_SEGMENTS],
            &nodes[source * MAX_TRAIL_SEGMENTS],
            MAX_TRAIL_SEGMENTS * sizeof(nodes[0])
        );
        trail_instances[target] = trail_instances[source];
    }
public:
    ParticleTrailRendererInstance()
        : mt_gen(m_seed()), u01(-1.0f, 1.f) {
        
    }

    void onParticlesSpawned(ptclParticleData* pd, int begin, int end) override {
        for (int i = begin; i < end; ++i) {
            addTrail(pd, i);
        }
    }
    void onParticleDespawn(ptclParticleData* pd, int i) override {
        removeTrail(i);
    }
    void onParticleMemMove(ptclParticleData* pd, int from, int to) override {
        
    }

    void init(ptclParticleData* pd) {
        scn_mesh.reset_acquire();

        const int MAX_TRAIL_COUNT = pd->maxParticles;

        trails.resize(MAX_TRAIL_COUNT);
        nodes.resize(MAX_TRAIL_COUNT * MAX_TRAIL_SEGMENTS);

        texture = loadResource<gpuTexture2d>("trail");

        instDesc.lut_.reset_acquire();
        instDesc.lut_->setData((void*)nodes.data(), nodes.size() * sizeof(nodes[0]));

        vertices.resize(2 * MAX_TRAIL_SEGMENTS);
        for (int i = 0; i < MAX_TRAIL_SEGMENTS; ++i) {
            vertices[i * 2] = gfxm::vec3(.0f, 1.f, .0f);
            vertices[i * 2 + 1] = gfxm::vec3(.0f, -1.f, .0f);
        }
        vertexBuffer.setArrayData(vertices.data(), vertices.size() * sizeof(vertices[0]));

        //prog = resGet<gpuShaderProgram>("shaders/trail_instanced.glsl");
        meshDesc.setAttribArray(VFMT::Position_GUID, &vertexBuffer);
        meshDesc.setVertexCount(vertices.size());
        meshDesc.setDrawMode(MESH_DRAW_TRIANGLE_STRIP);
        meshDesc.setType(GPU_MESH_DESC_TYPE::GENERIC);

        mat = loadResource<gpuMaterial>("materials/trail");

        trail_instances.resize(MAX_TRAIL_COUNT);
        //instance_data.resize(MAX_TRAIL_COUNT);
        //trailInstanceBuffer.setArrayData(instance_data.data(), instance_data.size() * sizeof(instance_data[0]));
        instDesc.setInstanceCount(0);

        scn_mesh->setMeshDesc(&meshDesc);
        scn_mesh->setMaterial(mat.get());
        scn_mesh->getRenderable(0)->setInstancingDesc(&instDesc);
        //renderable.reset(new gpuRenderable(mat, &meshDesc, &instDesc));
    }
    void update(const ParticleEmitterParams* params, ptclParticleData* pd, float dt) override {
        const float max_segment_distance = .2f;
        for (int i = 0; i < active_trail_count; ++i) {
            auto& t = trails[i];

            const auto& inst = pd->instances[i];

            gfxm::vec3 pt_pos = inst.pos;
            gfxm::vec3 pt_norm = gfxm::normalize(pd->particleStates[i].velocity);
            float distance_traveled_frame = gfxm::length(gfxm::vec3(pt_pos) - t.prev_head_state.position);
            float distance_traveled_step_prev = t.distance_traveled_step;
            t.distance_traveled_step += distance_traveled_frame;

            trail_instances[i].length_distance += distance_traveled_frame;
            
            float scl = inst.pos.w;
            int head_id = i * MAX_TRAIL_SEGMENTS;
            int tail_id = i * MAX_TRAIL_SEGMENTS + MAX_TRAIL_SEGMENTS - 1;

            nodes[head_id].position = pt_pos;
            nodes[head_id].scale = scl;
            nodes[head_id].color = inst.rgba;
            nodes[head_id].normal = pt_norm;
            //nodes[head_id].uv_offset = .0f;
            nodes[head_id].distance_traveled = trail_instances[i].length_distance;

            if (t.distance_traveled_step >= max_segment_distance) {
                t.distance_traveled_step = t.distance_traveled_step - max_segment_distance;

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
            t.prev_head_state = nodes[i * MAX_TRAIL_SEGMENTS];

            for (int j = MAX_TRAIL_SEGMENTS - 1; j > 0; --j) {
                int cur_node_idx = i * MAX_TRAIL_SEGMENTS + j;
                int next_node_idx = i * MAX_TRAIL_SEGMENTS + j - 1;
                auto& cur_node = nodes[cur_node_idx];
                auto& next_node = nodes[next_node_idx];

                cur_node.color = params->rgba_curve.at(inst.scale.w / params->max_lifetime);
            }
        }
        instDesc.lut_->setData((void*)nodes.data(), active_trail_count * MAX_TRAIL_SEGMENTS * sizeof(nodes[0]));

        instDesc.setArray(trail_instances.data(), active_trail_count);

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

