#include "game_common.hpp"

GameCommon* g_game_comn = 0;


void GameCommon::onViewportResize(int width, int height) {
    tex_albedo->changeFormat(GL_RGB, width, height, 3);
    tex_depth->changeFormat(GL_DEPTH_COMPONENT, width, height, 1);

    gui_root.size = gfxm::vec2(width, height);
}