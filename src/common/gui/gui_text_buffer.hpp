#pragma once

#include <string>
#include <list>

#include "math/gfxm.hpp"
#include "platform/platform.hpp"
#include "gui/gui_font.hpp"
#include "gui/gui_color.hpp"
#include "gpu/gpu_buffer.hpp"
#include "gpu/gpu_mesh_desc.hpp"
#include "gpu/gpu_shader_program.hpp"
#include "gui/gui_shaders.hpp"
#include "gui/gui_layout_helpers.hpp"

const int GUI_SPACES_PER_TAB = 4;

struct GuiTextLine {
    std::string str;
    int line_height;
    int width;
    bool is_wrapped;
};

class GuiTextBuffer {
private:
    bool is_dirty = false;
    void setDirty() {
        is_dirty = true;
    }
    bool isDirty() const {
        return is_dirty;
    }
public:
    std::list<GuiTextLine> line_list;

    int cur_line = 0;
    int cur_col = 0;
    int cur_line_prev = 0;
    int cur_col_prev = 0;

    float max_line_width = .0f;

    Font* font = 0; // NOTE: Only used to determine if font changed

    gfxm::vec2 bounding_size;

    std::vector<float> vertices;
    std::vector<float> uv;
    std::vector<float> uv_lookup;
    std::vector<uint32_t> colors;
    std::vector<uint32_t> indices;
    // selection rectangles
    std::vector<float>      verts_selection;
    std::vector<uint32_t>   indices_selection;

