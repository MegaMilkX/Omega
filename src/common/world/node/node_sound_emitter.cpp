#include "node_sound_emitter.hpp"


STATIC_BLOCK {
    type_register<SoundEmitterNode>("SoundEmitterNode")
        .parent<gameActorNode>();
};