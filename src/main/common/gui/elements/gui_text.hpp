#pragma once

#include "common/gui/elements/gui_element.hpp"

#include <list>

const int GUI_SPACES_PER_TAB = 4;

struct GuiTextLine {
    std::string str;
    int line_height;
    int width;
    bool is_wrapped;
};

class GuiTextCache {
public:
    Font* font = 0;
    //std::string str;
    std::list<GuiTextLine> line_list;

    int cur_line = 0;
    int cur_col = 0;
    int cur_line_prev = 0;
    int cur_col_prev = 0;

    float max_line_width = .0f;

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

    float measureStringScreenWidth(const char* str, int str_len) {
        float width = .0f;
        int char_offset = 0;
        for (int i = 0; i < str_len; ++i) {
            char ch = str[i];
            auto& g = font->getGlyph(ch);
            if (ch == '\t') {
                int rem = i % GUI_SPACES_PER_TAB;
                width += font->getGlyph(' ').horiAdvance / 64 * (GUI_SPACES_PER_TAB - rem);
                char_offset += GUI_SPACES_PER_TAB - rem;
            } else {
                width += g.horiAdvance / 64;
                ++char_offset;
            }
        }
        return width;
    }
    int countCharactersThatFitInScreenWidth(const char* str, int str_len, float screen_width) {
        float width = .0f;
        int char_count = 0;
        for (int i = 0; i < str_len; ++i) {
            char ch = str[i];
            auto& g = font->getGlyph(ch);

            if (width + g.horiAdvance / 64 > screen_width) {
                break;
            }

            if (isspace(ch)) {
                auto& g = font->getGlyph(ch);
                width += g.horiAdvance / 64;
                
                char_count = i + 1;
                continue;
            }

            int tok_pos = i;
            int tok_len = 0;
            for (int j = i; j < str_len; ++j) {
                ch = str[j];
                if (isspace(ch)) {
                    break;
                }
                ++tok_len;
            }
            // eat all the whitespace before next word too
            for (int j = tok_pos + tok_len; j < str_len; ++j) {
                ch = str[j];
                if (!isspace(ch)) {
                    break;
                }
                ++tok_len;
            }
            float word_hori_advance = .0f;
            for (int j = tok_pos; j < tok_pos + tok_len; ++j) {
                ch = str[j];
                const auto& g = font->getGlyph(ch);
                word_hori_advance += g.horiAdvance / 64;
            }

            if (width + word_hori_advance > screen_width) {
                break;
            }

            width += word_hori_advance;
            i += tok_len - 1;
            char_count = i + 1;
        }
        return char_count;
    }

    void parseStringIntoLines(const char* str, const int str_len, std::list<GuiTextLine>& out_lines) {
        int from = 0;
        int to = 0;
        for (int i = 0; i < str_len; ++i) {
            char ch = str[i];
            to = i;

            if (ch == '\n') {
                GuiTextLine ln;
                ln.is_wrapped = false;
                ln.str.insert(ln.str.end(), str + from, str + to + 1);
                out_lines.push_back(ln);

                from = to + 1;
            }
        }
        GuiTextLine ln;
        ln.is_wrapped = false;
        ln.str.insert(ln.str.end(), str + from, str + to + 1);
        out_lines.push_back(ln);

        for (auto& it : out_lines) {
            it.width = measureStringScreenWidth(ln.str.c_str(), ln.str.size());
        }
    }

