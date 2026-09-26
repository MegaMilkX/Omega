#pragma once

#include "actor_node.auto.hpp"

#include <string>
#include "handle/phandle.hpp"
#include "reflection/reflection.hpp"
#include "math/gfxm.hpp"
#include "world/controller/actor_controller.hpp"
#include "world/world_system_registry.hpp"
#include "util/static_block.hpp"
#include "transform_node/transform_node.hpp"
#include "debug_draw/debug_draw.hpp"

#include "world/common_systems/dirty_system.hpp"

#include "reflection/serialization.hpp"
#include "util/enum_flags.hpp"


struct TestDummyLinkData {};


enum class FActorNode : uint8_t {
    None = 0,
    TreeOwned = 0x01,
    Incomplete = 0x02,
    Spawned = 0x04
};
ENUM_FLAGS(FActorNode);


typedef uint32_t slot_flags_t;
constexpr slot_flags_t LINK_READ  = 0x1;
constexpr slot_flags_t LINK_WRITE = 0x2;
constexpr slot_flags_t LINK_READWRITE = LINK_READ | LINK_WRITE;

enum eNodeSlotRW {
    eNodeSlotRead,
    eNodeSlotWrite,
};
enum eNodeSlotKind {
    eSlotUpstream,
    eSlotDownstream,
};


struct NodeSlotDesc {
    rtti::type link_type;
    uint32_t flags;
    eNodeSlotKind kind;
};
using NodeSlotDescArray = std::vector<NodeSlotDesc>;

class ActorNode;
struct NodeSlot {
    ActorNode* node = nullptr;
    NodeSlotDesc desc;
    int slot_idx = 0;
};
using NodeSlotArray = std::vector<NodeSlot>;

struct NodeLink {
    ActorNode* writer = nullptr;
    ActorNode* reader = nullptr;
    int writer_slot = 0;
    int reader_slot = 0;
    int order = 0;
    rtti::type link_type; // used for debug
    bool is_downstream;
    int priority = 0;
};
using NodeLinkArray = std::vector<NodeLink>;


class ActorNode;

template<typename T>
using HActorNode = PHandle<ActorNode, T>;

class RuntimeWorld;
class Actor;
[[cppi_class]];
class ActorNode : public rtti::MetaObject {
    friend struct ActorOps;
    friend RuntimeWorld;
    friend Actor;

    Actor* actor = nullptr;
public:
    TYPE_ENABLE();
private:

    HActorNode<ActorNode> myhandle;

    std::string name;
    FActorNode flags;

    Handle<TransformNode> transform;
    ActorNode* parent = 0;
    std::vector<ActorNode*> children;

    void _registerGraph(ActorDriver* controller);
    void _unregisterGraph(ActorDriver* controller);
    void onSpawnNodeInternal(WorldSystemRegistry& reg);
    void onDespawnNodeInternal(WorldSystemRegistry& reg);

protected:
    void _buildLinks(NodeSlotArray& out_slots);
    void _buildLinksImpl(NodeLinkArray& out_links, NodeSlotArray& out_slots, int depth);

    template<typename T>
    T* findNearestAncestor() {
        static_assert(std::is_base_of_v<ActorNode, T>, "T must be an ActorNode");
        if (!parent) {
            return nullptr;
        }
        rtti::type pt = parent->get_type();
        if (pt == rtti::type_get<T>()) {
            return static_cast<T*>(parent);
        }
        return parent->findNearestAncestor<T>();
    }

    void requestRebuild();

    virtual const NodeSlotDescArray& getSlots() {
        static NodeSlotDescArray slots = {};
        return slots;
    }

    void _resetLinks() {
        onLinksReset();
    }

    virtual void onBuild() {}

    virtual void onLinksReset() {}
    virtual void onLinkWrite(int slot, rtti::varying& out) {
        //LOG_DBG(get_type().get_name() << " writes slot " << slot);
    }
    virtual void onLinkRead(int slot, const rtti::varying& in) {
        //LOG_DBG(get_type().get_name() << " reads slot " << slot << ": " << in.get_type().get_name());
    }
    virtual void onReady() {}
    virtual bool checkComplete() const { return true; }
public:
    ActorNode()
    : flags(FActorNode::None) {
        transform.acquire();
    }
    ActorNode(const ActorNode&) = delete;
    ActorNode& operator=(const ActorNode&) = delete;
    virtual ~ActorNode() {
        transform.release();
        if(myhandle) {
            myhandle.release();
        }
        for (int i = 0; i < children.size(); ++i) {
            if (!has_flags(children[i]->flags, FActorNode::TreeOwned)) {
                continue;
            }
            delete children[i];
        }
    }

