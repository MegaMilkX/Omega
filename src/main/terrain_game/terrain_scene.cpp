#include "terrain_scene.hpp"

#include "m3d/m3d_model.hpp"
#include "mesh3d/generate_primitive.hpp"
#include "FastNoiseSIMD.h"


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
        sys->registerQueryHandler<GeometryQuery>([this](const GeometryQuery& q){ query(q); });
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
        sys->clearQueryHandlers();
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
void TerrainScene::query(const GeometryQuery& q) {
    // TODO: retire collectVisible()
    collectVisible(q.query, q.bucket);
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
        
        float min_x = s->offset.x;
        float max_x = s->offset.x + SECTOR_WIDTH;
        float min_y = s->offset.y;
        float max_y = s->offset.y + SECTOR_DEPTH;
        float px = query.view_pos.x;
        float py = query.view_pos.z;

        float dx = gfxm::clamp(px, min_x, max_x) - px;
        float dy = gfxm::clamp(py, min_y, max_y) - py;
        float dist = gfxm::sqrt(dx * dx + dy * dy);

        if (dist > SECTOR_WIDTH * .75f) {
            continue;
        }
        for (int i = 0; i < s->decorations.size(); ++i) {
            auto deco = s->decorations[i].get();
            for (int j = 0; j < deco->subsets.size(); ++j) {
                auto subs = deco->subsets[j].get();
                for (int k = 0; k < subs->renderables.size(); ++k) {
                    auto r = subs->renderables[k].get();
                    bucket->add(r);
                }
            }
        }
    }
    bucket->add(water_renderable.get());

    

    for (auto p : proxies) {
        if (!gfxm::intersect_frustum_aabb(query.fru, p->getBoundingBox())) {
            continue;
        }
        //dbgDrawAabb(p->getBoundingBox(), 0xFFFFFFFF);
        //dbgDrawSphere(p->getBoundingSphereOrigin(), .1f, 0xFFFF00FF);
        //dbgDrawSphere(p->getBoundingSphereOrigin(), p->getBoundingRadius(), 0xFFFFFFFF);
        p->submit(bucket);
    }
}

void TerrainScene::makeSector(
    Sector& sector, const gfxm::vec2& offset,
    const gfxm::vec2& img_min, const gfxm::vec2& img_max
) {
    ktImage& img = img_heightmap;

    sector.bounding_box = gfxm::aabb(
        gfxm::vec3(offset.x, -1000.f, offset.y), gfxm::vec3(offset.x + SECTOR_WIDTH, 1000.f, offset.y + SECTOR_DEPTH)
    );
    sector.img_min = img_min;
    sector.img_max = img_max;
    sector.offset = offset;

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
        sector.heightfield_shape.init(height_data.data(), SAMPLE_WIDTH, SAMPLE_DEPTH, SECTOR_WIDTH, SECTOR_DEPTH);
    }

    sector.terrain_body.mass = .0f;
    sector.terrain_body.setFlags(COLLIDER_STATIC);
    sector.terrain_body.setShape(&sector.heightfield_shape);
    sector.terrain_body.setPosition(gfxm::vec3(offset.x, 0, offset.y));

    // Decorations
    DistribData distrib;
    //loadResource<m3dModel>("models/cube");
    
    makeDistribution(sector, img, img_forestmap, distrib, 1.f, 32);
    addDecorations(sector, distrib, loadResource<m3dModel>("models/fantasy_tree"));
    makeDistribution(sector, img, img_shoremap, distrib, .5f, 2048, .0f);
    addDecorations(sector, distrib, loadResource<m3dModel>("models/grass/grass"));
    makeDistribution(sector, img, img_inv_slopemap, distrib, .75f, 2048);
    addDecorations(sector, distrib, loadResource<m3dModel>("models/grass/grass"));
    makeDistribution(sector, img, img_slopemap, distrib, .025f, 16);
    addDecorations(sector, distrib, loadResource<m3dModel>("models/rocks-props--00016/Rocks_props__00016_"));
    makeDistribution(sector, img, img_slopemap, distrib, .025f, 16);
    addDecorations(sector, distrib, loadResource<m3dModel>("models/rocks-props--00017/Rocks_props__00017_"));
    
    // Water
    {
        water_material = loadResource<gpuMaterial>("materials/water2");

        float WIDTH = SECTOR_WIDTH * NSECTORS_X;
        float DEPTH = SECTOR_DEPTH * NSECTORS_Z;

        Mesh3d mesh;
        meshGeneratePlane(&mesh, WIDTH, DEPTH, 1.f / 40.f);
        water_mesh.setData(&mesh);

        water_renderable.reset(new gpuGeometryRenderable(water_material.get(), water_mesh.getMeshDesc()));

        auto transform_block = gpuGetDevice()->createParamBlock<gpuTransformBlock>();
        transform_block->setTransform(gfxm::translate(gfxm::mat4(1.f), gfxm::vec3(WIDTH * .5f, .5f, DEPTH * .5f)));
        water_renderable->attachParamBlock(transform_block);
    }
}

