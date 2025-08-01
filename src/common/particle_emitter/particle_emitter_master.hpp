#pragma once

#include <set>

#include "FastNoiseSIMD.h"

#include "uaf/uaf.hpp"

#include "particle_data.hpp"
#include "shape/particle_emitter_shape.hpp"
#include "component/particle_emitter_component.hpp"
#include "renderer/particle_emitter_renderer.hpp"


#include "shape/particle_emitter_sphere_shape.hpp"
#include "renderer/particle_emitter_quad_renderer.hpp"
#include "renderer/particle_emitter_trail_renderer.hpp"

enum PARTICLE_MOVEMENT_MODE {
    PARTICLE_MOVEMENT_WORLD,
    PARTICLE_MOVEMENT_SHAPE
};

#include "particle_emitter_instance.hpp"
struct ParticleEmitterMaster : public IRuntimeAsset {
private:
    std::set<ParticleEmitterInstance*> instances;

    std::random_device m_seed;
    mutable std::mt19937_64 mt_gen;
    mutable std::uniform_real_distribution<float> u01;

public:
    ParticleEmitterParams params;
    std::unique_ptr<IParticleEmitterShape> shape;
    //std::vector<std::unique_ptr<ptclComponent>> components;
    std::vector<std::unique_ptr<IParticleRendererMaster>> renderers;

    PARTICLE_MOVEMENT_MODE movement_mode = PARTICLE_MOVEMENT_WORLD;

    FastNoiseSIMD* noise = 0;
    const int fieldSize = 64;
    std::vector<float> noise_set;

    ParticleEmitterMaster() 
        : mt_gen(m_seed()), u01(.0f, 1.f) {
        noise = FastNoiseSIMD::NewFastNoiseSIMD();
        noise->SetNoiseType(FastNoiseSIMD::Cellular);
        noise->SetPerturbType(FastNoiseSIMD::PerturbType::GradientFractal);
        noise->SetPerturbAmp(1.0f);
        noise->SetPerturbFrequency(0.25f);
        noise->SetFrequency(.03f);
        noise->SetCellularReturnType(FastNoiseSIMD::CellularReturnType::Distance);
        auto ptr = noise->GetNoiseSet(0, 0, 0, fieldSize, fieldSize, fieldSize, 1.0f);
        noise_set.resize(fieldSize*fieldSize*fieldSize);
        memcpy(&noise_set[0], ptr, fieldSize*fieldSize*fieldSize * sizeof(float));
        noise->FreeNoiseSet(ptr);
    }
    ~ParticleEmitterMaster() {
        delete noise;

        for (auto inst : instances) {
            delete inst;
        }
        instances.clear();
    }

    ParticleEmitterInstance* createInstance() {
        auto inst = new ParticleEmitterInstance(params.max_count);
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
        params.pt_per_second_curve = c;
    }
    void setScaleOverLifetimeCurve(const curve<float>& c) {
        params.scale_curve = c;
    }
    void setRGBACurve(const curve<gfxm::vec4>& c) {
        params.rgba_curve = c;
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