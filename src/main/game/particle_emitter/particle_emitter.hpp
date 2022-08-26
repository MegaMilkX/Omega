#pragma once

#include "particle_data.hpp"
#include "shape/particle_emitter_shape.hpp"
#include "component/particle_emitter_component.hpp"
#include "renderer/particle_emitter_renderer.hpp"


#include "shape/particle_emitter_sphere_shape.hpp"
#include "renderer/particle_emitter_quad_renderer.hpp"
#include "renderer/particle_emitter_trail_renderer.hpp"


struct ptclEmitter {
    gfxm::mat4 world_transform = gfxm::mat4(1.0f);

    curve<float> pt_per_second_curve;
    float timeCache = .0f;

    ptclParticleData pd;

    // New stuff
    std::random_device m_seed;
    std::mt19937_64 mt_gen;
    std::uniform_real_distribution<float> u01;

    float duration = 25.5f/60.0f;
    float cursor = .0f;
    bool looping = true;
    bool is_alive = true;
    std::unique_ptr<ptclShape> shape;
    std::vector<std::unique_ptr<ptclComponent>> components;
    std::vector<std::unique_ptr<ptclRenderer>> renderers;

    curve<gfxm::vec3> initial_scale_curve;

    curve<gfxm::vec4> rgba_curve;
    curve<float> scale_curve;
    // ===

    ptclEmitter() : mt_gen(m_seed()), u01(.0f, 1.f) {
        pd.init(1000, 2.0f);
        pt_per_second_curve[.0f] = 20.0f;
        shape.reset(new ptclSphereShape);

        addRenderer<ptclQuadRenderer>();

        initial_scale_curve[.0f] = gfxm::vec3(1.f, 1.f, 1.f);
        initial_scale_curve[1.f] = gfxm::vec3(2.f, 2.f, 2.f);
    }

    void setWorldTransform(const gfxm::mat4& w) {
        world_transform = w;
    }

    void setParticlePerSecondCurve(const curve<float>& c) {
        pt_per_second_curve = c;
    }

    template<typename T>
    T* setShape() {
        T* ptr = new T();
        shape.reset(ptr);
        return ptr;
    }
    template<typename T>
    T* addComponent() {
        T* ptr = new T();
        components.push_back(std::unique_ptr<ptclComponent>(ptr));
        return ptr;
    }
    template<typename T>
    T* addRenderer() {
        T* ptr = new T();
        renderers.push_back(std::unique_ptr<ptclRenderer>(ptr));
        ptr->init(&pd);
        return ptr;
    }

    void init() {
        // New stuff
        rgba_curve[.0f] = gfxm::vec4(1, 0.65f, 0, 1);
        rgba_curve[1.f] = gfxm::vec4(.5f, .0f, 1.f, 1.f);
        scale_curve[.0f] = .1f;
        scale_curve[.1f] = .4f;
        scale_curve[.3f] = .4f;
        scale_curve[1.0f] = .0f;
        // ===
    }

    void update_emit(float dt) {
        if (cursor > duration) {
            if (looping) {
                cursor -= duration;
            }
            else {
                is_alive = false;
            }
        }

        timeCache += dt;
        float pt_per_sec = gfxm::_max(.0f, pt_per_second_curve.at(cursor / duration));
        int nParticlesToEmit = (int)(pt_per_sec * timeCache);
        if (nParticlesToEmit > 0) {
            timeCache = .0f;
            nParticlesToEmit = pd.getAvailableSlots(nParticlesToEmit);
            int begin_new = pd.aliveCount();
            int end_new = begin_new + nParticlesToEmit;
            shape->emitSome(&pd, nParticlesToEmit);
            for (int i = begin_new; i < end_new; ++i) {
                pd.particlePositions[i] = world_transform * gfxm::vec4(gfxm::vec3(pd.particlePositions[i]), 1.0f);
                pd.particleStates[i].velocity *= u01(mt_gen) * 5.0f;
                pd.particleStates[i].ang_velocity = gfxm::vec3(0, 0, (1.0f - u01(mt_gen) * 2.0f) * 5.0f);
                pd.particleRotation[i] = gfxm::angle_axis(gfxm::pi * u01(mt_gen) * 2.0f, gfxm::vec3(0, 0, 1));
                pd.particleScale[i] = gfxm::vec4(initial_scale_curve.at(u01(mt_gen)), pd.particleScale[i].w);
            }
            for (auto& r : renderers) {
                r->onParticlesSpawned(&pd, begin_new, end_new);
            }
        }
    }
    void update(float dt) {
        for (auto& c : components) {
            c->update(&pd, dt);
        }

        for (int i = 0; i < pd.aliveCount(); ++i) {
            float& lifetime = pd.particleScale[i].w;

            float& size = pd.particlePositions[i].w;
            size = scale_curve.at(lifetime / pd.maxLifetime);

            gfxm::vec3 pos = pd.particlePositions[i];
            pos += pd.particleStates[i].velocity * dt;
            pd.particlePositions[i] = gfxm::vec4(pos, size);
            
            pd.particleRotation[i]
                = gfxm::euler_to_quat(pd.particleStates[i].ang_velocity * dt)
                * pd.particleRotation[i];

            pd.particleColors[i] = rgba_curve.at(lifetime / pd.maxLifetime);

            lifetime += dt;

            if (lifetime > pd.maxLifetime) {                
                int vacant_slot = pd.recycleParticle(i);
                for (auto& r : renderers) {
                    r->onParticleDespawn(&pd, i);
                    r->onParticleMemMove(&pd, vacant_slot, i);                    
                }
            }
        }

        pd.updateBuffers();

        for (auto& r : renderers) {
            r->update(&pd, dt);
        }

        cursor += dt;
    }
    /*
    void draw(float dt) {
        for (auto& r : renderers) {
            r->draw(&pd, dt);
        }
    }*/

    void spawn(scnRenderScene* scn) {
        for (auto& r : renderers) {
            r->onSpawn(scn);
        }
    }
    void despawn(scnRenderScene* scn) {
        for (auto& r : renderers) {
            r->onDespawn(scn);
        }
    }
};