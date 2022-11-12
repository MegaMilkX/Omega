#include <stack>

#include "gui/gui_draw.hpp"

#include "platform/platform.hpp"

#include "gui/gui_color.hpp"
#include "gui/gui_values.hpp"


static std::stack<gfxm::mat4> view_tr_stack;
void guiPushViewTransform(const gfxm::mat4& tr) {
    view_tr_stack.push(tr);
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


static std::stack<gfxm::rect> scissor_stack;

void guiDrawPushScissorRect(const gfxm::rect& rect) {
    scissor_stack.push(rect);

    int sw = 0, sh = 0;
    platformGetWindowSize(sw, sh);
    glScissor(
        rect.min.x,
        sh - rect.max.y,
        rect.max.x - rect.min.x,
        rect.max.y - rect.min.y
    );
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
        glScissor(
            rc.min.x,
            sh - rc.max.y,
            rc.max.x - rc.min.x,
            rc.max.y - rc.min.y
        );
    } else {
        glScissor(0, 0, sw, sh);
    }
}

#include "math/bezier.hpp"
void guiDrawCurveSimple(const gfxm::vec2& from, const gfxm::vec2& to, float thickness, uint32_t col) {
    int screen_w = 0, screen_h = 0;
    platformGetWindowSize(screen_w, screen_h);
    
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

    std::vector<uint32_t> colors;
    colors.resize(vertices.size());
    std::fill(colors.begin(), colors.end(), col);

    gpuBuffer vertexBuffer;
    gpuBuffer colorBuffer;
    vertexBuffer.setArrayData(vertices.data(), vertices.size() * sizeof(vertices[0]));
    colorBuffer.setArrayData(colors.data(), colors.size() * sizeof(colors[0]));

    gpuShaderProgram* prog = _guiGetShaderRect();

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer.getId());
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ARRAY_BUFFER, colorBuffer.getId());
    glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, 0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    gfxm::mat4 model = gfxm::mat4(1.0f);
    gfxm::mat4 view = guiGetViewTransform();
    gfxm::mat4 proj = gfxm::ortho(.0f, (float)screen_w, (float)screen_h, .0f, .0f, 100.0f);

    glUseProgram(prog->getId());
    glUniformMatrix4fv(prog->getUniformLocation("matView"), 1, GL_FALSE, (float*)&view);
    glUniformMatrix4fv(prog->getUniformLocation("matProjection"), 1, GL_FALSE, (float*)&proj);
    glUniformMatrix4fv(prog->getUniformLocation("matModel"), 1, GL_FALSE, (float*)&model);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, (segments + 1) * 2);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
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
    std::vector<uint32_t> colors;
    colors.resize(vertices.size());
    std::fill(colors.begin(), colors.end(), col);

    gpuBuffer vertexBuffer;
    gpuBuffer colorBuffer;
    vertexBuffer.setArrayData(vertices.data(), vertices.size() * sizeof(vertices[0]));
    colorBuffer.setArrayData(colors.data(), colors.size() * sizeof(colors[0]));

    gpuShaderProgram* prog = _guiGetShaderRect();

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer.getId());
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ARRAY_BUFFER, colorBuffer.getId());
    glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, 0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    gfxm::mat4 model = gfxm::translate(gfxm::mat4(1.0f), gfxm::vec3(pos.x, pos.y, .0f));
    gfxm::mat4 view = guiGetViewTransform();
    gfxm::mat4 proj = gfxm::ortho(.0f, (float)screen_w, (float)screen_h, .0f, .0f, 100.0f);

    glUseProgram(prog->getId());
    glUniformMatrix4fv(prog->getUniformLocation("matView"), 1, GL_FALSE, (float*)&view);
    glUniformMatrix4fv(prog->getUniformLocation("matProjection"), 1, GL_FALSE, (float*)&proj);
    glUniformMatrix4fv(prog->getUniformLocation("matModel"), 1, GL_FALSE, (float*)&model);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, (segments + 1) * 2);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
}

