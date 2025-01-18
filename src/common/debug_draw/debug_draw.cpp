#include "debug_draw/debug_draw.hpp"

#include <assert.h>
#include <vector>
#include "platform/gl/glextutil.h"
#include "log/log.hpp"
#include "typeface/font.hpp"


#define DEBUG_DRAW_TEXT_SIZE 24


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

constexpr int MAX_VERTEX_COUNT = 0xFFFFF;
struct dbgDebugDrawContext {
    struct TEXT_BLOCK {
        gfxm::vec3 position;
        int vertex_begin;
        int vertex_count;
    };

    std::vector<gfxm::vec3> vertices;
    std::vector<uint32_t>   colors;
    size_t vertex_count = 0;

    std::vector<gfxm::vec3> text_vertices;
    std::vector<gfxm::vec2> text_uvs;
    std::vector<uint32_t> text_colors;
    size_t text_vertex_count = 0;

    std::vector<TEXT_BLOCK> text_blocks;

    // TODO: TIMED DRAW

    GLuint vert_buf = 0;
    GLuint col_buf = 0;

    GLuint text_vert_buf = 0;
    GLuint text_col_buf = 0;
    GLuint text_uv_buf = 0;

    GLuint shader_program;
    GLuint text_shader_program;

    GLuint vao;

    GLint attr_pos_loc;
    GLint attr_col_loc;
    GLint text_attr_pos_loc;
    GLint text_attr_col_loc;
    GLint text_attr_uv_loc;

    std::shared_ptr<Typeface> typeface;
    std::shared_ptr<Font> font;
    std::shared_ptr<gpuTexture2d> font_texture;

    void initLineProgram() {
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
        glDeleteShader(vid);
        glDeleteShader(fid);

        // TODO: Uniforms?
        
        // Attribs
        {
            attr_pos_loc = glGetAttribLocation(shader_program, "inPosition");
            attr_col_loc = glGetAttribLocation(shader_program, "inColorRGBA");
        }

        // TODO: Uniform buffers?
    }

