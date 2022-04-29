#include "gpu_text.hpp"

#include "common/math/gfxm.hpp"

#include "common/mesh3d/generate_primitive.hpp"


gpuText::gpuText(Font* font)
: font(font) {

}
gpuText::~gpuText() {

}

void gpuText::setString(const char* str) {
    this->str = str;
}
void gpuText::commit() {
    std::vector<float> vertices;
    std::vector<float> uv;
    std::vector<float> uv_lookup;
    std::vector<unsigned char> colors;
    std::vector<uint32_t> indices;

    int line_offset = font->getLineHeight();
    int hori_advance = 0;

    int adv_space = font->getGlyph('\s').horiAdvance;
    int adv_tab = adv_space * 8;

    unsigned char color[] = {
        255, 255, 255
    };

    for (int i = 0; i < str.size(); ++i) {
        char ch = str[i];
        if (ch == '\n') {
            line_offset += font->getLineHeight();
            hori_advance = 0;
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
        }

        const auto& g = font->getGlyph(ch);
        int y_ofs = g.height - g.bearingY;
        int x_ofs = g.bearingX;

        uint32_t base_index = vertices.size() / 3;
        
        float glyph_vertices[] = {
            hori_advance + x_ofs,           0 - y_ofs - line_offset,        0,
            hori_advance + g.width + x_ofs, 0 - y_ofs - line_offset,        0,
            hori_advance + x_ofs,           g.height - y_ofs - line_offset, 0,
            hori_advance + g.width + x_ofs, g.height - y_ofs - line_offset, 0
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
