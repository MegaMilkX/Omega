#include "gpu_text.hpp"

#include "math/gfxm.hpp"

#include "mesh3d/generate_primitive.hpp"


gpuText::gpuText(const std::shared_ptr<Font>& font)
: font(font) {
    text_layout.space = TextLayout::SPACE::Y_UP;
    text_layout.setFont(font.get());
}
gpuText::~gpuText() {

}

void gpuText::setFont(const std::shared_ptr<Font>& fnt) {
    this->font = fnt;
    text_layout.setFont(font.get());
}
void gpuText::setString(const std::string& str) {
    this->str = str;
    text_layout.setString(str.data(), str.size());
}
const char* gpuText::getString() const {
    return str.c_str();
}
void gpuText::commit(float max_width, float scale) {
    text_layout.build();

    std::vector<gfxm::vec3> vertices;
    std::vector<gfxm::vec2> uv;
    std::vector<float> uv_lookup;
    std::vector<unsigned char> colors;

    unsigned char color[] = {
        255, 255, 255
    };

    for (int i = 0; i < text_layout.glyphs.size(); ++i) {
        auto g = text_layout.glyphs[i];
        if (!g.renderable) {
            continue;
        }
        auto q = g.makeQuad();
        
        vertices.insert(vertices.end(), {
            q.pos[0], q.pos[1], q.pos[2],
            q.pos[1], q.pos[3], q.pos[2],
        });

        uv.insert(uv.end(), { // Flipped
            q.uv[0], q.uv[1], q.uv[2],
            q.uv[1], q.uv[3], q.uv[2],
        });
        
        uv_lookup.insert(uv_lookup.end(), {
            q.lut_values[0], q.lut_values[1], q.lut_values[2],
            q.lut_values[1], q.lut_values[3], q.lut_values[2],
        });

        colors.insert(colors.end(), color, color + sizeof(color));
        colors.insert(colors.end(), color, color + sizeof(color));
        colors.insert(colors.end(), color, color + sizeof(color));
        colors.insert(colors.end(), color, color + sizeof(color));
        colors.insert(colors.end(), color, color + sizeof(color));
        colors.insert(colors.end(), color, color + sizeof(color));
    }

    bounding_size = gfxm::vec2(text_layout.bounding_width, text_layout.bounding_height);

    for (int i = 0; i < vertices.size(); ++i) {
        auto& p = vertices[i];
        p.x -= (bounding_size.x * .5f);
        p.y += (bounding_size.y);
        p *= scale;
    }

    vertices_buf.setArrayData(vertices.data(), vertices.size() * sizeof(vertices[0]));
    uv_buf.setArrayData(uv.data(), uv.size() * sizeof(uv[0]));
    rgb_buf.setArrayData(colors.data(), colors.size() * sizeof(colors[0]));
    text_uv_lookup_buf.setArrayData(uv_lookup.data(), uv_lookup.size() * sizeof(uv_lookup[0]));

    mesh_desc.setDrawMode(MESH_DRAW_MODE::MESH_DRAW_TRIANGLES);
    mesh_desc.setAttribArray(VFMT::Position_GUID, &vertices_buf);
    mesh_desc.setAttribArray(VFMT::UV_GUID, &uv_buf);
    mesh_desc.setAttribArray(VFMT::ColorRGB_GUID, &rgb_buf);
    mesh_desc.setAttribArray(VFMT::TextUVLookup_GUID, &text_uv_lookup_buf);
    mesh_desc.setVertexCount(vertices.size());
    mesh_desc.setType(GPU_MESH_DESC_TYPE::TEXT);
}

gpuMeshDesc* gpuText::getMeshDesc() {
    return &mesh_desc;
}
