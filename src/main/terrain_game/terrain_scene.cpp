#include "terrain_scene.hpp"

#pragma pack(push, 1)
struct COLOR24 {
    uint8_t R;
    uint8_t G;
    uint8_t B;
    COLOR24& operator=(const uint32_t& other) {
        R = other & 0xFF;
        G = (other & 0xFF00) >> 8;
        B = (other & 0xFF0000) >> 16;
        return *this;
    }
};
#pragma pack(pop)

void TerrainScene::onSpawnScene(IWorld& world) {
    if (auto sys = world.getSystem<PlayerStartSystem>()) {
        sys->points.push_back(PlayerStartSystem::Location{ gfxm::vec3(512, 30.f, 512), gfxm::vec3() });
    }

    if (auto sys = world.getSystem<phyWorld>()) {
        for (auto& s : sectors) {
            sys->addCollider(&s->terrain_body);
        }
    }
    if (auto sys = world.getSystem<SceneSystem>()) {
        sys->registerProvider(this);
    }
    
    // TESTING MODEL
    auto old_scene_sys = world.getSystem<scnRenderScene>();
    auto scene_sys = world.getSystem<SceneSystem>();
    model_instance->spawnModel(scene_sys, old_scene_sys);
    HTransform ht;
    ht.acquire();
    ht->setTranslation(480, 0, 480);
    ht->setRotation(gfxm::quat(0, 0, 0, 1));
    ht->setScale(gfxm::vec3(1.3, 1.3, 1.3));
    model_instance->setExternalRootTransform(ht);
    // TESTING MODEL
}
void TerrainScene::onDespawnScene(IWorld& world) {
    if (auto sys = world.getSystem<PlayerStartSystem>()) {
        sys->points.clear();
    }

    if (auto sys = world.getSystem<phyWorld>()) {
        for (auto& s : sectors) {
            sys->removeCollider(&s->terrain_body);
        }
    }
    if (auto sys = world.getSystem<SceneSystem>()) {
        sys->unregisterProvider(this);
    }

    // TESTING MODEL
    auto old_scene_sys = world.getSystem<scnRenderScene>();
    auto scene_sys = world.getSystem<SceneSystem>();
    model_instance->despawnModel(scene_sys, old_scene_sys);
    // TESTING MODEL
}


void TerrainScene::onAddProxy(VisibilityProxyItem* item) {
    proxies.insert(item->proxy);
}
void TerrainScene::onRemoveProxy(VisibilityProxyItem* item) {
    proxies.erase(item->proxy);
}
void TerrainScene::updateProxies(VisibilityProxyItem* items, int count) {
    for (int i = 0; i < count; ++i) {
        auto prox = items[i].proxy;
        // TODO: Update internal representation of the proxy,
        // with spatial data, like which cell it's in
    }
}
void TerrainScene::collectVisible(const VisibilityQuery& query, gpuRenderBucket* bucket) {
    for (auto& s : sectors) {
        /*
        dbgDrawFrustum(query.fru, 0xFFFFFFFF);
        dbgDrawAabb(s->bounding_box, 0xFFFFFFFF);
        */
        if (!gfxm::intersect_frustum_aabb(query.fru, s->bounding_box)) {
            continue;
        }
        bucket->add(&s->terrain_renderable);
    }

    for (auto p : proxies) {
        if (!gfxm::intersect_frustum_aabb(query.fru, p->getBoundingBox())) {
            continue;
        }
        dbgDrawAabb(p->getBoundingBox(), 0xFFFFFFFF);
        //dbgDrawSphere(p->getBoundingSphereOrigin(), .1f, 0xFFFF00FF);
        //dbgDrawSphere(p->getBoundingSphereOrigin(), p->getBoundingRadius(), 0xFFFFFFFF);
        p->submit(bucket);
    }
}