    void splitLine(std::list<GuiTextLine>::iterator it, int cur_col) {
        GuiTextLine new_line;
        new_line.str.insert(new_line.str.end(), it->str.begin() + cur_col, it->str.end());
        new_line.width = measureStringScreenWidth(new_line.str.c_str(), new_line.str.size());
        new_line.is_wrapped = false;

        auto it_copy = it;
        std::advance(it_copy, 1);
        line_list.insert(it_copy, new_line);

        it->is_wrapped = false;
        it->str.erase(it->str.begin() + cur_col, it->str.end());
        it->width = measureStringScreenWidth(it->str.c_str(), it->str.size());
    }
    void wordWrapClear() {
        for (auto& it = line_list.begin(); it != line_list.end(); ++it) {
            if (!it->is_wrapped) {
                continue;
            }
            
            auto it_next = it;
            std::advance(it_next, 1);
            assert(it_next != line_list.end());
            if (it_next == line_list.end()) {
                break;
            }

            // move characters
            while (it->is_wrapped) {
                int n_chars_to_move = it_next->str.size();

                it->str.insert(it->str.end(), it_next->str.begin(), it_next->str.begin() + n_chars_to_move);
                it->is_wrapped = it_next->is_wrapped;

                line_list.erase(it_next);
                if (it->is_wrapped) {
                    it_next = it;
                    std::advance(it_next, 1);
                }
            }
            it->width = measureStringScreenWidth(it->str.c_str(), it->str.size());
        }
    }
    void wordWrapExpandLine(std::list<GuiTextLine>::iterator it, float max_line_width) {
        if (!it->is_wrapped) {
            return;
        }
            
        auto it_next = it;
        std::advance(it_next, 1);
        assert(it_next != line_list.end());
        if (it_next == line_list.end()) {
            return;
        }

        // move characters
        while (it->is_wrapped) {
            int n_chars_to_move = countCharactersThatFitInScreenWidth(it_next->str.c_str(), it_next->str.size(), max_line_width - it->width);
            int n_chars_in_next_line = it_next->str.size();
            if (n_chars_to_move == 0) {
                break;
            }
            it->str.insert(it->str.end(), it_next->str.begin(), it_next->str.begin() + n_chars_to_move);
            it_next->str.erase(it_next->str.begin(), it_next->str.begin() + n_chars_to_move);
                
            it->width = measureStringScreenWidth(it->str.c_str(), it->str.size());

            if (n_chars_to_move == n_chars_in_next_line) {
                it->is_wrapped = it_next->is_wrapped;
                line_list.erase(it_next);
                if (it->is_wrapped) {
                    it_next = it;
                    std::advance(it_next, 1);
                }
            } else {
                it_next->width = measureStringScreenWidth(it_next->str.c_str(), it_next->str.size());
                break;
            }                
        }
    }
    void wordWrapExpand(float max_line_width) {
        for (auto& it = line_list.begin(); it != line_list.end(); ++it) {
            wordWrapExpandLine(it, max_line_width);
        }
    }
    void wordWrapShrinkLine(std::list<GuiTextLine>::iterator it, float max_line_width) {
        if (it->width <= max_line_width) {
            return;
        }

        int n_words_fit = 0;
        int wrap_pos = -1;
        float wrapped_width = it->width;
        float hori_advance = .0f;
        for (int i = 0; i < it->str.size(); ++i) {
            char ch = it->str[i];

            if (isspace(it->str[i])) {
                auto& g = font->getGlyph(ch);
                hori_advance += g.horiAdvance / 64;
                continue;
            }
                
            int tok_pos = i;
            int tok_len = 0;
            for (int j = i; j < it->str.size(); ++j) {
                ch = it->str[j];
                if (isspace(ch)) {
                    break;
                }
                ++tok_len;
            }
            // eat all the whitespace before next word too
            for (int j = tok_pos + tok_len; j < it->str.size(); ++j) {
                ch = it->str[j];
                if (!isspace(ch)) {
                    break;
                }
                ++tok_len;
            }
            float word_hori_advance = .0f;
            for (int j = tok_pos; j < tok_pos + tok_len; ++j) {
                ch = it->str[j];
                const auto& g = font->getGlyph(ch);
                word_hori_advance += g.horiAdvance / 64;
            }
            if (hori_advance + word_hori_advance > max_line_width && max_line_width > .0f && n_words_fit > 0) {
                wrap_pos = tok_pos;
                wrapped_width = hori_advance;
                break;
            } else {
                ++n_words_fit;
                hori_advance += word_hori_advance;
            }
            i += tok_len - 1;
        }

        if (wrap_pos >= 0) {
            if (it->is_wrapped) {
                auto it_next = it;
                std::advance(it_next, 1);
                it_next->str.insert(it_next->str.begin(), it->str.begin() + wrap_pos, it->str.end());
                it_next->width = measureStringScreenWidth(it_next->str.c_str(), it_next->str.size());
            } else {
                GuiTextLine wrapped_line;
                wrapped_line.str.insert(wrapped_line.str.end(), it->str.begin() + wrap_pos, it->str.end());
                wrapped_line.width = measureStringScreenWidth(wrapped_line.str.c_str(), wrapped_line.str.size());
                wrapped_line.is_wrapped = false;

                auto it_copy = it;
                std::advance(it_copy, 1);
                line_list.insert(it_copy, wrapped_line);
            }
            it->str.erase(it->str.begin() + wrap_pos, it->str.end());
            it->is_wrapped = true;
            it->width = wrapped_width;                
        }
    }
    void wordWrapShrink(float max_line_width) {
        for (auto it = line_list.begin(); it != line_list.end(); ++it) {
            wordWrapShrinkLine(it, max_line_width);
        }
    }

