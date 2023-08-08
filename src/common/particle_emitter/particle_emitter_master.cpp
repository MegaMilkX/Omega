#include "particle_emitter_master.hpp"

#include <assert.h>
#include "reflection/reflection.hpp"



bool ParticleEmitterMaster::serialize(std::vector<unsigned char>& buf) const {
    assert(false);
    return false;
}
bool deserialize(const void* data, size_t sz) {
    assert(false);
    return false;
}
void ParticleEmitterMaster::serializeJson(nlohmann::json& json) const {
    json["max_count"] = max_count;
    json["max_lifetime"] = max_lifetime;

    json["duration"] = duration;
    json["looping"] = looping;

    type_write_json(json["gravity"], gravity);
    type_write_json(json["terminal_velocity"], terminal_velocity);
    type_write_json(json["particles_per_second"], pt_per_second_curve);
    type_write_json(json["initial_scale_curve"], initial_scale_curve);
    type_write_json(json["rgba_curve"], rgba_curve);
    type_write_json(json["scale_curve"], scale_curve);

    type_write_json(json["renderers"], renderers);

    type_write_json(json["shape"], shape);
}
bool ParticleEmitterMaster::deserializeJson(const nlohmann::json& json) {
    if (!json.is_object()) {
        assert(false);
        return false;
    }

    max_count = json["max_count"].get<int>();
    max_lifetime = json["max_lifetime"].get<float>();

    duration = json["duration"].get<float>();
    looping = json["looping"].get<bool>();

    type_read_json(json["gravity"], gravity);
    type_read_json(json["terminal_velocity"], terminal_velocity);
    type_read_json(json["particles_per_second"], pt_per_second_curve);
    type_read_json(json["initial_scale_curve"], initial_scale_curve);
    type_read_json(json["rgba_curve"], rgba_curve);
    type_read_json(json["scale_curve"], scale_curve);

    type_read_json(json["renderers"], renderers);

    type_read_json(json["shape"], shape);

    for (int i = 0; i < renderers.size(); ++i) {
        renderers[i]->init();
    }

    return true;
}
