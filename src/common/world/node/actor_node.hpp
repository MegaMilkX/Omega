#pragma once

#include "actor_node.auto.hpp"

#include <string>
#include "reflection/reflection.hpp"
#include "math/gfxm.hpp"
#include "world/controller/actor_controller.hpp"
#include "world/world_system_registry.hpp"
#include "util/static_block.hpp"
#include "transform_node/transform_node.hpp"
#include "debug_draw/debug_draw.hpp"

#include "world/common_systems/dirty_system.hpp"

struct TestDummyLinkData {};

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
/*
class NodeLink {
public:
    type link_type;
    node_link_flags_t flags;
};

template<typename T, node_link_flags_t FLAGS>
class TNodeLink {
    static type getType() { return type_get<T>(); }
    static node_link_flags_t getFlags() { return FLAGS; };
};

class NodeLinkRegistry {
    std::vector<NodeLink> links;
protected:
    template<typename T>
    void registerLink(node_link_flags_t flags) {
        links.push_back(NodeLink{ T::getType(), T::getFlags() });
    }
public:
    virtual ~NodeLinkRegistry() {}
    int count() const {
        return links.size();
    }
    const NodeLink& getLink(int i) const {
        return links[i];
    }
};

template<typename... ARGS>
class TNodeLinkRegistry {
public:
    TNodeLinkRegistry() {
        { registerLink<ARGS>()... };
    }
};*/

struct NodeSlotDesc {
    type link_type;
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
    type link_type; // used for debug
    bool is_downstream;
    int priority = 0;
};
using NodeLinkArray = std::vector<NodeLink>;

/*
class NodeLinkView {
    void* data = nullptr;
    type link_type;
public:
    template<typename T>
    static NodeLinkView make(T* data) {
        NodeLinkView view;
        view.link_type = type_get<T>();
        view.data = data;
        return view;
    }

    type getType() const { return link_type; }
    template<typename T>
    T* get() {
        if (type_get<T>() != link_type) {
            return nullptr;
        }
        return static_cast<T*>(data);
    }
    template<typename T>
    const T* get() const {
        if (type_get<T>() != link_type) {
            return nullptr;
        }
        return static_cast<T*>(data);
    }
};*/


class RuntimeWorld;
class Actor;
[[cppi_class]];
class ActorNode : public MetaObject {
public:
    TYPE_ENABLE();
private:
    friend RuntimeWorld;
    friend Actor;

    std::string name;

    Handle<TransformNode> transform;
    ActorNode* parent = 0;
    std::vector<std::unique_ptr<ActorNode>> children;

    void _registerGraph(ActorDriver* controller) {
        controller->onActorNodeRegister(get_type(), this, name);
        for (auto& c : children) {
            c->_registerGraph(controller);
        }
    }
    void _unregisterGraph(ActorDriver* controller) {
        for (auto& c : children) {
            c->_unregisterGraph(controller);
        }
        controller->onActorNodeUnregister(get_type(), this, name);
    }
    void onSpawnNodeInternal(WorldSystemRegistry& reg) {
        onSpawnActorNode(reg);
        for (auto& c : children) {
            c->onSpawnNodeInternal(reg);
        }
    }
    void onDespawnNodeInternal(WorldSystemRegistry& reg) {
        for (auto& c : children) {
            c->onDespawnNodeInternal(reg);
        }
        onDespawnActorNode(reg);
    }

