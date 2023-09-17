#include <stack>

#include "gui/gui_draw.hpp"

#include "platform/platform.hpp"

#include "gui/gui_color.hpp"
#include "gui/gui_values.hpp"


static std::vector<GuiDrawCmd> draw_commands;
static std::vector<gfxm::vec3> g_vertices;
static std::vector<gfxm::vec2> g_uv;
static std::vector<uint32_t> g_colors;
static std::vector<uint32_t> g_indices;

static std::vector<gfxm::vec3> g_text_vertices;
static std::vector<gfxm::vec2> g_text_uv;
static std::vector<uint32_t> g_text_colors;
static std::vector<float> g_text_uv_lookup;
static std::vector<uint32_t> g_text_indices;

void guiRender() {
    glBindVertexArray(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    int screen_w = 0, screen_h = 0;
    platformGetWindowSize(screen_w, screen_h);
    glViewport(0, 0, screen_w, screen_h);
    glScissor(0, 0, screen_w, screen_h);

    glEnable(GL_SCISSOR_TEST);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    GLuint vao_default;
    GLuint vao_text;
    glGenVertexArrays(1, &vao_default);
    glGenVertexArrays(1, &vao_text);

    gpuBuffer vertexBuffer;
    gpuBuffer uvBuffer;
    gpuBuffer colorBuffer;
    gpuBuffer indexBuffer;
    {        
        vertexBuffer.setArrayData(g_vertices.data(), g_vertices.size() * sizeof(g_vertices[0]));
        uvBuffer.setArrayData(g_uv.data(), g_uv.size() * sizeof(g_uv[0]));
        colorBuffer.setArrayData(g_colors.data(), g_colors.size() * sizeof(g_colors[0]));
        indexBuffer.setArrayData(g_indices.data(), g_indices.size() * sizeof(g_indices[0]));

        glBindVertexArray(vao_default);

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer.getId());
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer.getId());
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glBindBuffer(GL_ARRAY_BUFFER, uvBuffer.getId());
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glBindBuffer(GL_ARRAY_BUFFER, colorBuffer.getId());
        glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, 0);
    }
    glBindVertexArray(0);

    
    gpuBuffer textVertexBuffer;
    gpuBuffer textUvBuffer;
    gpuBuffer textColorBuffer;
    gpuBuffer textLookupBuffer;
    gpuBuffer textIndexBuffer;
    {
        textVertexBuffer.setArrayData(g_text_vertices.data(), g_text_vertices.size() * sizeof(g_text_vertices[0]));
        textUvBuffer.setArrayData(g_text_uv.data(), g_text_uv.size() * sizeof(g_text_uv[0]));
        textColorBuffer.setArrayData(g_text_colors.data(), g_text_colors.size() * sizeof(g_text_colors[0]));
        textLookupBuffer.setArrayData(g_text_uv_lookup.data(), g_text_uv_lookup.size() * sizeof(g_text_uv_lookup[0]));
        textIndexBuffer.setArrayData(g_text_indices.data(), g_text_indices.size() * sizeof(g_text_indices[0]));

        glBindVertexArray(vao_text);

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);
        glEnableVertexAttribArray(3);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, textIndexBuffer.getId());
        glBindBuffer(GL_ARRAY_BUFFER, textVertexBuffer.getId());
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glBindBuffer(GL_ARRAY_BUFFER, textUvBuffer.getId());
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glBindBuffer(GL_ARRAY_BUFFER, textLookupBuffer.getId());
        glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 0, 0);
        glBindBuffer(GL_ARRAY_BUFFER, textColorBuffer.getId());
        glVertexAttribPointer(3, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, 0);
    }
    glBindVertexArray(0);


    //gfxm::mat4 proj = gfxm::ortho(.0f, (float)screen_w, (float)screen_h, .0f, .0f, 100.0f);
    for (int i = 0; i < draw_commands.size(); ++i) {
        const auto& cmd = draw_commands[i];
        const gfxm::mat4 model = cmd.model_transform;
        const gfxm::mat4 view = cmd.view_transform;
        const gfxm::mat4 proj = cmd.projection;

        const gfxm::rect scsr = cmd.scissor_rect;
        float scsr_x = scsr.min.x;
        float scsr_y = screen_h - scsr.max.y;
        float scsr_w = gfxm::_max(.0f, scsr.max.x - scsr.min.x);
        float scsr_h = gfxm::_max(.0f, scsr.max.y - scsr.min.y);
        assert(scsr_w >= .0f && scsr_h >= .0f);
        glScissor(
            scsr_x,
            scsr_y,
            scsr_w,
            scsr_h
        );
        const gfxm::rect vp = cmd.viewport_rect;
        float vp_x = vp.min.x;
        float vp_y = screen_h - vp.max.y;
        float vp_w = vp.max.x - vp.min.x;
        float vp_h = vp.max.y - vp.min.y;
        assert(vp_w > .0f && vp_h > .0f);
        glViewport(vp_x, vp_y, vp_w, vp_h);

        if (cmd.cmd == GUI_DRAW_LINE_STRIP) {
            glBindVertexArray(vao_default);
            auto prog = _guiGetShaderRect();
            glUseProgram(prog->getId());
            glUniformMatrix4fv(prog->getUniformLocation("matView"), 1, GL_FALSE, (float*)&view);
            glUniformMatrix4fv(prog->getUniformLocation("matProjection"), 1, GL_FALSE, (float*)&proj);
            glUniformMatrix4fv(prog->getUniformLocation("matModel"), 1, GL_FALSE, (float*)&model);
            glUniform1i(prog->getUniformLocation("texAlbedo"), 0);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, cmd.tex0);
            
            glDrawArrays(GL_LINE_STRIP, cmd.vertex_first, cmd.vertex_count);
        } else if (cmd.cmd == GUI_DRAW_LINES) {
            glBindVertexArray(vao_default);
            auto prog = _guiGetShaderRect();
            glUseProgram(prog->getId());
            glUniformMatrix4fv(prog->getUniformLocation("matView"), 1, GL_FALSE, (float*)&view);
            glUniformMatrix4fv(prog->getUniformLocation("matProjection"), 1, GL_FALSE, (float*)&proj);
            glUniformMatrix4fv(prog->getUniformLocation("matModel"), 1, GL_FALSE, (float*)&model);
            glUniform1i(prog->getUniformLocation("texAlbedo"), 0);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, cmd.tex0);
            
            glDrawArrays(GL_LINES, cmd.vertex_first, cmd.vertex_count);
        } else if (cmd.cmd == GUI_DRAW_TRIANGLE_STRIP) {
            glBindVertexArray(vao_default);
            auto prog = _guiGetShaderRect();
            glUseProgram(prog->getId());
            glUniformMatrix4fv(prog->getUniformLocation("matView"), 1, GL_FALSE, (float*)&view);
            glUniformMatrix4fv(prog->getUniformLocation("matProjection"), 1, GL_FALSE, (float*)&proj);
            glUniformMatrix4fv(prog->getUniformLocation("matModel"), 1, GL_FALSE, (float*)&model);
            glUniform1i(prog->getUniformLocation("texAlbedo"), 0);
            
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, cmd.tex0);
        
            glDrawArrays(GL_TRIANGLE_STRIP, cmd.vertex_first, cmd.vertex_count);
        } else if(cmd.cmd == GUI_DRAW_TRIANGLE_FAN) {
            glBindVertexArray(vao_default);
            auto prog = _guiGetShaderRect();
            glUseProgram(prog->getId());
            glUniformMatrix4fv(prog->getUniformLocation("matView"), 1, GL_FALSE, (float*)&view);
            glUniformMatrix4fv(prog->getUniformLocation("matProjection"), 1, GL_FALSE, (float*)&proj);
            glUniformMatrix4fv(prog->getUniformLocation("matModel"), 1, GL_FALSE, (float*)&model);
            glUniform1i(prog->getUniformLocation("texAlbedo"), 0);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, cmd.tex0);

            glDrawArrays(GL_TRIANGLE_FAN, cmd.vertex_first, cmd.vertex_count);
        } else if(cmd.cmd == GUI_DRAW_TRIANGLES) {
            glBindVertexArray(vao_default);
            auto prog = _guiGetShaderRect();
            glUseProgram(prog->getId());
            glUniformMatrix4fv(prog->getUniformLocation("matView"), 1, GL_FALSE, (float*)&view);
            glUniformMatrix4fv(prog->getUniformLocation("matProjection"), 1, GL_FALSE, (float*)&proj);
            glUniformMatrix4fv(prog->getUniformLocation("matModel"), 1, GL_FALSE, (float*)&model);
            glUniform1i(prog->getUniformLocation("texAlbedo"), 0);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, cmd.tex0);

            glDrawArrays(GL_TRIANGLES, cmd.vertex_first, cmd.vertex_count);
        } else if (cmd.cmd == GUI_DRAW_TRIANGLES_INDEXED) {
            glBindVertexArray(vao_default);
            auto prog = _guiGetShaderRect();
            glUseProgram(prog->getId());
            glUniformMatrix4fv(prog->getUniformLocation("matView"), 1, GL_FALSE, (float*)&view);
            glUniformMatrix4fv(prog->getUniformLocation("matProjection"), 1, GL_FALSE, (float*)&proj);
            glUniformMatrix4fv(prog->getUniformLocation("matModel"), 1, GL_FALSE, (float*)&model);
            glUniform1i(prog->getUniformLocation("texAlbedo"), 0);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, cmd.tex0);

            glDrawElements(GL_TRIANGLES, cmd.index_count, GL_UNSIGNED_INT, (void*)(cmd.index_first * sizeof(uint32_t)));
        } else if(cmd.cmd == GUI_DRAW_TEXT) {
            glBindVertexArray(vao_text);
            gpuShaderProgram* prog_text = _guiGetShaderText();
            glUseProgram(prog_text->getId());

            glUniformMatrix4fv(prog_text->getUniformLocation("matView"), 1, GL_FALSE, (float*)&view);
            glUniformMatrix4fv(prog_text->getUniformLocation("matProjection"), 1, GL_FALSE, (float*)&proj);
            glUniform1i(prog_text->getUniformLocation("texAlbedo"), 0);
            glUniform1i(prog_text->getUniformLocation("texTextUVLookupTable"), 1);

            gfxm::vec4 colorf;
            colorf[0] = ((cmd.color & 0xff000000) >> 24) / 255.0f;
            colorf[1] = ((cmd.color & 0x00ff0000) >> 16) / 255.0f;
            colorf[2] = ((cmd.color & 0x0000ff00) >> 8) / 255.0f;
            colorf[3] = (cmd.color & 0x000000ff) / 255.0f;

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, cmd.tex0);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, cmd.tex1);
            
            gfxm::mat4 model_shadow
                = gfxm::translate(model, gfxm::vec3(.0f, 1.f, .0f));
            glUniformMatrix4fv(prog_text->getUniformLocation("matModel"), 1, GL_FALSE, (float*)&model_shadow);
            glUniform4fv(prog_text->getUniformLocation("color"), 1, (float*)&gfxm::vec4(0, 0, 0, 1));
            // TODO: glDrawElementsBaseVertex
            glDrawElements(GL_TRIANGLES, cmd.index_count, GL_UNSIGNED_INT, (void*)(cmd.index_first * sizeof(uint32_t)));
            
            glUniformMatrix4fv(prog_text->getUniformLocation("matModel"), 1, GL_FALSE, (float*)&model);
            glUniform4fv(prog_text->getUniformLocation("color"), 1, (float*)&colorf);
            glDrawElements(GL_TRIANGLES, cmd.index_count, GL_UNSIGNED_INT, (void*)(cmd.index_first * sizeof(uint32_t)));
        } else if (cmd.cmd == GUI_DRAW_TEXT_HIGHLIGHT) {
            glBindVertexArray(vao_default);
            auto prog = _guiGetShaderTextSelection();
            glUseProgram(prog->getId());
            glUniformMatrix4fv(prog->getUniformLocation("matView"), 1, GL_FALSE, (float*)&view);
            glUniformMatrix4fv(prog->getUniformLocation("matProjection"), 1, GL_FALSE, (float*)&proj);
            glUniformMatrix4fv(prog->getUniformLocation("matModel"), 1, GL_FALSE, (float*)&model);
            gfxm::vec4 colorf;
            colorf[3] = ((cmd.color & 0xff000000) >> 24) / 255.0f;
            colorf[2] = ((cmd.color & 0x00ff0000) >> 16) / 255.0f;
            colorf[1] = ((cmd.color & 0x0000ff00) >> 8) / 255.0f;
            colorf[0] = (cmd.color & 0x000000ff) / 255.0f;
            glUniform4fv(prog->getUniformLocation("color"), 1, (float*)&colorf);

            glDrawElements(GL_TRIANGLES, cmd.index_count, GL_UNSIGNED_INT, (void*)(cmd.index_first * sizeof(uint32_t)));
        }
    }

    g_vertices.clear();
    g_uv.clear();
    g_colors.clear();
    g_indices.clear();

    g_text_vertices.clear();
    g_text_uv.clear();
    g_text_colors.clear();
    g_text_uv_lookup.clear();
    g_text_indices.clear();

    draw_commands.clear();

    glBindVertexArray(0);
    glDeleteVertexArrays(1, &vao_text);
    glDeleteVertexArrays(1, &vao_default);
}

