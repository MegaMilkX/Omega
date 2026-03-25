#include "transform_dirty_list.hpp"

#include "transform_node.hpp"

void TransformTicket::markDirty() {
    list->markDirty(this);
}

TransformTicket* TransformDirtyList::createTicket(void* user_ptr) {
    TransformTicket* t = new TransformTicket;
    t->index = list.size();
    t->list = this;
    t->user_ptr = user_ptr;
    list.push_back(t);
    markDirty(t);
    return t;
}
void TransformDirtyList::destroyTicket(TransformTicket* t) {
    if (t->list != this) {
        assert(false);
        return;
    }

    // Move out of the dirty segment
    if (t->index < dirty_count) {
        int di = t->index;
        std::swap(list[di], list[dirty_count - 1]);
        list[di]->index = di;
        t->index = dirty_count - 1;
        --dirty_count;
    }
    // Now can swap and remove
    int idx = t->index;
    if (idx != list.size() - 1) {
        std::swap(list[idx], list.back());
        list[idx]->index = idx;
    }
    list.pop_back();
    delete t;
}

void TransformDirtyList::markDirty(TransformTicket* t) {
    if(t->list != this) {
        assert(false);
        return;
    }
    if (t->index < dirty_count) {
        return;
    }
    if (t->index == dirty_count) {
        ++dirty_count;
        return;
    }
    auto t_left = list[dirty_count];
    auto t_right = t;
    int old_idx = t->index;
    std::swap(list[dirty_count], list[old_idx]);
    t_left->index = old_idx;
    t_right->index = dirty_count;
    ++dirty_count;
}