    void _decay(RuntimeWorld* world) {
        onDecay(world);
        for (auto& c : children) {
            c->_decay(world);
        }
    }
    void _updateDecay(RuntimeWorld* world, float dt) {
        onUpdateDecay(world, dt);
        for (auto& c : children) {
            c->_updateDecay(world, dt);
        }
    }
    bool hasDecayed_actor() const { 
        if (!hasDecayed()) {
            return false;
        }
        for (auto& c : children) {
            if (!c->hasDecayed_actor()) {
                return false;
            }
        }
        return true;
    }
protected:
    void _buildLinks(NodeSlotArray& out_slots) {
        NodeLinkArray link_array;
        _buildLinksImpl(link_array, out_slots, 0);

        std::sort(link_array.begin(), link_array.end(), [](const NodeLink& a, const NodeLink& b)->bool {
            /*
            if (a.order == b.order) {
                return a.priority < b.priority;
            }*/
            if (a.is_downstream && b.is_downstream) {
                return a.order < b.order;
            } else {
                return a.order > b.order;
            }

        });

        if(!link_array.empty()) {
            LOG_ERR("Link resolution:");
            for (int i = 0; i < link_array.size(); ++i) {
                NodeLink& l = link_array[i];
            
                if (auto d = dynamic_cast<IDirty*>(l.writer)) {
                    d->resolveDirty();
                }

                LOG_DBG(l.order << ": " << l.writer->get_type().get_name() << " -> " << l.link_type.get_name() << " -> " << l.reader->get_type().get_name());
            
                varying var;
                l.writer->onLinkWrite(l.writer_slot, var);
                l.reader->onLinkRead(l.reader_slot, var);
            }
        }
    }
    void _buildLinksImpl(NodeLinkArray& out_links, NodeSlotArray& out_slots, int depth) {
        _resetLinks();

        const NodeSlotDescArray& my_slots = getSlots();
        for (int i = 0; i < children.size(); ++i) {
            auto ch = children[i].get();
            ch->_buildLinksImpl(out_links, out_slots, depth + 1);
            
            for (int j = 0; j < my_slots.size(); ++j) {
                const NodeSlotDesc& aslot = my_slots[j];
            
                if (aslot.kind == eSlotUpstream) {
                    continue;
                }

                for (int k = 0; k < out_slots.size(); ++k) {
                    const NodeSlot& dslot = out_slots[k];
                    if (aslot.link_type != dslot.desc.link_type) {
                        continue;
                    }

                    if ((aslot.flags & LINK_READWRITE) == 0) {
                        LOG_ERR("Ancestor advertised a slot that does not read or write");
                        assert(false);
                        continue;
                    }
                    if ((dslot.desc.flags & LINK_READWRITE) == 0) {
                        LOG_ERR("Descendant advertised a slot that does not read or write");
                        assert(false);
                        continue;
                    }
                    if ((aslot.flags & LINK_READWRITE) == LINK_READWRITE) {
                        LOG_ERR("Downward slots with both READ and WRITE capabilities are forbidden");
                        assert(false);
                        continue;
                    }

                    ActorNode* writer = nullptr;
                    ActorNode* reader = nullptr;
                    int writer_slot = 0;
                    int reader_slot = 0;

                    // Ancestor's downstream slot can only be read or write, never both
                    bool is_downstream_flow = true;
                    if ((aslot.flags & LINK_WRITE) && (dslot.desc.flags & LINK_READ)) {
                        writer = this;
                        reader = dslot.node;
                        writer_slot = j;
                        reader_slot = dslot.slot_idx;
                    }
                
                    if((aslot.flags & LINK_READ) && (dslot.desc.flags & LINK_WRITE)) {
                        writer = dslot.node;
                        reader = this;
                        writer_slot = dslot.slot_idx;
                        reader_slot = j;
                        is_downstream_flow = false;
                    }

                    if (!writer) {
                        // Not a compatible combination
                        continue;
                    }

                    out_links.push_back(NodeLink{
                        writer, reader, writer_slot, reader_slot,
                        depth, aslot.link_type, is_downstream_flow,
                        std::min(writer_slot, reader_slot)
                    });

                    out_slots.erase(out_slots.begin() + k);
                    --k;
                }
            }
        }

        // Store my upward slots
        for (int i = 0; i < my_slots.size(); ++i) {
            const NodeSlotDesc& slot = my_slots[i];
            if (slot.kind != eSlotUpstream) {
                continue;
            }
            out_slots.push_back(NodeSlot{ this, slot, i });
        }
    }
    template<typename T>
    T* findNearestAncestor() {
        static_assert(std::is_base_of_v<ActorNode, T>, "T must be an ActorNode");
        if (!parent) {
            return nullptr;
        }
        type pt = parent->get_type();
        if (pt == type_get<T>()) {
            return static_cast<T*>(parent);
        }
        return parent->findNearestAncestor<T>();
    }

    virtual const NodeSlotDescArray& getSlots() {
        static NodeSlotDescArray slots = {};
        return slots;
    }

    void _resetLinks() {
        onLinksReset();
    }
    virtual void onLinksReset() {}
    virtual void onLinkWrite(int slot, varying& out) {
        //LOG_DBG(get_type().get_name() << " writes slot " << slot);
    }
    virtual void onLinkRead(int slot, const varying& in) {
        //LOG_DBG(get_type().get_name() << " reads slot " << slot << ": " << in.get_type().get_name());
    }
public:
    ActorNode() {
        transform.acquire();
    }
    ActorNode(const ActorNode&) = delete;
    ActorNode& operator=(const ActorNode&) = delete;
    virtual ~ActorNode() {
        transform.release();
    }

    bool isRoot() const { return parent == 0; }