    void getSelection(int& sel_ln_a, int& sel_ln_b, int& sel_col_a, int& sel_col_b) const {
        sel_ln_b = cur_line;
        sel_ln_a = cur_line_prev;
        sel_col_b = cur_col;
        sel_col_a = cur_col_prev;
        if (sel_ln_a > sel_ln_b) {
            std::swap(sel_ln_a, sel_ln_b);
            std::swap(sel_col_a, sel_col_b);
        }
        else if (sel_ln_a == sel_ln_b && sel_col_a > sel_col_b) {
            std::swap(sel_col_a, sel_col_b);
        }
    }
public:
    GuiTextCache(Font* font)
    : font(font) {
        reset();
    }

    void reset() {
        line_list.clear();

        GuiTextLine ln;
        ln.is_wrapped = false;
        ln.str.push_back(0x03); // ETX - end of text
        line_list.push_back(ln);

        cur_line = cur_line_prev = 0;
        cur_col = cur_col_prev = 0;
    }

    void replaceAll(const char* string, int str_len) {
        reset();
        putString(string, str_len);
        //parseStringIntoLines(string, str_len, line_list);
    }
    void setMaxLineWidth(float max_line_width) {
        if (this->max_line_width < max_line_width) {
            wordWrapExpand(max_line_width);
        } else if(this->max_line_width > max_line_width) {
            wordWrapShrink(max_line_width);
        } else if(max_line_width == .0f) {
            wordWrapClear();
        }
        this->max_line_width = max_line_width;
    }

    void findCursor(const gfxm::vec2& pointer, int& out_line, int& out_col) {
        out_line = (pointer.y) / font->getLineHeight();
        if (out_line < 0) {
            out_line = 0;
            out_col = 0;
            return;
        } else if(out_line >= line_list.size()) {
            out_line = line_list.size() - 1;
            out_col = line_list.back().str.size() - 1;
            return;
        }

        out_col = 0;
        float hori_advance = .0f;
        auto it = line_list.begin();
        std::advance(it, out_line);
        int char_offset = 0;
        for (int i = 0; i < it->str.size(); ++i) {
            const auto& g = font->getGlyph(it->str[i]);
            if (it->str[i] == '\t') {
                int rem = i % GUI_SPACES_PER_TAB;
                hori_advance += font->getGlyph(' ').horiAdvance / 64 * (GUI_SPACES_PER_TAB - rem);
                char_offset += GUI_SPACES_PER_TAB - rem;
            } else {
                hori_advance += g.horiAdvance / 64;
                ++char_offset;
            }
            out_col = i;
            if (hori_advance > pointer.x) {
                break;
            }
        }
    }

