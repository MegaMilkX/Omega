#pragma once

#include "actor.auto.hpp"

#include "node/actor_node.hpp"
#include "message.hpp"
#include "world/component/actor_component.hpp"
#include "world/controller/actor_controller.hpp"
#include "game_messaging/game_messaging.hpp"


class Actor;
bool actorWriteJson(Actor* actor, const char* path);
Actor* actorReadJson(const char* path);


typedef uint64_t actor_flags_t;

const actor_flags_t ACTOR_FLAGS_NOTSET = 0x00000000;
const actor_flags_t ACTOR_FLAG_DEFAULT = 0x00000001;
const actor_flags_t ACTOR_FLAG_UPDATE = 0x00000010;

class IPlayer;

[[cppi_class]];
class Actor {
    friend RuntimeWorld;

    int transient_id = -1;
    type current_state_type = 0;
    size_t current_state_array_index = 0;

    IPlayer* attached_player = 0;
    RuntimeWorld* current_world = 0;
public:
    TYPE_ENABLE();
protected:
    actor_flags_t flags = ACTOR_FLAG_DEFAULT;

    std::unique_ptr<ActorNode> root_node;
    std::unordered_map<type, std::unique_ptr<ActorComponent>> components;
    std::unordered_map<type, std::unique_ptr<ActorController>> controllers;

    void _onSpawn(RuntimeWorld* world) {
        current_world = world;
        onSpawn(world);
        for (auto& kv : controllers) {
            kv.second->onReset();
        }

        for (auto& kv : controllers) {
            kv.second->onSpawn(this);
        }

        if (root_node) { 
            root_node->_spawn(world);
            root_node->_registerGraphWorld(world);
            for (auto& kv : controllers) {
                root_node->_registerGraph(kv.second.get());
            }
        }
    }
    void _onDespawn(RuntimeWorld* world) {
        current_world = 0;
        if (root_node) {
            for (auto& kv : controllers) {
                root_node->_unregisterGraph(kv.second.get());
            }
            root_node->_despawn(world); 
        }

        for (auto& kv : controllers) {
            kv.second->onDespawn(this);
        }

        onDespawn(world);
    }
    void _onUpdate(RuntimeWorld* world, float dt) {
        if (root_node) { root_node->_update(world, dt); }
        onUpdate(world, dt);
    }
    void _onDecay(RuntimeWorld* world) {
        if (root_node) { root_node->_decay(world); }
        onDecay(world);
    }
    void _onUpdateDecay(RuntimeWorld* world, float dt) {
        if (root_node) { root_node->_updateDecay(world, dt); }
        onUpdateDecay(world, dt);
    }
    bool hasDecayed_world() const {
        if (root_node) {
            return root_node->hasDecayed_actor() && hasDecayed();
        } else {
            return hasDecayed();
        }
    }
public:
    Actor();
    virtual ~Actor() {}

    RuntimeWorld* getWorld() { return current_world; }

    // Node access
    template<typename NODE_T>
    NODE_T* setRoot(const char* name) {
        auto ptr = new NODE_T;
        root_node.reset(ptr);
        root_node->name = name;
        root_node->onDefault();
        return ptr;
    }
    ActorNode* getRoot() { return root_node.get(); }
    // Trigger a callback for each occurance of NODE_T node in the node tree
    template<typename NODE_T>
    void forEachNode(std::function<void(NODE_T*)> cb) {
        assert(getRoot());
        getRoot()->forEachNode<NODE_T>(cb);
    }

    template<typename NODE_T>
    NODE_T* findNode(const char* name) {
        return getRoot()->findNode<NODE_T>(name);
    }

    // Component access
    template<typename COMPONENT_T>
    COMPONENT_T* addComponent() {
        type t = type_get<COMPONENT_T>();
        auto it = components.find(t);
        if (it != components.end()) {
            assert(false);
            LOG_ERR("Component " << t.get_name() << " already exists");
            return (COMPONENT_T*)it->second.get();
        }
        auto ptr = new COMPONENT_T;
        components.insert(
            std::make_pair(t, std::unique_ptr<ActorComponent>(ptr))
        );
        return ptr;
    }
    template<typename COMPONENT_T>
    COMPONENT_T* getComponent() {
        type t = type_get<COMPONENT_T>();
        auto it = components.find(t);
        if (it == components.end()) {
            return 0;
        }
        return (COMPONENT_T*)it->second.get();
    }
    template<typename COMPONENT_T>
    void removeComponent() {
        type t = type_get<COMPONENT_T>();
        auto it = components.find(t);
        if (it == components.end()) {
            assert(false);
            LOG_ERR("Component " << t.get_name() << " does not exist");
            return;
        }
        components.erase(it);
    }