    [[cppi_decl, set("name")]]
    void setName(const std::string& name) { this->name = name; }
    [[cppi_decl, get("name")]]
    const std::string& getName() const { return name; }

    template<typename NODE_T>
    void forEachNode(std::function<void(NODE_T*)> cb) {
        if (get_type() == type_get<NODE_T>()) {
            cb((NODE_T*)this);
        }
        for (auto& ch : children) {
            ch->forEachNode<NODE_T>(cb);
        }
    }

    template<typename NODE_T>
    NODE_T* findNode(const char* name) {
        if (get_type() == type_get<NODE_T>() && getName() == name) {
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
    [[cppi_decl, set("scale")]]
    void setScale(const gfxm::vec3& s) { transform->setScale(s); }

    void lookAtDir(const gfxm::vec3& dir) {
        gfxm::mat3 orient(1.f);
        orient[2] = dir;
        orient[1] = gfxm::vec3(0, 1, 0);
        orient[0] = gfxm::cross(orient[1], orient[2]);
        orient[1] = gfxm::cross(orient[0], orient[2]);
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

    ActorNode* createChild(type t) {
        ActorNode* child = t.construct_new<ActorNode>();
        assert(child);
        child->parent = this;
        transformNodeAttach(transform, child->transform);
        children.push_back(std::unique_ptr<ActorNode>(child));
        child->onDefault();
        return child;
    }
    template<typename CHILD_T>
    CHILD_T* createChild(const char* name) {
        CHILD_T* child = new CHILD_T;
        child->name = name;
        child->parent = this;
        transformNodeAttach(transform, child->transform);
        children.push_back(std::unique_ptr<ActorNode>(child));
        child->onDefault();
        return child;
    }

    int childCount() const {
        return children.size();
    }
    const ActorNode* getChild(int i) const {
        return children[i].get();
    }
    ActorNode* getChild(int i) {
        return children[i].get();
    }

    virtual void onDefault() {}
    virtual void onSpawnActorNode(WorldSystemRegistry& reg) = 0;
    virtual void onDespawnActorNode(WorldSystemRegistry& reg) = 0;
    virtual void onResolveDependencies() {}
    virtual void onDecay(RuntimeWorld* world) {}
    virtual void onUpdateDecay(RuntimeWorld* world, float dt) {}
    virtual bool hasDecayed() const { return true; }

    virtual void dbgDraw() const {}

    [[cppi_decl, serialize_json]]
    virtual void toJson(nlohmann::json& j) {
        type_write_json(j["name"], name);
        type_write_json(j["translation"], transform->getTranslation());
        type_write_json(j["rotation"], transform->getRotation());
        type t = type_get<decltype(children)>();
        t.serialize_json(j["children"], &children);
    }
    [[cppi_decl, deserialize_json]]
    virtual bool fromJson(const nlohmann::json& j) {
        type_read_json(j["name"], name);
        gfxm::vec3 translation;
        gfxm::quat rotation;
        type_read_json(j["translation"], translation);
        type_read_json(j["rotation"], rotation);
        transform->setTranslation(translation);
        transform->setRotation(rotation);

        using namespace nlohmann;
        const json& jchildren = j["children"];
        for (const json& jchild : jchildren) {
            std::unique_ptr<ActorNode> uptr;
            type_read_json(jchild, uptr);
            if (!uptr) {
                LOG_ERR("Failed to read child node json");
                assert(false);
                continue;
            }
            /*
            gameActorNode* node = type_new_from_json<gameActorNode>(jchild);
            if (!node) {
                LOG_ERR("Failed to read child node json");
                assert(false);
                continue;
            }*/
            uptr->parent = this;
            transformNodeAttach(transform, uptr->transform);
            children.push_back(std::move(uptr));
        }

        return true;
    }
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
            LOG_WARN("World system '" << type_get<SYSTEM_T>().get_name() << "' not found");
            return;
        }
        TActorNodeHelper<SYSTEM_T>* helper = this;
        helper->onSpawnActorNode(sys);
    }
    template<typename SYSTEM_T>
    void tryDespawnForSystem(WorldSystemRegistry& reg) {
        auto sys = reg.getSystem<SYSTEM_T>();
        if (!sys) {
            LOG_WARN("World system '" << type_get<SYSTEM_T>().get_name() << "' not found");
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
            NodeSlotDesc{ type_get<TestDummyLinkData>(), LINK_READ, eSlotUpstream }
        };
        return slots;
    }

    void onDefault() override {}
    void onSpawnActorNode(WorldSystemRegistry& reg) override {};
    void onDespawnActorNode(WorldSystemRegistry& reg) override {};
};

