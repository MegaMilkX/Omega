#pragma once

#include "common/gui/elements/gui_element.hpp"

#include <list>


class GuiTextCache {
public:
    Font* font = 0;
    std::string str;
    //std::list<std::string> line_list;
    float max_line_width = .0f;

    std::vector<gfxm::ivec2> lines;
    int line_count = 0;
    int first_selected_line = 0;
    int last_selected_line = 0;
    int first_selected = 0;
    int last_selected = 0;

    gpuBuffer vertices_buf;
    gpuBuffer uv_buf;
    gpuBuffer rgb_buf;
    gpuBuffer text_uv_lookup_buf;
    gpuBuffer index_buf;
    gpuMeshDesc mesh_desc;

    // selection rectangles
    gpuBuffer sel_vertices_buf;
    gpuBuffer sel_index_buf;
    gpuMeshDesc sel_mesh_desc;

    void updateCache() {
        lines.clear();

        const int str_len = str.size();
        const int line_height = font->getLineHeight();
        const int adv_space = font->getGlyph('\s').horiAdvance;
        const int adv_tab = adv_space * 8;

        int vert_advance = line_height;
        int hori_advance = 0;
        int max_hori_advance = 0;
        line_count = 1;

        gfxm::ivec2 line(0, 0);
        
        auto putGlyph = [&line_height, &hori_advance, &max_hori_advance]
        (const FontGlyph& g, char ch, int i) {
            hori_advance += g.horiAdvance / 64;
            max_hori_advance = gfxm::_max(max_hori_advance, hori_advance);
        };
        auto putNewline = [&line, &vert_advance, &hori_advance, line_height, this](int i) {
            line.y = i;
            lines.push_back(line);
            line.x = line.y = line.y + 1;
            ++line_count;
            vert_advance += line_height;
            hori_advance = 0;
        };

        for (int i = 0; i < str_len; ++i) {
            char ch = str[i];
            if (hori_advance >= max_line_width && max_line_width > .0f) {
                vert_advance += line_height;
                hori_advance = 0;
            }
            if (ch == '\n') {
                putNewline(i);
                continue;
            } else if(ch == '\t') {
                int tab_reminder = hori_advance % (adv_tab / 64);
                int adv = adv_tab / 64 - tab_reminder;
                hori_advance += adv;
            } else if(ch == '#') {
                int characters_left = str_len - i - 1;
                if (characters_left >= 8) {
                    i += 8;
                    continue;
                }
            } else if(isspace(ch)) {
                const auto& g = font->getGlyph(ch);
                putGlyph(g, ch, i);
            } else {
                int tok_pos = i;
                int tok_len = 0;
                for (int j = i; j < str_len; ++j) {
                    ch = str[j];
                    if (isspace(ch)) {
                        break;
                    }
                    ++tok_len;
                }
                int word_hori_advance = 0; 
                for (int j = tok_pos; j < tok_pos + tok_len; ++j) {
                    ch = str[j];
                    const auto& g = font->getGlyph(ch);
                    word_hori_advance += g.horiAdvance / 64;
                }
                if (hori_advance + word_hori_advance >= max_line_width && max_line_width > .0f) {
                    putNewline(i);
                }
                for (int j = tok_pos; j < tok_pos + tok_len; ++j) {
                    ch = str[j];
                    const auto& g = font->getGlyph(ch);
                    putGlyph(g, ch, j);
                }            

                i += tok_len - 1;
            }
        }

        line.y = str.size();
        lines.push_back(line);
        /*
        ret_cursor = str_len;
        (*out_screen_x) = (float)max_hori_advance;
        
    abort:
        return ret_cursor;*/
    }
public:
    GuiTextCache(Font* font)
    : font(font) {

    }

    void setStr(const char* string, float max_line_width = .0f) {
        str = string;
        this->max_line_width = max_line_width;
        updateCache();
    }
    void setMaxLineWidth(float max_line_width = .0f) {
        this->max_line_width = max_line_width;
        updateCache();
    }