void guiDrawRectShadow(const gfxm::rect& rc, uint32_t col) {
    int screen_w = 0, screen_h = 0;
    platformGetWindowSize(screen_w, screen_h);

    gfxm::rect rco = rc;
    gfxm::expand(rco, 10.f);

    float vertices[] = {
        rco.min.x, rco.min.y, .0f,
        rc.min.x, rc.min.y, .0f,
        rco.max.x, rco.min.y, .0f,
        rc.max.x, rc.min.y, .0f,
        rco.max.x, rco.max.y, .0f,
        rc.max.x, rc.max.y, .0f,
        rco.min.x, rco.max.y, .0f,
        rc.min.x, rc.max.y, .0f,
        rco.min.x, rco.min.y, .0f,
        rc.min.x, rc.min.y, .0f
    };
    uint32_t col_b = col;
    col_b &= ~(0xFF000000);
    uint32_t colors[] = {
        0x00000000,
        col,
        0x00000000,
        col,
        0x00000000,
        col,
        0x00000000,
        col,
        0x00000000,
        col
    };
    gpuBuffer vertexBuffer;
    vertexBuffer.setArrayData(vertices, sizeof(vertices));
    gpuBuffer colorBuffer;
    colorBuffer.setArrayData(colors, sizeof(colors));

    gpuShaderProgram* prog = _guiGetShaderRect();

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer.getId());
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ARRAY_BUFFER, colorBuffer.getId());
    glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, 0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    gfxm::mat4 model(1.0f);
    gfxm::mat4 view = guiGetViewTransform();
    gfxm::mat4 proj = gfxm::ortho(.0f, (float)screen_w, (float)screen_h, .0f, .0f, 100.0f);

    glUseProgram(prog->getId());
    glUniformMatrix4fv(prog->getUniformLocation("matView"), 1, GL_FALSE, (float*)&view);
    glUniformMatrix4fv(prog->getUniformLocation("matProjection"), 1, GL_FALSE, (float*)&proj);
    glUniformMatrix4fv(prog->getUniformLocation("matModel"), 1, GL_FALSE, (float*)&model);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 10);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
}

void guiDrawRect(const gfxm::rect& rect, uint32_t col) {
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
    gpuBuffer vertexBuffer;
    vertexBuffer.setArrayData(vertices, sizeof(vertices));
    gpuBuffer colorBuffer;
    colorBuffer.setArrayData(colors, sizeof(colors));

    gpuShaderProgram* prog = _guiGetShaderRect();

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer.getId());
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ARRAY_BUFFER, colorBuffer.getId());
    glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, 0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    gfxm::mat4 model(1.0f);
    gfxm::mat4 view = guiGetViewTransform();
    gfxm::mat4 proj = gfxm::ortho(.0f, (float)screen_w, (float)screen_h, .0f, .0f, 100.0f);

    glUseProgram(prog->getId());
    glUniformMatrix4fv(prog->getUniformLocation("matView"), 1, GL_FALSE, (float*)&view);
    glUniformMatrix4fv(prog->getUniformLocation("matProjection"), 1, GL_FALSE, (float*)&proj);
    glUniformMatrix4fv(prog->getUniformLocation("matModel"), 1, GL_FALSE, (float*)&model);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
}