    void initTextProgram() {
        const char* vs = R"(
        #version 450 
        in vec3 inPosition;
        in vec4 inColorRGBA;
        in vec2 inUV;
        out vec4 frag_color;
        out vec2 frag_uv;

        uniform mat4 matProjection;
        uniform mat4 matView;
        uniform mat4 matModel;

        void main(){
            frag_color = inColorRGBA;
            frag_uv = inUV;

            vec4 pos = matView * matModel * vec4(inPosition, 1);
            //pos.x = round(pos.x);
            //pos.y = round(pos.y);
            gl_Position = matProjection * pos;
        })";
        const char* fs = R"(
        #version 450
        in vec4 frag_color;
        in vec2 frag_uv;
        uniform sampler2D tex;
        out vec4 outAlbedo;
        void main(){
            vec4 color = texture(tex, frag_uv);
            outAlbedo = vec4(color.xxx * frag_color.xyz, color.x);
        })";

        GLuint vid, fid;
        vid = glCreateShader(GL_VERTEX_SHADER);
        fid = glCreateShader(GL_FRAGMENT_SHADER);
        text_shader_program = glCreateProgram();
        glShaderSource(vid, 1, &vs, 0);
        glShaderSource(fid, 1, &fs, 0);
        compileShader(vid);
        compileShader(fid);

        glAttachShader(text_shader_program, vid);
        glAttachShader(text_shader_program, fid);

        // TODO: Outputs?

        glLinkProgram(text_shader_program);
        {
            GLint res = GL_FALSE;
            int infoLogLen;
            glGetProgramiv(text_shader_program, GL_LINK_STATUS, &res);
            glGetProgramiv(text_shader_program, GL_INFO_LOG_LENGTH, &infoLogLen);
            if (infoLogLen > 1) {
                std::vector<char> errMsg(infoLogLen + 1);
                glGetProgramInfoLog(text_shader_program, infoLogLen, NULL, &errMsg[0]);
                LOG_ERR("GLSL link: " << &errMsg[0]);
            }
        }
        glDeleteShader(vid);
        glDeleteShader(fid);

        // TODO: Uniforms?
        
        // Attribs
        {
            text_attr_pos_loc = glGetAttribLocation(text_shader_program, "inPosition");
            text_attr_col_loc = glGetAttribLocation(text_shader_program, "inColorRGBA");
            text_attr_uv_loc = glGetAttribLocation(text_shader_program, "inUV");
        }
        // Uniforms
        {
            glUseProgram(text_shader_program);
            glUniform1i(glGetUniformLocation(text_shader_program, "tex"), 0);
            glUseProgram(0);
        }

        // TODO: Uniform buffers?
    }

    dbgDebugDrawContext()
    : vertex_count(0) {
        vertices.resize(MAX_VERTEX_COUNT);
        colors.resize(MAX_VERTEX_COUNT);

        text_vertices.resize(MAX_VERTEX_COUNT);
        text_uvs.resize(MAX_VERTEX_COUNT);
        text_colors.resize(MAX_VERTEX_COUNT);

        initLineProgram();
        initTextProgram();

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vert_buf);
        glGenBuffers(1, &col_buf);

        glGenBuffers(1, &text_vert_buf);
        glGenBuffers(1, &text_col_buf);
        glGenBuffers(1, &text_uv_buf);

        typeface.reset(new Typeface);
        typefaceLoad(typeface.get(), "core/fonts/ProggyClean.ttf");
        font = typeface->getFont(DEBUG_DRAW_TEXT_SIZE, 96);
        assert(font.get());
        ktImage img_font_atlas;
        font->buildAtlas(&img_font_atlas, 0);
        font_texture.reset(new gpuTexture2d);
        font_texture->setData(&img_font_atlas);
        font_texture->setFilter(GPU_TEXTURE_FILTER_NEAREST);
    }
};


static dbgDebugDrawContext& getDbgDrawContext() {
    static dbgDebugDrawContext ctx;
    return ctx;
}