    gfxm::ivec2 select(const gfxm::rect& rc_selection) {
        gfxm::rect rc = rc_selection;

        first_selected_line = gfxm::_min(gfxm::_max((rc.min.y - font->getDescender()) / font->getLineHeight(), .0f), (float)line_count - 1);
        last_selected_line = gfxm::_max(gfxm::_min((rc.max.y - font->getDescender()) / font->getLineHeight(), (float)line_count - 1), .0f);
        if (first_selected_line > last_selected_line) {
            first_selected_line += last_selected_line;
            last_selected_line = first_selected_line - last_selected_line;
            first_selected_line = first_selected_line - last_selected_line;

            std::swap(rc.min.x, rc.max.x);
        }

        const int line_height = font->getLineHeight();

        auto findPos = [this, line_height](float pointer, int line, int* cursor) {
            int hori_advance = 0;
            int line_offset = line_height * (line + 1);
            for (int j = lines[line].x; j < lines[line].y; ++j) {
                char ch = str[j];
                if (ch == '\n' || ch == '\t') {
                    continue;
                }

                auto& g = font->getGlyph(ch);
                if (hori_advance > pointer) {
                    *cursor = j;
                    //(*out_screen_x) = (float)hori_advance;
                    return;
                }

                hori_advance += g.horiAdvance / 64;
            }
        };

        if (rc.min.x > rc.max.x && first_selected_line == last_selected_line) {
            std::swap(rc.min.x, rc.max.x);
        }
        findPos(rc.min.x, first_selected_line, &first_selected);
        findPos(rc.max.x, last_selected_line, &last_selected);

        return gfxm::ivec2(first_selected_line, last_selected_line);
    }

    int getFirstSelectedLine() const {
        return first_selected_line;
    }
    int getLastSelectedLine() const {
        return last_selected_line;
    }

    void prepareDraw() {
        const int line_height = font->getLineHeight();

        std::vector<float> vertices;
        std::vector<float> uv;
        std::vector<float> uv_lookup;
        std::vector<uint32_t> colors;
        std::vector<uint32_t> indices;

        std::vector<float>      verts_selection;
        std::vector<uint32_t>   indices_selection;

        uint32_t color_default = GUI_COL_TEXT;
        uint32_t color_selected = GUI_COL_TEXT_HIGHLIGHTED;

        for (int i = 0; i < lines.size(); ++i) {
            int hori_advance = 0;
            int line_offset = line_height * (i + 1);
            for (int j = lines[i].x; j < lines[i].y; ++j) {
                uint32_t color = GUI_COL_TEXT;
                char ch = str[j];
                if (ch == '\n' || ch == '\t') {
                    continue;
                }

                auto& g = font->getGlyph(ch);
                int y_ofs = g.height - g.bearingY;
                int x_ofs = g.bearingX;
                int glyph_advance = g.horiAdvance / 64;

                // selection
                if (j >= first_selected && j < last_selected) {
                    color = color_selected;

                    uint32_t base_index = verts_selection.size() / 3;
                    float sel_verts[] = {
                        hori_advance,                       0 - line_offset - font->getDescender(),            0,
                        hori_advance + glyph_advance,       0 - line_offset - font->getDescender(),            0,
                        hori_advance,                       line_height - line_offset - font->getDescender(),     0,
                        hori_advance + glyph_advance,       line_height - line_offset - font->getDescender(),     0
                    };
                    verts_selection.insert(verts_selection.end(), sel_verts, sel_verts + sizeof(sel_verts) / sizeof(sel_verts[0]));
                    uint32_t sel_indices[] = {
                        base_index, base_index + 1, base_index + 2,
                        base_index + 1, base_index + 3, base_index + 2
                    };
                    indices_selection.insert(indices_selection.end(), sel_indices, sel_indices + sizeof(sel_indices) / sizeof(sel_indices[0]));
                }
                // --

                if (isspace(ch)) {
                    hori_advance += g.horiAdvance / 64;
                    continue;
                }
                
                uint32_t base_index = vertices.size() / 3;
                float glyph_verts[] = {
                    hori_advance + x_ofs,               0 - y_ofs - line_offset,            0,
                    hori_advance + g.width + x_ofs,     0 - y_ofs - line_offset,            0,
                    hori_advance + x_ofs,               g.height - y_ofs - line_offset,     0,
                    hori_advance + g.width + x_ofs,     g.height - y_ofs - line_offset,     0
                };
                vertices.insert(vertices.end(), glyph_verts, glyph_verts + sizeof(glyph_verts) / sizeof(glyph_verts[0]));

                uint32_t glyph_indices[] = {
                    base_index, base_index + 1, base_index + 2,
                    base_index + 1, base_index + 3, base_index + 2
                };
                indices.insert(indices.end(), glyph_indices, glyph_indices + sizeof(glyph_indices) / sizeof(glyph_indices[0]));

                float glyph_uv[] = { // Flipped
                    0.0f, 1.0f,     1.0f, 1.0f,     0.0f, 0.0f,     1.0f, 0.0f
                };
                uv.insert(uv.end(), glyph_uv, glyph_uv + sizeof(glyph_uv) / sizeof(glyph_uv[0]));

                float glyph_uv_lookup[] = {
                    ch * 4, ch * 4 + 1, ch * 4 + 3, ch * 4 + 2
                };
                uv_lookup.insert(uv_lookup.end(), glyph_uv_lookup, glyph_uv_lookup + sizeof(glyph_uv_lookup) / sizeof(glyph_uv_lookup[0]));

                colors.insert(colors.end(), 4, color);

                hori_advance += g.horiAdvance / 64;
            }
        }

        // text
        vertices_buf.setArrayData(vertices.data(), vertices.size() * sizeof(vertices[0]));
        uv_buf.setArrayData(uv.data(), uv.size() * sizeof(uv[0]));
        rgb_buf.setArrayData(colors.data(), colors.size() * sizeof(colors[0]));
        text_uv_lookup_buf.setArrayData(uv_lookup.data(), uv_lookup.size() * sizeof(uv_lookup[0]));
        index_buf.setArrayData(indices.data(), indices.size() * sizeof(indices[0]));

        mesh_desc.setDrawMode(MESH_DRAW_MODE::MESH_DRAW_TRIANGLES);
        mesh_desc.setAttribArray(VFMT::Position_GUID, &vertices_buf);
        mesh_desc.setAttribArray(VFMT::UV_GUID, &uv_buf);
        mesh_desc.setAttribArray(VFMT::ColorRGBA_GUID, &rgb_buf);
        mesh_desc.setAttribArray(VFMT::TextUVLookup_GUID, &text_uv_lookup_buf);
        mesh_desc.setIndexArray(&index_buf);

        // selection rectangles
        sel_vertices_buf.setArrayData(verts_selection.data(), verts_selection.size() * sizeof(verts_selection[0]));
        sel_index_buf.setArrayData(indices_selection.data(), indices_selection.size() * sizeof(indices_selection[0]));
        sel_mesh_desc.setDrawMode(MESH_DRAW_MODE::MESH_DRAW_TRIANGLES);
        sel_mesh_desc.setAttribArray(VFMT::Position_GUID, &sel_vertices_buf);
        sel_mesh_desc.setIndexArray(&sel_index_buf);
    }