GuiDrawCmd& guiDrawTriangles(
    const gfxm::vec3* vertices,
    const uint32_t* colors,
    const gfxm::vec2* uvs,
    int vertex_count
) {
    GuiDrawCmd cmd;
    cmd.cmd = GUI_DRAW_TRIANGLES;
    cmd.vertex_first = g_vertices.size();
    cmd.vertex_count = vertex_count;
    cmd.index_first = 0;
    cmd.index_count = 0;
    cmd.view_transform = guiGetViewTransform();
    cmd.projection = guiGetCurrentProjection();
    cmd.model_transform = gfxm::mat4(1.0f);
    cmd.color = 0xFFFFFFFF;
    cmd.tex0 = _guiGetTextureWhite();
    cmd.scissor_rect = guiDrawGetCurrentScissor();
    cmd.viewport_rect = guiGetCurrentViewportRect();
    draw_commands.push_back(cmd);

    g_vertices.insert(g_vertices.end(), vertices, vertices + vertex_count);
    g_colors.insert(g_colors.end(), colors, colors + vertex_count);
    if (uvs) {
        g_uv.insert(g_uv.end(), uvs, uvs + vertex_count);
    } else {
        g_uv.resize(g_uv.size() + vertex_count);
    }

    return draw_commands.back();
}

GuiDrawCmd& guiDrawTriangleFan(
    const gfxm::vec3* vertices,
    const uint32_t* colors,
    int vertex_count,
    bool no_view_projection = false
) {
    GuiDrawCmd cmd;
    cmd.cmd = GUI_DRAW_TRIANGLE_FAN;
    cmd.vertex_first = g_vertices.size();
    cmd.vertex_count = vertex_count;
    cmd.index_first = 0;
    cmd.index_count = 0;
    cmd.view_transform = no_view_projection ? gfxm::mat4(1.f) : guiGetViewTransform();
    cmd.projection = no_view_projection ? gfxm::mat4(1.f) : guiGetCurrentProjection();
    cmd.model_transform = gfxm::mat4(1.0f);
    cmd.color = 0xFFFFFFFF;
    cmd.tex0 = _guiGetTextureWhite();
    cmd.scissor_rect = guiDrawGetCurrentScissor();
    cmd.viewport_rect = guiGetCurrentViewportRect();
    draw_commands.push_back(cmd);

    std::vector<gfxm::vec2> uv;
    uv.resize(vertex_count);
    std::fill(uv.begin(), uv.end(), gfxm::vec2(.0f, .0f));

    g_vertices.insert(g_vertices.end(), vertices, vertices + vertex_count);
    g_colors.insert(g_colors.end(), colors, colors + vertex_count);
    g_uv.insert(g_uv.end(), uv.begin(), uv.end());

    return draw_commands.back();
}

GuiDrawCmd& guiDrawTriangleStrip(
    const gfxm::vec3* vertices,
    const uint32_t* colors,
    const gfxm::vec2* uvs,
    int vertex_count
) {
    GuiDrawCmd cmd;
    cmd.cmd = GUI_DRAW_TRIANGLE_STRIP;
    cmd.vertex_first = g_vertices.size();
    cmd.vertex_count = vertex_count;
    cmd.index_count = 0;
    cmd.index_first = 0;
    cmd.view_transform = guiGetViewTransform();
    cmd.projection = guiGetCurrentProjection();
    cmd.model_transform = gfxm::mat4(1.0f);
    cmd.color = 0xFFFFFFFF;
    cmd.tex0 = _guiGetTextureWhite();
    cmd.scissor_rect = guiDrawGetCurrentScissor();
    cmd.viewport_rect = guiGetCurrentViewportRect();
    draw_commands.push_back(cmd);

    g_vertices.insert(g_vertices.end(), vertices, vertices + vertex_count);
    g_colors.insert(g_colors.end(), colors, colors + vertex_count);
    g_uv.insert(g_uv.end(), uvs, uvs + vertex_count);

    return draw_commands.back();
}

