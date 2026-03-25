#include "scene_system.hpp"

#include <algorithm>


void SceneProxy::setTransformNode(HTransform node) {
    if (!sys) {
        transform_node = node;
        return;
    }
    sys->_replaceTransformNode(this, node);
}

void SceneProxy::markDirty() {
    if (!sys) {
        return;
    }
    sys->markDirty(this);
}

SceneSystem::~SceneSystem() {
    while (!proxies.empty()) {
        removeProxy(proxies.back().proxy);
    }
}

void SceneSystem::addProxy(SceneProxy* prox) {
    if (prox->sys) {
        LOG_ERR("SceneProxy already added to a SceneSystem");
        assert(false);
        return;
    }

    {
        auto& item = proxies.emplace_back();
        item.proxy = prox;
        item.proxy->index = proxies.size() - 1;
        item.proxy->sys = this;
    }

    {
        if (prox->transform_node.isValid()) {
            prox->transform_ticket = transform_dirty_list.createTicket(prox);
            prox->transform_node->attachTicket(prox->transform_ticket);
        }
    }

    markDirty(prox);
    if (provider) {
        provider->onAddProxy(&proxies[prox->index]);
    }
}
void SceneSystem::removeProxy(SceneProxy* prox) {
    if (prox->sys != this) {
        LOG_ERR("Trying to remove SceneProxy from the wrong SceneSystem");
        assert(false);
        return;
    }

    if (prox->transform_ticket) {
        prox->transform_node->detachTicket(prox->transform_ticket);
        transform_dirty_list.destroyTicket(prox->transform_ticket);
        prox->transform_ticket = nullptr;
    }

    if (provider) {
        provider->onRemoveProxy(&proxies[prox->index]);
    }

    {
        // Move out of the dirty segment
        if (prox->index < dirty_count) {
            int di = prox->index;
            std::swap(proxies[di], proxies[dirty_count - 1]);
            proxies[di].proxy->index = di;
            prox->index = dirty_count - 1;
            --dirty_count;
        }
        // Now can swap and remove
        int idx = prox->index;
        if (idx != proxies.size() - 1) {
            std::swap(proxies[idx], proxies.back());
            proxies[idx].proxy->index = idx;
        }
        proxies.pop_back();
    }

    /*
    int idx = prox->index;
    proxies.erase(proxies.begin() + idx);
    for (int i = idx; i < proxies.size(); ++i) {
        proxies[i].proxy->index = i;
    }*/
    prox->sys = nullptr;
}

void SceneSystem::markDirty(SceneProxy* prox) {
    if(prox->sys != this) {
        assert(prox->sys == this);
        return;
    }

    if (prox->index < dirty_count) {
        return;
    }
    if (prox->index == dirty_count) {
        ++dirty_count;
        return;
    }

    auto prox_left = proxies[dirty_count].proxy;
    auto prox_right = prox;

    int old_idx = prox->index;

    std::swap(proxies[dirty_count], proxies[old_idx]);

    prox_left->index = old_idx;
    prox_right->index = dirty_count;
    ++dirty_count;
}

void SceneSystem::_replaceTransformNode(SceneProxy* prox, HTransform node) {
    if (prox->transform_ticket) {
        prox->transform_node->detachTicket(prox->transform_ticket);
        if (!node.isValid()) {
            transform_dirty_list.destroyTicket(prox->transform_ticket);
            prox->transform_ticket = nullptr;
            return;
        }
        node->attachTicket(prox->transform_ticket);
    } else {
        if (!node.isValid()) {
            return;
        }
        prox->transform_ticket = transform_dirty_list.createTicket(prox);
        prox->transform_node = node;
        node->attachTicket(prox->transform_ticket);
    }
}

