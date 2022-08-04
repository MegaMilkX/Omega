#include "debug_draw/debug_draw.hpp"

#include <assert.h>
#include <vector>
#include "platform/gl/glextutil.h"
#include "log/log.hpp"


static bool compileShader(GLuint sh) {
    glCompileShader(sh);
    GLint res = GL_FALSE;
    int infoLogLen;
    glGetShaderiv(sh, GL_COMPILE_STATUS, &res);
    glGetShaderiv(sh, GL_INFO_LOG_LENGTH, &infoLogLen);
    if (infoLogLen > 1) {
        std::vector<char> errMsg(infoLogLen + 1);
        LOG_ERR("dbgDraw: GLSL compile: " << &errMsg[0]);
    }
    if (res == GL_FALSE) {
        return false;
    }
    return true;
}

constexpr int MAX_VERTEX_COUNT = 0xFFFF;
struct dbgDebugDrawContext {
    std::vector<gfxm::vec3> vertices;
    std::vector<uint32_t>   colors;
    size_t vertex_count;

    GLuint shader_program;

    GLint attr_pos_loc;
    GLint attr_col_loc;

    dbgDebugDrawContext()
    : vertex_count(0) {
        vertices.resize(MAX_VERTEX_COUNT);
        colors.resize(MAX_VERTEX_COUNT);

        const char* vs = R"(
        #version 450 
        in vec3 inPosition;
        in vec4 inColorRGBA;
        out vec4 frag_color;

        uniform mat4 matProjection;
        uniform mat4 matView;    

        void main(){
            frag_color = inColorRGBA;
            vec4 pos = vec4(inPosition, 1);
            gl_Position = matProjection * matView * pos;
        })";
        const char* fs = R"(
        #version 450
        in vec4 frag_color;
        out vec4 outAlbedo;
        void main(){
            outAlbedo = frag_color;
        })";

        GLuint vid, fid;
        vid = glCreateShader(GL_VERTEX_SHADER);
        fid = glCreateShader(GL_FRAGMENT_SHADER);
        shader_program = glCreateProgram();
        glShaderSource(vid, 1, &vs, 0);
        glShaderSource(fid, 1, &fs, 0);
        compileShader(vid);
        compileShader(fid);

        glAttachShader(shader_program, vid);
        glAttachShader(shader_program, fid);

        // TODO: Outputs?

        glLinkProgram(shader_program);
        {
            GLint res = GL_FALSE;
            int infoLogLen;
            glGetProgramiv(shader_program, GL_LINK_STATUS, &res);
            glGetProgramiv(shader_program, GL_INFO_LOG_LENGTH, &infoLogLen);
            if (infoLogLen > 1) {
                std::vector<char> errMsg(infoLogLen + 1);
                glGetProgramInfoLog(shader_program, infoLogLen, NULL, &errMsg[0]);
                LOG_ERR("GLSL link: " << &errMsg[0]);
            }
        }

        // TODO: Uniforms?
        
        // Attribs
        {
            attr_pos_loc = glGetAttribLocation(shader_program, "inPosition");
            attr_col_loc = glGetAttribLocation(shader_program, "inColorRGBA");
        }

        // TODO: Uniform buffers?
    }
};


static dbgDebugDrawContext& getDbgDrawContext() {
    static dbgDebugDrawContext ctx;
    return ctx;
}


