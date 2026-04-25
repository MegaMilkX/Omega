#include "gl_renderer.hpp"

#include "platform/gl/glextutil.h"
#include "platform/platform.hpp"


namespace xui {

    GLRenderer::GLRenderer() {
        {
            uint32_t white = 0xFFFFFFFF;
            uint32_t black = 0xFF000000;
            glGenTextures(1, &tex_white);
            glGenTextures(1, &tex_black);

            glBindTexture(GL_TEXTURE_2D, tex_white);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, (void*)&white);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

            glBindTexture(GL_TEXTURE_2D, tex_black);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, (void*)&black);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        }

        {
            const char* vs_text = R"(
            #version 460 
            layout (location = 0) in vec3 vertexPosition;
            layout (location = 1) in vec2 uv;
            layout (location = 2) in vec4 colorRGBA;
            uniform mat4 matView;
            uniform mat4 matProjection;
            uniform mat4 matModel;
            out vec4 fragColor;
            out vec2 fragUV;
            void main() {
                fragColor = colorRGBA;
                fragUV = uv;
                gl_Position = matProjection * matView * matModel * vec4(vertexPosition, 1.0);
            })";
            const char* fs_text = R"(
            #version 460
            uniform sampler2D texAlbedo;
            in vec4 fragColor;
            in vec2 fragUV;
            out vec4 outAlbedo;
            void main(){
                vec4 t = texture(texAlbedo, fragUV);
                outAlbedo = fragColor * t;
            })";
            prog_default = glCreateProgram();
            GLuint vs = glCreateShader(GL_VERTEX_SHADER);
            GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
            glShaderSource(vs, 1, &vs_text, nullptr);
            glShaderSource(fs, 1, &fs_text, nullptr);
            glCompileShader(vs);
            glCompileShader(fs);
            glAttachShader(prog_default, vs);
            glAttachShader(prog_default, fs);
            glLinkProgram(prog_default);
            glDeleteShader(vs);
            glDeleteShader(fs);
        }
        {
            const char* vs_text = R"(
                #version 460 
                layout (location = 0) in vec3 inPosition;
                layout (location = 1) in vec2 inUV;
                layout (location = 2) in float inTextUVLookup;
                layout (location = 3) in vec4 inColorRGBA;
                out vec2 uv_frag;
                out vec4 col_frag;

                uniform sampler2D texTextUVLookupTable;

                uniform mat4 matProjection;
                uniform mat4 matView;
                uniform mat4 matModel;
        
                void main(){
                    vec2 uv_ = texelFetch(texTextUVLookupTable, ivec2(inTextUVLookup, 0), 0).xy;
                    uv_frag = uv_;
                    col_frag = inColorRGBA;        

                    vec3 pos3 = inPosition;
	                pos3.x = round(pos3.x);
	                pos3.y = round(pos3.y);

                    vec3 scale = vec3(length(matModel[0]),
                            length(matModel[1]),
                            length(matModel[2])); 
                
                    mat4 scaleHack;
                    scaleHack[0] = vec4(1, 0, 0, 0);
                    scaleHack[1] = vec4(0, -1, 0, 0);
                    scaleHack[2] = vec4(0, 0, 1, 0);
                    scaleHack[3] = vec4(0, 0, 0, 1);
	                vec4 pos = matProjection * matView * matModel * scaleHack * vec4(inPosition, 1);
                    gl_Position = pos;
                })";
            const char* fs_text = R"(
                #version 460
                in vec2 uv_frag;
                in vec4 col_frag;
                out vec4 outAlbedo;

                uniform sampler2D texAlbedo;
                uniform vec4 color;
                void main(){
                    float c = texture(texAlbedo, uv_frag).x;
                    outAlbedo = vec4(1, 1, 1, c) * col_frag * color;
                })";
            prog_text = glCreateProgram();
            GLuint vs = glCreateShader(GL_VERTEX_SHADER);
            GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
            glShaderSource(vs, 1, &vs_text, nullptr);
            glShaderSource(fs, 1, &fs_text, nullptr);
            glCompileShader(vs);
            glCompileShader(fs);
            glAttachShader(prog_text, vs);
            glAttachShader(prog_text, fs);
            glLinkProgram(prog_text);
            glDeleteShader(vs);
            glDeleteShader(fs);
        }
    }
    GLRenderer::~GLRenderer() {
        glDeleteProgram(prog_default);
        glDeleteProgram(prog_text);

        glDeleteTextures(1, &tex_white);
        glDeleteTextures(1, &tex_black);
    }

    void GLRenderer::drawLineStrip(const Vertex* vertices, int count) {
        DrawCmd& cmd = emplaceCmd();
        cmd.primitive = DRAW_PRIM_LINE_STRIP;
        cmd.vertex_first = this->vertices.size();
        cmd.vertex_count = count;
        cmd.color = 0xFFFFFFFF;
        cmd.tex0 = tex_white;
        cmd.tex1 = 0;
        cmd.offset = getOffset();

        this->vertices.insert(this->vertices.end(), vertices, vertices + count);
    }

    void GLRenderer::drawTriangleStrip(const Vertex* vertices, int count, uint64_t texture) {
        DrawCmd& cmd = emplaceCmd();
        cmd.primitive = DRAW_PRIM_TRIANGLE_STRIP;
        cmd.vertex_first = this->vertices.size();
        cmd.vertex_count = count;
        cmd.color = 0xFFFFFFFF;
        cmd.tex0 = texture == 0 ? tex_white : texture;
        cmd.tex1 = 0;
        cmd.offset = getOffset();

        this->vertices.insert(this->vertices.end(), vertices, vertices + count);
    }

    void GLRenderer::drawText(const TextVertex* vertices, int count, const Font* font) {
        auto textures = const_cast<Font*>(font)->getTextureData();

        DrawCmd& cmd = emplaceCmd();
        cmd.primitive = DRAW_PRIM_TEXT;
        cmd.vertex_first = text_vertices.size();
        cmd.vertex_count = count;
        cmd.color = 0xFFFFFFFF;
        cmd.tex0 = textures->atlas->getId();
        cmd.tex1 = textures->lut->getId();
        cmd.offset = getOffset();

        text_vertices.insert(text_vertices.end(), vertices, vertices + count);
    }

    void GLRenderer::render(const DrawCmd* commands, int count, int width, int height, bool clear) {
        glBindVertexArray(0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, width, height);
        glScissor(0, 0, width, height);

        current_clip_rect = gfxm::rect(0, 0, width, height);
        scissor_stack.push(current_clip_rect);

        glEnable(GL_SCISSOR_TEST);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glDepthMask(GL_FALSE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);

        if (clear) {
            glClearColor(.09f, .1f, .2f, 0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        }

        {
            GLuint vao_text;
            glGenVertexArrays(1, &vao_text);

            gpuBuffer textVertexBuffer;
            {
                textVertexBuffer.setArrayData(text_vertices.data(), text_vertices.size() * sizeof(text_vertices[0]));

                glBindVertexArray(vao_text);

                glEnableVertexAttribArray(0);
                glEnableVertexAttribArray(1);
                glEnableVertexAttribArray(2);
                glEnableVertexAttribArray(3);

                glBindBuffer(GL_ARRAY_BUFFER, textVertexBuffer.getId());
                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(TextVertex), 0);
                glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(TextVertex), (const void*)offsetof(TextVertex, uv));
                glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(TextVertex), (const void*)offsetof(TextVertex, uv_lookup));
                glVertexAttribPointer(3, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(TextVertex), (const void*)offsetof(TextVertex, rgba));
                glBindVertexArray(0);
            }

            GLuint vao_verts;
            glGenVertexArrays(1, &vao_verts);
            gpuBuffer vertexBuffer;
            {
                vertexBuffer.setArrayData(vertices.data(), vertices.size() * sizeof(vertices[0]));

                glBindVertexArray(vao_verts);

                glEnableVertexAttribArray(0);
                glEnableVertexAttribArray(1);
                glEnableVertexAttribArray(2);

                glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer.getId());
                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
                glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, uv));
                glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), (const void*)offsetof(Vertex, rgba));
                glBindVertexArray(0);
            }

            for (int i = 0; i < count; ++i) {
                const auto& cmd = commands[i];
                const gfxm::mat4 view = (gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(cmd.offset.x, cmd.offset.y, .0f)));
                const gfxm::mat4 proj = gfxm::ortho(.0f, float(width), float(height), .0f, -100.f, 100.f);
                const gfxm::mat4 model = gfxm::mat4(1.f);
                if (cmd.primitive == DRAW_PRIM_TRIANGLE_STRIP) {
                    glBindVertexArray(vao_verts);
                    auto prog = prog_default;
                    glUseProgram(prog);
                    glUniformMatrix4fv(glGetUniformLocation(prog, "matView"), 1, GL_FALSE, (float*)&view);
                    glUniformMatrix4fv(glGetUniformLocation(prog, "matProjection"), 1, GL_FALSE, (float*)&proj);
                    glUniformMatrix4fv(glGetUniformLocation(prog, "matModel"), 1, GL_FALSE, (float*)&model);
                    glUniform1i(glGetUniformLocation(prog, "texAlbedo"), 0);

                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, cmd.tex0);

                    glDrawArrays(GL_TRIANGLE_STRIP, cmd.vertex_first, cmd.vertex_count);
                } else if(cmd.primitive == DRAW_PRIM_TEXT) {
                    glBindVertexArray(vao_text);
                    glUseProgram(prog_text);

                    glUniformMatrix4fv(glGetUniformLocation(prog_text, "matView"), 1, GL_FALSE, (float*)&view);
                    glUniformMatrix4fv(glGetUniformLocation(prog_text, "matProjection"), 1, GL_FALSE, (float*)&proj);
                    glUniform1i(glGetUniformLocation(prog_text, "texAlbedo"), 0);
                    glUniform1i(glGetUniformLocation(prog_text, "texTextUVLookupTable"), 1);

                    gfxm::vec4 colorf;
                    colorf[0] = ((cmd.color & 0xff000000) >> 24) / 255.0f;
                    colorf[1] = ((cmd.color & 0x00ff0000) >> 16) / 255.0f;
                    colorf[2] = ((cmd.color & 0x0000ff00) >> 8) / 255.0f;
                    colorf[3] = (cmd.color & 0x000000ff) / 255.0f;

                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, cmd.tex0);
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, cmd.tex1);

                    glUniformMatrix4fv(glGetUniformLocation(prog_text, "matModel"), 1, GL_FALSE, (float*)&model);
                    glUniform4fv(glGetUniformLocation(prog_text, "color"), 1, (float*)&colorf);
                    glDrawArrays(GL_TRIANGLES, cmd.vertex_first, cmd.vertex_count);
                } else if (cmd.primitive == DRAW_PRIM_LINE_STRIP) {
                    glBindVertexArray(vao_verts);
                    auto prog = prog_default;
                    glUseProgram(prog);
                    glUniformMatrix4fv(glGetUniformLocation(prog, "matView"), 1, GL_FALSE, (float*)&view);
                    glUniformMatrix4fv(glGetUniformLocation(prog, "matProjection"), 1, GL_FALSE, (float*)&proj);
                    glUniformMatrix4fv(glGetUniformLocation(prog, "matModel"), 1, GL_FALSE, (float*)&model);
                    glUniform1i(glGetUniformLocation(prog, "texAlbedo"), 0);

                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, cmd.tex0);

                    glDrawArrays(GL_LINE_STRIP, cmd.vertex_first, cmd.vertex_count);
                } else if (cmd.primitive == DRAW_PRIM_CLIP_PUSH) {
                    gfxm::rect new_clip_rect(cmd.rc_x, height - cmd.rc_y - cmd.rc_h, cmd.rc_w, cmd.rc_h);
                    new_clip_rect.min.x = gfxm::_max(new_clip_rect.min.x, current_clip_rect.min.x);
                    new_clip_rect.min.y = gfxm::_max(new_clip_rect.min.y, current_clip_rect.min.y);
                    int overflow_x = gfxm::_max(.0f, (new_clip_rect.min.x + new_clip_rect.max.x) - (current_clip_rect.min.x + current_clip_rect.max.x));
                    new_clip_rect.max.x = gfxm::_max(.0f, new_clip_rect.max.x - overflow_x);
                    int overflow_y = gfxm::_max(.0f, (new_clip_rect.min.y + new_clip_rect.max.y) - (current_clip_rect.min.y + current_clip_rect.max.y));
                    new_clip_rect.max.y = gfxm::_max(.0f, new_clip_rect.max.y - overflow_y);
                    scissor_stack.push(new_clip_rect);
                    current_clip_rect = new_clip_rect;
                    glScissor(current_clip_rect.min.x, current_clip_rect.min.y, current_clip_rect.max.x, current_clip_rect.max.y);
                } else if (cmd.primitive == DRAW_PRIM_CLIP_POP) {
                    assert(scissor_stack.size() > 1);
                    scissor_stack.pop();
                    current_clip_rect = scissor_stack.top();
                    // max is actually size here, was lazy
                    glScissor(current_clip_rect.min.x, current_clip_rect.min.y, current_clip_rect.max.x, current_clip_rect.max.y);
                }
            }

            vertices.clear();
            text_vertices.clear();

            glBindVertexArray(0);
            glDeleteVertexArrays(1, &vao_text);
            glDeleteVertexArrays(1, &vao_verts);
        }

        glViewport(0, 0, width, height);
        glScissor(0, 0, width, height);

        while (!scissor_stack.empty()) {
            scissor_stack.pop();
        }
    }
}