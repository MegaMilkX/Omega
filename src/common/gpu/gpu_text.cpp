#include "gpu_text.hpp"

#include "math/gfxm.hpp"

#include "mesh3d/generate_primitive.hpp"


gpuText::gpuText(const std::shared_ptr<Font>& font)
: font(font) {

}
gpuText::~gpuText() {

}

void gpuText::setFont(const std::shared_ptr<Font>& fnt) {
    this->font = fnt;
}
void gpuText::setString(const char* str) {
    this->str = str;
}
void gpuText::commit(float max_width, float scale) {
    std::vector<float> vertices;
    std::vector<float> uv;
    std::vector<float> uv_lookup;
    std::vector<unsigned char> colors;
    std::vector<uint32_t> indices;

    int line_offset = font->getLineHeight() - font->getDescender();
    int hori_advance = 0;
    int max_hori_advance = 0;

    int adv_space = font->getGlyph('\s').horiAdvance;
    int adv_tab = adv_space * 8;

    unsigned char color[] = {
        255, 255, 255
    };

    auto putGlyph = [&scale, &vertices, &indices, &uv, &uv_lookup, &colors, &line_offset, &hori_advance, &max_hori_advance, &color]
    (const FontGlyph& g, char ch) {
        int y_ofs = g.height - g.bearingY;
        int x_ofs = g.bearingX;

        uint32_t base_index = vertices.size() / 3;

        float glyph_vertices[] = {
            (hori_advance + x_ofs) * scale,           (0 - y_ofs - line_offset) * scale,        0,
            (hori_advance + g.width + x_ofs) * scale, (0 - y_ofs - line_offset) * scale,        0,
            (hori_advance + x_ofs) * scale,           (g.height - y_ofs - line_offset) * scale, 0,
            (hori_advance + g.width + x_ofs) * scale, (g.height - y_ofs - line_offset) * scale, 0
        };
        vertices.insert(vertices.end(), glyph_vertices, glyph_vertices + sizeof(glyph_vertices) / sizeof(glyph_vertices[0]));

        uint32_t glyph_indices[] = {
            base_index, base_index + 1, base_index + 2,     base_index + 1, base_index + 3, base_index + 2
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

        colors.insert(colors.end(), color, color + sizeof(color));
        colors.insert(colors.end(), color, color + sizeof(color));
        colors.insert(colors.end(), color, color + sizeof(color));
        colors.insert(colors.end(), color, color + sizeof(color));

        hori_advance += g.horiAdvance / 64;
        max_hori_advance = gfxm::_max(max_hori_advance, hori_advance);
    };
    auto putNewline = [&line_offset, &hori_advance, this]() {
        line_offset += font->getLineHeight();
        hori_advance = 0;
    };

    for (int i = 0; i < str.size(); ++i) {
        char ch = str[i];
        if (hori_advance >= max_width && max_width > .0f) {
            line_offset += font->getLineHeight();
            hori_advance = 0;
        }
        if (ch == '\n') {
            putNewline();
            continue;
        } else if(ch == '\t') {
            int tab_reminder = hori_advance % (adv_tab / 64);
            int adv = adv_tab / 64 - tab_reminder;
            hori_advance += adv;
        } else if(ch == '#') {
            int characters_left = str.size() - i - 1;
            if (characters_left >= 8) {
                std::string str_col(&str[i + 1], &str[i + 1] + 8);
                uint64_t l_color = strtoll(str_col.c_str(), 0, 16);
                color[0] = ((l_color & 0xff000000) >> 24);
                color[1] = ((l_color & 0x00ff0000) >> 16);
                color[2] = ((l_color & 0x0000ff00) >> 8);
                //color.a = (l_color & 0x000000ff) / 255.0f;
                i += 8;
                continue;
            }
        } else if(isspace(ch)) {
            const auto& g = font->getGlyph(ch);
            putGlyph(g, ch);
        } else {
            int tok_pos = i;
            int tok_len = 0;
            for (int j = i; j < str.size(); ++j) {
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
            if (hori_advance + word_hori_advance >= max_width && max_width > .0f) {
                putNewline();
            }
            for (int j = tok_pos; j < tok_pos + tok_len; ++j) {
                ch = str[j];
                const auto& g = font->getGlyph(ch);
                putGlyph(g, ch);
            }
            i += tok_len - 1;
        }
    }

    bounding_size = gfxm::vec2(gfxm::_max((int)max_width, max_hori_advance), line_offset);

    for (int i = 0; i < vertices.size() / 3; ++i) {
        auto& x = vertices[i * 3];
        auto& y = vertices[i * 3 + 1];
        auto& z = vertices[i * 3 + 2];
        x -= (max_hori_advance * scale * .5f);
        y += (bounding_size.y * scale);
    }

    vertices_buf.setArrayData(vertices.data(), vertices.size() * sizeof(vertices[0]));
    uv_buf.setArrayData(uv.data(), uv.size() * sizeof(uv[0]));
    rgb_buf.setArrayData(colors.data(), colors.size() * sizeof(colors[0]));
    text_uv_lookup_buf.setArrayData(uv_lookup.data(), uv_lookup.size() * sizeof(uv_lookup[0]));
    index_buf.setArrayData(indices.data(), indices.size() * sizeof(indices[0]));

    mesh_desc.setDrawMode(MESH_DRAW_MODE::MESH_DRAW_TRIANGLES);
    mesh_desc.setAttribArray(VFMT::Position_GUID, &vertices_buf);
    mesh_desc.setAttribArray(VFMT::UV_GUID, &uv_buf);
    mesh_desc.setAttribArray(VFMT::ColorRGB_GUID, &rgb_buf);
    mesh_desc.setAttribArray(VFMT::TextUVLookup_GUID, &text_uv_lookup_buf);
    mesh_desc.setIndexArray(&index_buf);
}

gpuMeshDesc* gpuText::getMeshDesc() {
    return &mesh_desc;
}