    void draw(const gfxm::vec2& pos, uint32_t col, uint32_t selection_col) {
        int screen_w = 0, screen_h = 0;
        platformGetWindowSize(screen_w, screen_h);

        // selection rectangles
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
            gpuShaderProgram prog(vs, fs);

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDepthMask(GL_FALSE);

            gfxm::mat4 model
                = gfxm::translate(gfxm::mat4(1.0f), gfxm::vec3(pos.x, screen_h - pos.y, .0f));
            gfxm::mat4 view = gfxm::mat4(1.0f);
            gfxm::mat4 proj = gfxm::ortho(.0f, (float)screen_w, .0f, (float)screen_h, .0f, 100.0f);

            glUseProgram(prog.getId());
            glUniformMatrix4fv(prog.getUniformLocation("matView"), 1, GL_FALSE, (float*)&view);
            glUniformMatrix4fv(prog.getUniformLocation("matProjection"), 1, GL_FALSE, (float*)&proj);
            glUniformMatrix4fv(prog.getUniformLocation("matModel"), 1, GL_FALSE, (float*)&model);
            gfxm::vec4 colorf;
            colorf[3] = ((selection_col & 0xff000000) >> 24) / 255.0f;
            colorf[2] = ((selection_col & 0x00ff0000) >> 16) / 255.0f;
            colorf[1] = ((selection_col & 0x0000ff00) >> 8) / 255.0f;
            colorf[0] = (selection_col & 0x000000ff) / 255.0f;
            glUniform4fv(prog.getUniformLocation("color"), 1, (float*)&colorf);

            sel_mesh_desc._bindVertexArray(VFMT::Position_GUID, 0);
            sel_mesh_desc._bindIndexArray();
            sel_mesh_desc._draw();
        }