void guiDrawRectRound(const gfxm::rect& rc_, float radius, uint32_t col, uint8_t corner_flags) {
    int screen_w = 0, screen_h = 0;
    platformGetWindowSize(screen_w, screen_h);

    gfxm::rect rc = rc_;
    gfxm::expand(rc, -radius);
    float inner_radius = .0f;
    radius *= 1.0f;
    int segments = 16;
    std::vector<gfxm::vec3> vertices;
    std::vector<uint32_t> colors;
    gfxm::vec3 corner_pt(rc.min.x, rc.min.y, .0f);
    gfxm::vec3 corner_offset(radius, radius, .0f);
    float radian_start = gfxm::pi;
    if (corner_flags & GUI_DRAW_CORNER_NW) {
        for (int i = 0; i <= segments; ++i) {
            float a = radian_start + (i / (float)segments) * gfxm::pi * .5f;
            gfxm::vec3 pt_outer = gfxm::vec3(
                cosf(a), sinf(a), .0f
            ) * radius + corner_pt;
            gfxm::vec3 pt_inner = gfxm::vec3(
                cosf(a), sinf(a), .0f
            ) * inner_radius + corner_pt;
            vertices.push_back(pt_outer);
            vertices.push_back(pt_inner);
            colors.push_back(col);
            colors.push_back(col);
        }
    } else {
        vertices.push_back(gfxm::vec3(rc_.min.x, rc_.min.y, .0f));
        vertices.push_back(corner_pt);
        colors.push_back(col);
        colors.push_back(col);
    }
    corner_pt = gfxm::vec3(rc.max.x, rc.min.y, .0f);
    corner_offset = gfxm::vec3(-radius, radius, .0f);
    radian_start += gfxm::pi * .5f;
    if (corner_flags & GUI_DRAW_CORNER_NE) {
        for (int i = 0; i <= segments; ++i) {
            float a = radian_start + (i / (float)segments) * gfxm::pi * .5f;
            gfxm::vec3 pt_outer = gfxm::vec3(
                cosf(a), sinf(a), .0f
            ) * radius + corner_pt;
            gfxm::vec3 pt_inner = gfxm::vec3(
                cosf(a), sinf(a), .0f
            ) * inner_radius + corner_pt;
            vertices.push_back(pt_outer);
            vertices.push_back(pt_inner);
            colors.push_back(col);
            colors.push_back(col);
        }
    } else {
        vertices.push_back(gfxm::vec3(rc_.max.x, rc_.min.y, .0f));
        vertices.push_back(corner_pt);
        colors.push_back(col);
        colors.push_back(col);
    }
    corner_pt = gfxm::vec3(rc.max.x, rc.max.y, .0f);
    corner_offset = gfxm::vec3(-radius, -radius, .0f);
    radian_start += gfxm::pi * .5f;
    if (corner_flags & GUI_DRAW_CORNER_SE) {
        for (int i = 0; i <= segments; ++i) {
            float a = radian_start + (i / (float)segments) * gfxm::pi * .5f;
            gfxm::vec3 pt_outer = gfxm::vec3(
                cosf(a), sinf(a), .0f
            ) * radius + corner_pt;
            gfxm::vec3 pt_inner = gfxm::vec3(
                cosf(a), sinf(a), .0f
            ) * inner_radius + corner_pt;
            vertices.push_back(pt_outer);
            vertices.push_back(pt_inner);
            colors.push_back(col);
            colors.push_back(col);
        }
    } else {
        vertices.push_back(gfxm::vec3(rc_.max.x, rc_.max.y, .0f));
        vertices.push_back(corner_pt);
        colors.push_back(col);
        colors.push_back(col);
    }
    corner_pt = gfxm::vec3(rc.min.x, rc.max.y, .0f);
    corner_offset = gfxm::vec3(radius, -radius, .0f);
    radian_start += gfxm::pi * .5f;
    if (corner_flags & GUI_DRAW_CORNER_SW) {
        for (int i = 0; i <= segments; ++i) {
            float a = radian_start + (i / (float)segments) * gfxm::pi * .5f;
            gfxm::vec3 pt_outer = gfxm::vec3(
                cosf(a), sinf(a), .0f
            ) * radius + corner_pt;
            gfxm::vec3 pt_inner = gfxm::vec3(
                cosf(a), sinf(a), .0f
            ) * inner_radius + corner_pt;
            vertices.push_back(pt_outer);
            vertices.push_back(pt_inner);
            colors.push_back(col);
            colors.push_back(col);
        }
    } else {
        vertices.push_back(gfxm::vec3(rc_.min.x, rc_.max.y, .0f));
        vertices.push_back(corner_pt);
        colors.push_back(col);
        colors.push_back(col);
    }
    vertices.push_back(vertices[0]);
    vertices.push_back(vertices[1]);
    vertices.push_back(gfxm::vec3(rc.min.x, rc.max.y, .0f));
    vertices.push_back(gfxm::vec3(rc.max.x, rc.min.y, .0f));
    vertices.push_back(gfxm::vec3(rc.max.x, rc.max.y, .0f));
    colors.push_back(col);
    colors.push_back(col);
    colors.push_back(col);
    colors.push_back(col);
    colors.push_back(col);

    gpuBuffer vertexBuffer;
    vertexBuffer.setArrayData(vertices.data(), vertices.size() * sizeof(vertices[0]));
    gpuBuffer colorBuffer;
    colorBuffer.setArrayData(colors.data(), colors.size() * sizeof(colors[0]));

    gpuShaderProgram* prog = _guiGetShaderRect();

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer.getId());
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ARRAY_BUFFER, colorBuffer.getId());
    glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, 0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    gfxm::mat4 model(1.0f);
    gfxm::mat4 view = guiGetViewTransform();
    gfxm::mat4 proj = gfxm::ortho(.0f, (float)screen_w, (float)screen_h, .0f, .0f, 100.0f);

    glUseProgram(prog->getId());
    glUniformMatrix4fv(prog->getUniformLocation("matView"), 1, GL_FALSE, (float*)&view);
    glUniformMatrix4fv(prog->getUniformLocation("matProjection"), 1, GL_FALSE, (float*)&proj);
    glUniformMatrix4fv(prog->getUniformLocation("matModel"), 1, GL_FALSE, (float*)&model);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, vertices.size());

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
}

