#include "gui_shaders.hpp"


static gpuShaderProgram* shader_text_selection = 0;
static gpuShaderProgram* shader_text = 0;
static gpuShaderProgram* shader_rect = 0;

static GLuint texture_white = 0;
static GLuint texture_black = 0;

void _guiInitShaders() {
    uint32_t white = 0xFFFFFFFF;
    uint32_t black = 0xFF000000;
    glGenTextures(1, &texture_white);
    glGenTextures(1, &texture_black);

    glBindTexture(GL_TEXTURE_2D, texture_white);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, (void*)&white);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glBindTexture(GL_TEXTURE_2D, texture_black);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, (void*)&black);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    {
        const char* vs = R"(
                #version 450 
                layout (location = 0) in vec3 vertexPosition;
                uniform mat4 matView;
                uniform mat4 matProjection;
                uniform mat4 matModel;
                void main() {
                    gl_Position = matProjection * matView * matModel * vec4(vertexPosition, 1.0);
                })";
        const char* fs = R"(
                #version 450
                uniform vec4 color;
                out vec4 outAlbedo;
                void main(){
                    outAlbedo = color;
                })";
        shader_text_selection = new gpuShaderProgram(vs, fs);
    }
    {
        const char* vs_text = R"(
            #version 450 
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
        shader_text = new gpuShaderProgram(vs_text, fs_text);
    }
    {
        const char* vs = R"(
        #version 450 
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
        const char* fs = R"(
        #version 450
        uniform sampler2D texAlbedo;
        in vec4 fragColor;
        in vec2 fragUV;
        out vec4 outAlbedo;
        void main(){
            vec4 t = texture(texAlbedo, fragUV);
            outAlbedo = fragColor * t;
        })";
        shader_rect = new gpuShaderProgram(vs, fs);
    }
}
void _guiCleanupShaders() {
    delete shader_text;
    delete shader_text_selection;
}
gpuShaderProgram* _guiGetShaderTextSelection() {
    return shader_text_selection;
}
gpuShaderProgram* _guiGetShaderText() {
    return shader_text;
}
gpuShaderProgram* _guiGetShaderRect() {
    return shader_rect;
}

GLuint _guiGetTextureWhite() {
    return texture_white;
}
GLuint _guiGetTextureBlack() {
    return texture_black;
}