void TerrainScene::makeDistribution(
    Sector& sector, ktImage& img, ktImage& img_slopemap,
    DistribData& out, float in_scale, int budget, float threshold
) {
    const int w = SECTOR_WIDTH;
    const int h = SECTOR_DEPTH;
    std::vector<float> weights(w * h);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            float xf = x / float(SECTOR_WIDTH);
            float yf = y / float(SECTOR_DEPTH);
            gfxm::vec2 uv = sector.img_min + (sector.img_max - sector.img_min) * gfxm::vec2(xf, yf);
            weights[x + y * w] = gfxm::_max(.0f, img_slopemap.samplef(uv.x, uv.y).x - threshold);
        }
    }
    std::vector<float> cdf(w * h);
    float acc = .0f;
    for (int i = 0; i < w * h; ++i) {
        acc += weights[i];
        cdf[i] = acc;
    }
    const float total = acc;

    out.count = 0;
    out.pos.resize(budget);
    out.quat.resize(budget);
    std::vector<int> indexmap(cdf.size());
    for (int i = 0; i < cdf.size(); ++i) {
        indexmap[i] = i;
    }
    for (int i = 0; i < budget; ++i) {
        const float r = (rand() % 1000) * .001f * total;
        auto it = std::lower_bound(cdf.begin(), cdf.end(), r);
        if (it == cdf.end()) {
            break;
        }
        if (*it == .0f) {
            break;
        }
        const int cdf_idx = it - cdf.begin();
        const int idx = indexmap[cdf_idx];
        const int tx = idx % w;
        const int ty = idx / w;
        const float ux = tx / float(SECTOR_WIDTH);
        const float uy = ty / float(SECTOR_DEPTH);

        gfxm::vec2 uv = sector.img_min + (sector.img_max - sector.img_min) * gfxm::vec2(ux, uy);
        float h_sample = img.samplef(uv.x, uv.y).x;

        float scale = .01f * (rand() % 200);
        out.pos[i] = gfxm::vec4(sector.offset.x + tx, h_sample * MAX_DEPTH, sector.offset.y + ty, in_scale * (1.0f + scale));
        out.quat[i] = gfxm::angle_axis(.01f * (rand() % 100) * gfxm::pi * 2.f, gfxm::vec3(0, 1, 0));
        ++out.count;

        cdf.erase(cdf.begin() + cdf_idx);
        indexmap.erase(indexmap.begin() + cdf_idx);
    }
}