    bool isRoot() const { return parent == 0; }

    FActorNode getFlags() const { return flags; }

    HActorNode<ActorNode> getHandle() {
        if (!myhandle) {
            myhandle.acquire(this);
        }
        return myhandle;
    }

    [[cppi_decl, set("name")]]
    void setName(const std::string& name) { this->name = name; }
    [[cppi_decl, get("name")]]
    const std::string& getName() const { return name; }
    
    void forEachNode(std::function<void(ActorNode*)> cb) {
        cb(this);
        for (auto& ch : children) {
            ch->forEachNode(cb);
        }
    }

    template<typename NODE_T>
    void forEachNode(std::function<void(NODE_T*)> cb) {
        if (get_type() == rtti::type_get<NODE_T>()) {
            cb((NODE_T*)this);
        }
        for (auto& ch : children) {
            ch->forEachNode<NODE_T>(cb);
        }
    }

    template<typename NODE_T>
    NODE_T* findNode(const char* name) {
        if (get_type() == rtti::type_get<NODE_T>() && getName() == name) {
            return (NODE_T*)this;
        }
        for (auto& ch : children) {
            NODE_T* r = ch->findNode<NODE_T>(name);
            if (r) {
                return r;
            }
        }
        return 0;
    }

    Handle<TransformNode> getTransformHandle() { return transform; }
    void attachTransformTo(ActorNode* parent) {
        transformNodeAttach(parent->transform, transform);
    }
    void restoreTransformParent() {
        if (parent) {
            transformNodeAttach(parent->transform, transform);
        } else {
            transformNodeAttach(Handle<TransformNode>(0), transform);
        }
    }

    void translate(float x, float y, float z) { transform->translate(gfxm::vec3(x, y, z)); }
    void translate(const gfxm::vec3& t) { transform->translate(t); }
    void rotate(float angle, const gfxm::vec3& axis) { transform->rotate(angle, axis); }
    void rotate(const gfxm::quat& q) { transform->rotate(q); }
    void setTranslation(float x, float y, float z) { transform->setTranslation(gfxm::vec3(x, y, z)); }
    [[cppi_decl, set("translation")]]
    void setTranslation(const gfxm::vec3& t) { transform->setTranslation(t); }
    [[cppi_decl, set("rotation")]]
    void setRotation(const gfxm::quat& q) { transform->setRotation(q); }
    void setScale(float x, float y, float z) { setScale(gfxm::vec3(x, y, z)); }
    void setScale(float value) { setScale(gfxm::vec3(value, value, value)); }
    [[cppi_decl, set("scale")]]
    void setScale(const gfxm::vec3& s) { transform->setScale(s); }

    void lookAtDir(const gfxm::vec3& dir) {
        gfxm::mat3 orient(1.f);
        orient[2] = dir;
        orient[1] = gfxm::vec3(0, 1, 0);
        orient[0] = gfxm::normalize(gfxm::cross(orient[1], orient[2]));
        orient[1] = gfxm::normalize(gfxm::cross(orient[2], orient[0]));
        gfxm::quat tgt_rot = gfxm::to_quat(orient);
        setRotation(tgt_rot);
    }

    [[cppi_decl, get("translation")]]
    const gfxm::vec3& getTranslation() const { return transform->getTranslation(); }
    [[cppi_decl, get("rotation")]]
    const gfxm::quat& getRotation() const { return transform->getRotation(); }
    [[cppi_decl, get("scale")]]
    const gfxm::vec3& getScale() const { return transform->getScale(); }

    gfxm::vec3 getWorldTranslation() const { return transform->getWorldTranslation(); }
    gfxm::quat getWorldRotation() const { return transform->getWorldRotation(); }

    gfxm::vec3 getWorldForward() const { return transform->getWorldForward(); }
    gfxm::vec3 getWorldBack() const { return transform->getWorldBack(); }
    gfxm::vec3 getWorldLeft() const { return transform->getWorldLeft(); }
    gfxm::vec3 getWorldRight() const { return transform->getWorldRight(); }
    gfxm::vec3 getWorldUp() const { return transform->getWorldUp(); }
    gfxm::vec3 getWorldDown() const { return transform->getWorldDown(); }

