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

#include "m3d/skeletal_instance.hpp"

class ISpatial {
public:
    ~ISpatial() {}
    virtual HTransform& getTransformNode() = 0;
};

class SPW_StaticModel
    : public ISpawnable
    , public SceneProxy
    , public ISpatial
{
    HTransform root_transform;
    std::vector<HTransform> inner_nodes;
    ResourceRef<m3dModel> model;
    std::vector<std::unique_ptr<gpuRenderable>> renderables;
    std::vector<gpuTransformBlock*> transform_blocks;
public:
    SPW_StaticModel() {
        root_transform.acquire();
    }
    ~SPW_StaticModel() {
        for (int i = 0; i < transform_blocks.size(); ++i) {
            gpuRemoveTransformSync(transform_blocks[i]);
            gpuGetDevice()->destroyParamBlock(transform_blocks[i]);
        }
        transform_blocks.clear();
        for (int i = 0; i < inner_nodes.size(); ++i) {
            inner_nodes[i].release();
        }
        root_transform.release();
    }
    void setModel(const ResourceRef<m3dModel>& model) {
        renderables.clear();
        for (int i = 0; i < transform_blocks.size(); ++i) {
            gpuGetDevice()->destroyParamBlock(transform_blocks[i]);
        }

        struct TRANSFORM {
            HTransform node;
            gpuTransformBlock* block = nullptr;
        };
        std::map<std::string, TRANSFORM> node_map;

        this->model = model;
        for (int i = 0; i < model->mesh_instances.size(); ++i) {
            const auto& m3d_mesh_inst = model->mesh_instances[i];
            auto& m3d_mesh = model->meshes[m3d_mesh_inst.mesh_idx];
            auto bone = model->skeleton->findBone(m3d_mesh_inst.bone_name.c_str());
            
            auto it = node_map.find(m3d_mesh_inst.bone_name);
            if (it == node_map.end()) {
                HTransform node = HANDLE_MGR<TransformNode>::acquire();
                gpuTransformBlock* transform_block = gpuGetDevice()->createParamBlock<gpuTransformBlock>();
                transform_block->setTransform(node->getWorldTransform());
                gpuAddTransformSync(transform_block, node);
                transform_blocks.push_back(transform_block);

                it = node_map.insert(
                    std::make_pair(m3d_mesh_inst.bone_name, TRANSFORM{ node, transform_block })
                ).first;
                transformNodeAttach(root_transform, node);
                gfxm::mat4 bone_tr = bone->getWorldTransform();
                node->setTranslation(bone_tr[3]);
                node->setRotation(gfxm::to_quat(gfxm::to_orient_mat3(bone_tr)));
                node->setScale(gfxm::vec3(gfxm::length(bone_tr[0]), gfxm::length(bone_tr[1]), gfxm::length(bone_tr[2])));
                inner_nodes.push_back(node);
            }
            HTransform node = it->second.node;
            gpuTransformBlock* transform_block = it->second.block;

            auto& prdr = renderables.emplace_back();
            prdr.reset(new gpuRenderable);
            auto& m = model->meshes[i];
            auto mat = model->materials[m.material_idx];
            prdr->setMaterial(mat.get());
            prdr->setMeshDesc(m.mesh->getMeshDesc());
            prdr->setRole(GPU_Role_Geometry);
            //prdr->enableEffect(GPU_Effect_Outline);
            prdr->attachParamBlock(transform_block);
            prdr->compile();
        }
    }

    void onSpawn(WorldSystemRegistry& reg) {
        if (auto sys = reg.getSystem<SceneSystem>()) {
            sys->addProxy(this);
        }
    }
    void onDespawn(WorldSystemRegistry& reg) {
        if (auto sys = reg.getSystem<SceneSystem>()) {
            sys->removeProxy(this);
        }
    }
    void updateBounds() override {
        if (!model) {
            assert(false);
            return;
        }
        const gfxm::mat4 transform = root_transform->getWorldTransform();

        setBoundingSphere(
            model->bounding_radius,
            transform * gfxm::vec4(model->bounding_sphere_origin, 1.f)
        ); // TODO: scaling

        gfxm::aabb box = model->aabb;
        const gfxm::vec3 points[] = {
            transform * gfxm::vec4(box.from, 1.f),
            transform * gfxm::vec4(box.to.x, box.from.y, box.from.z, 1.f),
            transform * gfxm::vec4(box.to.x, box.from.y, box.to.z, 1.f),
            transform * gfxm::vec4(box.from.x, box.from.y, box.to.z, 1.f),
            transform * gfxm::vec4(box.from.x, box.to.y, box.from.z, 1.f),
            transform * gfxm::vec4(box.to.x, box.to.y, box.from.z, 1.f),
            transform * gfxm::vec4(box.to, 1.f),
            transform * gfxm::vec4(box.from.x, box.to.y, box.to.z, 1.f)
        };
        box.from = points[0];
        box.to = points[0];
        for (int i = 1; i < sizeof(points) / sizeof(points[0]); ++i) {
            gfxm::expand_aabb(box, points[i]);
        }
        setBoundingBox(box);
    }
    void submit(gpuRenderBucket* bucket) override {
        for (int i = 0; i < renderables.size(); ++i) {
            auto rdr = renderables[i].get();
            bucket->add(rdr);
        }
    }
    HTransform& getTransformNode() override { return root_transform; }
};