void TerrainScene::addDecorations(
    Sector& sector, const DistribData& distrib, ResourceRef<m3dModel> m3dref
) {
    const int deco_count = distrib.count;
    if (deco_count == 0) {
        return;
    }

    sector.decorations.push_back(std::make_unique<Decoration>());
    Decoration& deco = *sector.decorations.back().get();

    deco.model = m3dref;
    m3dModel* m3d = deco.model.get();

    std::map<std::string, Decoration::Subset*> subset_map;

    for (int i = 0; i < deco.model->mesh_instances.size(); ++i) {
        const auto& inst = deco.model->mesh_instances[i];
        const auto& mesh = deco.model->meshes[inst.mesh_idx];

        const std::string& bone_name = inst.bone_name;
        auto it = subset_map.find(bone_name);
        if (it == subset_map.end()) {
            deco.subsets.push_back(std::make_unique<Decoration::Subset>());
            auto ptr = deco.subsets.back().get();
            it = subset_map.insert(std::make_pair(bone_name, ptr)).first;
        }
        Decoration::Subset* subs = it->second;

        auto bone = deco.model->skeleton->findBone(inst.bone_name.c_str());
        gfxm::mat4 tr = bone->getWorldTransform();
        gfxm::mat4 inv_root = gfxm::inverse(deco.model->skeleton->getRoot()->getWorldTransform());
        tr[3] = gfxm::vec4(0, 0, 0, 1);

        const gpuMeshDesc* mesh_desc = mesh.mesh->getMeshDesc();
        ResourceRef<gpuMaterial>& mat = m3d->materials[mesh.material_idx];
        subs->renderables.push_back(std::unique_ptr<gpuRenderable>(new gpuRenderable()));
        gpuRenderable* rdr = subs->renderables.back().get();
        rdr->setMaterial(mat.get());
        rdr->setMeshDesc(mesh_desc);

        auto transform_block = gpuGetDevice()->createParamBlock<gpuTransformBlock>();
        transform_block->setTransform(tr, false);
        subs->transform_blocks.push_back(transform_block);
        subs->renderables.back()->attachParamBlock(transform_block);
        subs->renderables.back()->setRole(GPU_Role_Geometry);
    }

    const int n_subsets = subset_map.size();
    const int base = deco_count / n_subsets;
    const int remainder = deco_count % n_subsets;
    int i_subs = 0;
    for (auto& kv : subset_map) {
        auto subset = kv.second;

        const int sz = base + (i_subs < remainder ? 1 : 0);
        subset->instance_count = sz;
        if (sz == 0) {
            continue;
        }

        std::vector<gpuDefaultInstancingDesc::Instance> instances(sz);
        for (int i = 0; i < sz; ++i) {
            assert(i_subs + i * n_subsets < deco_count);
            instances[i].pos = distrib.pos[i_subs + i * n_subsets];
            instances[i].rot = distrib.quat[i_subs + i * n_subsets];
        }

        subset->inst_desc.setArray(instances.data(), instances.size());
        
        for (int i = 0; i < subset->renderables.size(); ++i) {
            auto rdr = subset->renderables[i].get();
            rdr->setInstancingDesc(&subset->inst_desc);
        }
        ++i_subs;
    }
    subset_map.clear();

    for (int i = 0; i < deco.subsets.size(); ++i) {
        if (deco.subsets[i]->instance_count == 0) {
            deco.subsets.erase(deco.subsets.begin() + i);
            --i;
        }
    }

    for (int i = 0; i < deco.subsets.size(); ++i) {
        auto subset = deco.subsets[i].get();
        for (int j = 0; j < subset->renderables.size(); ++j) {
            auto rdr = subset->renderables[j].get();
            rdr->compile();
        }
    }
}

void makeSlopemap(ktImage& img_in, ktImage& img_out, float HEIGHT) {
    const int h = img_in.getHeight();
    const int w = img_in.getWidth();
    std::vector<uint8_t> slopemap(w * h);
    img_out.reserve(w, h, 1, IMAGE_CHANNEL_UNSIGNED_BYTE);
    for (int y = 0; y < h - 1; ++y) {
        for (int x = 0; x < w - 1; ++x) {
            float h0 = HEIGHT * img_in.samplef(x / float(w), y / float(h)).x;
            float h1 = HEIGHT * img_in.samplef((x + 1) / float(w), y / float(h)).x;
            float h3 = HEIGHT * img_in.samplef(x / float(w), (y + 1) / float(h)).x;

            float dh0 = (h1 - h0);
            float dh1 = (h3 - h0);
            float slope = gfxm::sqrt(dh0 * dh0 + dh1 * dh1);
            slopemap[x + y * w] = gfxm::_min(255.f, slope * 255.f);
        }
    }
    img_out.setData(slopemap.data(), w, h, 1, IMAGE_CHANNEL_UNSIGNED_BYTE);
}

