#include "node_sound_emitter.hpp"


STATIC_BLOCK {
    type_register<SoundEmitterNode>("SoundEmitterNode")
        .parent<ActorNode>();
};


SoundEmitterNode::SoundEmitterNode() {
    getTransformHandle()->addDirtyCallback([](void* ctx) {
        SoundEmitterNode* node = (SoundEmitterNode*)ctx;
        node->emitter.setPosition(node->getWorldTranslation());
    }, this);
}