class SPW_SkeletalModel
    : public ISpawnable
    , public SceneProxy
    , public ISpatial
{
    HTransform root_transform;
    m3dSkeletalInstance m3d_instance;
public:
    SPW_SkeletalModel() {
        root_transform.acquire();
    }
    ~SPW_SkeletalModel() {
        root_transform.release();
    }

    void setModel(const ResourceRef<m3dModel>& mdl) {
        m3d_instance.attachTo(root_transform);
        m3d_instance.init(mdl);
    }

    SkeletonInstance* getSkeletonInstance() {
        return m3d_instance.getSkeletonInstance();
    }

    void onSpawn(WorldSystemRegistry& reg) {
        if (auto sys = reg.getSystem<SceneSystem>()) {
            sys->addProxy(this);
        }
    }
    void onDespawn(WorldSystemRegistry& reg) {
        if (auto sys = reg.getSystem<SceneSystem>()) {
            sys->removeProxy(this);
        }
    }
    void updateBounds() override {
        const m3dModel* model = m3d_instance.getModel();
        if (!model) {
            assert(false);
            return;
        }
        const gfxm::mat4& transform = root_transform->getWorldTransform();

        setBoundingSphere(
            model->bounding_radius,
            transform * gfxm::vec4(model->bounding_sphere_origin, 1.f)
        ); // TODO: scaling

        gfxm::aabb box = model->aabb;
        const gfxm::vec3 points[] = {
            transform * gfxm::vec4(box.from, 1.f),
            transform * gfxm::vec4(box.to.x, box.from.y, box.from.z, 1.f),
            transform * gfxm::vec4(box.to.x, box.from.y, box.to.z, 1.f),
            transform * gfxm::vec4(box.from.x, box.from.y, box.to.z, 1.f),
            transform * gfxm::vec4(box.from.x, box.to.y, box.from.z, 1.f),
            transform * gfxm::vec4(box.to.x, box.to.y, box.from.z, 1.f),
            transform * gfxm::vec4(box.to, 1.f),
            transform * gfxm::vec4(box.from.x, box.to.y, box.to.z, 1.f)
        };
        box.from = points[0];
        box.to = points[0];
        for (int i = 1; i < sizeof(points) / sizeof(points[0]); ++i) {
            gfxm::expand_aabb(box, points[i]);
        }
        setBoundingBox(box);
    }
    void submit(gpuRenderBucket* bucket) override {
        m3d_instance.submit(bucket);
    }
    HTransform& getTransformNode() override { return root_transform; }
};

