#include "terrain_game.hpp"

#include "engine_runtime/components/render_view_list.hpp"
#include "world/node/node_character_capsule.hpp"
#include "agents/fps_player_agent.hpp"
#include "world/common_systems/player_start_system.hpp"

#include "controllers/marble_controller.hpp"
#include "world/node/node_skeletal_model.hpp"
#include "agents/tps_player_agent.hpp"

#include "collision/phy.hpp"

#include "mesh3d/generate_primitive.hpp"
#include "image/image.hpp"

#include "agents/free_cam_agent.hpp"

class TerrainScene : public IScene, public IVisibilityProvider {
    HTransform transform;
    RHSHARED<gpuMaterial> terrain_material;
    gpuMesh terrain_mesh;
    gpuRenderable terrain_renderable;

    phyHeightfieldShape heightfield_shape;
    phyRigidBody terrain_body;
public:
    void onSpawnScene(WorldSystemRegistry& reg) override {
        if (auto sys = reg.getSystem<PlayerStartSystem>()) {
            sys->points.push_back(PlayerStartSystem::Location{ gfxm::vec3(512, 25.f, 512), gfxm::vec3() });
        }

        if (auto sys = reg.getSystem<phyWorld>()) {
            sys->addCollider(&terrain_body);
        }
        if (auto sys = reg.getSystem<VisibilitySystem>()) {
            sys->registerProvider(this);
        }
    }
    void onDespawnScene(WorldSystemRegistry& reg) override {
        if (auto sys = reg.getSystem<PlayerStartSystem>()) {
            sys->points.clear();
        }

        if (auto sys = reg.getSystem<phyWorld>()) {
            sys->removeCollider(&terrain_body);
        }
        if (auto sys = reg.getSystem<VisibilitySystem>()) {
            sys->unregisterProvider(this);
        }
    }

    void collectVisible(const VisibilityQuery& query, gpuRenderBucket* bucket) override {
        bucket->add(&terrain_renderable);
    }

    bool load(const std::string& path) {
        gfxm::vec3 position = gfxm::vec3(0, 0, 0);

        transform.acquire();
        transform->setTranslation(position);
        terrain_material = resGet<gpuMaterial>("materials/terrain.mat");

        ktImage img_heightmap;
        if (!loadImage(&img_heightmap, "textures/terrain/iceland_heightmap.png")) {
            return false;
        }
        /*
        if (!loadImage(&img_heightmap, "textures/terrain/heightmap.jpg")) {
            return false;
        }*/

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

        const float WIDTH = 2048.f;//4096.f;
        const float HEIGHT = 2048.f;//4096.f;
        const float MAX_DEPTH = 60.f;
        const int SEGMENTS_W = 2048;
        const int SEGMENTS_H = 2048;
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
                    float h = img_heightmap.samplef(x / float(SEGMENTS_W), y / float(SEGMENTS_H)).x;
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
                    float h = img_heightmap.samplef(x / float(SEGMENTS_W), y / float(SEGMENTS_H)).x;
                    
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
        terrain_mesh.setData(&mesh3d);

        auto transform_block = gpuGetPipeline()->getParamBlockContext()->createParamBlock<gpuTransformBlock>();
        terrain_renderable.attachParamBlock(transform_block);
        terrain_renderable.setMaterial(terrain_material.get());
        terrain_renderable.setMeshDesc(terrain_mesh.getMeshDesc());
        terrain_renderable.setRole(GPU_Role_Geometry);
        terrain_renderable.compile();

        {
            const int SAMPLE_WIDTH = SEGMENTS_W;//img_heightmap.getWidth();
            const int SAMPLE_DEPTH = SEGMENTS_H;//img_heightmap.getHeight();
            std::vector<float> height_data;
            height_data.resize(SAMPLE_WIDTH * SAMPLE_DEPTH);
            for (int z = 0; z < SAMPLE_DEPTH; ++z) {
                for (int x = 0; x < SAMPLE_WIDTH; ++x) {
                    float u = x / float(SAMPLE_WIDTH);
                    float v = z / float(SAMPLE_DEPTH);
                    auto col = img_heightmap.samplef(u, v);
                    float h = col.x;
                    //h *= h;
                    height_data[x + z * SAMPLE_WIDTH] = h * MAX_DEPTH;
                }
            }
            heightfield_shape.init(height_data.data(), SAMPLE_WIDTH, SAMPLE_DEPTH, WIDTH, HEIGHT);
        }

        terrain_body.mass = .0f;
        terrain_body.setFlags(COLLIDER_STATIC);
        terrain_body.setShape(&heightfield_shape);
        terrain_body.setPosition(position);

        return true;
    }
};