    gfxm::mat4 getLocalTransform() const { return transform->getLocalTransform(); }
    const gfxm::mat4& getWorldTransform() const { return transform->getWorldTransform(); }

    ActorNode* createChild(rtti::type t) {
        ActorNode* child = t.construct_new<ActorNode>();
        assert(child);
        child->parent = this;
        child->actor = this->actor;
        child->flags |= FActorNode::TreeOwned;
        transformNodeAttach(transform, child->transform);
        children.push_back(child);
        child->onDefault();
        requestRebuild();
        return child;
    }
    template<typename CHILD_T>
    CHILD_T* createChild(const char* name) {
        CHILD_T* child = new CHILD_T;
        child->name = name;
        child->parent = this;
        child->actor = this->actor;
        child->flags |= FActorNode::TreeOwned;
        transformNodeAttach(transform, child->transform);
        children.push_back(child);
        child->onDefault();
        requestRebuild();
        return child;
    }

    bool reparentChild(ActorNode* child);
    void removeThis();

    int childCount() const {
        return children.size();
    }
    const ActorNode* getChild(int i) const {
        return children[i];
    }
    ActorNode* getChild(int i) {
        return children[i];
    }

    ActorNode* getParent() { return parent; }

    virtual void onDefault() {}
    virtual void onSpawnActorNode(WorldSystemRegistry& reg) = 0;
    virtual void onDespawnActorNode(WorldSystemRegistry& reg) = 0;
    virtual void onResolveDependencies() {}

    virtual void dbgDraw() const {}
};


[[cppi_tpl]];
template<typename SYSTEM_T>
class TActorNodeHelper {
public:
    virtual ~TActorNodeHelper() {}
    virtual void onSpawnActorNode(SYSTEM_T* sys) = 0;
    virtual void onDespawnActorNode(SYSTEM_T* sys) = 0;
};


[[cppi_tpl]];
template<typename... SYSTEMS_T>
class TActorNode : public ActorNode, public TActorNodeHelper<SYSTEMS_T>... {
    template<typename SYSTEM_T>
    void trySpawnForSystem(WorldSystemRegistry& reg) {
        auto sys = reg.getSystem<SYSTEM_T>();
        if (!sys) {
            LOG_WARN("World system '" << rtti::type_get<SYSTEM_T>().get_name() << "' not found");
            return;
        }
        TActorNodeHelper<SYSTEM_T>* helper = this;
        helper->onSpawnActorNode(sys);
    }
    template<typename SYSTEM_T>
    void tryDespawnForSystem(WorldSystemRegistry& reg) {
        auto sys = reg.getSystem<SYSTEM_T>();
        if (!sys) {
            LOG_WARN("World system '" << rtti::type_get<SYSTEM_T>().get_name() << "' not found");
            return;
        }
        TActorNodeHelper<SYSTEM_T>* helper = this;
        helper->onDespawnActorNode(sys);
    }
    void onSpawnActorNode(WorldSystemRegistry& reg) override {
        (trySpawnForSystem<SYSTEMS_T>(reg), ...);
    }
    void onDespawnActorNode(WorldSystemRegistry& reg) override {
        (tryDespawnForSystem<SYSTEMS_T>(reg), ...);
    }
public:
    TYPE_ENABLE();
};


[[cppi_class]];
class EmptyNode : public ActorNode {
public:
    TYPE_ENABLE();
    void onDefault() override {}
    void onSpawnActorNode(WorldSystemRegistry& reg) override {};
    void onDespawnActorNode(WorldSystemRegistry& reg) override {};
    void dbgDraw() const override {
        dbgDrawText(getWorldTranslation(), getName(), 0xFFFFFFFF);
    }
};

[[cppi_class]];
class DummyReaderNode : public ActorNode {
public:
    TYPE_ENABLE();

    const NodeSlotDescArray& getSlots() override {
        static NodeSlotDescArray slots = {
            NodeSlotDesc{ rtti::type_get<TestDummyLinkData>(), LINK_READ, eSlotUpstream }
        };
        return slots;
    }

    void onDefault() override {}
    void onSpawnActorNode(WorldSystemRegistry& reg) override {};
    void onDespawnActorNode(WorldSystemRegistry& reg) override {};
};

