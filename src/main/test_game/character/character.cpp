#include "character.hpp"



STATIC_BLOCK {
    rtti::type_register<actorAnimatedSkeletalModel>("actorAnimatedSkeletalModel")
        .parent<Actor>();
};

STATIC_BLOCK {
    rtti::type_register<actorJukebox>("actorJukebox")
        .parent<Actor>();
};

STATIC_BLOCK {
    rtti::type_register<actorAnimTest>("actorAnimTest")
        .parent<Actor>();
};

STATIC_BLOCK {
    rtti::type_register<actorVfxTest>("actorVfxTest")
        .parent<Actor>();
};

STATIC_BLOCK {
    rtti::type_register<actorUltimaWeapon>("actorUltimaWeapon")
        .parent<Actor>();
};

STATIC_BLOCK {
    rtti::type_register<DoorActor>("DoorActor")
        .parent<Actor>();
};