void guiDrawRectRoundBorder(const gfxm::rect& rc_, float radius, float thickness, uint32_t col_a, uint32_t col_b, uint8_t corner_flags) {
    int screen_w = 0, screen_h = 0;
    platformGetWindowSize(screen_w, screen_h);

    gfxm::rect rc = rc_;
    gfxm::expand(rc, -radius);
    float inner_radius = radius + thickness;
    int segments = 16;
    std::vector<gfxm::vec3> vertices;
    std::vector<uint32_t> colors;
    gfxm::vec3 corner_pt(rc.min.x, rc.min.y, .0f);
    gfxm::vec3 corner_offset(radius, radius, .0f);
    float radian_start = gfxm::pi;
    for (int i = 0; i <= segments; ++i) {
        float a = radian_start + (i / (float)segments) * gfxm::pi * .5f;
        gfxm::vec3 pt_outer = gfxm::vec3(
            cosf(a), sinf(a), .0f
        ) * radius + corner_pt;
        gfxm::vec3 pt_inner = gfxm::vec3(
            cosf(a), sinf(a), .0f
        ) * inner_radius + corner_pt;
        vertices.push_back(pt_outer);
        vertices.push_back(pt_inner);
        colors.push_back(col_a);
        colors.push_back(col_b);
    }
    corner_pt = gfxm::vec3(rc.max.x, rc.min.y, .0f);
    corner_offset = gfxm::vec3(-radius, radius, .0f);
    radian_start += gfxm::pi * .5f;
    for (int i = 0; i <= segments; ++i) {
        float a = radian_start + (i / (float)segments) * gfxm::pi * .5f;
        gfxm::vec3 pt_outer = gfxm::vec3(
            cosf(a), sinf(a), .0f
        ) * radius + corner_pt;
        gfxm::vec3 pt_inner = gfxm::vec3(
            cosf(a), sinf(a), .0f
        ) * inner_radius + corner_pt;
        vertices.push_back(pt_outer);
        vertices.push_back(pt_inner);
        colors.push_back(col_a);
        colors.push_back(col_b);
    }
    corner_pt = gfxm::vec3(rc.max.x, rc.max.y, .0f);
    corner_offset = gfxm::vec3(-radius, -radius, .0f);
    radian_start += gfxm::pi * .5f;
    for (int i = 0; i <= segments; ++i) {
        float a = radian_start + (i / (float)segments) * gfxm::pi * .5f;
        gfxm::vec3 pt_outer = gfxm::vec3(
            cosf(a), sinf(a), .0f
        ) * radius + corner_pt;
        gfxm::vec3 pt_inner = gfxm::vec3(
            cosf(a), sinf(a), .0f
        ) * inner_radius + corner_pt;
        vertices.push_back(pt_outer);
        vertices.push_back(pt_inner);
        colors.push_back(col_a);
        colors.push_back(col_b);
    }
    corner_pt = gfxm::vec3(rc.min.x, rc.max.y, .0f);
    corner_offset = gfxm::vec3(radius, -radius, .0f);
    radian_start += gfxm::pi * .5f;
    for (int i = 0; i <= segments; ++i) {
        float a = radian_start + (i / (float)segments) * gfxm::pi * .5f;
        gfxm::vec3 pt_outer = gfxm::vec3(
            cosf(a), sinf(a), .0f
        ) * radius + corner_pt;
        gfxm::vec3 pt_inner = gfxm::vec3(
            cosf(a), sinf(a), .0f
        ) * inner_radius + corner_pt;
        vertices.push_back(pt_outer);
        vertices.push_back(pt_inner);
        colors.push_back(col_a);
        colors.push_back(col_b);
    }
    vertices.push_back(vertices[0]);
    vertices.push_back(vertices[1]);
    colors.push_back(col_a);
    colors.push_back(col_b);

    gpuBuffer vertexBuffer;
    vertexBuffer.setArrayData(vertices.data(), vertices.size() * sizeof(vertices[0]));
    gpuBuffer colorBuffer;
    colorBuffer.setArrayData(colors.data(), colors.size() * sizeof(colors[0]));

    gpuShaderProgram* prog = _guiGetShaderRect();

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer.getId());
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ARRAY_BUFFER, colorBuffer.getId());
    glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, 0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    gfxm::mat4 model(1.0f);
    gfxm::mat4 view = guiGetViewTransform();
    gfxm::mat4 proj = gfxm::ortho(.0f, (float)screen_w, (float)screen_h, .0f, .0f, 100.0f);

    glUseProgram(prog->getId());
    glUniformMatrix4fv(prog->getUniformLocation("matView"), 1, GL_FALSE, (float*)&view);
    glUniformMatrix4fv(prog->getUniformLocation("matProjection"), 1, GL_FALSE, (float*)&proj);
    glUniformMatrix4fv(prog->getUniformLocation("matModel"), 1, GL_FALSE, (float*)&model);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, vertices.size());

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
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
    gpuBuffer vertexBuffer;
    vertexBuffer.setArrayData(vertices, sizeof(vertices));
    gpuBuffer colorBuffer;
    colorBuffer.setArrayData(colors, sizeof(colors));
    gpuBuffer uvBuffer;
    uvBuffer.setArrayData(uvs, sizeof(uvs));

    const char* vs = R"(
        #version 450 
        layout (location = 0) in vec3 vertexPosition;
        layout (location = 1) in vec4 colorRGBA;
        layout (location = 2) in vec2 vertexUV;
        uniform mat4 matView;
        uniform mat4 matProjection;
        uniform mat4 matModel;
        out vec4 fragColor;
        out vec2 fragUV;
        void main() {
            fragColor = colorRGBA;
            fragUV = vertexUV;
            gl_Position = matProjection * matView * matModel * vec4(vertexPosition, 1.0);
        })";
    const char* fs = R"(
        #version 450
        in vec4 fragColor;
        in vec2 fragUV;
        out vec4 outAlbedo;

        uniform sampler2D tex;

        void main(){
            vec4 s = texture(tex, fragUV);
            outAlbedo = s * fragColor;
        })";
    gpuShaderProgram prog(vs, fs);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer.getId());
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ARRAY_BUFFER, colorBuffer.getId());
    glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, 0);

    glBindBuffer(GL_ARRAY_BUFFER, uvBuffer.getId());
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    gfxm::mat4 model(1.0f);
    gfxm::mat4 view = guiGetViewTransform();
    gfxm::mat4 proj = gfxm::ortho(.0f, (float)screen_w, (float)screen_h, .0f, .0f, 100.0f);

    glUseProgram(prog.getId());
    glUniformMatrix4fv(prog.getUniformLocation("matView"), 1, GL_FALSE, (float*)&view);
    glUniformMatrix4fv(prog.getUniformLocation("matProjection"), 1, GL_FALSE, (float*)&proj);
    glUniformMatrix4fv(prog.getUniformLocation("matModel"), 1, GL_FALSE, (float*)&model);
    glUniform1i(prog.getUniformLocation("tex"), 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture->getId());

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
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

    gpuBuffer vertexBuffer;
    vertexBuffer.setArrayData(vertices, sizeof(vertices));
    gpuBuffer colorBuffer;
    colorBuffer.setArrayData(colors, sizeof(colors));

    const char* vs = R"(
        #version 450 
        layout (location = 0) in vec3 vertexPosition;
        layout (location = 1) in vec4 colorRGBA;
        uniform mat4 matView;
        uniform mat4 matProjection;
        uniform mat4 matModel;
        out vec4 fragColor;
        void main() {
            fragColor = colorRGBA;
            gl_Position = matProjection * matView * matModel * vec4(vertexPosition, 1.0);
        })";
    const char* fs = R"(
        #version 450
        in vec4 fragColor;
        out vec4 outAlbedo;
        void main(){
            outAlbedo = fragColor;
        })";
    gpuShaderProgram prog(vs, fs);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer.getId());
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ARRAY_BUFFER, colorBuffer.getId());
    glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, 0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    gfxm::mat4 model(1.0f);
    gfxm::mat4 view = guiGetViewTransform();
    gfxm::mat4 proj = gfxm::ortho(.0f, (float)screen_w, (float)screen_h, .0f, .0f, 100.0f);

    glUseProgram(prog.getId());
    glUniformMatrix4fv(prog.getUniformLocation("matView"), 1, GL_FALSE, (float*)&view);
    glUniformMatrix4fv(prog.getUniformLocation("matProjection"), 1, GL_FALSE, (float*)&proj);
    glUniformMatrix4fv(prog.getUniformLocation("matModel"), 1, GL_FALSE, (float*)&model);

    glDrawArrays(GL_TRIANGLE_FAN, 0, n_points + 1);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
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
    gpuBuffer vertexBuffer;
    vertexBuffer.setArrayData(vertices, sizeof(vertices));
    gpuBuffer colorBuffer;
    colorBuffer.setArrayData(colors, sizeof(colors));

    const char* vs = R"(
        #version 450 
        layout (location = 0) in vec3 vertexPosition;
        layout (location = 1) in vec4 colorRGBA;
        uniform mat4 matView;
        uniform mat4 matProjection;
        uniform mat4 matModel;
        out vec4 fragColor;
        void main() {
            fragColor = colorRGBA;
            gl_Position = matProjection * matView * matModel * vec4(vertexPosition, 1.0);
        })";
    const char* fs = R"(
        #version 450
        in vec4 fragColor;
        out vec4 outAlbedo;
        void main(){
            outAlbedo = fragColor;
        })";
    gpuShaderProgram prog(vs, fs);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer.getId());
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ARRAY_BUFFER, colorBuffer.getId());
    glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, 0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    gfxm::mat4 model(1.0f);
    gfxm::mat4 view = guiGetViewTransform();
    gfxm::mat4 proj = gfxm::ortho(.0f, (float)screen_w, (float)screen_h, .0f, .0f, 100.0f);

    glUseProgram(prog.getId());
    glUniformMatrix4fv(prog.getUniformLocation("matView"), 1, GL_FALSE, (float*)&view);
    glUniformMatrix4fv(prog.getUniformLocation("matProjection"), 1, GL_FALSE, (float*)&proj);
    glUniformMatrix4fv(prog.getUniformLocation("matModel"), 1, GL_FALSE, (float*)&model);

    glDrawArrays(GL_LINE_STRIP, 0, 5);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
}

