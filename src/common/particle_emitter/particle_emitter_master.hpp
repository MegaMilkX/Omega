#pragma once

#include <set>

#include "uaf/uaf.hpp"

#include "particle_data.hpp"
#include "shape/particle_emitter_shape.hpp"
#include "component/particle_emitter_component.hpp"
#include "renderer/particle_emitter_renderer.hpp"


#include "shape/particle_emitter_sphere_shape.hpp"
#include "renderer/particle_emitter_quad_renderer.hpp"
#include "renderer/particle_emitter_trail_renderer.hpp"


#include "particle_emitter_instance.hpp"
struct ParticleEmitterMaster : public IRuntimeAsset {
private:
    std::set<ParticleEmitterInstance*> instances;

    std::random_device m_seed;
    mutable std::mt19937_64 mt_gen;
    mutable std::uniform_real_distribution<float> u01;
public:
    int max_count = 1000;
    float max_lifetime = 2.0f;

    float duration = 25.5f/60.0f;
    bool looping = true;
    std::unique_ptr<IParticleEmitterShape> shape;
    //std::vector<std::unique_ptr<ptclComponent>> components;
    std::vector<std::unique_ptr<IParticleRendererMaster>> renderers;

    curve<float> pt_per_second_curve;
    curve<gfxm::vec3> initial_scale_curve;
    curve<gfxm::vec4> rgba_curve;
    curve<float> scale_curve;

    ParticleEmitterMaster() 
        : max_count(1000), max_lifetime(2.f), mt_gen(m_seed()), u01(.0f, 1.f) {
        /*
        pt_per_second_curve[.0f] = 20.0f;
        shape.reset(new SphereParticleEmitterShape);

        addRenderer<QuadParticleRendererMaster>();

        initial_scale_curve[.0f] = gfxm::vec3(1.f, 1.f, 1.f);
        initial_scale_curve[1.f] = gfxm::vec3(2.f, 2.f, 2.f);
        // New stuff
        rgba_curve[.0f] = gfxm::vec4(1, 0.65f, 0, 1);
        rgba_curve[1.f] = gfxm::vec4(.5f, .0f, 1.f, 1.f);
        scale_curve[.0f] = .1f;
        scale_curve[.1f] = .4f;
        scale_curve[.3f] = .4f;
        scale_curve[1.0f] = .0f;*/
        // ===
    }
    ~ParticleEmitterMaster() {
        for (auto inst : instances) {
            delete inst;
        }
        instances.clear();
    }

    ParticleEmitterInstance* createInstance() {
        auto inst = new ParticleEmitterInstance(max_count);
        inst->master = this;/*
        for (auto& c : components) {
            auto component_instance = c->createInstance();
            inst->component_instances.push_back(component_instance);
        }*/
        for (auto& r : renderers) {
            auto renderer_instance = r->_createInstance();
            renderer_instance->init(&inst->particle_data);
            inst->renderer_instances.push_back(renderer_instance);
        }
        instances.insert(inst);
        return inst;
    }
    void destroyInstance(ParticleEmitterInstance* inst) {
        instances.erase(inst);/*
        for (int i = 0; i < components.size(); ++i) {
            components[i]->destroyInstance(inst->component_instances[i]);
        }*/
        for (int i = 0; i < renderers.size(); ++i) {
            renderers[i]->_destroyInstance(inst->renderer_instances[i]);
        }
        delete inst;
    }

    float getRandomNumber() const {
        return u01(mt_gen);
    }


    void setParticlePerSecondCurve(const curve<float>& c) {
        pt_per_second_curve = c;
    }
    void setScaleOverLifetimeCurve(const curve<float>& c) {
        scale_curve = c;
    }
    void setRGBACurve(const curve<gfxm::vec4>& c) {
        rgba_curve = c;
    }

    template<typename T>
    T* setShape() {
        T* ptr = new T();
        shape.reset(ptr);
        return ptr;
    }/*
    template<typename T>
    T* addComponent() {
        T* ptr = new T();
        components.push_back(std::unique_ptr<ptclComponent>(ptr));
        return ptr;
    }*/
    template<typename T>
    T* addRenderer() {
        T* ptr = new T();
        ptr->init();
        renderers.push_back(std::unique_ptr<IParticleRendererMaster>(ptr));
        return ptr;
    }

    bool serialize(std::vector<unsigned char>& buf) const;
    bool deserialize(const void* data, size_t sz);
    void serializeJson(nlohmann::json& json) const override;
    bool deserializeJson(const nlohmann::json& json) override;
};
STATIC_BLOCK{
    type_register<ParticleEmitterMaster>("ParticleEmitterMaster")
        .custom_serialize_json([](nlohmann::json& j, void* obj) {
            auto o = (ParticleEmitterMaster*)obj;
            o->serializeJson(j);
        })
        .custom_deserialize_json([](const nlohmann::json& j, void* obj) {
            auto o = (ParticleEmitterMaster*)obj;
            o->deserializeJson(j);
        });
};