GuiDrawCmd& guiDrawTriangleStrip(
    const gfxm::vec3* vertices,
    const uint32_t* colors,
    int vertex_count
) {
    std::vector<gfxm::vec2> uv;
    uv.resize(vertex_count);
    std::fill(uv.begin(), uv.end(), gfxm::vec2(.0f, .0f));

    return guiDrawTriangleStrip(vertices, colors, uv.data(), vertex_count);
}

GuiDrawCmd& guiDrawTriangleStrip(const gfxm::vec3* vertices, int vertex_count, uint32_t color) {
    std::vector<uint32_t> colors;
    colors.resize(vertex_count);
    std::fill(colors.begin(), colors.end(), color);
    std::vector<gfxm::vec2> uv;
    uv.resize(vertex_count);
    std::fill(uv.begin(), uv.end(), gfxm::vec2(.0f, .0f));

    return guiDrawTriangleStrip(vertices, colors.data(), uv.data(), vertex_count);
}

GuiDrawCmd& guiDrawTrianglesIndexed(
    const gfxm::vec3* vertices,
    int vertex_count,
    uint32_t* indices,
    int index_count,
    uint32_t color
) {
    GuiDrawCmd cmd;
    cmd.cmd = GUI_DRAW_TRIANGLES_INDEXED;
    cmd.vertex_first = g_vertices.size();
    cmd.vertex_count = vertex_count;
    cmd.index_first = g_indices.size();
    cmd.index_count = index_count;
    cmd.view_transform = guiGetViewTransform();
    cmd.projection = guiGetCurrentProjection();
    cmd.model_transform = gfxm::mat4(1.0f);
    cmd.color = color;
    cmd.tex0 = _guiGetTextureWhite();
    cmd.scissor_rect = guiDrawGetCurrentScissor();
    cmd.viewport_rect = guiGetCurrentViewportRect();
    draw_commands.push_back(cmd);

    std::vector<uint32_t> colors;
    colors.resize(vertex_count);
    std::fill(colors.begin(), colors.end(), color);
    std::vector<gfxm::vec2> uvs;
    uvs.resize(vertex_count);
    std::fill(uvs.begin(), uvs.end(), gfxm::vec2(.0f, .0f));

    std::vector<uint32_t> indices_;
    indices_.resize(index_count);
    for (int i = 0; i < indices_.size(); ++i) {
        indices_[i] = indices[i] + g_vertices.size();
    }
    g_indices.insert(g_indices.end(), indices_.begin(), indices_.end());
    g_vertices.insert(g_vertices.end(), vertices, vertices + vertex_count);    
    g_colors.insert(g_colors.end(), colors.begin(), colors.end());
    g_uv.insert(g_uv.end(), uvs.begin(), uvs.end());

    return draw_commands.back();
}

GuiDrawCmd& guiDrawLineStrip(
    const gfxm::vec3* vertices,
    const uint32_t* colors,
    int vertex_count
) {
    GuiDrawCmd cmd;
    cmd.cmd = GUI_DRAW_LINE_STRIP;
    cmd.vertex_first = g_vertices.size();
    cmd.vertex_count = vertex_count;
    cmd.index_first = 0;
    cmd.index_count = 0;
    cmd.view_transform = guiGetViewTransform();
    cmd.projection = guiGetCurrentProjection();
    cmd.model_transform = gfxm::mat4(1.0f);
    cmd.color = 0xFFFFFFFF;
    cmd.tex0 = _guiGetTextureWhite();
    cmd.scissor_rect = guiDrawGetCurrentScissor();
    cmd.viewport_rect = guiGetCurrentViewportRect();
    draw_commands.push_back(cmd);

    std::vector<gfxm::vec2> uvs;
    uvs.resize(vertex_count);
    std::fill(uvs.begin(), uvs.end(), gfxm::vec2(.0f, .0f));

    g_vertices.insert(g_vertices.end(), vertices, vertices + vertex_count);
    g_colors.insert(g_colors.end(), colors, colors + vertex_count);
    g_uv.insert(g_uv.end(), uvs.begin(), uvs.end());
    return draw_commands.back();
}
GuiDrawCmd& guiDrawLines(
    const gfxm::vec3* vertices,
    const uint32_t* colors,
    int vertex_count
) {
    GuiDrawCmd cmd;
    cmd.cmd = GUI_DRAW_LINES;
    cmd.vertex_first = g_vertices.size();
    cmd.vertex_count = vertex_count;
    cmd.index_first = 0;
    cmd.index_count = 0;
    cmd.view_transform = guiGetViewTransform();
    cmd.projection = guiGetCurrentProjection();
    cmd.model_transform = gfxm::mat4(1.0f);
    cmd.color = 0xFFFFFFFF;
    cmd.tex0 = _guiGetTextureWhite();
    cmd.scissor_rect = guiDrawGetCurrentScissor();
    cmd.viewport_rect = guiGetCurrentViewportRect();
    draw_commands.push_back(cmd);

    std::vector<gfxm::vec2> uvs;
    uvs.resize(vertex_count);
    std::fill(uvs.begin(), uvs.end(), gfxm::vec2(.0f, .0f));

    g_vertices.insert(g_vertices.end(), vertices, vertices + vertex_count);
    g_colors.insert(g_colors.end(), colors, colors + vertex_count);
    g_uv.insert(g_uv.end(), uvs.begin(), uvs.end());
    return draw_commands.back();
}

GuiDrawCmd& guiDrawTextHighlight(
    const gfxm::vec3* vertices,
    int vertex_count,
    uint32_t* indices,
    int index_count,
    uint32_t color
) {
    GuiDrawCmd cmd;
    cmd.cmd = GUI_DRAW_TEXT_HIGHLIGHT;
    cmd.vertex_first = g_vertices.size();
    cmd.vertex_count = vertex_count;
    cmd.index_first = g_indices.size();
    cmd.index_count = index_count;
    cmd.view_transform = guiGetViewTransform();
    cmd.projection = guiGetCurrentProjection();
    cmd.model_transform = gfxm::mat4(1.0f);
    cmd.color = color;
    cmd.tex0 = _guiGetTextureWhite();
    cmd.scissor_rect = guiDrawGetCurrentScissor();
    cmd.viewport_rect = guiGetCurrentViewportRect();
    draw_commands.push_back(cmd);

    std::vector<uint32_t> indices_;
    indices_.resize(index_count);
    for (int i = 0; i < indices_.size(); ++i) {
        indices_[i] = indices[i] + g_vertices.size();
    }
    g_indices.insert(g_indices.end(), indices_.begin(), indices_.end());
    g_vertices.insert(g_vertices.end(), vertices, vertices + vertex_count);
    std::vector<uint32_t> colors;
    colors.resize(vertex_count);
    std::fill(colors.begin(), colors.end(), color);
    g_colors.insert(g_colors.end(), colors.begin(), colors.end());

    std::vector<gfxm::vec2> uvs;
    uvs.resize(vertex_count);
    std::fill(uvs.begin(), uvs.end(), gfxm::vec2(.0f, .0f));
    
    g_uv.insert(g_uv.end(), uvs.begin(), uvs.end());

    return draw_commands.back();
}

