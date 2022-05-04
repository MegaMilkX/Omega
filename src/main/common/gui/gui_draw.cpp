#include "common/gui/gui_draw.hpp"

#include "platform/platform.hpp"

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
    gfxm::mat4 view = gfxm::mat4(1.0f);
    gfxm::mat4 proj = gfxm::ortho(.0f, (float)screen_w, (float)screen_h, .0f, .0f, 100.0f);

    glUseProgram(prog.getId());
    glUniformMatrix4fv(prog.getUniformLocation("matView"), 1, GL_FALSE, (float*)&view);
    glUniformMatrix4fv(prog.getUniformLocation("matProjection"), 1, GL_FALSE, (float*)&proj);
    glUniformMatrix4fv(prog.getUniformLocation("matModel"), 1, GL_FALSE, (float*)&model);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

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
    gfxm::mat4 view = gfxm::mat4(1.0f);
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
    gfxm::mat4 view = gfxm::mat4(1.0f);
    gfxm::mat4 proj = gfxm::ortho(.0f, (float)screen_w, (float)screen_h, .0f, .0f, 100.0f);

    glUseProgram(prog.getId());
    glUniformMatrix4fv(prog.getUniformLocation("matView"), 1, GL_FALSE, (float*)&view);
    glUniformMatrix4fv(prog.getUniformLocation("matProjection"), 1, GL_FALSE, (float*)&proj);
    glUniformMatrix4fv(prog.getUniformLocation("matModel"), 1, GL_FALSE, (float*)&model);

    glDrawArrays(GL_TRIANGLE_FAN, 0, n_points + 1);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
}

