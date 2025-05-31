#include "particle_impl.hpp"

#include "math/gfxm.hpp"

#include "particle_emitter/particle_emitter_master.hpp"
#include "particle_emitter/particle_emitter_instance.hpp"

void ptclUpdateEmit(float dt, ParticleEmitterInstance* instance) {
    auto master = instance->getMaster();

    const float     duration = master->duration;
    const bool      looping = master->looping;
    /* const */auto pt_per_second_curve = master->pt_per_second_curve;
    /* const */auto initial_scale_curve = master->initial_scale_curve;
    const auto      shape = master->shape.get();
    const float     rnd0 = master->getRandomNumber();
    const float     rnd1 = master->getRandomNumber();
    const float     rnd2 = master->getRandomNumber();

    bool    has_teleported = instance->hasTeleported();
    float&  cursor = instance->cursor;
    bool&   is_alive = instance->is_alive;
    float&  time_cache = instance->time_cache;
    auto&   particle_data = instance->particle_data;
    auto&   world_transform_old = instance->world_transform_old;
    auto&   world_transform = instance->world_transform;


    if (cursor > duration) {
        if (looping) {
            cursor -= duration;
        } else {
            is_alive = false;
        }
    }
    if (!is_alive) {
        return;
    }

    time_cache += dt;
    float pt_per_sec = gfxm::_max(.0f, pt_per_second_curve.at(cursor / duration));
    int nParticlesToEmit = (int)(pt_per_sec * time_cache);
    if (nParticlesToEmit > 0) {
        time_cache = .0f;
        nParticlesToEmit = particle_data.getAvailableSlots(nParticlesToEmit);
        int begin_new = particle_data.aliveCount();
        int end_new = begin_new + nParticlesToEmit;
        shape->emitSome(&particle_data, nParticlesToEmit);

        gfxm::vec3 base_pos_a = world_transform_old[3];
        gfxm::vec3 base_pos_b = world_transform[3];
        world_transform_old = world_transform;

        for (int i = begin_new; i < end_new; ++i) {
            float mul = ((float)(i - begin_new) / (float)nParticlesToEmit);
            if (has_teleported) {
                mul = 1.f;
            }
            gfxm::vec3 base_pos = base_pos_a + (base_pos_b - base_pos_a) * mul;

            particle_data.particlePositions[i] = gfxm::vec4(base_pos + gfxm::vec3(particle_data.particlePositions[i]), .0f);
            particle_data.particleStates[i].velocity *= .0f;// u01(mt_gen) * 5.0f;
            particle_data.particleStates[i].ang_velocity = gfxm::vec3(0, 0, (1.0f - rnd0 * 2.0f) * 5.0f);
            particle_data.particleRotation[i] = gfxm::angle_axis(gfxm::pi * rnd1 * 2.0f, gfxm::vec3(0, 0, 1));
            particle_data.particleScale[i] = gfxm::vec4(initial_scale_curve.at(rnd2), particle_data.particleScale[i].w);
        }
        for (auto& r : instance->renderer_instances) {
            r->onParticlesSpawned(&particle_data, begin_new, end_new);
        }
    }
}

void ptclUpdate(float dt, ParticleEmitterInstance* instance) {
    auto master = instance->getMaster();

    /*const */auto& scale_curve = master->scale_curve;
    /*const */auto& rgba_curve = master->rgba_curve;
    float max_lifetime = master->max_lifetime;
    gfxm::vec3 gravity = master->gravity;
    float terminal_velocity = master->terminal_velocity;

    auto& particle_data = instance->particle_data;
    
    /*
    for (auto& c : instance->components) {
        c->update(&particle_data, dt);
    }*/

    for (int i = 0; i < particle_data.aliveCount(); ++i) {
        float& lifetime = particle_data.particleScale[i].w;

        float& size = particle_data.particlePositions[i].w;
        size = scale_curve.at(lifetime / max_lifetime);

        gfxm::vec3 pos = particle_data.particlePositions[i];
        gfxm::vec3& velocity = particle_data.particleStates[i].velocity;
        pos += velocity * dt;
        velocity += gravity * dt;
        
        if (master->noise) {
            static gfxm::vec3 noise_offs;
            noise_offs.z += .01f * dt;
            gfxm::vec4 noise;
            // TODO: Stack gets corrupted
            
            master->noise->FillNoiseSet(
                &noise[0], pos.x + noise_offs.x, pos.y + noise_offs.y, pos.z + noise_offs.z, 1, 1, 1, 1.0f
            );
            noise = (noise * 2.f - gfxm::normalize(noise));
            //velocity += gfxm::vec3(noise) * dt * 10.f;
        }
        gfxm::vec3 velo_N = gfxm::normalize(velocity);
        float d = gfxm::dot(velocity, velo_N);
        if (d > terminal_velocity) {
            velocity *= terminal_velocity / d;
        }

        particle_data.particlePositions[i] = gfxm::vec4(pos, size);
            
        particle_data.particleRotation[i]
            = gfxm::euler_to_quat(particle_data.particleStates[i].ang_velocity * dt)
            * particle_data.particleRotation[i];

        particle_data.particleColors[i] = rgba_curve.at(lifetime / max_lifetime);

        lifetime += dt;

        if (lifetime > max_lifetime) {
            int vacant_slot = particle_data.recycleParticle(i);
            for (auto& r : instance->renderer_instances) {
                r->onParticleDespawn(&particle_data, i);
                r->onParticleMemMove(&particle_data, vacant_slot, i);
            }
        }
    }

    particle_data.updateBuffers();

    for (auto& r : instance->renderer_instances) {
        r->update(&particle_data, dt);
    }

    instance->cursor += dt;
}