GuiDrawCmd& _guiDrawText(
    const gfxm::vec3* vertices,
    const gfxm::vec2* uv,
    const uint32_t* colors,
    const float* uv_lookup,
    int vertex_count,
    uint32_t* indices,
    int index_count,
    uint32_t color,
    GLuint atlas,
    GLuint lut,
    int lut_width
) {
    GuiDrawCmd cmd;
    cmd.cmd = GUI_DRAW_TEXT;
    cmd.vertex_first = g_text_vertices.size();
    cmd.vertex_count = vertex_count;
    cmd.index_first = g_text_indices.size();
    cmd.index_count = index_count;
    cmd.view_transform = guiGetViewTransform();
    cmd.projection = guiGetCurrentProjection();
    cmd.model_transform = gfxm::mat4(1.0f);
    cmd.color = color;
    cmd.tex0 = atlas;
    cmd.tex1 = lut;
    cmd.usr0 = lut_width;
    cmd.scissor_rect = guiDrawGetCurrentScissor();
    cmd.viewport_rect = guiGetCurrentViewportRect();
    draw_commands.push_back(cmd);

    std::vector<uint32_t> indices_;
    indices_.resize(index_count);
    for (int i = 0; i < indices_.size(); ++i) {
        indices_[i] = indices[i] + g_text_vertices.size();
    }
    g_text_vertices.insert(g_text_vertices.end(), vertices, vertices + vertex_count);
    g_text_uv.insert(g_text_uv.end(), uv, uv + vertex_count);
    g_text_colors.insert(g_text_colors.end(), colors, colors + vertex_count);
    g_text_uv_lookup.insert(g_text_uv_lookup.end(), uv_lookup, uv_lookup + vertex_count);
    g_text_indices.insert(g_text_indices.end(), indices_.begin(), indices_.end());

    return draw_commands.back();
}


static std::stack<gfxm::mat4> transform_stack;
void guiPushTransform(const gfxm::vec3& pos) {
    guiPushTransform(gfxm::translate(gfxm::mat4(1.f), pos));
}
void guiPushTransform(const gfxm::mat4& tr) {
    transform_stack.push(tr);
}
void guiPopTransform() {
    assert(!transform_stack.empty());
    transform_stack.pop();
}
const gfxm::mat4&   guiGetCurrentTransform() {
    static gfxm::mat4 def(1.f);
    if (transform_stack.empty()) {
        return def;
    } else {
        return transform_stack.top();
    }
}

static std::stack<gfxm::mat4> view_tr_stack;
void guiPushViewTransform(const gfxm::mat4& tr) {
    view_tr_stack.push(tr * guiGetCurrentTransform());
}
void guiPopViewTransform() {
    assert(!view_tr_stack.empty());
    view_tr_stack.pop();
}
void guiClearViewTransform() {
    while (!view_tr_stack.empty()) {
        view_tr_stack.pop();
    }
}
const gfxm::mat4& guiGetViewTransform() {
    static gfxm::mat4 def(1.f);
    if (view_tr_stack.empty()) {
        return def;
    } else {
        return view_tr_stack.top();
    }
}

static gfxm::mat4 projection_default;
static std::stack<gfxm::mat4> projection_stack;
void guiPushProjection(const gfxm::mat4& p) {
    projection_stack.push(p);
}
void guiPushProjectionOrthographic(float left, float right, float bottom, float top) {
    guiPushProjection(gfxm::ortho(left, right, bottom, top, .0f, 100.f));
}
void guiPushProjectionPerspective(float fov_deg, float width, float height, float znear, float zfar) {
    guiPushProjection(gfxm::perspective(gfxm::radian(fov_deg), width / height, znear, zfar));
}
void guiPopProjection() {
    assert(!projection_stack.empty());
    if (!projection_stack.empty()) {
        projection_stack.pop();
    }
}
void guiClearProjection() {
    while (!projection_stack.empty()) {
        projection_stack.pop();
    }
}
const gfxm::mat4& guiGetCurrentProjection() {
    if (projection_stack.empty()) {
        return projection_default;
    } else {
        return projection_stack.top();
    }
}
void guiSetDefaultProjection(const gfxm::mat4& p) {
    projection_default = p;
}
const gfxm::mat4& guiGetDefaultProjection() {
    return projection_default;
}


static gfxm::rect viewport_rc_default;
static std::stack<gfxm::rect> viewport_rc_stack;
void guiPushViewportRect(const gfxm::rect& rc) {
    viewport_rc_stack.push(rc);
}
void guiPushViewportRect(float minx, float miny, float maxx, float maxy) {
    viewport_rc_stack.push(gfxm::rect(gfxm::vec2(minx, miny), gfxm::vec2(maxx, maxy)));
}
void guiPopViewportRect() {
    assert(!viewport_rc_stack.empty());
    if (!viewport_rc_stack.empty()) {
        viewport_rc_stack.pop();
    }
}
void guiClearViewportRectStack() {
    while (!viewport_rc_stack.empty()) {
        viewport_rc_stack.pop();
    }
}
const gfxm::rect& guiGetCurrentViewportRect() {
    if (viewport_rc_stack.empty()) {
        return viewport_rc_default;
    } else {
        return viewport_rc_stack.top();
    }
}
void guiSetDefaultViewportRect(const gfxm::rect& rc) {
    viewport_rc_default = rc;
}
const gfxm::rect& guiGetDefaultViewportRect() {
    return viewport_rc_default;
}


static std::stack<gfxm::rect> scissor_stack;
static gfxm::rect current_scissor_rect;

void guiDrawPushScissorRect(const gfxm::rect& rect) {
    gfxm::rect current = guiDrawGetCurrentScissor();
    gfxm::rect rc = rect;
    rc.min.x = gfxm::_max(current.min.x, rc.min.x);
    rc.min.y = gfxm::_max(current.min.y, rc.min.y);
    rc.max.x = gfxm::_min(current.max.x, rc.max.x);
    rc.max.y = gfxm::_min(current.max.y, rc.max.y);
    scissor_stack.push(rc);
    current_scissor_rect = rc;
}
void guiDrawPushScissorRect(float minx, float miny, float maxx, float maxy) {
    guiDrawPushScissorRect(gfxm::rect(minx, miny, maxx, maxy));
}
void guiDrawPopScissorRect() {
    int sw = 0, sh = 0;
    platformGetWindowSize(sw, sh);

    scissor_stack.pop();
    if (!scissor_stack.empty()) {
        gfxm::rect rc = scissor_stack.top();
        current_scissor_rect = rc;
    } else {
        current_scissor_rect = gfxm::rect(gfxm::vec2(0, 0), gfxm::vec2(sw, sh));
    }
}

const gfxm::rect& guiDrawGetCurrentScissor() {
    if (scissor_stack.empty()) {
        int sw = 0, sh = 0;
        platformGetWindowSize(sw, sh);
        current_scissor_rect = gfxm::rect(gfxm::vec2(0, 0), gfxm::vec2(sw, sh));
    }
    return current_scissor_rect;
}