        // text
        gpuTexture2d glyph_atlas;
        gpuTexture2d tex_font_lut;
        ktImage imgFontAtlas;
        ktImage imgFontLookupTexture;
        font->buildAtlas(&imgFontAtlas, &imgFontLookupTexture);
        glyph_atlas.setData(&imgFontAtlas);
        tex_font_lut.setData(&imgFontLookupTexture);
        tex_font_lut.setFilter(GPU_TEXTURE_FILTER_NEAREST);

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

            uniform int lookupTextureWidth;
        
            void main(){
                float lookup_x = (inTextUVLookup + 0.5) / float(lookupTextureWidth);
                vec2 uv_ = texture(texTextUVLookupTable, vec2(lookup_x, 0), 0).xy;
                uv_frag = uv_;
                col_frag = inColorRGBA;        

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

        mesh_desc._bindVertexArray(VFMT::Position_GUID, 0);
        mesh_desc._bindVertexArray(VFMT::UV_GUID, 1);
        mesh_desc._bindVertexArray(VFMT::TextUVLookup_GUID, 2);
        mesh_desc._bindVertexArray(VFMT::ColorRGBA_GUID, 3);
        mesh_desc._bindIndexArray();
        mesh_desc._draw();
    }
};

class GuiTextBox : public GuiElement {
    Font* font = 0;
    
    GuiTextCache text_cache;
    gfxm::ivec2 selected_lines;
    
    std::string caption = "TextBox";
    std::string text = R"(Then Fingolfin beheld (as it seemed to him) the utter ruin of the Noldor, 
and the defeat beyond redress of all their houses; 
and filled with wrath and despair he mounted upon Rochallor his great horse and rode forth alone, 
and none might restrain him. He passed over Dor-nu-Fauglith like a wind amid the dust, 
and all that beheld his onset fled in amaze, thinking that Orome himself was come: 
for a great madness of rage was upon him, so that his eyes shone like the eyes of the Valar. 
Thus he came alone to Angband's gates, and he sounded his horn, 
and smote once more upon the brazen doors, 
and challenged Morgoth to come forth to single combat. And Morgoth came.)";
    gfxm::rect rc_caption;
    gfxm::rect rc_box;
    gfxm::ivec2 selection;
    gfxm::vec2 selection_screen;
    int cursor = 0;
    float cursor_screen = .0f;
    gfxm::vec2 pressed_pos;
    gfxm::vec2 mouse_pos;
    bool pressing = false;
