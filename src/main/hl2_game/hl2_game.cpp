#include "hl2_game.hpp"

#include "world/node/node_character_capsule.hpp"


void HL2Game::onInit() {
    //gpuGetPipeline()->enableTechnique("EnvironmentIBL", false);

    hl2LoadBSP(
        "experimental/hl2/maps/d2_coast_08.bsp",
        //"experimental/hl2/maps/d2_coast_03.bsp",
        //"experimental/hl2/maps/d1_town_01.bsp",
        //"experimental/hl2/maps/d2_prison_02.bsp",
        //"experimental/hl2/maps/d1_trainstation_01.bsp",
        //"experimental/hl2/maps/d1_trainstation_02.bsp",
        //"experimental/hl2/maps/d1_trainstation_03.bsp",
        //"experimental/hl2/maps/d1_trainstation_04.bsp",
        //"experimental/hl2/maps/d1_trainstation_05.bsp",
        //"experimental/hl2/maps/d1_trainstation_06.bsp",
        //"experimental/hl2/maps/d1_canals_07.bsp",
        //"experimental/hl2/maps/d1_canals_13.bsp",
        //"experimental/hl2/maps/d1_eli_01.bsp",
        //"experimental/hl2/maps/test.bsp",
        //"experimental/hl2/maps/l4d_vs_hospital01_apartment.bsp",
        //"bsp/q1/e1m1.bsp",
        &hl2scene
    );
    hl2scene.addCollisionShapes(getWorld()->getCollisionWorld());
    
    // TODO: Should be loaded from config file
    // Input: bind actions and ranges
    inputCreateActionDesc("C")
        .linkKey(Key.Keyboard.C, 1.f);
    inputCreateActionDesc("V")
        .linkKey(Key.Keyboard.V, 1.f);
    inputCreateActionDesc("Z")
        .linkKey(Key.Keyboard.Z, 1.f);
    inputCreateActionDesc("ToggleWireframe")
        .linkKey(Key.Keyboard.I, 1.f);
    inputCreateActionDesc("Recover")
        .linkKey(Key.Keyboard.Q, 1.f);
    inputCreateActionDesc("SphereCast")
        .linkKey(Key.Keyboard.Z, 1.f);
    inputCreateActionDesc("CharacterInteract")
        .linkKey(Key.Keyboard.E, 1.f);
    inputCreateActionDesc("Shoot")
        .linkKey(Key.Mouse.BtnLeft, 1.f);
    inputCreateActionDesc("Sprint")
        .linkKey(Key.Keyboard.LeftShift, 1.f);
    inputCreateActionDesc("Jump")
        .linkKey(Key.Keyboard.Space, 1.f);
    inputCreateActionDesc("Crouch")
        .linkKey(Key.Keyboard.LeftControl, 1.f);

    inputCreateRangeDesc("CharacterLocomotion")
        .linkKeyX(Key.Keyboard.A, -1.0f)
        .linkKeyX(Key.Keyboard.D, 1.0f)
        .linkKeyZ(Key.Keyboard.W, -1.0f)
        .linkKeyZ(Key.Keyboard.S, 1.0f);
    inputCreateRangeDesc("CameraRotation")
        .linkKeyY(Key.Mouse.AxisX, 1.0f)
        .linkKeyX(Key.Mouse.AxisY, 1.0f);
    inputCreateRangeDesc("Scroll")
        .linkKeyX(Key.Mouse.Scroll, -1.0f);

    for (int i = 0; i < 12; ++i) {
        inputCreateActionDesc(MKSTR("F" << (i + 1)).c_str())
            .linkKey(Key.Keyboard.F1 + i, 1.f);
    }
    for (int i = 0; i < 9; ++i) {
        inputCreateActionDesc(MKSTR("_" << i).c_str())
            .linkKey(Key.Keyboard._0 + i, 1.f);
    }

    {
        fps_player_actor.setFlags(ACTOR_FLAG_UPDATE);
        auto capsule = fps_player_actor.setRoot<CharacterCapsuleNode>("capsule");
        capsule->shape.height = 1.2f;
        capsule->shape.radius = .2f;
        capsule->collider.setCenterOffset(gfxm::vec3(.0f, .8f + .2f, .0f));
        capsule->collider.collision_group
            = COLLISION_LAYER_CHARACTER;
        fps_player_actor.addController<FpsCharacterController>();
        //fps_player_actor.addController<FpsCameraController>();

        fps_player_actor.setTranslation(hl2scene.player_origin);
        //fps_player_actor.setRotation(hl2bspmodel.player_orientation);
        fps_player_actor.getController<FpsCharacterController>()->setOrientation(hl2scene.player_orientation);

        getWorld()->spawnActor(&fps_player_actor);
    }

    playerLinkAgent(playerGetPrimary(), &fps_player_actor);
}
void HL2Game::onCleanup() {

}

void HL2Game::onUpdate(float dt) {
    LocalPlayer* local_player = dynamic_cast<LocalPlayer*>(playerGetPrimary());
    assert(local_player);
    Viewport* viewport = local_player->getViewport();
    assert(viewport);
    gpuRenderTarget* render_target = viewport->getRenderTarget();

    // TODO:
}
void HL2Game::onDraw(float dt) {
    LocalPlayer* local_player = dynamic_cast<LocalPlayer*>(playerGetPrimary());
    assert(local_player);
    Viewport* viewport = local_player->getViewport();
    assert(viewport);
    gpuRenderBucket* render_bucket = viewport->getRenderBucket();
    gpuRenderTarget* render_target = viewport->getRenderTarget();
    
    hl2scene.draw(render_bucket);
}