#include "math/bezier.hpp"
void guiDrawBezierCurve(
    const gfxm::vec2& a, const gfxm::vec2& b,
    const gfxm::vec2& c, const gfxm::vec2& d,
    float thickness, uint32_t col, const gfxm::vec2& zoom_factor
) {
    int screen_w = 0, screen_h = 0;
    platformGetWindowSize(screen_w, screen_h);
    gfxm::vec3 zf = gfxm::vec3(zoom_factor, 1.f);

    int segments = 32;
    std::vector<gfxm::vec3> vertices;
    gfxm::vec3 p_last = bezierCubic_(
        gfxm::vec3(a.x, a.y, .0f), gfxm::vec3(b.x, b.y, .0f),
        gfxm::vec3(c.x, c.y, .0f), gfxm::vec3(d.x, d.y, .0f), .0f
    );
    gfxm::vec3 cross;
    for (int i = 1; i <= segments; ++i) {
        gfxm::vec3 p = bezierCubic_(
            gfxm::vec3(a.x, a.y, .0f), gfxm::vec3(b.x, b.y, .0f),
            gfxm::vec3(c.x, c.y, .0f), gfxm::vec3(d.x, d.y, .0f), i / (float)segments
        );
        cross = gfxm::cross(gfxm::normalize(p - p_last), gfxm::vec3(0, 0, 1));
        gfxm::vec3 pt0 = gfxm::vec3(p_last + cross * thickness * zf * .5f);
        gfxm::vec3 pt1 = gfxm::vec3(p_last - cross * thickness * zf * .5f);
        p_last = p;
        vertices.push_back(pt0);
        vertices.push_back(pt1);
    }
    gfxm::vec3 pt0 = gfxm::vec3(p_last + cross * thickness * zf * .5f);
    gfxm::vec3 pt1 = gfxm::vec3(p_last - cross * thickness * zf * .5f);
    vertices.push_back(pt0);
    vertices.push_back(pt1);

    guiDrawTriangleStrip(vertices.data(), vertices.size(), col);
}
void guiDrawCurveSimple(const gfxm::vec2& from, const gfxm::vec2& to, float thickness, uint32_t col) {    
    int segments = 32;
    std::vector<gfxm::vec3> vertices;
    gfxm::vec3 a_last = bezierCubic(
        gfxm::vec3(from.x, from.y, .0f), gfxm::vec3(to.x, to.y, .0f),
        gfxm::vec3((to.x - from.x) * .5f, 0, 0), gfxm::vec3((from.x - to.x) * .5f, 0, 0), .0f
    );
    gfxm::vec3 cross;
    for (int i = 1; i <= segments; ++i) {
        gfxm::vec3 a = bezierCubic(
            gfxm::vec3(from.x, from.y, .0f), gfxm::vec3(to.x, to.y, .0f),
            gfxm::vec3((to.x - from.x) * .5f, 0, 0), gfxm::vec3((from.x - to.x) * .5f, 0, 0), i / (float)segments
        );
        cross = gfxm::cross(gfxm::normalize(a - a_last), gfxm::vec3(0, 0, 1));
        gfxm::vec3 pt0 = gfxm::vec3(a_last + cross * thickness * .5f);
        gfxm::vec3 pt1 = gfxm::vec3(a_last - cross * thickness * .5f);
        a_last = a;
        vertices.push_back(pt0);
        vertices.push_back(pt1);
    }
    gfxm::vec3 pt0 = gfxm::vec3(a_last + cross * thickness * .5f);
    gfxm::vec3 pt1 = gfxm::vec3(a_last - cross * thickness * .5f);
    vertices.push_back(pt0);
    vertices.push_back(pt1);

    guiDrawTriangleStrip(vertices.data(), vertices.size(), col);
}
void guiDrawLineWithArrow(const gfxm::vec2& from, const gfxm::vec2& to, float thickness, uint32_t col) {  
    guiDrawLine(gfxm::rect(from, to), thickness, col);
    // Arrow
    {
        gfxm::vec3 a = gfxm::vec3(gfxm::lerp(from, to, .6f), .0f);
        gfxm::vec3 N = gfxm::normalize(gfxm::vec3(to, .0f) - a);
        gfxm::vec3 L = gfxm::cross(N, gfxm::vec3(0, 0, 1));
        gfxm::vec3 R = gfxm::cross(gfxm::vec3(0, 0, 1), N);
        gfxm::vec3 vertices[] = {
            a + N * thickness * 4.f,
            a + L * thickness * 2.f,
            a + R * thickness * 2.f
        };
        uint32_t colors[] = {
            col, col, col
        };
        guiDrawTriangles(vertices, colors, 0, 3);
    }
}

void guiDrawCircle(const gfxm::vec2& pos, float radius, bool is_filled, uint32_t col) {
    int screen_w = 0, screen_h = 0;
    platformGetWindowSize(screen_w, screen_h);

    float inner_radius = radius * .7f;
    if (is_filled) {
        inner_radius = .0f;
    }
    int segments = 16;
    std::vector<gfxm::vec3> vertices;
    for (int i = 0; i <= segments; ++i) {
        float a = (i / (float)segments) * gfxm::pi * 2.f;
        gfxm::vec3 pt_outer = gfxm::vec3(
            cosf(a), sinf(a), .0f
        ) * radius;
        gfxm::vec3 pt_inner = gfxm::vec3(
            cosf(a), sinf(a), .0f
        ) * inner_radius;
        vertices.push_back(pt_outer);
        vertices.push_back(pt_inner);
    }

    guiDrawTriangleStrip(vertices.data(), vertices.size(), col)
        .model_transform = gfxm::translate(gfxm::mat4(1.0f), gfxm::vec3(pos.x, pos.y, .0f));
}


void guiDrawDiamond(const gfxm::vec2& POS, float radius, uint32_t col0, const gfxm::vec2& zoom_factor) {
    guiDrawDiamond(POS, radius, col0, col0, col0, zoom_factor);
}
void guiDrawDiamond(const gfxm::vec2& pos, float radius, uint32_t col0, uint32_t col1, uint32_t col2, const gfxm::vec2& zoom_factor) {
    gfxm::vec3 c = gfxm::vec3(pos.x, pos.y, .0f);
    gfxm::vec3 vertices[3 * 4] = {
        c,        
        c + gfxm::vec3(.0f, -radius * zoom_factor.y, .0f),
        c + gfxm::vec3(-radius * zoom_factor.x, .0f, .0f),
        c,
        c + gfxm::vec3(radius * zoom_factor.x, .0f, .0f),
        c + gfxm::vec3(.0f, -radius * zoom_factor.y, .0f),
        c,
        c + gfxm::vec3(.0f, radius * zoom_factor.y, .0f),
        c + gfxm::vec3(radius * zoom_factor.x, .0f, .0f),
        c,
        c + gfxm::vec3(.0f, radius * zoom_factor.y, .0f),
        c + gfxm::vec3(-radius * zoom_factor.x, .0f, .0f)
    };
    uint32_t colors[3 * 4] = {
        col0, col0, col0,
        col1, col1, col1,
        col2, col2, col2,
        col1, col1, col1
    };
    gfxm::vec2 uv[3 * 4] = {
        gfxm::vec2(.0f, .0f),
        gfxm::vec2(.0f, .0f),
        gfxm::vec2(.0f, .0f),
        gfxm::vec2(.0f, .0f),
        gfxm::vec2(.0f, .0f),
        gfxm::vec2(.0f, .0f),
        gfxm::vec2(.0f, .0f),
        gfxm::vec2(.0f, .0f),
        gfxm::vec2(.0f, .0f),
        gfxm::vec2(.0f, .0f),
        gfxm::vec2(.0f, .0f),
        gfxm::vec2(.0f, .0f)
    };

    guiDrawTriangles(
        vertices,
        colors,
        uv,
        3 * 4
    );
}

void guiDrawRectShadow(const gfxm::rect& rc, uint32_t col) {
    gfxm::vec2 offs = gfxm::vec2(5.f, 5.f);

    gfxm::rect rc_ = gfxm::rect(rc.min + offs, rc.max + offs);
    gfxm::rect rco = rc_;
    gfxm::expand(rco, 5.f);
    gfxm::expand(rc_, -5.f);
    guiDrawRect(rc_, col);
    guiDrawRectRoundBorder(rco, 10.f, 10.f, col & 0x00FFFFFF, col);
}

void guiDrawRect(const gfxm::rect& rect, uint32_t col) {
    int screen_w = 0, screen_h = 0;
    platformGetWindowSize(screen_w, screen_h);

    gfxm::rect rct = rect;

    float vertices[] = {
        rct.min.x, rct.min.y, 0, rct.max.x, rct.min.y, 0,
        rct.min.x, rct.max.y, 0, rct.max.x, rct.max.y, 0
    };

    guiDrawTriangleStrip((gfxm::vec3*)vertices, 4, col);
}
void guiDrawRectGradient(const gfxm::rect& rect, uint32_t col_lt, uint32_t col_rt, uint32_t col_lb, uint32_t col_rb) {
    int screen_w = 0, screen_h = 0;
    platformGetWindowSize(screen_w, screen_h);

    gfxm::rect rct = rect;

    float vertices[] = {
        rct.min.x, rct.min.y, 0, rct.max.x, rct.min.y, 0,
        rct.min.x, rct.max.y, 0, rct.max.x, rct.max.y, 0
    };
    uint32_t colors[] = {
        col_lt, col_rt, col_lb, col_rb
    };

    guiDrawTriangleStrip((gfxm::vec3*)vertices, colors, 4);
}