void guiDrawRectLine(const gfxm::rect& rect) {
    int screen_w = 0, screen_h = 0;
    platformGetWindowSize(screen_w, screen_h);

    gfxm::rect rct = rect;

    float vertices[] = {
        rct.min.x, rct.min.y, 0, rct.max.x, rct.min.y, 0,
        rct.max.x, rct.max.y, 0, rct.min.x, rct.max.y, 0,
        rct.min.x, rct.min.y, 0
    };
    uint32_t colors[] = {
         0xFF00FF00, 0xFF00FF00,
         0xFF00FF00, 0xFF00FF00,
         0xFF00FF00
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
    gfxm::mat4 view = gfxm::mat4(1.0f);
    gfxm::mat4 proj = gfxm::ortho(.0f, (float)screen_w, (float)screen_h, .0f, .0f, 100.0f);

    glUseProgram(prog.getId());
    glUniformMatrix4fv(prog.getUniformLocation("matView"), 1, GL_FALSE, (float*)&view);
    glUniformMatrix4fv(prog.getUniformLocation("matProjection"), 1, GL_FALSE, (float*)&proj);
    glUniformMatrix4fv(prog.getUniformLocation("matModel"), 1, GL_FALSE, (float*)&model);

    glDrawArrays(GL_LINE_STRIP, 0, 5);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
}

gfxm::vec2 guiCalcTextRect(const char* text, Font* font, float max_width) {
    std::unique_ptr<gpuText> gpu_text(new gpuText(font));
    gpu_text->setString(text);
    gpu_text->commit(max_width);

    return gpu_text->getBoundingSize();
}
void guiDrawText(const gfxm::vec2& pos, const char* text, Font* font, float max_width, uint32_t col) {
    int screen_w = 0, screen_h = 0;
    platformGetWindowSize(screen_w, screen_h);

    gpuTexture2d glyph_atlas;
    gpuTexture2d tex_font_lut;
    ktImage imgFontAtlas;
    ktImage imgFontLookupTexture;
    font->buildAtlas(&imgFontAtlas, &imgFontLookupTexture);
    glyph_atlas.setData(&imgFontAtlas);
    tex_font_lut.setData(&imgFontLookupTexture);
    tex_font_lut.setFilter(GPU_TEXTURE_FILTER_NEAREST);

    std::unique_ptr<gpuText> gpu_text(new gpuText(font));
    gpu_text->setString(text);
    gpu_text->commit(max_width);

    const char* vs_text = R"(
        #version 450 
        layout (location = 0) in vec3 inPosition;
        layout (location = 1) in vec2 inUV;
        layout (location = 2) in float inTextUVLookup;
        layout (location = 3) in vec3 inColorRGB;
        out vec2 uv_frag;
        out vec4 col_frag;

        uniform sampler2D texTextUVLookupTable;

        uniform mat4 matProjection;
        uniform mat4 matView;
        uniform mat4 matModel;

        uniform int lookupTextureWidth;
        
        void main(){
            float lookup_x = (inTextUVLookup + 0.5) / float(lookupTextureWidth);
            vec2 uv_ = texture(texTextUVLookupTable, vec2(lookup_x, 0), 0).xy;
            uv_frag = uv_;
            col_frag = vec4(inColorRGB, 1);        

            vec3 pos3 = inPosition;
	        pos3.x = round(pos3.x);
	        pos3.y = round(pos3.y);

            vec3 scale = vec3(length(matModel[0]),
                    length(matModel[1]),
                    length(matModel[2])); 

	        vec4 pos = matProjection * matView * matModel * vec4(inPosition, 1);
	        gl_Position = pos;
        })";
    const char* fs_text = R"(
        #version 450
        in vec2 uv_frag;
        in vec4 col_frag;
        out vec4 outAlbedo;

        uniform sampler2D texAlbedo;
        uniform vec4 color;
        void main(){
            float c = texture(texAlbedo, uv_frag).x;
            outAlbedo = vec4(1, 1, 1, c) * col_frag * color;
        })";
    gpuShaderProgram prog_text(vs_text, fs_text);

    gfxm::mat4 model
        = gfxm::translate(gfxm::mat4(1.0f), gfxm::vec3(pos.x, screen_h - pos.y, .0f));
    gfxm::mat4 view = gfxm::mat4(1.0f);
    gfxm::mat4 proj = gfxm::ortho(.0f, (float)screen_w, .0f, (float)screen_h, .0f, 100.0f);

    glUseProgram(prog_text.getId());

    glUniformMatrix4fv(prog_text.getUniformLocation("matView"), 1, GL_FALSE, (float*)&view);
    glUniformMatrix4fv(prog_text.getUniformLocation("matProjection"), 1, GL_FALSE, (float*)&proj);
    glUniformMatrix4fv(prog_text.getUniformLocation("matModel"), 1, GL_FALSE, (float*)&model);
    glUniform1i(prog_text.getUniformLocation("texAlbedo"), 0);
    glUniform1i(prog_text.getUniformLocation("texTextUVLookupTable"), 1);
    
    gfxm::vec4 colorf;
    colorf[0] = ((col & 0xff000000) >> 24) / 255.0f;
    colorf[1] = ((col & 0x00ff0000) >> 16) / 255.0f;
    colorf[2] = ((col & 0x0000ff00) >> 8) / 255.0f;
    colorf[3] = (col & 0x000000ff) / 255.0f;
    glUniform4fv(prog_text.getUniformLocation("color"), 1, (float*)&colorf);
    glUniform1i(prog_text.getUniformLocation("lookupTextureWidth"), tex_font_lut.getWidth());

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, glyph_atlas.getId());
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, tex_font_lut.getId());

    gpu_text->getMeshDesc()->_bindVertexArray(VFMT::Position_GUID, 0);
    gpu_text->getMeshDesc()->_bindVertexArray(VFMT::UV_GUID, 1);
    gpu_text->getMeshDesc()->_bindVertexArray(VFMT::TextUVLookup_GUID, 2);
    gpu_text->getMeshDesc()->_bindVertexArray(VFMT::ColorRGB_GUID, 3);
    gpu_text->getMeshDesc()->_bindIndexArray();
    gpu_text->getMeshDesc()->_draw();
}