void TerrainGameInstance::onInit(IEngineRuntime* rt) {
    world.reset(new RuntimeWorld);
    scene_mgr.reset(new SceneManager());
    world->spawn(scene_mgr.get());

    primary_view.reset(new EngineRenderView(gfxm::rect(0, 0, 1, 1), 0, 0, false));
    primary_player.reset(new LocalPlayer(primary_view.get(), 0));

    primary_view->setRenderTarget(gpuGetDefaultRenderTarget());    
    if(auto list = rt->getComponent<RenderViewList>()) {
        list->push_back(primary_view.get());
    }

    playerAdd(primary_player.get());
    playerSetPrimary(primary_player.get());

    scene_mgr->loadScene<TerrainScene>("");

    // TODO: Should be loaded from config file
    // Input: bind actions and ranges
    inputCreateActionDesc("C")
        .linkKey(Key.Keyboard.C, 1.f);
    inputCreateActionDesc("V")
        .linkKey(Key.Keyboard.V, 1.f);
    inputCreateActionDesc("Z")
        .linkKey(Key.Keyboard.Z, 1.f);
    inputCreateActionDesc("X")
        .linkKey(Key.Keyboard.X, 1.f);
    inputCreateActionDesc("ToggleWireframe")
        .linkKey(Key.Keyboard.I, 1.f);
    inputCreateActionDesc("Recover")
        .linkKey(Key.Keyboard.Q, 1.f);
    inputCreateActionDesc("SphereCast")
        .linkKey(Key.Keyboard.Z, 1.f);
    inputCreateActionDesc("CharacterInteract")
        .linkKey(Key.Keyboard.E, 1.f);
    inputCreateActionDesc("Shoot")
        .linkKey(Key.Mouse.BtnLeft, 1.f);
    inputCreateActionDesc("ShootAlt")
        .linkKey(Key.Mouse.BtnRight, 1.f);
    inputCreateActionDesc("Sprint")
        .linkKey(Key.Keyboard.LeftShift, 1.f);
    inputCreateActionDesc("Jump")
        .linkKey(Key.Keyboard.Space, 1.f);
    inputCreateActionDesc("Crouch")
        .linkKey(Key.Keyboard.LeftControl, 1.f);

    inputCreateRangeDesc("CharacterLocomotion")
        .linkKeyX(Key.Keyboard.A, -1.0f)
        .linkKeyX(Key.Keyboard.D, 1.0f)
        .linkKeyZ(Key.Keyboard.W, -1.0f)
        .linkKeyZ(Key.Keyboard.S, 1.0f);
    inputCreateRangeDesc("CameraRotation")
        .linkKeyY(Key.Mouse.AxisX, 1.0f)
        .linkKeyX(Key.Mouse.AxisY, 1.0f);
    inputCreateRangeDesc("Scroll")
        .linkKeyX(Key.Mouse.Scroll, -1.0f);

    for (int i = 0; i < 12; ++i) {
        inputCreateActionDesc(MKSTR("F" << (i + 1)).c_str())
            .linkKey(Key.Keyboard.F1 + i, 1.f);
    }
    for (int i = 0; i < 9; ++i) {
        inputCreateActionDesc(MKSTR("_" << i).c_str())
            .linkKey(Key.Keyboard._0 + i, 1.f);
    }

    render_target_switcher.reset(new DbgRenderTargetSwitcher(playerGetPrimary()->getInputState()));

    // Fps actor
    {
        fps_player_actor.setFlags(ACTOR_FLAG_UPDATE);
        auto capsule = fps_player_actor.setRoot<CharacterCapsuleNode>("capsule");
        capsule->shape.height = 1.2f;
        capsule->shape.radius = .2f;
        capsule->collider.setCenterOffset(gfxm::vec3(.0f, .8f + .2f, .0f));
        capsule->collider.collision_group
            = COLLISION_LAYER_CHARACTER;
        capsule->createChild<EmptyNode>("head");
        fps_player_actor.addDriver<FpsCharacterDriver>();
        //fps_player_actor.addController<FpsCameraController>();

        //fps_player_actor.setTranslation(hl2scene.player_origin);
        //fps_player_actor.setRotation(hl2bspmodel.player_orientation);
        //fps_player_actor.getDriver<FpsCharacterDriver>()->setOrientation(hl2scene.player_orientation);

        getWorld()->spawn(&fps_player_actor);
        if (auto sys = getWorld()->getSystem<PlayerStartSystem>()) {
            if (!sys->points.empty()) {
                fps_player_actor.setTranslation(sys->points[0].origin);
                //fps_player_actor.setRotation(sys->points[0].rotation);
                fps_player_actor.getDriver<FpsCharacterDriver>()->setOrientation(sys->points[0].rotation);
            } else {
                LOG_ERR("No player start points available");
            }
        } else {
            LOG_ERR("Hl2GameInstance: PlayerStartSystem not found");
        }
    }

    // Marble actor
    {
        Actor* actor = &marble_actor;
        actor->addDriver<MarbleDriver>();
        auto rigid_body = actor->setRoot<RigidBodyNode>("body");
        auto cam_target = rigid_body->createChild<EmptyNode>("cam_target");
        cam_target->getTransformHandle()->setInheritFlags(TRANSFORM_INHERIT_POSITION);
        cam_target->setTranslation(gfxm::vec3(0, 1., 0));
        auto model = rigid_body->createChild<SkeletalModelNode>("model");
        model->setModel(loadResource<SkeletalModel>("models/ball/ball"));
        getWorld()->spawn(actor);
        
        if (auto sys = getWorld()->getSystem<PlayerStartSystem>()) {
            if (!sys->points.empty()) {
                actor->setTranslation(sys->points[0].origin + gfxm::vec3(0, 2, 0));
            } else {
                LOG_ERR("No player start points available");
            }
        } else {
            LOG_ERR("Hl2GameInstance: PlayerStartSystem not found");
        }
    }

    playerGetPrimary()->clearRoles();
    playerGetPrimary()->addRole<FpsPlayerController>(*getWorld(), &fps_player_actor);
    //playerGetPrimary()->addRole<FreeCamAgent>(*getWorld());
    playerGetPrimary()->addRole<FpsSpectator>(*getWorld(), &fps_player_actor);

    auto input_state = playerGetPrimary()->getInputState();
    input_state->pushContext(&input_ctx);

    inputRecover = input_ctx.createAction("Recover");
    inputStepPhysics = input_ctx.createAction("Z");
    inputRunPhysics = input_ctx.createAction("X");
    inputSphereCast = input_ctx.createAction("SphereCast");
    for (int i = 0; i < 9; ++i) {
        inputNumButtons[i] = input_ctx.createAction(MKSTR("_" << i).c_str());
    }

    {
        LocalPlayer* local_player = dynamic_cast<LocalPlayer*>(playerGetPrimary());
        assert(local_player);
        EngineRenderView* viewport = local_player->getViewport();
        assert(viewport);
        gpuRenderTarget* render_target = viewport->getRenderTarget();

        //render_target->setDefaultOutput("Albedo", RT_OUTPUT_RGB);
        //gpuGetPipeline()->enableTechnique("Posteffects/GammaTonemap", false);
        //gpuGetPipeline()->enableTechnique("Fog", false);
    }
}
void TerrainGameInstance::onCleanup() {
    world.reset();
}