void guiDrawRectRound(const gfxm::rect& rc_, float radius, uint32_t col, uint8_t corner_flags) {
    guiDrawRectRound(rc_, radius, radius, radius, radius, col, corner_flags);
}
void guiDrawRectRound(
    const gfxm::rect& rc,
    float radius_nw, float radius_ne, float radius_sw, float radius_se,
    uint32_t col, uint8_t corner_flags
) {
    int screen_w = 0, screen_h = 0;
    platformGetWindowSize(screen_w, screen_h);

    int segments = 16;
    std::vector<gfxm::vec3> vertices;
    std::vector<uint32_t> colors;

    float rc_width = rc.max.x - rc.min.x;
    float rc_height = rc.max.y - rc.min.y;
    float min_side = gfxm::_min(rc_width, rc_height);
    float half_min_side = min_side * .5f;

    radius_nw = gfxm::_min(radius_nw, half_min_side);
    radius_ne = gfxm::_min(radius_ne, half_min_side);
    radius_se = gfxm::_min(radius_se, half_min_side);
    radius_sw = gfxm::_min(radius_sw, half_min_side);

    std::array<gfxm::vec3, 4> corners = {
        gfxm::vec3(rc.min.x + radius_nw, rc.min.y + radius_nw, .0f),
        gfxm::vec3(rc.max.x - radius_ne, rc.min.y + radius_ne, .0f),
        gfxm::vec3(rc.max.x - radius_se, rc.max.y - radius_se, .0f),
        gfxm::vec3(rc.min.x + radius_sw, rc.max.y - radius_sw, .0f)
    };

    float radius = radius_nw;
    gfxm::vec3 corner_pt = corners[0];
    gfxm::vec3 corner_offset(radius, radius, .0f);
    float radian_start = gfxm::pi;
    if (corner_flags & GUI_DRAW_CORNER_NW) {
        for (int i = 0; i <= segments; ++i) {
            float a = radian_start + (i / (float)segments) * gfxm::pi * .5f;
            gfxm::vec3 pt_outer = gfxm::vec3(
                cosf(a), sinf(a), .0f
            ) * radius + corner_pt;
            gfxm::vec3 pt_inner = corner_pt;
            vertices.push_back(pt_outer);
            vertices.push_back(pt_inner);
            colors.push_back(col);
            colors.push_back(col);
        }
    } else {
        vertices.push_back(gfxm::vec3(rc.min.x, rc.min.y, .0f));
        vertices.push_back(corner_pt);
        colors.push_back(col);
        colors.push_back(col);
    }

    radius = radius_ne;
    corner_pt = corners[1];
    corner_offset = gfxm::vec3(-radius, radius, .0f);
    radian_start += gfxm::pi * .5f;
    if (corner_flags & GUI_DRAW_CORNER_NE) {
        for (int i = 0; i <= segments; ++i) {
            float a = radian_start + (i / (float)segments) * gfxm::pi * .5f;
            gfxm::vec3 pt_outer = gfxm::vec3(
                cosf(a), sinf(a), .0f
            ) * radius + corner_pt;
            gfxm::vec3 pt_inner = corner_pt;
            vertices.push_back(pt_outer);
            vertices.push_back(pt_inner);
            colors.push_back(col);
            colors.push_back(col);
        }
    } else {
        vertices.push_back(gfxm::vec3(rc.max.x, rc.min.y, .0f));
        vertices.push_back(corner_pt);
        colors.push_back(col);
        colors.push_back(col);
    }

    radius = radius_se;
    corner_pt = corners[2];
    corner_offset = gfxm::vec3(-radius, -radius, .0f);
    radian_start += gfxm::pi * .5f;
    if (corner_flags & GUI_DRAW_CORNER_SE) {
        for (int i = 0; i <= segments; ++i) {
            float a = radian_start + (i / (float)segments) * gfxm::pi * .5f;
            gfxm::vec3 pt_outer = gfxm::vec3(
                cosf(a), sinf(a), .0f
            ) * radius + corner_pt;
            gfxm::vec3 pt_inner = corner_pt;
            vertices.push_back(pt_outer);
            vertices.push_back(pt_inner);
            colors.push_back(col);
            colors.push_back(col);
        }
    } else {
        vertices.push_back(gfxm::vec3(rc.max.x, rc.max.y, .0f));
        vertices.push_back(corner_pt);
        colors.push_back(col);
        colors.push_back(col);
    }

    radius = radius_sw;
    corner_pt = corners[3];
    corner_offset = gfxm::vec3(radius, -radius, .0f);
    radian_start += gfxm::pi * .5f;
    if (corner_flags & GUI_DRAW_CORNER_SW) {
        for (int i = 0; i <= segments; ++i) {
            float a = radian_start + (i / (float)segments) * gfxm::pi * .5f;
            gfxm::vec3 pt_outer = gfxm::vec3(
                cosf(a), sinf(a), .0f
            ) * radius + corner_pt;
            gfxm::vec3 pt_inner = corner_pt;
            vertices.push_back(pt_outer);
            vertices.push_back(pt_inner);
            colors.push_back(col);
            colors.push_back(col);
        }
    } else {
        vertices.push_back(gfxm::vec3(rc.min.x, rc.max.y, .0f));
        vertices.push_back(corner_pt);
        colors.push_back(col);
        colors.push_back(col);
    }
    vertices.push_back(vertices[0]);
    vertices.push_back(vertices[1]);
    vertices.push_back(corners[3]);
    vertices.push_back(corners[1]);
    vertices.push_back(corners[2]);
    colors.push_back(col);
    colors.push_back(col);
    colors.push_back(col);
    colors.push_back(col);
    colors.push_back(col);

    guiDrawTriangleStrip(vertices.data(), vertices.size(), col);
}

void guiDrawRectRoundBorder(const gfxm::rect& rc_, float radius, float thickness, uint32_t col_a, uint32_t col_b, uint8_t corner_flags) {
    guiDrawRectRoundBorder(rc_, radius, radius, radius, radius, thickness, thickness, thickness, thickness, col_a, col_a, col_a, col_a, corner_flags);
}

void guiDrawRectRoundBorder(
    const gfxm::rect& rc_,
    float radius_top_left, float radius_top_right,
    float radius_bottom_left, float radius_bottom_right,
    float thickness_left, float thickness_top, float thickness_right, float thickness_bottom,
    uint32_t col_left, uint32_t col_top, uint32_t col_right, uint32_t col_bottom, uint8_t corner_flags
) {
    int screen_w = 0, screen_h = 0;
    platformGetWindowSize(screen_w, screen_h);

    gfxm::rect rc = rc_;
    int segments = 16;
    std::vector<gfxm::vec3> vertices;
    std::vector<uint32_t> colors;
    
    float rc_width = rc.max.x - rc.min.x;
    float rc_height = rc.max.y - rc.min.y;
    float min_side = gfxm::_min(rc_width, rc_height);
    float half_min_side = min_side * .5f;

    radius_top_left = gfxm::_min(radius_top_left, half_min_side);
    radius_top_right = gfxm::_min(radius_top_right, half_min_side);
    radius_bottom_left = gfxm::_min(radius_bottom_left, half_min_side);
    radius_bottom_right = gfxm::_min(radius_bottom_right, half_min_side);

    float radius = radius_top_left;
    float inner_radius_a = radius - thickness_left;
    float inner_radius_b = radius - thickness_top;
    gfxm::vec3 corner_pt(rc.min.x + radius, rc.min.y + radius, .0f);
    gfxm::vec3 inner_corner_pt = corner_pt + gfxm::vec3(thickness_left, thickness_top, .0f);
    float radian_start = gfxm::pi;
    for (int i = 0; i <= segments; ++i) {
        float a = radian_start + (i / (float)segments) * gfxm::pi * .5f;
        gfxm::vec3 pt_outer = gfxm::vec3(
            cosf(a), sinf(a), .0f
        ) * radius + corner_pt;
        gfxm::vec3 pt_inner = gfxm::vec3(
            cosf(a) * inner_radius_a, sinf(a) * inner_radius_b, .0f
        ) + corner_pt;
        vertices.push_back(pt_outer);
        vertices.push_back(pt_inner);
        uint32_t col = gfxm::lerp_color(col_left, col_top, (i / (float)segments));
        colors.push_back(col);
        colors.push_back(col);
    }

    radius = radius_top_right;
    inner_radius_a = radius - thickness_top;
    inner_radius_b = radius - thickness_right;
    corner_pt = gfxm::vec3(rc.max.x - radius, rc.min.y + radius, .0f);
    radian_start += gfxm::pi * .5f;
    for (int i = 0; i <= segments; ++i) {
        float a = radian_start + (i / (float)segments) * gfxm::pi * .5f;
        gfxm::vec3 pt_outer = gfxm::vec3(
            cosf(a), sinf(a), .0f
        ) * radius + corner_pt;
        gfxm::vec3 pt_inner = gfxm::vec3(
            cosf(a) * inner_radius_b, sinf(a) * inner_radius_a, .0f
        ) + corner_pt;
        vertices.push_back(pt_outer);
        vertices.push_back(pt_inner);
        uint32_t col = gfxm::lerp_color(col_top, col_right, (i / (float)segments));
        colors.push_back(col);
        colors.push_back(col);
    }

    radius = radius_bottom_right;
    inner_radius_a = radius - thickness_right;
    inner_radius_b = radius - thickness_bottom;
    corner_pt = gfxm::vec3(rc.max.x - radius, rc.max.y - radius, .0f);
    radian_start += gfxm::pi * .5f;
    for (int i = 0; i <= segments; ++i) {
        float a = radian_start + (i / (float)segments) * gfxm::pi * .5f;
        gfxm::vec3 pt_outer = gfxm::vec3(
            cosf(a), sinf(a), .0f
        ) * radius + corner_pt;
        gfxm::vec3 pt_inner = gfxm::vec3(
            cosf(a) * inner_radius_a, sinf(a) * inner_radius_b, .0f
        ) + corner_pt;
        vertices.push_back(pt_outer);
        vertices.push_back(pt_inner);
        uint32_t col = gfxm::lerp_color(col_right, col_bottom, (i / (float)segments));
        colors.push_back(col);
        colors.push_back(col);
    }

    radius = radius_bottom_left;
    inner_radius_a = radius - thickness_bottom;
    inner_radius_b = radius - thickness_left;
    corner_pt = gfxm::vec3(rc.min.x + radius, rc.max.y - radius, .0f);
    radian_start += gfxm::pi * .5f;
    for (int i = 0; i <= segments; ++i) {
        float a = radian_start + (i / (float)segments) * gfxm::pi * .5f;
        gfxm::vec3 pt_outer = gfxm::vec3(
            cosf(a), sinf(a), .0f
        ) * radius + corner_pt;
        gfxm::vec3 pt_inner = gfxm::vec3(
            cosf(a) * inner_radius_b, sinf(a) * inner_radius_a, .0f
        ) + corner_pt;
        vertices.push_back(pt_outer);
        vertices.push_back(pt_inner);
        uint32_t col = gfxm::lerp_color(col_bottom, col_left, (i / (float)segments));
        colors.push_back(col);
        colors.push_back(col);
    }
    vertices.push_back(vertices[0]);
    vertices.push_back(vertices[1]);
    colors.push_back(colors[0]);
    colors.push_back(colors[1]);

    guiDrawTriangleStrip(
        vertices.data(),
        colors.data(),
        vertices.size()
    );
}

