#include "game/game_common.hpp"

void GameCommon::Update(float dt) {
    gfxm::vec3 loco_vec = inputCharaTranslation->getVec3();
    gfxm::mat3 loco_rot;
    loco_rot[2] = gfxm::normalize(cam.getView() * gfxm::vec4(0, 0, 1, 0));
    loco_rot[1] = gfxm::vec3(0, 1, 0);
    loco_rot[0] = gfxm::cross(loco_rot[1], loco_rot[2]);
    loco_vec = loco_rot * loco_vec;
    loco_vec.y = .0f;
    loco_vec = gfxm::normalize(loco_vec);

    chara.setDesiredLocomotionVector(loco_vec);

    {
        gfxm::vec3 chara2_forward = chara2.getForward();
        gfxm::vec3 chara_pos = chara.getTranslation();
        gfxm::vec3 chara2_pos = chara2.getTranslation();
        float distance = gfxm::length(chara_pos - chara2_pos);
        gfxm::vec3 normal_to_target = gfxm::normalize(chara_pos - chara2_pos);
        float dot = gfxm::dot(chara2_forward, normal_to_target);
        bool is_target_visible = dot > FLT_EPSILON;
        if (is_target_visible && distance > 2.f) {
            chara2.setDesiredLocomotionVector(normal_to_target);
        } else {
            chara2.setDesiredLocomotionVector(gfxm::vec3(0, 0, 0));
        }
    }
    if (inputCharaUse->isJustPressed()) {
        chara.actionUse();
    }
    chara.update(dt);
    chara2.update(dt);
    door->update(dt);
    anim_test.update(dt);
    ultima_weapon.update(dt);

    world.update(dt);
}