bool TerrainScene::load(const std::string& path) {
    // TESTING MODEL
    model = loadResource<SkeletalModel>("models/house/house");
    model_instance = model->createInstance();
    // TESTING MODEL

    terrain_material = loadResource<gpuMaterial>("materials/terrain");

    if (!loadImage(&img_heightmap, "textures/terrain/iceland_heightmap.png")) {
        return false;
    }

    // slopemap
    makeSlopemap(img_heightmap, img_slopemap, MAX_DEPTH);
    
    {
        img_inv_slopemap = img_slopemap;
        img_inv_slopemap.negative();
    }

    {
        img_watermask.reserve(img_inv_slopemap.getWidth(), img_inv_slopemap.getHeight(), 1, IMAGE_CHANNEL_UNSIGNED_BYTE);
        for(int i = 0; i < img_watermask.getWidth() * img_watermask.getHeight(); ++i) {
            unsigned char* src = (unsigned char*)img_heightmap.getData();
            int x = i % img_heightmap.getWidth();
            int y = i / img_heightmap.getWidth();
            uint8_t d = src[(x + y * img_heightmap.getWidth()) * img_heightmap.getChannelCount()];
            float h = (d / 255.f) * MAX_DEPTH;
            unsigned char* dst = (unsigned char*)img_watermask.getData();
            if (h <= .25f) {
                dst[i] = 0;
            } else {
                dst[i] = 255;
            }
        }
    }

    {
        for(int i = 0; i < img_watermask.getWidth() * img_watermask.getHeight(); ++i) {
            int x = i % img_watermask.getWidth();
            int y = i / img_watermask.getWidth();
            unsigned char* data_water = (unsigned char*)img_watermask.getData();
            unsigned char* data_slope = (unsigned char*)img_inv_slopemap.getData();
            if (data_water[i] == 0) {
                data_slope[(x + y * img_inv_slopemap.getWidth()) * img_inv_slopemap.getChannelCount()] = 0;
            }
        }
    }

    makeSlopemap(img_watermask, img_shoremap, MAX_DEPTH);
    //writeImagePng("shoremap.png", &img_shoremap);

    // Forest map
    {
        FastNoiseSIMD* noise = FastNoiseSIMD::NewFastNoiseSIMD();

        noise->SetNoiseType(FastNoiseSIMD::Perlin);
        float* noiseSet = noise->GetPerlinSet(0, 0, 0, 512, 512, 1, 1.0f);

        img_forestmap.setData(noiseSet, 512, 512, 1, IMAGE_CHANNEL_FLOAT);

        FastNoiseSIMD::FreeNoiseSet(noiseSet);

        for(int i = 0; i < 512 * 512; ++i) {
            int x = i % 512;
            int y = i / 512;
            float sample = img_watermask.samplef(x / 512.f, y / 512.f).x;
            float* dst = (float*)img_forestmap.getData();
            if (sample == .0f) {
                dst[x + y * 512] = 0;
            }
        }
        //writeImagePng("forestmap.png", &img_forestmap);
    }

    const gfxm::vec2 SECTOR_SIZE(SECTOR_WIDTH, SECTOR_DEPTH);
    CELL_W = SECTOR_WIDTH / (SEGMENTS_W - 1);
    CELL_H = SECTOR_DEPTH / (SEGMENTS_H - 1);
    sectors.clear();
    for (int z = 0; z < NSECTORS_Z; ++z) {
        for (int x = 0; x < NSECTORS_X; ++x) {
            auto& ptr = sectors.emplace_back();
            ptr.reset(new Sector);
            makeSector(
                *ptr.get(), gfxm::vec2(x * SECTOR_SIZE.x, z * SECTOR_SIZE.y),
                gfxm::vec2(x / 10.f, z / 10.f), gfxm::vec2((x + 1) / 10.f, (z + 1) / 10.f)
            );
        }
    }

    return true;
}