    void pickCursor(const gfxm::vec2& pointer, bool drop_selection) {
        findCursor(pointer, cur_line, cur_col);
        if (drop_selection) {
            cur_line_prev = cur_line;
            cur_col_prev = cur_col;
        }
    }

    void advanceCursor(int n, bool discard_selection) {
        auto it = line_list.begin();
        std::advance(it, cur_line);

        cur_col += n;
        if (cur_col >= (int64_t)it->str.size()) {
            int overshoot = cur_col - it->str.size();
            while (cur_col >= it->str.size()) {
                if (cur_line == line_list.size() - 1) {
                    if (cur_col > it->str.size()) {
                        cur_col = it->str.size();
                    }
                    break;
                }
                cur_col = overshoot;
                overshoot = cur_col - it->str.size();
                ++cur_line;
                std::advance(it, 1);
            }
        } else if(cur_col < 0) {
            while (cur_col < 0) {
                if (cur_line == 0) {
                    if (cur_col < 0) {
                        cur_col = 0;
                    }
                    break;
                }
                --cur_line;
                std::advance(it, -1);
                cur_col += it->str.size();
            }
        }

        if (discard_selection) {
            cur_line_prev = cur_line;
            cur_col_prev = cur_col;
        }
    }
    void advanceCursorLine(int n, bool discard_selection) {
        cur_line += n;
        bool to_end = false;
        if (cur_line > line_list.size() - 1) {
            to_end = true;
        }
        cur_line = gfxm::_max(gfxm::_min(cur_line, (int)line_list.size() - 1), 0);
        auto it = line_list.begin();
        std::advance(it, cur_line);
        if (to_end) {
            cur_col = it->str.size() - 1;
        } else {
            cur_col = gfxm::_min(cur_col, (int)it->str.size() - 1);
        }

        if (discard_selection) {
            cur_line_prev = cur_line;
            cur_col_prev = cur_col;
        }
    }
    void selectAll() {
        cur_line_prev = 0;
        cur_line = line_list.size() - 1;
        cur_col_prev = 0;
        cur_col = line_list.back().str.size() - 1;
    }