void dbgDrawClearBuffers() {
    auto& ctx = getDbgDrawContext();
    ctx.vertex_count = 0;
}
void dbgDrawDraw(const gfxm::mat4& projection, const gfxm::mat4& view) {
    auto& ctx = getDbgDrawContext();
    if (ctx.vertex_count == 0) {
        return;
    }

    glUseProgram(ctx.shader_program);
    glUniformMatrix4fv(glGetUniformLocation(ctx.shader_program, "matProjection"), 1, GL_FALSE, (float*)&projection);
    glUniformMatrix4fv(glGetUniformLocation(ctx.shader_program, "matView"), 1, GL_FALSE, (float*)&view);
    
    //glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    /*
    glEnable(GL_CULL_FACE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_LINE_SMOOTH);*/

    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    GLuint vert_buf = 0;
    GLuint col_buf = 0;
    glGenBuffers(1, &vert_buf);
    glGenBuffers(1, &col_buf);

    glBindBuffer(GL_ARRAY_BUFFER, vert_buf);
    glBufferData(GL_ARRAY_BUFFER, ctx.vertices.size() * sizeof(ctx.vertices[0]), ctx.vertices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(ctx.attr_pos_loc);
    glVertexAttribPointer(
        ctx.attr_pos_loc, 3, GL_FLOAT, GL_FALSE, 0, (void*)0
    );
    glBindBuffer(GL_ARRAY_BUFFER, col_buf);
    glBufferData(GL_ARRAY_BUFFER, ctx.colors.size() * sizeof(ctx.colors[0]), ctx.colors.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(ctx.attr_col_loc);
    glVertexAttribPointer(
        ctx.attr_col_loc, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, (void*)0
    );    

    glDrawArrays(GL_LINES, 0, ctx.vertex_count);

    glDeleteBuffers(1, &col_buf);
    glDeleteBuffers(1, &vert_buf);
    glDeleteVertexArrays(1, &vao);
}

void dbgDrawLines(const gfxm::vec3* vertices, size_t count, uint32_t color) {
    auto& ctx = getDbgDrawContext();
    auto first = ctx.vertex_count;
    if (MAX_VERTEX_COUNT < (first + count)) {
        assert(false);
        return;
    }
    memcpy(&ctx.vertices[first], vertices, count * sizeof(ctx.vertices[0]));
    for (int i = 0; i < count; ++i) {
        ctx.colors[first + i] = color;
    }
    ctx.vertex_count += count;
}
void dbgDrawLine(const gfxm::vec3& from, const gfxm::vec3& to, uint32_t color) {
    auto& ctx = getDbgDrawContext();
    auto first = ctx.vertex_count;
    ctx.vertices[first] = from;
    ctx.vertices[first + 1] = to;
    ctx.colors[first] = color;
    ctx.colors[first + 1] = color;
    ctx.vertex_count += 2;
}
void dbgDrawCross(const gfxm::vec3& pos, float size, uint32_t color) {
    dbgDrawCross(gfxm::translate(gfxm::mat4(1.0f), pos), size, color);
}
void dbgDrawCross(const gfxm::mat4& tr, float size, uint32_t color) {
    gfxm::vec3 vertices[6] = {
        { -size, .0f, .0f }, { size, .0f, .0f },
        { .0f, -size, .0f }, { .0f, size, .0f },
        { .0f, .0f, -size }, { .0f, .0f, size } 
    };
    for (int i = 0; i < 6; ++i) {
        auto& v = vertices[i];
        v = tr * gfxm::vec4(v, 1.0f);
    }
    dbgDrawLines(vertices, 6, color);
}
void dbgDrawRay(const gfxm::vec3& origin, const gfxm::vec3& dir, uint32_t color) {
    dbgDrawLine(origin, origin + dir, color);
}

void dbgDrawArrow(const gfxm::vec3& origin, const gfxm::vec3& dir, uint32_t color) {
    gfxm::vec3 perp = gfxm::normalize(gfxm::vec3(-dir.y, dir.x, dir.z));
    gfxm::vec3 mperp = -perp;
    gfxm::vec3 cr = gfxm::cross(gfxm::normalize(dir), perp);
    gfxm::vec3 mcr = -cr;
    dbgDrawRay(origin, dir, color);
    gfxm::vec3 arrow_rim[4] = {
        { origin + dir * 0.8f + perp * 0.1f },
        { origin + dir * 0.8f + cr * 0.1f },
        { origin + dir * 0.8f + mperp * 0.1f },
        { origin + dir * 0.8f + mcr * 0.1f }
    };
    dbgDrawLine(arrow_rim[0], origin + dir, color);
    dbgDrawLine(arrow_rim[1], origin + dir, color);
    dbgDrawLine(arrow_rim[2], origin + dir, color);
    dbgDrawLine(arrow_rim[3], origin + dir, color);

    dbgDrawLine(arrow_rim[0], arrow_rim[1], color);
    dbgDrawLine(arrow_rim[1], arrow_rim[2], color);
    dbgDrawLine(arrow_rim[2], arrow_rim[3], color);
    dbgDrawLine(arrow_rim[3], arrow_rim[0], color);
}
void dbgDrawDome(const gfxm::vec3& pos, float radius, uint32_t color) {
    dbgDrawDome(gfxm::translate(gfxm::mat4(1.0f), pos), radius, color);
}
void dbgDrawDome(const gfxm::mat4& tr, float radius, uint32_t color) {
    int segments = 12;
    int half_segments = segments / 2;
    std::vector<gfxm::vec3> vertices(segments * 2 + half_segments * 2 * 2);
    float prev_x = 1.0f * radius;
    float prev_z = .0f;
    gfxm::vec3* begin = &vertices[0];
    for (int i = 0; i < segments; ++i) {
        float rad = ((i + 1) / (float)segments) * gfxm::pi * 2.0f;
        float x = cosf(rad) * radius;
        float z = sinf(rad) * radius;
        begin[i * 2] = gfxm::vec3(prev_x, .0f, prev_z);
        begin[i * 2 + 1] = gfxm::vec3(x, .0f, z);
        prev_x = x;
        prev_z = z;
    }
    prev_x = 1.0f * radius;
    float prev_y = .0f;
    begin = &vertices[segments * 2];
    for (int i = 0; i < half_segments; ++i) {
        float rad = ((i + 1) / (float)half_segments) * gfxm::pi;
        float x = cosf(rad) * radius;
        float y = sinf(rad) * radius;
        begin[i * 2] = gfxm::vec3(prev_x, prev_y, .0f);
        begin[i * 2 + 1] = gfxm::vec3(x, y, .0f);
        prev_x = x;
        prev_y = y;
    }
    prev_z = 1.0f * radius;
    prev_y = .0f;
    begin = &vertices[segments * 2 + half_segments * 2];
    for (int i = 0; i < half_segments; ++i) {
        float rad = ((i + 1) / (float)half_segments) * gfxm::pi;
        float z = cosf(rad) * radius;
        float y = sinf(rad) * radius;
        begin[i * 2] = gfxm::vec3(.0f, prev_y, prev_z);
        begin[i * 2 + 1] = gfxm::vec3(.0f, y, z);
        prev_z = z;
        prev_y = y;
    }
    for (int i = 0; i < vertices.size(); ++i) {
        auto& v = vertices[i];
        v = tr * gfxm::vec4(v, 1.0f);
    }
    dbgDrawLines(vertices.data(), vertices.size(), color);
}
void dbgDrawSphere(const gfxm::vec3& pos, float radius, uint32_t color) {
    dbgDrawSphere(gfxm::translate(gfxm::mat4(1.0f), pos), radius, color);
}
void dbgDrawSphere(const gfxm::mat4& tr, float radius, uint32_t color) {
    int segments = 12;
    std::vector<gfxm::vec3> vertices(segments * 2 * 3);
    float prev_x = 1.0f * radius;
    float prev_z = .0f;
    gfxm::vec3* begin = &vertices[0];
    for (int i = 0; i < segments; ++i) {
        float rad = ((i + 1) / (float)segments) * gfxm::pi * 2.0f;
        float x = cosf(rad) * radius;
        float z = sinf(rad) * radius;
        begin[i * 2] = gfxm::vec3(prev_x, .0f, prev_z);
        begin[i * 2 + 1] = gfxm::vec3(x, .0f, z);
        prev_x = x;
        prev_z = z;
    }
    prev_x = 1.0f * radius;
    float prev_y = .0f;
    begin = &vertices[segments * 2];
    for (int i = 0; i < segments; ++i) {
        float rad = ((i + 1) / (float)segments) * gfxm::pi * 2.0f;
        float x = cosf(rad) * radius;
        float y = sinf(rad) * radius;
        begin[i * 2] = gfxm::vec3(prev_x, prev_y, .0f);
        begin[i * 2 + 1] = gfxm::vec3(x, y, .0f);
        prev_x = x;
        prev_y = y;
    }
    prev_z = 1.0f * radius;
    prev_y = .0f;
    begin = &vertices[segments * 2 * 2];
    for (int i = 0; i < segments; ++i) {
        float rad = ((i + 1) / (float)segments) * gfxm::pi * 2.0f;
        float z = cosf(rad) * radius;
        float y = sinf(rad) * radius;
        begin[i * 2] = gfxm::vec3(.0f, prev_y, prev_z);
        begin[i * 2 + 1] = gfxm::vec3(.0f, y, z);
        prev_z = z;
        prev_y = y;
    }
    for (int i = 0; i < vertices.size(); ++i) {
        auto& v = vertices[i];
        v = tr * gfxm::vec4(v, 1.0f);
    }
    dbgDrawLines(vertices.data(), vertices.size(), color);
}
void dbgDrawCapsule(const gfxm::vec3& pos, float height, float radius, uint32_t color) {
    const float half_height = height * .5f;
    const gfxm::vec3 top = pos + gfxm::vec3(0, height * .5f, 0);
    const gfxm::vec3 bottom = pos - gfxm::vec3(0, height * .5f, 0);
    dbgDrawDome(top, radius, color);
    dbgDrawDome(
        gfxm::translate(gfxm::mat4(1.0f), bottom) * gfxm::to_mat4(gfxm::angle_axis(gfxm::pi, gfxm::vec3(1, 0, 0))),
        radius, color
    );
    gfxm::vec3 back(0, 0, radius);
    gfxm::vec3 fwd(0, 0, -radius);
    gfxm::vec3 left(-radius, 0, 0);
    gfxm::vec3 right(radius, 0, 0);
    dbgDrawLine(top + back, bottom + back, color);
    dbgDrawLine(top + fwd, bottom + fwd, color);
    dbgDrawLine(top + left, bottom + left, color);
    dbgDrawLine(top + right, bottom + right, color);
}
void dbgDrawCapsule(const gfxm::mat4& tr, float height, float radius, uint32_t color) {
    const float half_height = height * .5f;
    gfxm::mat4 tr_top = tr * gfxm::translate(gfxm::mat4(1.0f), gfxm::vec3(0, half_height, 0));
    gfxm::mat4 tr_bottom = tr * (gfxm::translate(gfxm::mat4(1.0f), gfxm::vec3(0, -half_height, 0)) * gfxm::to_mat4(gfxm::angle_axis(gfxm::pi, gfxm::vec3(1, 0, 0))));
    dbgDrawDome(tr_top, radius, color);
    dbgDrawDome(tr_bottom, radius, color);
    gfxm::vec3 vertices[8] = {
        { -radius, -half_height, .0f },
        { -radius,  half_height, .0f },
        {  radius, -half_height, .0f },
        {  radius,  half_height, .0f },
        { .0f, -half_height, -radius },
        { .0f,  half_height, -radius },
        { .0f, -half_height,  radius },
        { .0f,  half_height,  radius }
    };
    for (int i = 0; i < 8; ++i) {
        auto& v = vertices[i];
        v = tr * gfxm::vec4(v, 1.0f);
    }
    dbgDrawLines(vertices, 8, color);
}
void dbgDrawBox(const gfxm::mat4& transform, const gfxm::vec3& half_extents, uint32_t color) {
    gfxm::vec3 vertices[24] = {
        { -half_extents.x, -half_extents.y, -half_extents.z },
        {  half_extents.x, -half_extents.y, -half_extents.z },
        { -half_extents.x, -half_extents.y,  half_extents.z },
        {  half_extents.x, -half_extents.y,  half_extents.z },
        { -half_extents.x,  half_extents.y, -half_extents.z },
        {  half_extents.x,  half_extents.y, -half_extents.z },
        { -half_extents.x,  half_extents.y,  half_extents.z },
        {  half_extents.x,  half_extents.y,  half_extents.z },

        { -half_extents.x, -half_extents.y, -half_extents.z },
        { -half_extents.x, -half_extents.y,  half_extents.z },
        {  half_extents.x, -half_extents.y, -half_extents.z },
        {  half_extents.x, -half_extents.y,  half_extents.z },
        { -half_extents.x,  half_extents.y, -half_extents.z },
        { -half_extents.x,  half_extents.y,  half_extents.z },
        {  half_extents.x,  half_extents.y, -half_extents.z },
        {  half_extents.x,  half_extents.y,  half_extents.z },

        { -half_extents.x, -half_extents.y, -half_extents.z },
        { -half_extents.x,  half_extents.y, -half_extents.z },
        {  half_extents.x, -half_extents.y, -half_extents.z },
        {  half_extents.x,  half_extents.y, -half_extents.z },
        { -half_extents.x, -half_extents.y,  half_extents.z },
        { -half_extents.x,  half_extents.y,  half_extents.z },
        {  half_extents.x, -half_extents.y,  half_extents.z },
        {  half_extents.x,  half_extents.y,  half_extents.z }
    };
    for (int i = 0; i < 24; ++i) {
        vertices[i] = transform * gfxm::vec4(vertices[i], 1.f);
    }
    dbgDrawLines(vertices, 24, color);
}