#pragma once

#include "actor.auto.hpp"

#include "node/actor_node.hpp"
#include "message.hpp"
#include "world/component/actor_component.hpp"
#include "world/controller/actor_controller.hpp"
#include "game_messaging/game_messaging.hpp"

#include "spawnable/spawnable.hpp"

#include "actor_prefab.hpp"


typedef uint64_t actor_flags_t;

const actor_flags_t ACTOR_FLAGS_NOTSET = 0x00000000;
const actor_flags_t ACTOR_FLAG_DEFAULT = 0x00000001;
const actor_flags_t ACTOR_FLAG_UPDATE = 0x00000010;

class IPlayer;

[[cppi_class]];
class Actor : public rtti::MetaObject, public ISpawnable {
    friend struct ActorOps;

protected:
    actor_flags_t flags = ACTOR_FLAG_DEFAULT;
    std::string name;

    ActorNode* root_node = nullptr;
    std::unordered_map<rtti::type, std::unique_ptr<ActorComponent>> components;
    std::unordered_map<rtti::type, std::unique_ptr<ActorDriver>> drivers;

    void _resolveDirtyNodes();

    void _removeRoot() {
        if(!root_node) return;
        if (!has_flags(root_node->flags, FActorNode::TreeOwned)) {
            root_node = nullptr;
            return;
        }

        if (isSpawned()) {
            root_node->onDespawnNodeInternal(*getRegistry());
        }

        delete root_node;
        root_node = nullptr;
    }

    void registerNode(ActorNode* parent, ActorNode* child);

public:
    TYPE_ENABLE();

    Actor();
    Actor(const Actor&) = delete;
    Actor& operator=(const Actor&) = delete;
    virtual ~Actor() {
        _removeRoot();
        tryDespawn();
    }

    void requestRebuild();

    [[cppi_decl, set("name")]]
    void setName(const std::string& n) { name = n; }
    [[cppi_decl, get("name")]]
    const std::string& getName() const { return name; }

    // Node access
    ActorNode* setRoot(rtti::type t) {
        _removeRoot();
        root_node = t.construct_new<ActorNode>();
        assert(root_node);
        root_node->actor = this;
        root_node->flags |= FActorNode::TreeOwned;
        root_node->onDefault();
        requestRebuild();
        return root_node;
    }
    template<typename NODE_T>
    NODE_T* setRoot(const char* name) {
        static_assert(std::is_base_of_v<ActorNode, NODE_T>, "NODE_T must derive from ActorNode");
        _removeRoot();
        auto ptr = new NODE_T;
        root_node = ptr;
        root_node->name = name;
        root_node->actor = this;
        root_node->flags |= FActorNode::TreeOwned;
        root_node->onDefault();
        requestRebuild();
        return ptr;
    }
    ActorNode* getRoot() { return root_node; }
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
    int componentCount() const {
        return components.size();
    }
    ActorComponent* getComponent(int i) {
        auto it = components.begin();
        std::advance(it, i);
        return it->second.get();
    }
    ActorComponent* addComponent(rtti::type t) {
        auto it = components.find(t);
        if (it != components.end()) {
            assert(false);
            LOG_ERR("Component " << t.get_name() << " already exists");
            return it->second.get();
        }
        ActorComponent* ptr = t.construct_new<ActorComponent>();
        components.insert(
            std::make_pair(t, std::unique_ptr<ActorComponent>(ptr))
        );
        return ptr;
    }
    template<typename COMPONENT_T>
    COMPONENT_T* addComponent() {
        rtti::type t = rtti::type_get<COMPONENT_T>();
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
        rtti::type t = rtti::type_get<COMPONENT_T>();
        auto it = components.find(t);
        if (it == components.end()) {
            return 0;
        }
        return (COMPONENT_T*)it->second.get();
    }
    template<typename COMPONENT_T>
    void removeComponent() {
        rtti::type t = rtti::type_get<COMPONENT_T>();
        auto it = components.find(t);
        if (it == components.end()) {
            assert(false);
            LOG_ERR("Component " << t.get_name() << " does not exist");
            return;
        }
        components.erase(it);
    }

    // Controller access
    int driverCount() const {
        return drivers.size();
    }
    ActorDriver* getDriver(int i) {
        auto it = drivers.begin();
        std::advance(it, i);
        return it->second.get();
    }
    ActorDriver* addDriver(rtti::type t) {
        auto it = drivers.find(t);
        if (it != drivers.end()) {
            assert(false);
            LOG_ERR("Driver " << t.get_name() << " already exists");
            return it->second.get();
        }
        ActorDriver* drv = t.construct_new<ActorDriver>();
        drv->owner = this;
        drivers.insert(std::make_pair(t, std::unique_ptr<ActorDriver>(drv)));
        return drv;
    }
    template<typename DRIVER_T>
    DRIVER_T* addDriver() {
        rtti::type t = rtti::type_get<DRIVER_T>();
        auto it = drivers.find(t);
        if (it != drivers.end()) {
            assert(false);
            LOG_ERR("Driver " << t.get_name() << " already exists");
            return (DRIVER_T*)it->second.get();
        }
        auto ptr = new DRIVER_T;
        ptr->owner = this;
        drivers.insert(std::make_pair(t, std::unique_ptr<ActorDriver>(ptr)));
        return ptr;
    }
    template<typename DRIVER_T>
    DRIVER_T* getDriver() {
        rtti::type t = rtti::type_get<DRIVER_T>();
        auto it = drivers.find(t);
        if (it == drivers.end()) {
            return 0;
        }
        return (DRIVER_T*)it->second.get();
    }

    void setFlags(actor_flags_t flags) { this->flags = flags; }
    void setFlagsDefault() { flags = ACTOR_FLAG_DEFAULT; }
    actor_flags_t getFlags() const { return flags; }
    
    void onUpdateInternal(float dt) {
        onUpdate(dt);
    }

    // Callbacks
    void onSpawn(WorldSystemRegistry& reg) override;
    void onDespawn(WorldSystemRegistry& reg) override;
    virtual void onUpdate(float dt) {}
    virtual GAME_MESSAGE onMessage(GAME_MESSAGE msg);

    // Messaging
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
    void setTranslation(float x, float y, float z) { setTranslation(gfxm::vec3(x, y, z)); }
    void setRotation(const gfxm::quat& q) { getRoot()->setRotation(q); }
    void setScale(const gfxm::vec3& s) { getRoot()->setScale(s); }

    void translate(const gfxm::vec3& t) { getRoot()->translate(t); }
    void rotate(const gfxm::quat& q) { getRoot()->rotate(q); }

    gfxm::vec3 getTranslation() { return getRoot()->getWorldTranslation(); }
    gfxm::quat getRotation() { return getRoot()->getWorldRotation(); }

    gfxm::vec3 getForward() { return gfxm::normalize(getWorldTransform()[2]); }
    gfxm::vec3 getLeft() { return gfxm::normalize(-getWorldTransform()[0]); }

    gfxm::mat4 getWorldTransform() { return getRoot()->getWorldTransform(); }

    void makePrefab(ActorPrefab& prefab);

    void dbgDraw() const;
};

