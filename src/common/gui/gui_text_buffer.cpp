#include "gui_text_buffer.hpp"

#include "gui_draw.hpp"


void GuiTextBuffer::draw(const gfxm::vec2& pos, uint32_t col, uint32_t selection_col) {
    int screen_w = 0, screen_h = 0;
    platformGetWindowSize(screen_w, screen_h);

    gfxm::ivec2 pos_{ (int)pos.x, (int)pos.y };

    // selection rectangles
    {
        gpuShaderProgram* prog = _guiGetShaderTextSelection();

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);

        gfxm::mat4 model
            = gfxm::translate(gfxm::mat4(1.0f), gfxm::vec3(pos_.x, screen_h - pos_.y, .0f));
        gfxm::mat4 view = gfxm::mat4(1.0f);
        gfxm::mat4 proj = gfxm::ortho(.0f, (float)screen_w, .0f, (float)screen_h, .0f, 100.0f);

        glUseProgram(prog->getId());
        glUniformMatrix4fv(prog->getUniformLocation("matView"), 1, GL_FALSE, (float*)&view);
        glUniformMatrix4fv(prog->getUniformLocation("matProjection"), 1, GL_FALSE, (float*)&proj);
        glUniformMatrix4fv(prog->getUniformLocation("matModel"), 1, GL_FALSE, (float*)&model);
        gfxm::vec4 colorf;
        colorf[3] = ((selection_col & 0xff000000) >> 24) / 255.0f;
        colorf[2] = ((selection_col & 0x00ff0000) >> 16) / 255.0f;
        colorf[1] = ((selection_col & 0x0000ff00) >> 8) / 255.0f;
        colorf[0] = (selection_col & 0x000000ff) / 255.0f;
        glUniform4fv(prog->getUniformLocation("color"), 1, (float*)&colorf);

        sel_mesh_desc._bindVertexArray(VFMT::Position_GUID, 0);
        sel_mesh_desc._bindIndexArray();
        sel_mesh_desc._draw();
    }

    gpuShaderProgram* prog_text = _guiGetShaderText();

    gfxm::mat4 view = guiGetViewTransform();
    gfxm::mat4 proj = gfxm::ortho(.0f, (float)screen_w, (float)screen_h, .0f, .0f, 100.0f);
    //gfxm::mat4 proj = gfxm::ortho(.0f, (float)screen_w, .0f, (float)screen_h, .0f, 100.0f);

    glUseProgram(prog_text->getId());

    glUniformMatrix4fv(prog_text->getUniformLocation("matView"), 1, GL_FALSE, (float*)&view);
    glUniformMatrix4fv(prog_text->getUniformLocation("matProjection"), 1, GL_FALSE, (float*)&proj);
    glUniform1i(prog_text->getUniformLocation("texAlbedo"), 0);
    glUniform1i(prog_text->getUniformLocation("texTextUVLookupTable"), 1);

    gfxm::vec4 colorf;
    colorf[0] = ((col & 0xff000000) >> 24) / 255.0f;
    colorf[1] = ((col & 0x00ff0000) >> 16) / 255.0f;
    colorf[2] = ((col & 0x0000ff00) >> 8) / 255.0f;
    colorf[3] = (col & 0x000000ff) / 255.0f;

    glUniform1i(prog_text->getUniformLocation("lookupTextureWidth"), font->lut->getWidth());

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, font->atlas->getId());
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, font->lut->getId());

    mesh_desc._bindVertexArray(VFMT::Position_GUID, 0);
    mesh_desc._bindVertexArray(VFMT::UV_GUID, 1);
    mesh_desc._bindVertexArray(VFMT::TextUVLookup_GUID, 2);
    mesh_desc._bindVertexArray(VFMT::ColorRGBA_GUID, 3);
    mesh_desc._bindIndexArray();

    gfxm::mat4 model
        = gfxm::translate(gfxm::mat4(1.0f), gfxm::vec3(pos_.x, pos_.y + 1.f, .0f));
    glUniformMatrix4fv(prog_text->getUniformLocation("matModel"), 1, GL_FALSE, (float*)&model);
    glUniform4fv(prog_text->getUniformLocation("color"), 1, (float*)&gfxm::vec4(0, 0, 0, 1));
    mesh_desc._draw();

    model
        = gfxm::translate(gfxm::mat4(1.0f), gfxm::vec3(pos_.x, pos_.y, .0f));
    glUniformMatrix4fv(prog_text->getUniformLocation("matModel"), 1, GL_FALSE, (float*)&model);
    glUniform4fv(prog_text->getUniformLocation("color"), 1, (float*)&colorf);
    mesh_desc._draw();
}