void TerrainScene::makeSector(
    Sector& sector, ktImage& img,
    const gfxm::vec2& size, const gfxm::vec2& offset,
    const gfxm::vec2& img_min, const gfxm::vec2& img_max
) {
    sector.bounding_box = gfxm::aabb(
        gfxm::vec3(offset.x, -1000.f, offset.y), gfxm::vec3(offset.x + size.x, 1000.f, offset.y + size.y)
    );

    const float WIDTH = size.x;//2048.f;//4096.f;
    const float HEIGHT = size.y;//2048.f;//4096.f;
    const float MAX_DEPTH = 60.f;
    const int SEGMENTS_W = 200;
    const int SEGMENTS_H = 200;
    const float CELL_W = WIDTH / (SEGMENTS_W - 1);
    const float CELL_H = HEIGHT / (SEGMENTS_H - 1);
    std::vector<gfxm::vec3> vertices;
    std::vector<gfxm::vec3> normals;
    std::vector<gfxm::vec3> tangents;
    std::vector<gfxm::vec3> bitangents;
    std::vector<gfxm::vec2> uvs;
    std::vector<COLOR24> colors;
    std::vector<uint32_t> indices;
    {
        vertices.resize(SEGMENTS_W * SEGMENTS_H);
        for (int y = 0; y < SEGMENTS_H; ++y) {
            for (int x = 0; x < SEGMENTS_W; ++x) {
                gfxm::vec2 uv = img_min + (img_max - img_min) * gfxm::vec2(x / float(SEGMENTS_W - 1), y / float(SEGMENTS_H - 1));
                float h = img.samplef(uv.x, uv.y).x;
                //h *= h;
                //vertices[x + y * SEGMENTS_W] = gfxm::vec3(x * CELL_W - WIDTH * .5f, h * MAX_DEPTH, y * CELL_H - HEIGHT * .5f);
                vertices[x + y * SEGMENTS_W] = gfxm::vec3(x * CELL_W, h * MAX_DEPTH, y * CELL_H);
            }
        }

        uvs.resize(vertices.size());
        for (int y = 0; y < SEGMENTS_H; ++y) {
            for (int x = 0; x < SEGMENTS_W; ++x) {
                uvs[x + y * SEGMENTS_W] = gfxm::vec2(x * CELL_W * .25f, y * CELL_H * .25f);
            }
        }

        gfxm::vec3 gradient[4] = {
            gfxm::vec3(.2f, .3f, .9f),
            gfxm::vec3(.05f, .3f, .12f),
            gfxm::vec3(.2f, .2f, .2f),
            gfxm::vec3(1.f, 1.f, 1.f)
        };
        int grad_count = sizeof(gradient) / sizeof(gradient[0]);
        colors.resize(vertices.size());
        for (int y = 0; y < SEGMENTS_H; ++y) {
            for (int x = 0; x < SEGMENTS_W; ++x) {
                gfxm::vec2 uv = img_min + (img_max - img_min) * gfxm::vec2(x / float(SEGMENTS_W - 1), y / float(SEGMENTS_H - 1));
                float h = img.samplef(uv.x, uv.y).x;

                colors[x + y * SEGMENTS_W] = gfxm::make_rgba32(h, h, h, 1.f);

                /*
                int grad_at = h * grad_count;
                int grad_next = gfxm::_min(grad_count - 1, grad_at + 1);
                float f = gfxm::fract(h * grad_count);
                gfxm::vec3 col = h * gfxm::lerp(gradient[grad_at], gradient[grad_next], f);
                colors[x + y * SEGMENTS_W] = gfxm::make_rgba32(col.x, col.y, col.z, 1.f);
                */
            }
        }

        indices.resize((SEGMENTS_W - 1) * (SEGMENTS_H - 1) * 6);
        for (int y = 0; y < SEGMENTS_H - 1; ++y) {
            for (int x = 0; x < SEGMENTS_W - 1; ++x) {
                int at = 6 * (x + y * (SEGMENTS_W - 1));
                indices[at + 0] = x + y * SEGMENTS_W;
                indices[at + 2] = x + 1 + y * SEGMENTS_W;
                indices[at + 1] = x + 1 + (y + 1) * SEGMENTS_W;
                indices[at + 3] = x + 1 + (y + 1) * SEGMENTS_W;
                indices[at + 5] = x + (y + 1) * SEGMENTS_W;
                indices[at + 4] = x + y * SEGMENTS_W;
            }
        }

        normals.resize(vertices.size());
        tangents.resize(vertices.size());
        bitangents.resize(vertices.size());
        for (int i = 0; i < indices.size() / 3; ++i) {
            int a = indices[i * 3];
            int b = indices[i * 3 + 1];
            int c = indices[i * 3 + 2];

            gfxm::vec3 N = gfxm::normalize(gfxm::cross(vertices[b] - vertices[a], vertices[c] - vertices[a]));
            normals[a] = gfxm::normalize(normals[a] + N);
            normals[b] = gfxm::normalize(normals[b] + N);
            normals[c] = gfxm::normalize(normals[c] + N);
        }

        {
            const int index_count = indices.size();
            const int vertex_count = vertices.size();
            for (int l = 0; l < index_count; l += 3) {
                uint32_t a = indices[l];
                uint32_t b = indices[l + 1];
                uint32_t c = indices[l + 2];

                gfxm::vec3 Va = vertices[a];
                gfxm::vec3 Vb = vertices[b];
                gfxm::vec3 Vc = vertices[c];
                gfxm::vec3 Na = normals[a];
                gfxm::vec3 Nb = normals[b];
                gfxm::vec3 Nc = normals[c];
                gfxm::vec2 UVa = uvs[a];
                gfxm::vec2 UVb = uvs[b];
                gfxm::vec2 UVc = uvs[c];

                float x1 = Vb.x - Va.x;
                float x2 = Vc.x - Va.x;
                float y1 = Vb.y - Va.y;
                float y2 = Vc.y - Va.y;
                float z1 = Vb.z - Va.z;
                float z2 = Vc.z - Va.z;

                float s1 = UVb.x - UVa.x;
                float s2 = UVc.x - UVa.x;
                float t1 = UVb.y - UVa.y;
                float t2 = UVc.y - UVa.y;

                float r = 1.f / (s1 * t2 - s2 * t1);
                gfxm::vec3 sdir(
                    (t2 * x1 - t1 * x2) * r,
                    (t2 * y1 - t1 * y2) * r,
                    (t2 * z1 - t1 * z2) * r
                );
                gfxm::vec3 tdir(
                    (s1 * x2 - s2 * x1) * r,
                    (s1 * y2 - s2 * y1) * r,
                    (s1 * z2 - s2 * z1) * r
                );

                tangents[a] += sdir;
                tangents[b] += sdir;
                tangents[c] += sdir;
                bitangents[a] += tdir;
                bitangents[b] += tdir;
                bitangents[c] += tdir;
            }
            for (int k = 0; k < vertex_count; ++k) {
                tangents[k] = gfxm::normalize(tangents[k]);
                bitangents[k] = gfxm::normalize(bitangents[k]);
            }
        }
    }
    /*
    Mesh3d mesh3d;
    meshGenerateCube(&mesh3d, 20, .5, 20);
    terrain_mesh.setData(&mesh3d);
    */
    Mesh3d mesh3d;
    mesh3d.setAttribArray(VFMT::Position_GUID, vertices.data(), vertices.size() * sizeof(vertices[0]));
    mesh3d.setAttribArray(VFMT::Normal_GUID, normals.data(), normals.size() * sizeof(normals[0]));
    mesh3d.setAttribArray(VFMT::Tangent_GUID, tangents.data(), tangents.size() * sizeof(tangents[0]));
    mesh3d.setAttribArray(VFMT::Bitangent_GUID, bitangents.data(), bitangents.size() * sizeof(bitangents[0]));
    mesh3d.setAttribArray(VFMT::UV_GUID, uvs.data(), uvs.size() * sizeof(uvs[0]));
    mesh3d.setAttribArray(VFMT::ColorRGB_GUID, colors.data(), colors.size() * sizeof(colors[0]));
    mesh3d.setIndexArray(indices.data(), indices.size() * sizeof(indices[0]));
    sector.terrain_mesh.setData(&mesh3d);

    auto transform_block = gpuGetDevice()->createParamBlock<gpuTransformBlock>();
    transform_block->setTransform(gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(offset.x, 0, offset.y)), false);
    sector.terrain_renderable.attachParamBlock(transform_block);
    sector.terrain_renderable.setMaterial(terrain_material.get());
    sector.terrain_renderable.setMeshDesc(sector.terrain_mesh.getMeshDesc());
    sector.terrain_renderable.setRole(GPU_Role_Geometry);
    sector.terrain_renderable.compile();

    {
        const int SAMPLE_WIDTH = SEGMENTS_W;//img_heightmap.getWidth();
        const int SAMPLE_DEPTH = SEGMENTS_H;//img_heightmap.getHeight();
        std::vector<float> height_data;
        height_data.resize(SAMPLE_WIDTH * SAMPLE_DEPTH);
        std::fill(height_data.begin(), height_data.end(), .0f);
        for (int z = 0; z < SAMPLE_DEPTH; ++z) {
            for (int x = 0; x < SAMPLE_WIDTH; ++x) {
                gfxm::vec2 uv = img_min + (img_max - img_min) * gfxm::vec2(x / float(SEGMENTS_W - 1), z / float(SEGMENTS_H - 1));
                auto col = img.samplef(uv.x, uv.y);
                float h = col.x;
                //h *= h;
                height_data[x + z * SAMPLE_WIDTH] = h * MAX_DEPTH;
            }
        }
        sector.heightfield_shape.init(height_data.data(), SAMPLE_WIDTH, SAMPLE_DEPTH, WIDTH, HEIGHT);
    }

    sector.terrain_body.mass = .0f;
    sector.terrain_body.setFlags(COLLIDER_STATIC);
    sector.terrain_body.setShape(&sector.heightfield_shape);
    sector.terrain_body.setPosition(gfxm::vec3(offset.x, 0, offset.y));
}

bool TerrainScene::load(const std::string& path) {
    // TESTING MODEL
    model = loadResource<SkeletalModel>("models/house/house");
    model_instance = model->createInstance();
    // TESTING MODEL

    terrain_material = loadResource<gpuMaterial>("materials/terrain");

    ktImage img_heightmap;
    if (!loadImage(&img_heightmap, "textures/terrain/iceland_heightmap.png")) {
        return false;
    }

    const gfxm::vec2 SECTOR_SIZE(SECTOR_WIDTH, SECTOR_DEPTH);
    sectors.clear();
    for (int z = 0; z < NSECTORS_Z; ++z) {
        for (int x = 0; x < NSECTORS_X; ++x) {
            auto& ptr = sectors.emplace_back();
            ptr.reset(new Sector);
            makeSector(
                *ptr.get(),
                img_heightmap,
                SECTOR_SIZE, gfxm::vec2(x * SECTOR_SIZE.x, z * SECTOR_SIZE.y),
                gfxm::vec2(x / 10.f, z / 10.f), gfxm::vec2((x + 1) / 10.f, (z + 1) / 10.f)
            );
        }
    }

    return true;
}