    float measureStringScreenWidth(Font* font, const char* str, int str_len) {
        float width = .0f;
        int char_offset = 0;
        for (int i = 0; i < str_len; ++i) {
            char ch = str[i];
            const auto& g = font->getGlyph(ch);
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
    int countCharactersThatFitInScreenWidth(Font* font, const char* str, int str_len, float screen_width) {
        float width = .0f;
        int char_count = 0;
        for (int i = 0; i < str_len; ++i) {
            char ch = str[i];
            const auto& g = font->getGlyph(ch);

            if (width + g.horiAdvance / 64 > screen_width) {
                break;
            }

            if (isspace(ch)) {
                const auto& g = font->getGlyph(ch);
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

    void parseStringIntoLines(Font* font, const char* str, const int str_len, std::list<GuiTextLine>& out_lines) {
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
            it.width = measureStringScreenWidth(font, ln.str.c_str(), ln.str.size());
        }
    }

    void splitLine(Font* font, std::list<GuiTextLine>::iterator it, int cur_col) {
        setDirty();

        GuiTextLine new_line;
        new_line.str.insert(new_line.str.end(), it->str.begin() + cur_col, it->str.end());
        new_line.width = measureStringScreenWidth(font, new_line.str.c_str(), new_line.str.size());
        new_line.is_wrapped = false;

        auto it_copy = it;
        std::advance(it_copy, 1);
        line_list.insert(it_copy, new_line);

        it->is_wrapped = false;
        it->str.erase(it->str.begin() + cur_col, it->str.end());
        it->width = measureStringScreenWidth(font, it->str.c_str(), it->str.size());
    }
    void wordWrapClear(Font* font) {
        setDirty();
        for (auto it = line_list.begin(); it != line_list.end(); ++it) {
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
            it->width = measureStringScreenWidth(font, it->str.c_str(), it->str.size());
        }
    }
    void wordWrapExpandLine(Font* font, std::list<GuiTextLine>::iterator it, float max_line_width) {
        if (!it->is_wrapped) {
            return;
        }
        setDirty();
            
        auto it_next = it;
        std::advance(it_next, 1);
        assert(it_next != line_list.end());
        if (it_next == line_list.end()) {
            return;
        }

        // move characters
        while (it->is_wrapped) {
            int n_chars_to_move = countCharactersThatFitInScreenWidth(font, it_next->str.c_str(), it_next->str.size(), max_line_width - it->width);
            int n_chars_in_next_line = it_next->str.size();
            if (n_chars_to_move == 0) {
                break;
            }
            it->str.insert(it->str.end(), it_next->str.begin(), it_next->str.begin() + n_chars_to_move);
            it_next->str.erase(it_next->str.begin(), it_next->str.begin() + n_chars_to_move);
                
            it->width = measureStringScreenWidth(font, it->str.c_str(), it->str.size());

            if (n_chars_to_move == n_chars_in_next_line) {
                it->is_wrapped = it_next->is_wrapped;
                line_list.erase(it_next);
                if (it->is_wrapped) {
                    it_next = it;
                    std::advance(it_next, 1);
                }
            } else {
                it_next->width = measureStringScreenWidth(font, it_next->str.c_str(), it_next->str.size());
                break;
            }                
        }
    }
    void wordWrapExpand(Font* font, float max_line_width) {
        for (auto it = line_list.begin(); it != line_list.end(); ++it) {
            wordWrapExpandLine(font, it, max_line_width);
        }
    }
    void wordWrapShrinkLine(Font* font, std::list<GuiTextLine>::iterator it, float max_line_width) {
        if (it->width <= max_line_width) {
            return;
        }
        setDirty();

        int n_words_fit = 0;
        int wrap_pos = -1;
        float wrapped_width = it->width;
        float hori_advance = .0f;
        for (int i = 0; i < it->str.size(); ++i) {
            char ch = it->str[i];

            if (isspace(it->str[i])) {
                const auto& g = font->getGlyph(ch);
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
                it_next->width = measureStringScreenWidth(font, it_next->str.c_str(), it_next->str.size());
            } else {
                GuiTextLine wrapped_line;
                wrapped_line.str.insert(wrapped_line.str.end(), it->str.begin() + wrap_pos, it->str.end());
                wrapped_line.width = measureStringScreenWidth(font, wrapped_line.str.c_str(), wrapped_line.str.size());
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
    void wordWrapShrink(Font* font, float max_line_width) {
        for (auto it = line_list.begin(); it != line_list.end(); ++it) {
            wordWrapShrinkLine(font, it, max_line_width);
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
    GuiTextBuffer() {
        reset();
    }

    const gfxm::vec2& getBoundingSize() const {
        return bounding_size;
    }

    void reset() {
        setDirty();
        line_list.clear();

        GuiTextLine ln;
        ln.is_wrapped = false;
        ln.str.push_back(0x03); // ETX - end of text
        line_list.push_back(ln);

        cur_line = cur_line_prev = 0;
        cur_col = cur_col_prev = 0;
    }

    void replaceAll(Font* font, const char* string, int str_len) {
        reset();
        putString(font, string, str_len);
        //parseStringIntoLines(string, str_len, line_list);
    }
    void setMaxLineWidth(Font* font, float max_line_width) {
        setDirty();
        if (this->max_line_width < max_line_width) {
            wordWrapExpand(font, max_line_width);
        } else if(this->max_line_width > max_line_width || this->max_line_width == .0f && max_line_width > .0f) {
            wordWrapShrink(font, max_line_width);
        } else if(max_line_width == .0f) {
            wordWrapClear(font);
        }
        this->max_line_width = max_line_width;
    }

    void findCursor(Font* font, const gfxm::vec2& pointer, int& out_line, int& out_col) {
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

    void pickCursor(Font* font, const gfxm::vec2& pointer, bool drop_selection) {
        setDirty();
        findCursor(font, pointer, cur_line, cur_col);
        if (drop_selection) {
            cur_line_prev = cur_line;
            cur_col_prev = cur_col;
        }
    }

    void advanceCursor(int n, bool discard_selection) {
        setDirty();
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
        setDirty();
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
        setDirty();
        cur_line_prev = 0;
        cur_line = line_list.size() - 1;
        cur_col_prev = 0;
        cur_col = line_list.back().str.size() - 1;
    }
    void clearSelection() {
        setDirty();
        cur_line_prev = 0;
        cur_line = 0;
        cur_col_prev = 0;
        cur_col = 0;
    }

    void putChar(Font* font, char ch) {
        setDirty();
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
            splitLine(font, it, cur_col + 1);
        } else if (isspace(ch) && cur_line > 0) {
            std::advance(it, -1);
            wordWrapExpandLine(font, it, max_line_width);
        } else {
            wordWrapShrinkLine(font, it, max_line_width);
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
        setDirty();
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
    void putString(Font* font, const char* str, int str_len) {
        setDirty();
        int sel_ln_a, sel_ln_b, sel_col_a, sel_col_b;
        getSelection(sel_ln_a, sel_ln_b, sel_col_a, sel_col_b);
        
        std::list<GuiTextLine> src_lines;
        parseStringIntoLines(font, str, str_len, src_lines);
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
        setDirty();
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
            setDirty();
            eraseSelected();
        }

        return true;
    }
    void getWholeText(std::string& out) {
        auto it_last = std::next(line_list.end(), -1);
        for (auto it = line_list.begin(); it != it_last; ++it) {
            out.insert(out.end(), it->str.begin(), it->str.end());
        }        
        out.insert(out.end(), it_last->str.begin(), it_last->str.begin() + it_last->str.size() - 1);
    }
    
    void prepareDraw(Font* font, bool displayHighlight) {
        if (this->font != font) {
            this->font = font;
            setDirty();
            // TODO: Font changed
        }

        if (!isDirty()) {
            return;
        }
        const int line_height = font->getLineHeight();

        vertices.clear();
        uv.clear();
        uv_lookup.clear();
        colors.clear();
        indices.clear();

        verts_selection.clear();
        indices_selection.clear();

        uint32_t color_default = GUI_COL_TEXT;
        uint32_t color_selected = GUI_COL_TEXT_HIGHLIGHTED;

        bounding_size = gfxm::vec2(.0f, .0f);

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
                if (ch == 0x03) { // ETX - end of text
                    break;
                }

                const auto& g = font->getGlyph(ch);
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
                    is_selected = is_selected && j < sel_col_b;
                }
                if(is_selected && displayHighlight) {
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
                    hori_advance + x_ofs - 1.f,               0 - y_ofs - line_offset - 1.f,            0,
                    hori_advance + g.width + x_ofs + 1.f,     0 - y_ofs - line_offset - 1.f,            0,
                    hori_advance + x_ofs - 1.f,               g.height - y_ofs - line_offset + 1.f,     0,
                    hori_advance + g.width + x_ofs + 1.f,     g.height - y_ofs - line_offset + 1.f,     0
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
            bounding_size.y = n_line * font->getLineHeight();
            bounding_size.x = gfxm::_max(bounding_size.x, (float)hori_advance);
        }
    }

    float findCenterOffsetY(const gfxm::rect& rc) {
        float rc_size_y = rc.max.y - rc.min.y;
        float mid = rc_size_y * .5f;
        mid -= bounding_size.y * .5f;
        return rc.min.y + mid;
    }

    gfxm::rect calcTextRect(const gfxm::rect& rc_container, GUI_ALIGNMENT align, int char_offset = 0);

    void draw(Font* font, const gfxm::vec2& pos, uint32_t col, uint32_t selection_col, const gfxm::vec2& zoom_factor = gfxm::vec2(1.f, 1.f));
    void draw(Font* font, const gfxm::rect& rc, GUI_ALIGNMENT align, uint32_t col, uint32_t selection_col);
};