    // Controller access
    template<typename CONTROLLER_T>
    CONTROLLER_T* addController() {
        type t = type_get<CONTROLLER_T>();
        auto it = controllers.find(t);
        if (it != controllers.end()) {
            assert(false);
            LOG_ERR("Controller " << t.get_name() << " already exists");
            return (CONTROLLER_T*)it->second.get();
        }
        auto ptr = new CONTROLLER_T;
        ptr->owner = this;
        controllers.insert(std::make_pair(t, std::unique_ptr<ActorController>(ptr)));
        return ptr;
    }
    template<typename CONTROLLER_T>
    CONTROLLER_T* getController() {
        type t = type_get<CONTROLLER_T>();
        auto it = controllers.find(t);
        if (it == controllers.end()) {
            return 0;
        }
        return (CONTROLLER_T*)it->second.get();
    }

    // Misc. (TODO: Remove decay feature)
    void setFlags(actor_flags_t flags) { this->flags = flags; }
    void setFlagsDefault() { flags = ACTOR_FLAG_DEFAULT; }
    actor_flags_t getFlags() const { return flags; }
    bool isTransient() const { return transient_id >= 0; }
    virtual bool hasDecayed() const { return true; }
    bool isSpawned() const { return current_world != 0; }
    
    // Callbacks
    virtual void onSpawn(RuntimeWorld* world) {};
    virtual void onDespawn(RuntimeWorld* world) {};
    virtual void onUpdate(RuntimeWorld* world, float dt) {}
    virtual void onDecay(RuntimeWorld* world) {}
    virtual void onUpdateDecay(RuntimeWorld* world, float dt) {}
    //virtual wRsp onMessage(wMsg msg) { return 0; }
    virtual GAME_MESSAGE onMessage(GAME_MESSAGE msg);

    // Messaging
    //wRsp sendMessage(wMsg msg) { return onMessage(msg); }
    GAME_MESSAGE sendMessage(GAME_MSG msg) { return onMessage(GAME_MESSAGE(msg)); }
    GAME_MESSAGE sendMessage(GAME_MESSAGE msg) { return onMessage(msg); }
    template<typename PAYLOAD_T>
    GAME_MESSAGE sendMessage(PAYLOAD_T payload) {
        GAME_MESSAGE message = makeGameMessage(payload);
        return sendMessage(message);
    }

    void attachToTransform(Handle<TransformNode> node) {
        if (!getRoot()) {
            assert(false);
            return;
        }
        transformNodeAttach(node, getRoot()->getTransformHandle());
    }
    void detachFromTransform() {
        if (!getRoot()) {
            assert(false);
            return;
        }
        transformNodeAttach(0, getRoot()->getTransformHandle());
    }

    // Transform (TODO: remove)
    void setTranslation(const gfxm::vec3& t) { getRoot()->setTranslation(t); }
    void setRotation(const gfxm::quat& q) { getRoot()->setRotation(q); }
    void setScale(const gfxm::vec3& s) { getRoot()->setScale(s); }

    void translate(const gfxm::vec3& t) { getRoot()->translate(t); }
    void rotate(const gfxm::quat& q) { getRoot()->rotate(q); }

    gfxm::vec3 getTranslation() { return getRoot()->getWorldTranslation(); }
    gfxm::quat getRotation() { return getRoot()->getWorldRotation(); }

    gfxm::vec3 getForward() { return gfxm::normalize(getWorldTransform()[2]); }
    gfxm::vec3 getLeft() { return gfxm::normalize(-getWorldTransform()[0]); }

    gfxm::mat4 getWorldTransform() { return getRoot()->getWorldTransform(); }

    // ======

    void updateNodeTransform() {
        if (root_node) {
            root_node->_updateTransform();
        }
    }
    
    // ======

    [[cppi_decl, serialize_json]]
    void toJson(nlohmann::json& j);
    [[cppi_decl, deserialize_json]]
    bool fromJson(const nlohmann::json& j);
};