class SPW_M3DTest
    : public ISpawnable
    , public SceneProxy
    , public ISpatial
    , public ITickable
{
    HTransform root_transform;
    m3dSkeletalInstance m3d_instance;

    int current_anim = 0;
    float t_anim = .0f;
public:
    SPW_M3DTest() {
        root_transform.acquire();
    }
    ~SPW_M3DTest() {
        root_transform.release();
    }

    void setModel(const ResourceRef<m3dModel>& mdl) {
        m3d_instance.attachTo(root_transform);
        m3d_instance.init(mdl);
    }

    SkeletonInstance* getSkeletonInstance() {
        return m3d_instance.getSkeletonInstance();
    }

    void onSpawn(WorldSystemRegistry& reg) {
        if (auto sys = reg.getSystem<SceneSystem>()) {
            sys->addProxy(this);
        }
        if (auto sys = reg.getSystem<TickSystem>()) {
            sys->add(this);
        }
    }
    void onDespawn(WorldSystemRegistry& reg) {
        if (auto sys = reg.getSystem<SceneSystem>()) {
            sys->removeProxy(this);
        }
        if (auto sys = reg.getSystem<TickSystem>()) {
            sys->remove(this);
        }
    }
    void updateBounds() override {
        const m3dModel* model = m3d_instance.getModel();
        if (!model) {
            assert(false);
            return;
        }
        const gfxm::mat4& transform = root_transform->getWorldTransform();

        setBoundingSphere(
            model->bounding_radius,
            transform * gfxm::vec4(model->bounding_sphere_origin, 1.f)
        ); // TODO: scaling

        gfxm::aabb box = model->aabb;
        const gfxm::vec3 points[] = {
            transform * gfxm::vec4(box.from, 1.f),
            transform * gfxm::vec4(box.to.x, box.from.y, box.from.z, 1.f),
            transform * gfxm::vec4(box.to.x, box.from.y, box.to.z, 1.f),
            transform * gfxm::vec4(box.from.x, box.from.y, box.to.z, 1.f),
            transform * gfxm::vec4(box.from.x, box.to.y, box.from.z, 1.f),
            transform * gfxm::vec4(box.to.x, box.to.y, box.from.z, 1.f),
            transform * gfxm::vec4(box.to, 1.f),
            transform * gfxm::vec4(box.from.x, box.to.y, box.to.z, 1.f)
        };
        box.from = points[0];
        box.to = points[0];
        for (int i = 1; i < sizeof(points) / sizeof(points[0]); ++i) {
            gfxm::expand_aabb(box, points[i]);
        }
        setBoundingBox(box);
    }

    void submit(gpuRenderBucket* bucket) override {
        m3d_instance.submit(bucket);
    }
    
    HTransform& getTransformNode() override { return root_transform; }
    
    void onTick(float dt) override {
        auto model = m3d_instance.getModel();
        if (!model) {
            return;
        }
        if (model->animations.empty()) {
            return;
        }
        auto skl_inst = m3d_instance.getSkeletonInstance();
        auto& anim = model->animations[current_anim];
        animSampler sampler(skl_inst->getSkeletonMaster(), const_cast<Animation*>(anim.get()));
        animSampleBuffer buf;
        buf.init(skl_inst->getSkeletonMaster());
        sampler.sample(buf.data(), buf.count(), t_anim);
        buf.applySamples(skl_inst);
        t_anim += dt * anim->fps;
        if (t_anim > anim->length) {
            ++current_anim;
            current_anim = current_anim % model->animations.size();
            t_anim -= anim->length;
        }
    }
};

void TerrainGameInstance::onInit(IEngineRuntime* rt) {
    world.reset(new RuntimeWorld);
    scene.reset(new TerrainScene);
    scene->load("");
    world->attachScene(scene.get());

    primary_view.reset(new EngineRenderView(gfxm::rect(0, 0, 1, 1), 0, 0, false));
    primary_player.reset(new LocalPlayer(primary_view.get(), 0));

    primary_view->setRenderTarget(gpuGetDefaultRenderTarget());    
    if(auto list = rt->getComponent<RenderViewList>()) {
        list->push_back(primary_view.get());
    }

    playerAdd(primary_player.get());
    playerSetPrimary(primary_player.get());

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
        //gpuGetPipeline()->enableTechnique("Posteffects/Lens", true);
        //gpuGetPipeline()->enableTechnique("Fog", false);
    }


    // m3d test
    {
        /*
        m3dpProject proj;
        proj.initFromSource(
            //"models/DamagedHelmet/glTF-Binary/DamagedHelmet.glb"
            //"models/ren/ren.fbx"
            "models/ren/ren.glb"
            //"models/chara_24.fbx"
            //"models/chara_26.fbx"
            //"models/chara_26.glb"
            //"models/ultima_weapon.fbx"
        );
        ResourceRef<m3dModel> model = ResourceManager::get()->create<m3dModel>("my_model");
        proj.import(*model.get());
        */

        ResourceRef<m3dModel> model = loadResource<m3dModel>(
            "models/ren/ren"
            //"models/cube"
        );

        auto spw = new SPW_StaticModel;
        spw->setModel(model);
        spw->getTransformNode()->setTranslation(512, .5, 500);
        getWorld()->spawn(spw);

        auto spw2 = new SPW_M3DTest;
        spw2->setModel(model);
        spw2->getTransformNode()->setTranslation(515, .5, 500);
        getWorld()->spawn(spw2);
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
    cam->setZNear(.1f);
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

