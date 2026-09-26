#include "actor_node.hpp"

#include "world/world.hpp"
#include "world/actor.hpp"


void ActorNode::_registerGraph(ActorDriver* controller) {
    controller->onActorNodeRegister(get_type(), this, name);
    for (auto& c : children) {
        c->_registerGraph(controller);
    }
}
void ActorNode::_unregisterGraph(ActorDriver* controller) {
    for (auto& c : children) {
        c->_unregisterGraph(controller);
    }
    controller->onActorNodeUnregister(get_type(), this, name);
}
void ActorNode::onSpawnNodeInternal(WorldSystemRegistry& reg) {
    if(!has_flags(flags, FActorNode::Incomplete)) {
        onSpawnActorNode(reg);
        flags |= FActorNode::Spawned;
    } else {
        flags &= ~FActorNode::Spawned;
    }

    for (auto& c : children) {
        c->onSpawnNodeInternal(reg);
    }
}
void ActorNode::onDespawnNodeInternal(WorldSystemRegistry& reg) {
    for (auto& c : children) {
        c->onDespawnNodeInternal(reg);
    }

    if(has_flags(flags, FActorNode::Spawned)) {
        onDespawnActorNode(reg);
        flags &= ~FActorNode::Spawned;
    }
}


void ActorNode::_buildLinks(NodeSlotArray& out_slots) {
    NodeLinkArray link_array;
    _buildLinksImpl(link_array, out_slots, 0);

    std::sort(link_array.begin(), link_array.end(), [](const NodeLink& a, const NodeLink& b)->bool {
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

            const auto writer_type = l.writer->get_type();
            const auto reader_type = l.reader->get_type();
            const auto link_type = l.link_type;
            if(l.is_downstream) {
                LOG_DBG(l.order << ": " << writer_type.get_name() << " -> " << link_type.get_name() << " -> " << reader_type.get_name());
            } else {
                LOG_DBG(l.order << ": " << writer_type.get_name() << " <- " << link_type.get_name() << " <- " << reader_type.get_name());
            }

            rtti::varying var;
            l.writer->onLinkWrite(l.writer_slot, var);
            l.reader->onLinkRead(l.reader_slot, var);
        }
    }
}
void ActorNode::_buildLinksImpl(NodeLinkArray& out_links, NodeSlotArray& out_slots, int depth) {
    _resetLinks();

    const NodeSlotDescArray& my_slots = getSlots();
    for (int i = 0; i < children.size(); ++i) {
        ActorNode* ch = children[i];
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

                // Ancestor's downstream slot can only be read or written, never both
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

void ActorNode::requestRebuild() {
    if(!actor) return;
    actor->requestRebuild();
}


bool ActorNode::reparentChild(ActorNode* child) {
    auto old_parent = child->parent;
    if (!old_parent) {
        LOG_ERR("Root node reparenting not supported");
        assert(false);
        return false;
    }
    if (old_parent == this) {
        return false;
    }

    {
        auto cur = this;
        while (cur) {
            if (cur == child) {
                LOG_ERR("Invalid reparenting operation");
                return false;
            }
            cur = cur->parent;
        }
    }


    for (int i = 0; i < old_parent->children.size(); ++i) {
        if (old_parent->children[i] != child) {
            continue;
        }

        old_parent->children.erase(old_parent->children.begin() + i);

        children.push_back(child);
        child->parent = this;
        child->attachTransformTo(this);
        old_parent->requestRebuild();
        requestRebuild();
        return true;
    }

    LOG_ERR("ActorNode::parent ptr doesn't match parent's children array");
    assert(false);
    return false;
}
void ActorNode::removeThis() {
    if (!has_flags(flags, FActorNode::TreeOwned)) {
        return;
    }

    if (parent == nullptr) {
        return;
    }

    for (int i = 0; i < parent->children.size(); ++i) {
        if (parent->children[i] != this) {
            continue;
        }
        
        if(actor->isSpawned()) {
            onDespawnNodeInternal(*actor->getRegistry());
        }

        parent->requestRebuild();
        parent->children.erase(parent->children.begin() + i);
        delete this; // NOTE: DO NOTHING past this point
        break;
    }
}