void dbgDrawClearBuffers() {
    auto& ctx = getDbgDrawContext();
    ctx.vertex_count = 0;
    ctx.text_vertex_count = 0;
    ctx.text_blocks.clear();
}
void dbgDrawDraw(const gfxm::mat4& projection, const gfxm::mat4& view, int vp_x, int vp_y, int vp_w, int vp_h) {
    auto& ctx = getDbgDrawContext();
    
    if (ctx.vertex_count > 0) {
        glUseProgram(ctx.shader_program);
        glUniformMatrix4fv(glGetUniformLocation(ctx.shader_program, "matProjection"), 1, GL_FALSE, (float*)&projection);
        glUniformMatrix4fv(glGetUniformLocation(ctx.shader_program, "matView"), 1, GL_FALSE, (float*)&view);

        //glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glEnable(GL_CULL_FACE);
        //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_BLEND);
        //glEnable(GL_DEPTH_TEST);
        glDisable(GL_SCISSOR_TEST);
        glDisable(GL_LINE_SMOOTH);

        glBindVertexArray(ctx.vao);

        glBindBuffer(GL_ARRAY_BUFFER, ctx.vert_buf);
        glBufferData(GL_ARRAY_BUFFER, ctx.vertex_count * sizeof(ctx.vertices[0]), ctx.vertices.data(), GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(ctx.attr_pos_loc);
        glVertexAttribPointer(
            ctx.attr_pos_loc, 3, GL_FLOAT, GL_FALSE, 0, (void*)0
        );
        glBindBuffer(GL_ARRAY_BUFFER, ctx.col_buf);
        glBufferData(GL_ARRAY_BUFFER, ctx.vertex_count * sizeof(ctx.colors[0]), ctx.colors.data(), GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(ctx.attr_col_loc);
        glVertexAttribPointer(
            ctx.attr_col_loc, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, (void*)0
        );

        glDrawArrays(GL_LINES, 0, ctx.vertex_count);

        // Cleanup
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        glUseProgram(0);
    }

    if (ctx.text_blocks.size() > 0) {
        // Text
        gfxm::mat4 ortho = gfxm::ortho(.0f, (float)vp_w, .0f, (float)vp_h, -100.f, 100.f);
        gfxm::mat4 text_view = gfxm::mat4(1.f);
        glUseProgram(ctx.text_shader_program);
        glUniformMatrix4fv(glGetUniformLocation(ctx.text_shader_program, "matProjection"), 1, GL_FALSE, (float*)&ortho);
        glUniformMatrix4fv(glGetUniformLocation(ctx.text_shader_program, "matView"), 1, GL_FALSE, (float*)&text_view);
        //glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_CULL_FACE);
        //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_BLEND);
        //glEnable(GL_DEPTH_TEST);
        glDisable(GL_SCISSOR_TEST);
        glDisable(GL_LINE_SMOOTH);

        glBindVertexArray(ctx.vao);

        glBindBuffer(GL_ARRAY_BUFFER, ctx.text_vert_buf);
        glBufferData(GL_ARRAY_BUFFER, ctx.text_vertex_count * sizeof(ctx.text_vertices[0]), ctx.text_vertices.data(), GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(ctx.text_attr_pos_loc);
        glVertexAttribPointer(
            ctx.text_attr_pos_loc, 3, GL_FLOAT, GL_FALSE, 0, (void*)0
        );
        glBindBuffer(GL_ARRAY_BUFFER, ctx.text_col_buf);
        glBufferData(GL_ARRAY_BUFFER, ctx.text_vertex_count * sizeof(ctx.text_colors[0]), ctx.text_colors.data(), GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(ctx.text_attr_col_loc);
        glVertexAttribPointer(
            ctx.text_attr_col_loc, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, (void*)0
        );
        glBindBuffer(GL_ARRAY_BUFFER, ctx.text_uv_buf);
        glBufferData(GL_ARRAY_BUFFER, ctx.text_vertex_count * sizeof(ctx.text_uvs[0]), ctx.text_uvs.data(), GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(ctx.text_attr_uv_loc);
        glVertexAttribPointer(
            ctx.text_attr_uv_loc, 2, GL_FLOAT, GL_FALSE, 0, (void*)0
        );

        glActiveTexture(GL_TEXTURE0 + 0);
        glBindTexture(GL_TEXTURE_2D, ctx.font_texture->getId());

        gfxm::mat4 cam = gfxm::inverse(view);
        for (int i = 0; i < ctx.text_blocks.size(); ++i) {
            const auto& block = ctx.text_blocks[i];
            if (gfxm::dot(gfxm::vec3(cam[2]), gfxm::vec3(cam[3]) - block.position) < .0f) {
                continue;
            }
            gfxm::vec2 pos = gfxm::world_to_screen(block.position, projection, view, vp_w, vp_h);
            pos.x = roundf(pos.x);
            pos.y = roundf(pos.y);
            gfxm::mat4 model = gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(pos, .0f));
            glUniformMatrix4fv(glGetUniformLocation(ctx.text_shader_program, "matModel"), 1, GL_FALSE, (float*)&model);
            glDrawArrays(GL_TRIANGLES, block.vertex_begin, block.vertex_count);
        }

        // Cleanup
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        glUseProgram(0);
    }

}

void dbgDrawText(const gfxm::vec3& at, const std::string& text, uint32_t color, float time) {
    auto& ctx = getDbgDrawContext();
    auto first = ctx.text_vertex_count;
    if (MAX_VERTEX_COUNT < first + text.size() * 6) {
        return;
    }
    int hori_adv = 0;
    for (int i = 0; i < text.size(); ++i) {
        FontGlyph g = ctx.font->getGlyph(text[i]);
        float w = g.width;
        float h = g.height;
        float oy = g.height - g.bearingY;
        // TODO: border is hardcoded rn
        const float border_fix = 1;
        const float minx = hori_adv - border_fix;
        const float maxx = hori_adv + w + border_fix;
        const float miny = .0f - oy - border_fix;
        const float maxy = h - oy + border_fix;

        ctx.text_vertices[first + i * 6]        = gfxm::vec3(minx, miny, .0f);
        ctx.text_vertices[first + i * 6 + 1]    = gfxm::vec3(maxx, miny, .0f);
        ctx.text_vertices[first + i * 6 + 2]    = gfxm::vec3(maxx, maxy, .0f);
        ctx.text_vertices[first + i * 6 + 3]    = gfxm::vec3(maxx, maxy, .0f);
        ctx.text_vertices[first + i * 6 + 4]    = gfxm::vec3(minx, maxy, .0f);
        ctx.text_vertices[first + i * 6 + 5]    = gfxm::vec3(minx, miny, .0f);

        ctx.text_uvs[first + i * 6] = gfxm::vec2(g.uv_rect.min.x, g.uv_rect.min.y);
        ctx.text_uvs[first + i * 6 + 1] = gfxm::vec2(g.uv_rect.max.x, g.uv_rect.min.y);
        ctx.text_uvs[first + i * 6 + 2] = gfxm::vec2(g.uv_rect.max.x, g.uv_rect.max.y);
        ctx.text_uvs[first + i * 6 + 3] = gfxm::vec2(g.uv_rect.max.x, g.uv_rect.max.y);
        ctx.text_uvs[first + i * 6 + 4] = gfxm::vec2(g.uv_rect.min.x, g.uv_rect.max.y);
        ctx.text_uvs[first + i * 6 + 5] = gfxm::vec2(g.uv_rect.min.x, g.uv_rect.min.y);

        ctx.text_colors[first + i * 6] = color;
        ctx.text_colors[first + i * 6 + 1] = color;
        ctx.text_colors[first + i * 6 + 2] = color;
        ctx.text_colors[first + i * 6 + 3] = color;
        ctx.text_colors[first + i * 6 + 4] = color;
        ctx.text_colors[first + i * 6 + 5] = color;

        hori_adv += g.horiAdvance / 64;
    }
    ctx.text_vertex_count += text.size() * 6;

    for (int i = 0; i < text.size(); ++i) {
        /*
        ctx.text_vertices[first + i * 6].x      -= (hori_adv * .5f);
        ctx.text_vertices[first + i * 6 + 1].x  -= (hori_adv * .5f);
        ctx.text_vertices[first + i * 6 + 2].x  -= (hori_adv * .5f);
        ctx.text_vertices[first + i * 6 + 3].x  -= (hori_adv * .5f);
        ctx.text_vertices[first + i * 6 + 4].x  -= (hori_adv * .5f);
        ctx.text_vertices[first + i * 6 + 5].x  -= (hori_adv * .5f);
        */
        /*
        ctx.text_vertices[first + i * 6].x      = roundf(ctx.text_vertices[first + i * 6].x);
        ctx.text_vertices[first + i * 6 + 1].x  = roundf(ctx.text_vertices[first + i * 6 + 1].x);
        ctx.text_vertices[first + i * 6 + 2].x  = roundf(ctx.text_vertices[first + i * 6 + 2].x);
        ctx.text_vertices[first + i * 6 + 3].x  = roundf(ctx.text_vertices[first + i * 6 + 3].x);
        ctx.text_vertices[first + i * 6 + 4].x  = roundf(ctx.text_vertices[first + i * 6 + 4].x);
        ctx.text_vertices[first + i * 6 + 5].x  = roundf(ctx.text_vertices[first + i * 6 + 5].x);*/
    }

    dbgDebugDrawContext::TEXT_BLOCK block;
    block.position = at;
    block.vertex_begin = first;
    block.vertex_count = text.size() * 6;
    ctx.text_blocks.push_back(block);
}
void dbgDrawLines(const gfxm::vec3* vertices, size_t count, uint32_t color, float time) {
    auto& ctx = getDbgDrawContext();
    auto first = ctx.vertex_count;
    if (MAX_VERTEX_COUNT < (first + count)) {
        //assert(false);
        return;
    }
    memcpy(&ctx.vertices[first], vertices, count * sizeof(ctx.vertices[0]));
    for (int i = 0; i < count; ++i) {
        ctx.colors[first + i] = color;
    }
    ctx.vertex_count += count;
}
void dbgDrawLine(const gfxm::vec3& from, const gfxm::vec3& to, uint32_t color, float time) {
    auto& ctx = getDbgDrawContext();
    auto first = ctx.vertex_count;
    if (MAX_VERTEX_COUNT < (first + 2)) {
        //assert(false);
        return;
    }
    ctx.vertices[first] = from;
    ctx.vertices[first + 1] = to;
    ctx.colors[first] = color;
    ctx.colors[first + 1] = color;
    ctx.vertex_count += 2;
}
void dbgDrawCross(const gfxm::vec3& pos, float size, uint32_t color, float time) {
    dbgDrawCross(gfxm::translate(gfxm::mat4(1.0f), pos), size, color, time);
}
void dbgDrawCross(const gfxm::mat4& tr, float size, uint32_t color, float time) {
    gfxm::vec3 vertices[6] = {
        { -size, .0f, .0f }, { size, .0f, .0f },
        { .0f, -size, .0f }, { .0f, size, .0f },
        { .0f, .0f, -size }, { .0f, .0f, size } 
    };
    for (int i = 0; i < 6; ++i) {
        auto& v = vertices[i];
        v = tr * gfxm::vec4(v, 1.0f);
    }
    dbgDrawLines(vertices, 6, color, time);
}
void dbgDrawRay(const gfxm::vec3& origin, const gfxm::vec3& dir, uint32_t color, float time) {
    dbgDrawLine(origin, origin + dir, color);
}

void dbgDrawArrow(const gfxm::vec3& origin, const gfxm::vec3& dir, uint32_t color, float time) {
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
void dbgDrawDome(const gfxm::vec3& pos, float radius, uint32_t color, float time) {
    dbgDrawDome(gfxm::translate(gfxm::mat4(1.0f), pos), radius, color, time);
}
void dbgDrawDome(const gfxm::mat4& tr, float radius, uint32_t color, float time) {
    const int segments = 12;
    const int half_segments = segments / 2;
    gfxm::vec3 vertices[segments * 2 + half_segments * 2 * 2];
    const int vertex_count = sizeof(vertices) / sizeof(vertices[0]);

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
    for (int i = 0; i < vertex_count; ++i) {
        auto& v = vertices[i];
        v = tr * gfxm::vec4(v, 1.0f);
    }
    dbgDrawLines(vertices, vertex_count, color, time);
}
void dbgDrawSphere(const gfxm::vec3& pos, float radius, uint32_t color, float time) {
    dbgDrawSphere(gfxm::translate(gfxm::mat4(1.0f), pos), radius, color, time);
}
void dbgDrawSphere(const gfxm::mat4& tr, float radius, uint32_t color, float time) {
    const int segments = 12;
    gfxm::vec3 vertices[segments * 2 * 3];
    const int vertex_count = sizeof(vertices) / sizeof(vertices[0]);
    
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
    for (int i = 0; i < vertex_count; ++i) {
        auto& v = vertices[i];
        v = tr * gfxm::vec4(v, 1.0f);
    }
    dbgDrawLines(vertices, vertex_count, color, time);
}
void dbgDrawCapsule(const gfxm::vec3& pos, float height, float radius, uint32_t color, float time) {
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
void dbgDrawCapsule(const gfxm::mat4& tr, float height, float radius, uint32_t color, float time) {
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
    dbgDrawLines(vertices, 8, color, time);
}
void dbgDrawBox(const gfxm::mat4& transform, const gfxm::vec3& half_extents, uint32_t color, float time) {
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
    dbgDrawLines(vertices, 24, color, time);
}

void dbgDrawAabb(const gfxm::aabb& aabb, uint32_t color, float time) {
    gfxm::vec3 vertices[24] = {
        { aabb.from.x, aabb.from.y, aabb.from.z },
        {  aabb.to.x, aabb.from.y, aabb.from.z },
        { aabb.from.x, aabb.from.y,  aabb.to.z },
        {  aabb.to.x, aabb.from.y,  aabb.to.z },
        { aabb.from.x,  aabb.to.y, aabb.from.z },
        {  aabb.to.x,  aabb.to.y, aabb.from.z },
        { aabb.from.x,  aabb.to.y,  aabb.to.z },
        {  aabb.to.x,  aabb.to.y,  aabb.to.z },

        { aabb.from.x, aabb.from.y, aabb.from.z },
        { aabb.from.x, aabb.from.y,  aabb.to.z },
        {  aabb.to.x, aabb.from.y, aabb.from.z },
        {  aabb.to.x, aabb.from.y,  aabb.to.z },
        { aabb.from.x,  aabb.to.y, aabb.from.z },
        { aabb.from.x,  aabb.to.y,  aabb.to.z },
        {  aabb.to.x,  aabb.to.y, aabb.from.z },
        {  aabb.to.x,  aabb.to.y,  aabb.to.z },

        { aabb.from.x, aabb.from.y, aabb.from.z },
        { aabb.from.x,  aabb.to.y, aabb.from.z },
        {  aabb.to.x, aabb.from.y, aabb.from.z },
        {  aabb.to.x,  aabb.to.y, aabb.from.z },
        { aabb.from.x, aabb.from.y,  aabb.to.z },
        { aabb.from.x,  aabb.to.y,  aabb.to.z },
        {  aabb.to.x, aabb.from.y,  aabb.to.z },
        {  aabb.to.x,  aabb.to.y,  aabb.to.z }
    };
    dbgDrawLines(vertices, 24, color, time);
}

static gfxm::vec3 makeVertex(const gfxm::plane& A, const gfxm::plane& B, const gfxm::plane& C) {
    float d0 = gfxm::dot(A.normal, B.normal);
    float d1 = gfxm::dot(A.normal, C.normal);
    float d2 = gfxm::dot(B.normal, C.normal);
    
    const gfxm::vec3 lineN = gfxm::cross(A.normal, B.normal);
    const float det = lineN.length2();
    const gfxm::vec3 lineP = (gfxm::cross(B.normal, lineN) * A.d + gfxm::cross(lineN, A.normal) * B.d) / det;

    const float denom = gfxm::dot(C.normal, lineN);
    const gfxm::vec3 vec = C.normal * C.d - lineP;
    const float t = gfxm::dot(vec, C.normal) / denom;

    return lineP + lineN * t;
}

void dbgDrawFrustum(const gfxm::frustum& frust, uint32_t color, float time) {
    const gfxm::vec3 points[8] = {
        makeVertex(frust.planes[gfxm::FRUSTUM_PLANE_NX], frust.planes[gfxm::FRUSTUM_PLANE_PZ], frust.planes[gfxm::FRUSTUM_PLANE_PY]),
        makeVertex(frust.planes[gfxm::FRUSTUM_PLANE_PZ], frust.planes[gfxm::FRUSTUM_PLANE_PX], frust.planes[gfxm::FRUSTUM_PLANE_PY]),
        makeVertex(frust.planes[gfxm::FRUSTUM_PLANE_PX], frust.planes[gfxm::FRUSTUM_PLANE_NZ], frust.planes[gfxm::FRUSTUM_PLANE_PY]),
        makeVertex(frust.planes[gfxm::FRUSTUM_PLANE_NZ], frust.planes[gfxm::FRUSTUM_PLANE_NX], frust.planes[gfxm::FRUSTUM_PLANE_PY]),
        makeVertex(frust.planes[gfxm::FRUSTUM_PLANE_NX], frust.planes[gfxm::FRUSTUM_PLANE_PZ], frust.planes[gfxm::FRUSTUM_PLANE_NY]),
        makeVertex(frust.planes[gfxm::FRUSTUM_PLANE_PZ], frust.planes[gfxm::FRUSTUM_PLANE_PX], frust.planes[gfxm::FRUSTUM_PLANE_NY]),
        makeVertex(frust.planes[gfxm::FRUSTUM_PLANE_PX], frust.planes[gfxm::FRUSTUM_PLANE_NZ], frust.planes[gfxm::FRUSTUM_PLANE_NY]),
        makeVertex(frust.planes[gfxm::FRUSTUM_PLANE_NZ], frust.planes[gfxm::FRUSTUM_PLANE_NX], frust.planes[gfxm::FRUSTUM_PLANE_NY]),
    };

    dbgDrawLine(points[0], points[1], color);
    dbgDrawLine(points[1], points[2], color);
    dbgDrawLine(points[2], points[3], color);
    dbgDrawLine(points[3], points[0], color);

    dbgDrawLine(points[4], points[5], color);
    dbgDrawLine(points[5], points[6], color);
    dbgDrawLine(points[6], points[7], color);
    dbgDrawLine(points[7], points[4], color);

    dbgDrawLine(points[0], points[4], color);
    dbgDrawLine(points[1], points[5], color);
    dbgDrawLine(points[2], points[6], color);
    dbgDrawLine(points[3], points[7], color);


    dbgDrawArrow((points[7] + points[4] + points[0] + points[3]) / 4.f, frust.planes[0].normal, DBG_COLOR_RED);
    dbgDrawArrow((points[1] + points[2] + points[6] + points[5]) / 4.f, frust.planes[1].normal, DBG_COLOR_RED);
    dbgDrawArrow((points[4] + points[5] + points[6] + points[7]) / 4.f, frust.planes[2].normal, DBG_COLOR_GREEN);
    dbgDrawArrow((points[0] + points[1] + points[2] + points[3]) / 4.f, frust.planes[3].normal, DBG_COLOR_GREEN);
    dbgDrawArrow((points[2] + points[3] + points[6] + points[7]) / 4.f, frust.planes[4].normal, DBG_COLOR_BLUE);
    dbgDrawArrow((points[0] + points[1] + points[4] + points[5]) / 4.f, frust.planes[5].normal, DBG_COLOR_BLUE | DBG_COLOR_RED);
}

void dbgDrawPortal(const gfxm::mat4& transform, const gfxm::vec2& half_extents, uint32_t color, float time) {
    const gfxm::vec3 points[5] = {
        transform * gfxm::vec4(-half_extents.x, -half_extents.y, .0f, 1.f),
        transform * gfxm::vec4(half_extents.x, -half_extents.y, .0f, 1.f),
        transform * gfxm::vec4(half_extents.x, half_extents.y, .0f, 1.f),
        transform * gfxm::vec4(-half_extents.x, half_extents.y, .0f, 1.f),
        transform * gfxm::vec4(.0f, .0f, .0f, 1.f)
    };

    dbgDrawLine(points[0], points[1], color, time);
    dbgDrawLine(points[1], points[2], color, time);
    dbgDrawLine(points[2], points[3], color, time);
    dbgDrawLine(points[3], points[0], color, time);

    dbgDrawArrow(points[4], transform[2], DBG_COLOR_BLUE, time);
}

