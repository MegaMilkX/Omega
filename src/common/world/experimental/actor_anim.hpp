#pragma once

#include "reflection/reflection.hpp"
#include "world/world.hpp"

using actor_anim_sample_flags_t = uint32_t;

class ActorSampleBuffer {
    struct sample_info {
        int offset;
        int size;
        type type_;
    };
    std::unordered_map<std::string, sample_info> sample_offsets;
    std::vector<uint8_t> buffer;

    std::vector<std::function<void(void)>> appliers;

    template<typename T>
    sample_info add_sample_block(const std::string& name) {
        return add_sample_block(name, type_get<T>());
    }
    sample_info add_sample_block(const std::string& name, type t) {
        int buffer_offset = int(buffer.size());
        sample_info inf = sample_info{ buffer_offset, int(t.get_size()), t };
        sample_offsets[name] = inf;
        buffer.resize(buffer.size() + sizeof(actor_anim_sample_flags_t) + t.get_size());
        return inf;
    }

    void initialize_nodes(const gameActorNode* node, const std::string& name_chain) {
        //std::string node_name = name_chain;
        //node_name += std::string("/") + node->getName();
        std::string node_name = node->getName();
        //LOG_WARN(node_name);

        add_sample_block<gfxm::vec3>(node_name + ".translation");
        add_sample_block<gfxm::quat>(node_name + ".rotation");
        add_sample_block<gfxm::vec3>(node_name + ".scale");

        auto type = node->get_type();
        type.dbg_print();
        for (int i = 0; i < type.prop_count(); ++i) {
            auto prop = type.get_prop(i);
            std::string prop_name = node_name + std::string(".") + prop->name;
            //LOG_WARN(prop_name);

            auto inf = add_sample_block(prop_name, prop->t);
            appliers.push_back([this, node, prop, type, inf]() {
                gameActorNode* n = const_cast<gameActorNode*>(node);
                void* pdata = (void*)(buffer.data() + inf.offset + sizeof(actor_anim_sample_flags_t));
                uint32_t* pflags = (uint32_t*)(buffer.data() + inf.offset);
                if (*pflags != 0) {
                    prop->setValue(n, pdata);
                }
            });
        }

        for (int i = 0; i < node->childCount(); ++i) {
            initialize_nodes(node->getChild(i), node_name);
        }
    }
public:
    void initialize(Actor* actor) {
        sample_offsets.clear();

        auto type = actor->get_type();
        type.dbg_print();
        for (int i = 0; i < type.prop_count(); ++i) {
            auto prop = type.get_prop(i);
            std::string prop_name = std::string(".") + prop->name;
            //LOG_WARN(prop_name);

            auto inf = add_sample_block(prop_name, prop->t);
            appliers.push_back([this, actor, prop, type, inf]() {
                void* pdata = (void*)(buffer.data() + inf.offset + sizeof(actor_anim_sample_flags_t));
                uint32_t* pflags = (uint32_t*)(buffer.data() + inf.offset);
                if (*pflags != 0) {
                    prop->setValue(actor, pdata);
                }
            });
        }

        if (actor->getRoot()) {
            initialize_nodes(actor->getRoot(), "");
        }

        dbg_print();
        LOG_DBG("ActorSampleBuffer size: " << buffer.size());
    }

    void clear_flags() {
        for (auto& kv : sample_offsets) {
            uint32_t* pflags = (uint32_t*)(buffer.data() + kv.second.offset);
            *pflags = 0;
        }
    }

    void apply() {
        for (auto& a : appliers) {
            a();
        }
    }

    template<typename T>
    void setValue(const std::string& name, const T& value) {
        auto sample_info = getSampleInfo(name);
        if (sample_info == nullptr) {
            return;
        }
        if (sizeof(value) != sample_info->size) {
            assert(false);
            return;
        }
        uint32_t* pflags = (uint32_t*)(buffer.data() + sample_info->offset);
        auto pdata = (T*)(buffer.data() + sizeof(actor_anim_sample_flags_t) + sample_info->offset);
        *pdata = value;
    }

    uint8_t* getBuffer() { return buffer.data(); }
    const sample_info* getSampleInfo(const std::string& name) const {
        auto it = sample_offsets.find(name);
        if (it == sample_offsets.end()) {
            return 0;
        }
        return &it->second;
    }
    int getBufferOffset(const std::string& name) const {
        auto it = sample_offsets.find(name);
        if (it == sample_offsets.end()) {
            return -1;
        }
        return it->second.offset;
    }

    void dbg_print() const {
        for (auto& kv : sample_offsets) {
            LOG_DBG(kv.first << ": " << kv.second.type_.get_name() << ", " << kv.second.offset);
        }
    }
};

class ActorAnimNode {
public:
    virtual ~ActorAnimNode() {}
    virtual type get_type() const {
        return type(0);
    }
    virtual void sampleAt(float cur, void* destination) = 0;
};
template<typename T>
class ActorAnimNodeT : public ActorAnimNode {
public:
    type get_type() const override { return type_get<T>(); }
    curve<T> curve_;

    void sampleAt(float cur, void* destination) override {
        *((T*)destination) = curve_.at(cur);
    }
};
class ActorAnimation {
public:
    float fps = 60.f;
    float length = 100.f;
    std::map<std::string, std::unique_ptr<ActorAnimNode>> nodes;

    ActorAnimNodeT<float>* createFloatNode(const std::string& name) {
        auto ptr = new ActorAnimNodeT<float>;
        nodes[name] = std::unique_ptr<ActorAnimNode>(ptr);
        return ptr;
    }
    ActorAnimNodeT<gfxm::vec2>* createVec2Node(const std::string& name) {
        auto ptr = new ActorAnimNodeT<gfxm::vec2>;
        nodes[name] = std::unique_ptr<ActorAnimNode>(ptr);
        return ptr;
    }
    ActorAnimNodeT<gfxm::vec3>* createVec3Node(const std::string& name) {
        auto ptr = new ActorAnimNodeT<gfxm::vec3>;
        nodes[name] = std::unique_ptr<ActorAnimNode>(ptr);
        return ptr;
    }
    ActorAnimNodeT<gfxm::vec4>* createVec4Node(const std::string& name) {
        auto ptr = new ActorAnimNodeT<gfxm::vec4>;
        nodes[name] = std::unique_ptr<ActorAnimNode>(ptr);
        return ptr;
    }
};

class ActorAnimSampler {
    struct NodeToPose {
        ActorAnimNode* node;
        int pose_buffer_offset;
    };
    std::vector<NodeToPose> mapping;
    ActorAnimation* animation = 0;
    ActorSampleBuffer* pose = 0;
public:
    void init(ActorAnimation* anim, ActorSampleBuffer* pose) {
        mapping.clear();

        animation = anim;
        this->pose = pose;
        for (auto& kv : anim->nodes) {
            auto& name = kv.first;
            const auto sample_info = pose->getSampleInfo(name);
            if (sample_info == nullptr) {
                continue;
            }
            if (sample_info->type_ != kv.second->get_type()) {
                continue;
            }
            
            mapping.push_back(NodeToPose{ kv.second.get(), sample_info->offset });
        }
    }
    void sampleAt(float cur) {
        for (auto& m : mapping) {
            uint32_t* pflags = (uint32_t*)(pose->getBuffer() + m.pose_buffer_offset);
            void* pdata = (void*)(pose->getBuffer() + sizeof(actor_anim_sample_flags_t) + m.pose_buffer_offset);
            m.node->sampleAt(cur, pdata);
            *pflags = 1;
        }
    }
};