void guiDrawLine(const gfxm::rect& rc, uint32_t col) {
    int screen_w = 0, screen_h = 0;
    platformGetWindowSize(screen_w, screen_h);

    gfxm::rect rct = rc;

    float vertices[] = {
        rct.min.x, rct.min.y, 0, rct.max.x, rct.max.y, 0
    };
    uint32_t colors[] = {
         col, col
    };
    gpuBuffer vertexBuffer;
    vertexBuffer.setArrayData(vertices, sizeof(vertices));
    gpuBuffer colorBuffer;
    colorBuffer.setArrayData(colors, sizeof(colors));

    gpuShaderProgram* prog = _guiGetShaderRect();

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer.getId());
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ARRAY_BUFFER, colorBuffer.getId());
    glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, 0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    gfxm::mat4 model(1.0f);
    gfxm::mat4 view = guiGetViewTransform();
    gfxm::mat4 proj = gfxm::ortho(.0f, (float)screen_w, (float)screen_h, .0f, .0f, 100.0f);

    glUseProgram(prog->getId());
    glUniformMatrix4fv(prog->getUniformLocation("matView"), 1, GL_FALSE, (float*)&view);
    glUniformMatrix4fv(prog->getUniformLocation("matProjection"), 1, GL_FALSE, (float*)&proj);
    glUniformMatrix4fv(prog->getUniformLocation("matModel"), 1, GL_FALSE, (float*)&model);

    glDrawArrays(GL_LINE_STRIP, 0, 2);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
}