    void putChar(char ch) {
        if (ch <= 0x1F && ch != 0x0A) {
            // TODO: ?
            return;
        }
        auto it = line_list.begin();
        std::advance(it, cur_line);

        
        it->str.insert(it->str.begin() + cur_col, ch);

        const auto& g = font->getGlyph(ch);
        it->width += g.horiAdvance / 64;

        if (ch == '\n') {
            splitLine(it, cur_col + 1);
        } else if (isspace(ch) && cur_line > 0) {
            std::advance(it, -1);
            wordWrapExpandLine(it, max_line_width);
        } else {
            wordWrapShrinkLine(it, max_line_width);
        }
        advanceCursor(1, true);
    }
    void backspace() {
        if (cur_col == 0 && cur_line == 0 && cur_col_prev == 0 && cur_line_prev == 0) {
            return;
        }
        if (cur_col == cur_col_prev && cur_line == cur_line_prev) {
            advanceCursor(-1, false);
        }
        eraseSelected();
    }
    void del() {
        if (cur_col == cur_col_prev && cur_line == cur_line_prev) {
            advanceCursor(1, false);
        }
        eraseSelected();
    }
    void newline() {
        eraseSelected();

        auto it = std::next(line_list.begin(), cur_line);
        int whitespace_to_copy = 0;
        for (int i = 0; i < it->str.size(); ++i) {
            if (it->str[i] != '\t' && it->str[i] != ' ') {
                break;
            }
            whitespace_to_copy = i + 1;
        }

        GuiTextLine ln;
        ln.is_wrapped = false;
        ln.str.insert(ln.str.end(), it->str.begin(), it->str.begin() + whitespace_to_copy);
        ln.str.insert(ln.str.end(), it->str.begin() + cur_col, it->str.end());

        it->str.erase(it->str.begin() + cur_col, it->str.end());
        it->str.push_back('\n');

        line_list.insert(std::next(it), ln);

        advanceCursor(1 + whitespace_to_copy, true);
    }
    void putString(const char* str, int str_len) {
        int sel_ln_a, sel_ln_b, sel_col_a, sel_col_b;
        getSelection(sel_ln_a, sel_ln_b, sel_col_a, sel_col_b);
        
        std::list<GuiTextLine> src_lines;
        parseStringIntoLines(str, str_len, src_lines);
        // TODO: parseStringIntoLines should return total amount of actual characters in all lines
        //  because some of them might be discarded, like \r        
        

        auto it_first = std::next(line_list.begin(), sel_ln_a);
        auto it_last = std::next(it_first, sel_ln_b - sel_ln_a);
        auto it_src_first = src_lines.begin();
        auto it_src_last = std::next(src_lines.begin(), src_lines.size() - 1);
        it_src_first->str.insert(it_src_first->str.begin(), it_first->str.begin(), it_first->str.begin() + sel_col_a);
        int col_new = it_src_last->str.size();
        it_src_last->str.insert(it_src_last->str.end(), it_last->str.begin() + sel_col_b, it_last->str.end());

        auto it_current = std::next(it_last);
        line_list.erase(it_first, it_current);

        line_list.insert(it_current, src_lines.begin(), src_lines.end());
        
        cur_line = sel_ln_a + src_lines.size() - 1;
        cur_line_prev = cur_line;
        cur_col = col_new;
        cur_col_prev = col_new;
    }
    void eraseSelected() {
        int sel_ln_a, sel_ln_b, sel_col_a, sel_col_b;
        getSelection(sel_ln_a, sel_ln_b, sel_col_a, sel_col_b);
        if (sel_ln_a == sel_ln_b && sel_col_a == sel_col_b) {
            return;
        }

        auto it_first = std::next(line_list.begin(), sel_ln_a);
        auto it_last = std::next(it_first, sel_ln_b - sel_ln_a);
        std::string part(it_last->str.begin() + sel_col_b, it_last->str.end());
        it_first->str.replace(it_first->str.begin() + sel_col_a, it_first->str.end(), part);

        line_list.erase(std::next(it_first), std::next(it_last));
        cur_line = cur_line_prev = sel_ln_a;
        cur_col = cur_col_prev = sel_col_a;
    }
    bool copySelected(std::string& out, bool cut = false) {
        int sel_ln_a, sel_ln_b, sel_col_a, sel_col_b;
        getSelection(sel_ln_a, sel_ln_b, sel_col_a, sel_col_b);

        if (sel_ln_a == sel_ln_b && sel_col_a == sel_col_b) {
            return false;
        }

        const bool single_line = sel_ln_a == sel_ln_b;

        auto it = std::next(line_list.begin(), sel_ln_a);
        auto it_last = std::next(line_list.begin(), sel_ln_b);
        out.insert(out.end(), it->str.begin() + sel_col_a, (single_line) ? it->str.begin() + sel_col_b : it->str.end());
        it = std::next(it);
        if (!single_line) {
            for (; it != it_last; ++it) {
                out.insert(out.end(), it->str.begin(), it->str.end());
            }
            out.insert(out.end(), it_last->str.begin(), it_last->str.begin() + sel_col_b);
        }

        if (cut) {
            eraseSelected();
        }

        return true;
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

        int sel_ln_b = cur_line;
        int sel_ln_a = cur_line_prev;
        int sel_col_b = cur_col;
        int sel_col_a = cur_col_prev;
        if (sel_ln_a > sel_ln_b) {
            std::swap(sel_ln_a, sel_ln_b);
            std::swap(sel_col_a, sel_col_b);
        } else if(sel_ln_a == sel_ln_b && sel_col_a > sel_col_b) {
            std::swap(sel_col_a, sel_col_b);
        }

        int n_line = 0;
        for(auto line_str : line_list) {
            int hori_advance = 0;
            int line_offset = line_height * (n_line + 1) - font->getDescender();
            int char_offset = 0; // used to count actual character offset (tabs are multiple chars in width)
            for (int j = 0; j < line_str.str.size(); ++j) {
                uint32_t color = GUI_COL_TEXT;
                char ch = line_str.str[j];/*
                if (ch == ' ') {
                    ch = '_';
                } else if(ch == '\n') {
                    ch = '\\';
                }*/

                auto& g = font->getGlyph(ch);
                int y_ofs = g.height - g.bearingY;
                int x_ofs = g.bearingX;
                int glyph_advance = 0;// g.horiAdvance / 64;
                if (ch == '\t') {
                    int rem = char_offset % GUI_SPACES_PER_TAB;
                    glyph_advance = font->getGlyph(' ').horiAdvance / 64 * (GUI_SPACES_PER_TAB - rem);
                    char_offset += GUI_SPACES_PER_TAB - rem;
                } else {
                    glyph_advance = g.horiAdvance / 64;
                    ++char_offset;
                }

                // selection
                bool is_selected = n_line >= sel_ln_a && n_line <= sel_ln_b;
                if (n_line == sel_ln_a) {
                    is_selected = is_selected && j >= sel_col_a;
                }
                if (n_line == sel_ln_b) {
                    is_selected = is_selected && j <= sel_col_b;
                }
                if(is_selected) {
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
                    hori_advance += glyph_advance;
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

            ++n_line;
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
    gfxm::rect rc_text;
    
    gfxm::vec2 pressed_pos;
    gfxm::vec2 mouse_pos;
    bool pressing = false;
public:
    GuiTextBox(Font* fnt)
    : font(fnt), text_cache(font) {

        text_cache.replaceAll(text.c_str(), text.size());
    }

    void onMessage(GUI_MSG msg, uint64_t a_param, uint64_t b_param) override {
        switch (msg) {
        case GUI_MSG::KEYDOWN:
            // TODO: Remove winapi usage
            switch (a_param) {
            case VK_RIGHT: text_cache.advanceCursor(1, !(guiGetModifierKeysState() & GUI_KEY_SHIFT)); break;                
            case VK_LEFT: text_cache.advanceCursor(-1, !(guiGetModifierKeysState() & GUI_KEY_SHIFT)); break;
            case VK_UP: text_cache.advanceCursorLine(-1, !(guiGetModifierKeysState() & GUI_KEY_SHIFT)); break;
            case VK_DOWN: text_cache.advanceCursorLine(1, !(guiGetModifierKeysState() & GUI_KEY_SHIFT)); break;
            case VK_DELETE:
                text_cache.del();
                break;
            // CTRL+V key
            case 0x0056: if (guiGetModifierKeysState() & GUI_KEY_CONTROL) {
                if (IsClipboardFormatAvailable(CF_TEXT) && OpenClipboard(0)) {
                    HGLOBAL hglb = { 0 };
                    hglb = GetClipboardData(CF_TEXT);
                    if (hglb != NULL) {
                        LPTSTR lptstr = (LPTSTR)GlobalLock(hglb);
                        if (lptstr != NULL) {
                            std::string str = lptstr;
                            text_cache.putString(str.c_str(), str.size());
                            GlobalUnlock(hglb);
                        }
                    }
                    CloseClipboard();
                }
            } break;
            // CTRL+C, CTRL+X
            case 0x0043: // C
            case 0x0058: /* X */ if (guiGetModifierKeysState() & GUI_KEY_CONTROL) {
                std::string copied;
                if (text_cache.copySelected(copied, a_param == 0x0058)) {
                    if (OpenClipboard(0)) {
                        HGLOBAL hglbCopy = GlobalAlloc(GMEM_MOVEABLE, copied.size() + 1);
                        if (hglbCopy) {
                            EmptyClipboard();
                            LPTSTR lptstrCopy = (LPTSTR)GlobalLock(hglbCopy);
                            memcpy(lptstrCopy, copied.data(), copied.size());
                            lptstrCopy[copied.size() + 1] = '\0';
                            GlobalUnlock(hglbCopy);
                            SetClipboardData(CF_TEXT, hglbCopy);
                            CloseClipboard();
                        } else {
                            CloseClipboard();
                            assert(false);
                        }
                    }
                }
            } break;
            // CTRL+A
            case 0x41: if (guiGetModifierKeysState() & GUI_KEY_CONTROL) { // A
                text_cache.selectAll();
            } break;
            }
            break;
        case GUI_MSG::UNICHAR:
            switch ((GUI_CHAR)a_param) {
            case GUI_CHAR::BACKSPACE:
                text_cache.backspace();
                break;
            case GUI_CHAR::ESCAPE:
                break;
            case GUI_CHAR::LINEFEED:
                text_cache.newline();
                break;
            case GUI_CHAR::RETURN:
                text_cache.newline();;
                break;
            case GUI_CHAR::TAB:
                text_cache.putString("\t", 1);
                break;
            default:
                char ch = (char)a_param;
                text_cache.putString(&ch, 1);
            }
            break;
        case GUI_MSG::MOUSE_MOVE:
            mouse_pos = gfxm::vec2(a_param, b_param);
            if (pressing) {
                text_cache.pickCursor(mouse_pos - rc_text.min, false);
            }
            break;
        case GUI_MSG::LBUTTON_DOWN:
            guiCaptureMouse(this);
            pressed_pos = mouse_pos;
            pressing = true;

            text_cache.pickCursor(mouse_pos - rc_text.min, true);
            break;
        case GUI_MSG::LBUTTON_UP:
            pressing = false;
            break;
        }

        GuiElement::onMessage(msg, a_param, b_param);
    }

    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        const float box_height = rc.max.y - rc.min.y - GUI_MARGIN * 2.0f;// font->getLineHeight() * 12.0f;
        this->bounding_rect = gfxm::rect(
            rc.min,
            gfxm::vec2(rc.max.x, rc.min.y + box_height + GUI_MARGIN * 2.0f)
        );
        this->client_area = gfxm::rect(
            bounding_rect.min + gfxm::vec2(GUI_MARGIN, GUI_MARGIN),
            bounding_rect.max - gfxm::vec2(GUI_MARGIN, GUI_MARGIN)
        );
        
        gfxm::vec2 sz = gfxm::vec2(.0f, .0f);//guiCalcTextRect(caption.c_str(), font, .0f);
        rc_caption = gfxm::rect(
            client_area.min, client_area.min + sz
        );
        rc_box = gfxm::rect(
            client_area.min + gfxm::vec2(sz.x, .0f),
            client_area.max
        );
        rc_text = gfxm::rect(
            rc_box.min + gfxm::vec2(GUI_MARGIN, GUI_MARGIN),
            rc_box.max - gfxm::vec2(GUI_MARGIN, GUI_MARGIN)
        );
        //text_cache.setMaxLineWidth(rc_text.max.x - rc_text.min.x);
    }

    void onDraw() override {
        //guiDrawText(rc_caption.min, caption.c_str(), font, .0f, GUI_COL_TEXT);
        
        uint32_t col = GUI_COL_HEADER;
        guiDrawRect(rc_box, col);
        if (guiGetFocusedWindow() == this) {
            guiDrawRectLine(rc_box, GUI_COL_BUTTON_HOVER);
        }

        int sw = 0, sh = 0;
        platformGetWindowSize(sw, sh);
        glScissor(
            rc_box.min.x,
            sh - rc_box.max.y,
            rc_box.max.x - rc_box.min.x,
            rc_box.max.y - rc_box.min.y
        );
        text_cache.prepareDraw();
        text_cache.draw(rc_text.min, GUI_COL_TEXT, GUI_COL_TEXT_HIGHLIGHT);
    }
};