void guiDrawRectTextured(const gfxm::rect& rect, gpuTexture2d* texture, uint32_t col) {
    int screen_w = 0, screen_h = 0;
    platformGetWindowSize(screen_w, screen_h);

    gfxm::rect rct = rect;

    float vertices[] = {
        rct.min.x, rct.min.y, 0, rct.max.x, rct.min.y, 0,
        rct.min.x, rct.max.y, 0, rct.max.x, rct.max.y, 0
    };
    uint32_t colors[] = {
        col,      col,
        col,      col
    };
    float uvs[] = {
        .0f, 1.f,   1.f, 1.f,
        .0f, .0f,   1.f, .0f
    };

    guiDrawTriangleStrip(
        (gfxm::vec3*)vertices,
        colors,
        (gfxm::vec2*)uvs,
        sizeof(vertices) / sizeof(vertices[0]) / 3
    ).tex0 = texture->getId();
}

gfxm::vec3 hsv2rgb_gui(float H, float S, float V) {
    if (H > 360 || H < 0 || S>100 || S < 0 || V>100 || V < 0) {
        // TODO: assert?
        return gfxm::vec3(0, 0, 0);
    }
    float s = S / 100.0f;
    float v = V / 100.0f;
    float C = s * v;
    float X = C * (1 - abs(fmod(H / 60.0, 2) - 1));
    float m = v - C;
    float r, g, b;
    if (H >= 0 && H < 60) {
        r = C, g = X, b = 0;
    }
    else if (H >= 60 && H < 120) {
        r = X, g = C, b = 0;
    }
    else if (H >= 120 && H < 180) {
        r = 0, g = C, b = X;
    }
    else if (H >= 180 && H < 240) {
        r = 0, g = X, b = C;
    }
    else if (H >= 240 && H < 300) {
        r = X, g = 0, b = C;
    }
    else {
        r = C, g = 0, b = X;
    }
    int R = (r + m) * 255;
    int G = (g + m) * 255;
    int B = (b + m) * 255;

    return gfxm::vec3(R / 255.0f, G / 255.0f, B / 255.0f);
}
void guiDrawColorWheel(const gfxm::rect& rect) {
    const gfxm::vec2 center = rect.min + (rect.max - rect.min) * .5f;
    const float radius = gfxm::_min(rect.max.x - rect.min.x, rect.max.y - rect.min.y) * .5f;
    int screen_w = 0, screen_h = 0;
    platformGetWindowSize(screen_w, screen_h);

    const int n_points = 32;

    float vertices[(n_points + 1) * 3];
    memset(vertices, 0, sizeof(vertices));
    vertices[0] = center.x;
    vertices[1] = center.y;
    uint32_t colors[(n_points + 1)];
    colors[0] = 0xFFFFFFFF;

    for (int i = 1; i < n_points + 1; ++i) {
        float v = (i - 1) / (float)(n_points - 1);
        float vpi = v * gfxm::pi * 2.0f;
        vertices[i * 3]     = center.x + sinf(vpi) * radius;
        vertices[i * 3 + 1] = center.y - cosf(vpi) * radius;

        gfxm::vec3 col = hsv2rgb_gui(v * 360.0f, 100.0f, 100.0f);
        char R = col.x * 255.0f;
        char G = col.y * 255.0f;
        char B = col.z * 255.0f;
        uint32_t color = 0xFF000000;
        color |= 0x000000FF & R;
        color |= 0x0000FF00 & (G << 8);
        color |= 0x00FF0000 & (B << 16);

        colors[i] = color;
    }

    guiDrawTriangleFan(
        (gfxm::vec3*)vertices,
        colors,
        n_points + 1
    );
}
void guiDrawCheckBox(const gfxm::rect& rc, bool is_checked, bool is_hovered) {
    if (is_hovered) {
        guiDrawRect(rc, GUI_COL_BUTTON_HOVER);
    } else if(is_checked) {
        guiDrawRect(rc, GUI_COL_ACCENT);
    } else {
        guiDrawRect(rc, GUI_COL_HEADER);
    }
    guiDrawRectLine(rc, GUI_COL_BUTTON);
    
    if (is_checked) {
        float margin = (rc.max.y - rc.min.y) * .2f;
        gfxm::rect rc_small = rc;
        gfxm::expand(rc_small, -margin);
        float check_thickness = (rc_small.max.y - rc_small.min.y) * .4f;
        float b = (rc_small.max.y - rc_small.min.y) * .1f;
        float c = (rc_small.max.y - rc_small.min.y) * .3f;
        float center_x = rc_small.center().x - b;
        gfxm::vec3 vertices[] = {
            gfxm::vec3(rc_small.min.x, rc_small.min.y + c, .0f),
            gfxm::vec3(rc_small.min.x, rc_small.min.y + check_thickness + c, .0f),
            gfxm::vec3(center_x, rc_small.max.y - check_thickness, .0f),
            gfxm::vec3(center_x, rc_small.max.y, .0f),
            gfxm::vec3(rc_small.max.x, rc_small.min.y, .0f),
            gfxm::vec3(rc_small.max.x, rc_small.min.y + check_thickness, .0f)
        };
        guiDrawTriangleStrip(vertices, sizeof(vertices) / sizeof(vertices[0]), GUI_COL_TEXT);
    }
}

void guiDrawRectLine(const gfxm::rect& rect, uint32_t col) {
    int screen_w = 0, screen_h = 0;
    platformGetWindowSize(screen_w, screen_h);

    gfxm::rect rct = rect;

    float vertices[] = {
        rct.min.x, rct.min.y, 0, rct.max.x, rct.min.y, 0,
        rct.max.x, rct.max.y, 0, rct.min.x, rct.max.y, 0,
        rct.min.x, rct.min.y, 0
    };
    uint32_t colors[] = {
         col, col,
         col, col,
         col
    };

    guiDrawLineStrip(
        (gfxm::vec3*)vertices,
        colors,
        sizeof(vertices) / sizeof(vertices[0]) / 3
    );
}