public:
    GuiTextBox(Font* fnt)
        : font(fnt), text_cache(font) {
        cursor = text.size();

        text_cache.setStr(text.c_str());
    }

    void onMessage(GUI_MSG msg, uint64_t a_param, uint64_t b_param) override {
        switch (msg) {
        case GUI_MSG::UNICHAR:
            switch ((GUI_CHAR)a_param) {
            case GUI_CHAR::BACKSPACE:
                if (!text.empty()) {
                    if (selection.x == selection.y) {
                        if (cursor > 0) {
                            --cursor;
                            text.erase(text.begin() + cursor);
                        }
                    } else {
                        text.erase(text.begin() + selection.x, text.begin() + selection.y);
                        cursor = gfxm::_min(selection.x, selection.y);
                        selection = gfxm::ivec2(0, 0);
                    }
                }
                break;
            case GUI_CHAR::ESCAPE:
                break;
            case GUI_CHAR::LINEFEED:
                break;
            case GUI_CHAR::RETURN:
                break;
            case GUI_CHAR::TAB:
                break;
            default:
                text.insert(text.begin() + cursor, (char)a_param);
                ++cursor;
                cursor_screen += font->getGlyph((char)a_param).horiAdvance / 64;
            }
            break;
        case GUI_MSG::MOUSE_MOVE:
            mouse_pos = gfxm::vec2(a_param, b_param);
            if (pressing) {
                selection.y = font->findCursorPos(text.c_str(), text.size(), (mouse_pos.x - rc_box.min.x), .0f, &selection_screen.x);
                selected_lines = text_cache.select(
                    gfxm::rect(
                        pressed_pos - rc_box.min,
                        mouse_pos - rc_box.min
                    )
                );
            }
            break;
        case GUI_MSG::LBUTTON_DOWN:
            guiCaptureMouse(this);
            pressed_pos = mouse_pos;
            selection.x = font->findCursorPos(text.c_str(), text.size(), (mouse_pos.x - rc_box.min.x), .0f, &selection_screen.y);
            selection.y = selection.x;
            cursor = selection.y;
            cursor_screen = selection_screen.y;
            pressing = true;
            break;
        case GUI_MSG::LBUTTON_UP:
            pressing = false;
            cursor = selection.y;
            cursor_screen = selection_screen.y;
            if (selection.x > selection.y) {
                selection.x += selection.y;
                selection.y = selection.x - selection.y;
                selection.x = selection.x - selection.y;
            }
            break;
        }

        GuiElement::onMessage(msg, a_param, b_param);
    }

    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        const float box_height = font->getLineHeight() * 12.0f;
        this->bounding_rect = gfxm::rect(
            rc.min,
            gfxm::vec2(rc.max.x, rc.min.y + box_height + GUI_MARGIN * 2.0f)
        );
        this->client_area = gfxm::rect(
            bounding_rect.min + gfxm::vec2(GUI_MARGIN, GUI_MARGIN),
            bounding_rect.max - gfxm::vec2(GUI_MARGIN, GUI_MARGIN)
        );
        
        gfxm::vec2 sz = guiCalcTextRect(caption.c_str(), font, .0f);
        rc_caption = gfxm::rect(
            client_area.min, client_area.min + sz
        );
        rc_box = gfxm::rect(
            client_area.min + gfxm::vec2(sz.x + GUI_MARGIN, .0f),
            client_area.max
        );
    }

    void onDraw() override {
        guiDrawText(rc_caption.min, caption.c_str(), font, .0f, GUI_COL_TEXT);
        
        uint32_t col = GUI_COL_HEADER;
        guiDrawRect(rc_box, col);
        if (guiGetFocusedWindow() == this) {
            guiDrawRectLine(rc_box, GUI_COL_ACCENT);
            /*
            if (selection.x != selection.y) {
                gfxm::rect selection_rc(
                    rc_box.min + gfxm::vec2(selection_screen.x, .0f),
                    rc_box.min + gfxm::vec2(selection_screen.y, font->getLineHeight() + font->getDescender())
                );
                guiDrawRect(selection_rc, GUI_COL_ACCENT);
            } else {
                gfxm::vec2 text_sz = guiCalcTextRect(text.c_str(), font, .0f);
                gfxm::vec2 caret_pos = rc_box.min + gfxm::vec2(cursor_screen, .0f);
                gfxm::rect caret_rc(caret_pos, caret_pos + gfxm::vec2(.0f, font->getLineHeight() + font->getDescender()));
                guiDrawLine(caret_rc, GUI_COL_TEXT);
            }
            
            for (int i = text_cache.getFirstSelectedLine(); i <= text_cache.getLastSelectedLine(); ++i) {
                gfxm::vec2 min(rc_box.min.x, rc_box.min.y + i * font->getLineHeight());
                gfxm::rect selection_rc(
                    min,
                    gfxm::vec2(rc_box.max.x, min.y) + gfxm::vec2(.0f, font->getLineHeight() + font->getDescender())
                );
                guiDrawRect(selection_rc, GUI_COL_ACCENT);
            }*/
        }

        text_cache.prepareDraw();
        text_cache.draw(rc_box.min, GUI_COL_TEXT, GUI_COL_ACCENT);
        //guiDrawText(rc_box.min, text.c_str(), font, .0f, GUI_COL_TEXT);
        
        // dbg
        guiDrawText(gfxm::vec2(rc_box.min.x, rc_box.max.y), MKSTR("Lines selected from " << selected_lines.x << " to " << selected_lines.y).c_str(), font, .0f, GUI_COL_TEXT);
        guiDrawText(
            gfxm::vec2(rc_box.min.x, rc_box.max.y + font->getLineHeight()),
            MKSTR("Selection from " << text_cache.first_selected << " to " << text_cache.last_selected).c_str(),
            font, .0f, GUI_COL_TEXT
        );
    }
};