extern bool dbg_stepPhysics;
void TerrainGameInstance::onUpdate(float dt) {
    LocalPlayer* local_player = dynamic_cast<LocalPlayer*>(playerGetPrimary());
    assert(local_player);
    EngineRenderView* viewport = local_player->getViewport();
    assert(viewport);
    gpuRenderTarget* render_target = viewport->getRenderTarget();

    if (inputRecover->isJustPressed()) {
        if (auto sys = getWorld()->getSystem<PlayerStartSystem>()) {
            if (!sys->points.empty()) {
                static int i = 0;
                const auto& pt = sys->points[i++];
                i = i % sys->points.size();
                fps_player_actor.setTranslation(pt.origin);
                //fps_player_actor.setRotation(pt.rotation);
                fps_player_actor.getDriver<FpsCharacterDriver>()->setOrientation(pt.rotation);
            } else {
                LOG_ERR("No player start points available");
            }
        } else {
            LOG_ERR("Hl2GameInstance: PlayerStartSystem not found");
        }
    }

    if (inputStepPhysics->isJustPressed()) {
        dbg_stepPhysics = true;
    }
    if (inputRunPhysics->isPressed()) {
        dbg_stepPhysics = true;
    }
    
    if (inputNumButtons[1]->isJustPressed()) {
        /*
        playerGetPrimary()->clearRoles();
        playerGetPrimary()->addRole<TpsPlayerController>(*getWorld(), chara_actor.get());
        playerGetPrimary()->addRole<TpsSpectator>(*getWorld(), chara_actor.get());
        audioPlayOnce(clip_whsh->getBuffer(), .5f, .0f);
        */
    } else if(inputNumButtons[2]->isJustPressed()) {
        /*playerGetPrimary()->clearRoles();
        playerGetPrimary()->addRole<TpsPlayerController>(*getWorld(), chara_actor.get());
        audioPlayOnce(clip_whsh->getBuffer(), .5f, .0f);*/
        playerGetPrimary()->clearRoles();
        playerGetPrimary()->addRole<FpsPlayerController>(*getWorld(), &fps_player_actor);
    } else if(inputNumButtons[3]->isJustPressed()) {
        playerGetPrimary()->clearRoles();
        playerGetPrimary()->addRole<FreeCamAgent>(*getWorld());
    } else if(inputNumButtons[4]->isJustPressed()) {
        playerGetPrimary()->clearRoles();
        playerGetPrimary()->addRole<FpsPlayerController>(*getWorld(), &fps_player_actor);
        playerGetPrimary()->addRole<FpsSpectator>(*getWorld(), &fps_player_actor);
    } else if(inputNumButtons[5]->isJustPressed()) {
        /*
        playerGetPrimary()->clearRoles();
        playerGetPrimary()->addRole<TpsPlayerController>(*getWorld(), chara_actor_2.get());
        playerGetPrimary()->addRole<TpsSpectator>(*getWorld(), chara_actor_2.get());
        audioPlayOnce(clip_whsh->getBuffer(), .5f, .0f);
        */
    } else if(inputNumButtons[6]->isJustPressed()) {
        /*
        playerGetPrimary()->clearAgents();
        playerLinkAgent(playerGetPrimary(), &demo_camera_actor);
        audioPlayOnce(clip_whsh->getBuffer(), .5f, .0f);
        */
    } else if(inputNumButtons[7]->isJustPressed()) {
        playerGetPrimary()->clearRoles();
        playerGetPrimary()->addRole<TpsPlayerController>(*getWorld(), &marble_actor);
        playerGetPrimary()->addRole<TpsSpectator>(*getWorld(), &marble_actor);
    } else if(inputNumButtons[0]->isJustPressed()) {
        static bool dbg_enableCollisionDbgDraw = false;
        dbg_enableCollisionDbgDraw = !dbg_enableCollisionDbgDraw;
        getWorld()->getSystem<phyWorld>()->enableDbgDraw(dbg_enableCollisionDbgDraw);
    }

    render_target_switcher->update(render_target);
    
    {
        // Sphere cast
        static gfxm::vec3 from = gfxm::vec3(0, 3, 0);
        static gfxm::vec3 to = gfxm::vec3(0,0,0);
        static float radius = .1f;
        if (inputSphereCast->isPressed()) {
            int mx, my;
            platformGetMousePos(&mx, &my);
            gfxm::rect rc = platformGetViewportRect();
            gfxm::vec2 vp_size(rc.max.x - rc.min.x, rc.max.y - rc.min.y);
            my = vp_size.y - my;
            auto ray = gfxm::ray_viewport_to_world(
                vp_size, gfxm::vec2(mx, my),
                viewport->getProjection(), viewport->getViewTransform()
            );
            from = ray.origin;
            to = ray.origin + ray.direction * 5.f;
            //from = ray.origin;
            //to = from + gfxm::vec3(.0f, -3.f, .0f);
        }
        auto res = getWorld()->getSystem<phyWorld>()->sphereSweep(from, to, radius);
        if (res.hasHit) {
            dbgDrawSphere(res.sphere_pos, radius, 0xFF00FF00);
        }
    }

    // TODO:

    world->update(dt);
}
void TerrainGameInstance::onDraw(float dt) {
    /*
    LocalPlayer* local_player = dynamic_cast<LocalPlayer*>(playerGetPrimary());
    assert(local_player);
    EngineRenderView* viewport = local_player->getViewport();
    assert(viewport);
    gpuRenderBucket* render_bucket = viewport->getRenderBucket();
    gpuRenderTarget* render_target = viewport->getRenderTarget();
    
    hl2scene.draw(render_bucket);*/
}


void TerrainGameInstance::onPlayerJoined(IPlayer* player) {
    LOG("IGameInstance: onPlayerJoined");
    LocalPlayer* local = dynamic_cast<LocalPlayer*>(player);
    if (!local) {
        return;
    }
    assert(local->getViewport());

    Camera* cam = new Camera;
    world->spawn(cam);
    local->getViewport()->setCamera(cam);
    cam->setZNear(.01f);
    cam->setZFar(1000.f);
}
void TerrainGameInstance::onPlayerLeft(IPlayer* player) {
    LOG("IGameInstance: onPlayerLeft");
    LocalPlayer* local = dynamic_cast<LocalPlayer*>(player);
    if (!local) {
        return;
    }
    assert(local->getViewport());

    auto cam = local->getViewport()->getCamera();
    world->despawn(cam);
    delete cam;
}