gfxm::vec2 guiCalcTextRect(const char* text, Font* font, float max_width) {
    std::unique_ptr<gpuText> gpu_text(new gpuText(font));
    gpu_text->setString(text);
    gpu_text->commit(max_width);

    return gpu_text->getBoundingSize();
}
gfxm::vec2 guiCalcTextPosInRect(const gfxm::rect& rc_text, const gfxm::rect& rc, int alignment, const gfxm::rect& margin, Font* font) {
    gfxm::vec2 mid = rc.min + (rc.max - rc.min) * .5f;
    gfxm::vec2 pos(
        rc.min.x + GUI_PADDING,
        mid.y - (font->getAscender() + font->getDescender()) * .5f
    );

    return pos;
}

void guiDrawText(const gfxm::vec2& pos, const char* text, GuiFont* font, float max_width, uint32_t col) {
    GuiTextBuffer text_buf(font);
    text_buf.replaceAll(text, strlen(text));
    text_buf.prepareDraw(font, false);
    text_buf.draw(pos, col, col);
}

#include "gui/elements/gui_element.hpp"
#include "gui/gui_system.hpp"
void guiDrawTitleBar(GuiElement* elem, GuiTextBuffer* buf, const gfxm::rect& rc) {
    uint32_t col = GUI_COL_HEADER;
    if (guiGetActiveWindow() == elem) {
        col = GUI_COL_ACCENT;
    }
    guiDrawRect(rc, col);
    
    buf->prepareDraw(buf->font, false);
    gfxm::vec2 text_sz = buf->getBoundingSize();
    gfxm::vec2 text_pos = guiCalcTextPosInRect(
        gfxm::rect(gfxm::vec2(0, 0), text_sz), 
        rc, 0, gfxm::rect(GUI_MARGIN, GUI_MARGIN, GUI_MARGIN, GUI_MARGIN), buf->font->font
    );
    buf->draw(text_pos, GUI_COL_TEXT, GUI_COL_ACCENT);
}