void guiDrawLine(const gfxm::vec2& a, const gfxm::vec2& b, float thickness, uint32_t col, const gfxm::vec2& zoom_factor) {
    guiDrawLine(gfxm::rect(a, b), thickness, col, zoom_factor);
}
void guiDrawLine(const gfxm::rect& rc, float thickness, uint32_t col, const gfxm::vec2& zoom_factor) {
    gfxm::vec3 zf = gfxm::vec3(zoom_factor, 1.f);

    gfxm::vec3 A = gfxm::vec3(rc.min, .0f);
    gfxm::vec3 B = gfxm::vec3(rc.max, .0f);
    gfxm::vec3 dir = gfxm::vec3(rc.max - rc.min, .0f);
    gfxm::vec3 L = gfxm::normalize(gfxm::vec3(-dir.y, dir.x, .0f)) * thickness * zf * .5f;
    gfxm::vec3 R = gfxm::normalize(gfxm::vec3(dir.y, -dir.x, .0f)) * thickness * zf * .5f;

    gfxm::vec3 vertices[] = {
        A + R, A + L, B + L, B + R
    };
    guiDrawTriangleStrip(vertices, 4, col);
}
GuiDrawCmd& guiDrawLine3(const gfxm::vec3& a, const gfxm::vec3& b, uint32_t col) {
    gfxm::vec3 vertices[] = {
        a, b
    };
    uint32_t colors[] = {
        col, col
    };
    return guiDrawLineStrip(vertices, colors, sizeof(vertices) / sizeof(vertices[0]));
}
GuiDrawCmd& guiDrawLine3d2(const gfxm::vec3& a, const gfxm::vec3& b, uint32_t col0, uint32_t col1) {
    gfxm::vec3 vertices[] = {
        a, b
    };
    uint32_t colors[] = {
        col0, col1
    };
    return guiDrawLineStrip(vertices, colors, sizeof(vertices) / sizeof(vertices[0]));
}
GuiDrawCmd& guiDrawCircle3(float radius, uint32_t col) {
    const int segments = 32;
    const int vertex_count = (segments);
    gfxm::vec3 vertices[vertex_count];
    for (int i = 0; i < segments; ++i) {
        float a = (1.0f - (i - 1) / (float)(segments - 1)) * gfxm::pi * 2.f;
        gfxm::vec3 pt = gfxm::vec3(
            cosf(a), .0f, sinf(a)
        ) * radius;
        vertices[i] = pt;
    }
    uint32_t colors[vertex_count];
    for (int i = 0; i < vertex_count; ++i) {
        colors[i] = col;
    }

    return guiDrawLineStrip(vertices, colors, vertex_count);
}
GuiDrawCmd& guiDrawAABB(const gfxm::aabb& aabb, const gfxm::mat4& transform, uint32_t col) {
    gfxm::vec3 vertices[] = {
        gfxm::vec3(aabb.from.x, aabb.from.y, aabb.from.z), gfxm::vec3(aabb.from.x, aabb.to.y, aabb.from.z),
        gfxm::vec3(aabb.to.x, aabb.from.y, aabb.from.z), gfxm::vec3(aabb.to.x, aabb.to.y, aabb.from.z),
        gfxm::vec3(aabb.to.x, aabb.from.y, aabb.to.z), gfxm::vec3(aabb.to.x, aabb.to.y, aabb.to.z),
        gfxm::vec3(aabb.from.x, aabb.from.y, aabb.to.z), gfxm::vec3(aabb.from.x, aabb.to.y, aabb.to.z),

        gfxm::vec3(aabb.from.x, aabb.from.y, aabb.from.z), gfxm::vec3(aabb.to.x, aabb.from.y, aabb.from.z),
        gfxm::vec3(aabb.from.x, aabb.to.y, aabb.from.z), gfxm::vec3(aabb.to.x, aabb.to.y, aabb.from.z),
        gfxm::vec3(aabb.from.x, aabb.from.y, aabb.to.z), gfxm::vec3(aabb.to.x, aabb.from.y, aabb.to.z),
        gfxm::vec3(aabb.from.x, aabb.to.y, aabb.to.z), gfxm::vec3(aabb.to.x, aabb.to.y, aabb.to.z),

        gfxm::vec3(aabb.from.x, aabb.from.y, aabb.from.z), gfxm::vec3(aabb.from.x, aabb.from.y, aabb.to.z),
        gfxm::vec3(aabb.to.x, aabb.from.y, aabb.from.z), gfxm::vec3(aabb.to.x, aabb.from.y, aabb.to.z),
        gfxm::vec3(aabb.from.x, aabb.to.y, aabb.from.z), gfxm::vec3(aabb.from.x, aabb.to.y, aabb.to.z),
        gfxm::vec3(aabb.to.x, aabb.to.y, aabb.from.z), gfxm::vec3(aabb.to.x, aabb.to.y, aabb.to.z)
    };
    const int vertex_count = 24;
    uint32_t colors[vertex_count];
    for (int i = 0; i < vertex_count; ++i) {
        colors[i] = col;
    }
    auto& cmd = guiDrawLines(vertices, colors, vertex_count);
    cmd.model_transform = transform;
    return cmd;
}
GuiDrawCmd& guiDrawCone(float radius, float height, uint32_t color) {
    const int segments = 16;
    const int vertex_count = (segments + 1);
    gfxm::vec3 vertices[vertex_count];
    vertices[0] = gfxm::vec3(.0f, height, .0f);
    for (int i = 1; i < segments + 1; ++i) {
        float a = (1.0f - (i - 1) / (float)(segments - 1)) * gfxm::pi * 2.f;
        gfxm::vec3 pt = gfxm::vec3(
            cosf(a), .0f, sinf(a)
        ) * radius;
        vertices[i] = pt;
    }
    uint32_t colors[vertex_count];
    for (int i = 0; i < vertex_count; ++i) {
        colors[i] = color;
    }
    return guiDrawTriangleFan(vertices, colors, vertex_count);
}
GuiDrawCmd& guiDrawQuad3d(const gfxm::vec3& a, const gfxm::vec3& b, const gfxm::vec3& c, const gfxm::vec3& d, uint32_t col) {
    const gfxm::vec3 vertices[] = { a, b, c, d };
    uint32_t colors[] = { col, col, col, col };
    return guiDrawTriangleFan(vertices, colors, 4);
}
GuiDrawCmd& guiDrawPointSquare3d(const gfxm::vec3& pt, float side, uint32_t col) {
    const gfxm::rect& vprc = guiGetCurrentViewportRect();
    const gfxm::vec2 screen_size = vprc.max - vprc.min;
    float w = side / screen_size.x * .5f;
    float h = side / screen_size.y * .5f;
    const gfxm::vec3 offsets[] = {
        gfxm::vec4(-w, -h, .0f, .0f),
        gfxm::vec4(w, -h, .0f, .0f),
        gfxm::vec4(w, h, .0f, .0f),
        gfxm::vec4(-w, h, .0f, .0f)
    };
    gfxm::vec4 pt4 = guiGetCurrentProjection() * guiGetViewTransform() * gfxm::vec4(pt, 1.f);
    pt4 /= pt4.w;
    gfxm::vec3 ptt = pt4;
    gfxm::vec3 vertices[] = { 
        ptt + offsets[0], 
        ptt + offsets[1], 
        ptt + offsets[2], 
        ptt + offsets[3]
    };
    uint32_t colors[] = { col, col, col, col };
    return guiDrawTriangleFan(vertices, colors, 4, true);
}
GuiDrawCmd& guiDrawPolyConvex3d(const gfxm::vec3* vertices, size_t count, uint32_t col) {
    std::vector<uint32_t> colors(count);
    for (int i = 0; i < count; ++i) {
        colors[i] = col;
    }
    return guiDrawTriangleFan(vertices, colors.data(), count);
}

gfxm::vec2 guiCalcTextPosInRect(const gfxm::rect& rc_text, const gfxm::rect& rc, int alignment, const gfxm::rect& margin, Font* font) {
    gfxm::vec2 mid = rc.min + (rc.max - rc.min) * .5f;
    gfxm::vec2 pos(
        rc.min.x + GUI_MARGIN,
        mid.y - (font->getAscender() + font->getDescender()) * .5f
    );

    return pos;
}

void guiDrawText(const gfxm::vec2& pos, const char* text, Font* font, float max_width, uint32_t col, const gfxm::vec2& zoom_factor) {
    GuiTextBuffer text_buf;
    text_buf.replaceAll(font, text, strlen(text));
    text_buf.prepareDraw(font, false);
    text_buf.draw(font, pos, col, col, zoom_factor);
}
void guiDrawText(const gfxm::rect& rc, const char* text, Font* font, GUI_ALIGNMENT align, uint32_t col) {
    GuiTextBuffer text_buf;
    text_buf.replaceAll(font, text, strlen(text));
    text_buf.prepareDraw(font, false);
    text_buf.draw(font, rc, align, col, col);
}
