#pragma once

/*

    Red-black tree implementation (unfinished)

*/

#include <assert.h>
#include <string>


class cset {
    enum NODE_TYPE { NONE, LEFT, RIGHT };

    struct node_t {
        enum color_t {
            red,
            black
        };

        node_t* parent = 0;
        node_t* left = 0;
        node_t* right = 0;
        color_t color;
        std::string key;

        bool is_leaf() const {
            return left == 0 && right == 0;
        }
    };

    node_t* root = 0;
    
    void rotate_left(node_t*& n) {
        node_t* ch = n->right;
        n->right = ch->left;
        if (n->right != nullptr) {
            n->right->parent = n;
        }
        ch->parent = n->parent;
        if (n->parent == nullptr) {
            root = ch;
        } else if (n == n->parent->left) {
            n->parent->left = ch;
        } else {
            n->parent->right = ch;
        }
        ch->left = n;
        n->parent = ch;
    }

    void rotate_right(node_t*& n) {
        node_t* ch = n->left;
        n->left = ch->right;
        if (n->left != nullptr) {
            n->left->parent = n;
        }
        ch->parent = n->parent;
        if (n->parent == nullptr) {
            root = ch;
        } else if (n == n->parent->left) {
            n->parent->left = ch;
        } else {
            n->parent->right = ch;
        }
        ch->right = n;
        n->parent = ch;
    }

    node_t* fix_insert(node_t* n) {
        n->color = node_t::red;

        if (!n->parent) {
            n->color = node_t::black;
            return n;
        }

        node_t* parent = nullptr;
        node_t* grandparent = nullptr;
        while (n != root && n->color == node_t::red
            && n->parent->color == node_t::red) {
            parent = n->parent;
            grandparent = parent->parent;
            if (parent == grandparent->left) {
                node_t* unc = grandparent->right;
                if (unc != nullptr && unc->color == node_t::red) {
                    grandparent->color = node_t::red;
                    parent->color = node_t::black;
                    unc->color = node_t::black;
                    n = grandparent;
                } else {
                    if (n == parent->right) {
                        rotate_left(parent);
                        n = parent;
                        parent = n->parent;
                    }
                    rotate_right(grandparent);
                    std::swap(parent->color, grandparent->color);
                    n = parent;
                }
            } else {
                node_t* unc = grandparent->left;
                if (unc != nullptr && unc->color == node_t::red) {
                    grandparent->color = node_t::red;
                    parent->color = node_t::black;
                    unc->color = node_t::black;
                    n = grandparent;
                } else {
                    if (n == parent->left) {
                        rotate_right(parent);
                        n = parent;
                        parent = n->parent;
                    }
                    rotate_left(grandparent);
                    std::swap(parent->color, grandparent->color);
                    n = parent;
                }
            }
        }

        root->color = node_t::black;
        return n;
    }

    node_t* fix_erase(node_t* n) {
        while (n != root && n->color == node_t::black) {
            if (n == n->parent->left) {
                node_t* sib = n->parent->right;
                if (sib->color == node_t::red) {
                    sib->color = node_t::black;
                    n->parent->color = node_t::red;
                    rotate_left(n->parent);
                    sib = n->parent->right;
                }
                if ((sib->left == nullptr || sib->left->color == node_t::black)
                    && (sib->right == nullptr || sib->right->color == node_t::black)) {
                    sib->color = node_t::red;
                    n = n->parent;
                } else {
                    if (sib->right == nullptr || sib->right->color == node_t::black) {
                        if (sib->left != nullptr) {
                            sib->left->color = node_t::black;
                        }
                        sib->color = node_t::red;
                        rotate_right(sib);
                        sib = n->parent->right;
                    }
                    sib->color = n->parent->color;
                    n->parent->color = node_t::black;
                    if (sib->right != nullptr) {
                        sib->right->color = node_t::black;
                    }
                    rotate_left(n->parent);
                    n = root;
                }
            } else {
                node_t* sib = n->parent->left;
                if (sib->color == node_t::red) {
                    sib->color = node_t::black;
                    n->parent->color = node_t::red;
                    rotate_right(n->parent);
                    sib = n->parent->left;
                }
                if ((sib->left == nullptr || sib->left->color == node_t::black)
                    && (sib->right == nullptr || sib->right->color
                               == node_t::black)) {
                    sib->color = node_t::red;
                    n = n->parent;
                } else {
                    if (sib->left == nullptr || sib->left->color == node_t::black) {
                        if (sib->right != nullptr) {
                            sib->right->color = node_t::black;
                        }
                        sib->color = node_t::red;
                        rotate_left(sib);
                        sib = n->parent->left;
                    }
                    sib->color = n->parent->color;
                    n->parent->color = node_t::black;
                    if (sib->left != nullptr) {
                        sib->left->color = node_t::black;
                    }
                    rotate_right(n->parent);
                    n = root;
                }
            }
        }
        n->color = node_t::black;
        return n;
    }
    
    node_t* erase_node(node_t* node) {
        if (!node) {
            return nullptr;
        }
        node_t* current = node;

        node_t::color_t orig_color = node->color;

        node_t* out = nullptr;
        if (current->left && current->right) {
            // Both children
            node_t* successor = find_min(current->right);
            node_t* successor_parent = successor->parent;
            orig_color = successor->color;
            current->key = successor->key;
            erase_node(successor);
            out = successor_parent;
        } else if (current->left) {
            // Only left child
            node_t* parent = current->parent;
            out = current->left;
            if (current->parent) {
                if (parent->left == current) {
                    parent->left = current->left;
                    current->left->parent = parent;
                } else if (parent->right == current) {
                    parent->right = current->left;
                    current->left->parent = parent;
                } else {
                    assert(false);
                }
            } else {
                root = current->left;
                root->parent = nullptr;
            }
            delete current;
        } else if (current->right) {
            // Only right child
            node_t* parent = current->parent;
            out = current->right;
            if (parent) {
                if (parent->left == current) {
                    out = current->right;
                    parent->left = current->right;
                    current->right->parent = parent;
                } else if (parent->right == current) {
                    parent->right = current->right;
                    current->right->parent = parent;
                } else {
                    assert(false);
                }
            } else {
                root = current->right;
                root->parent = nullptr;
            }
            delete current;
        } else {
            // No children
            node_t* parent = current->parent;
            if (parent) {
                if (parent->left == current) {
                    parent->left = nullptr;
                } else if (parent->right == current) {
                    parent->right = nullptr;
                } else {
                    assert(false);
                }
                out = parent;
            } else {
                root = nullptr;
                delete current;
                return nullptr;
            }
            delete current;
        }

        if (orig_color == node_t::black) {
            return fix_erase(out);
        }
        return out;
    }

    node_t* find_min(node_t* n) {
        while (n->left) {
            n = n->left;
        }
        return n;
    }

    const node_t* find_min(const node_t* n) const {
        while (n->left) {
            n = n->left;
        }
        return n;
    }

    node_t* find_node(const std::string& key, NODE_TYPE& out_type) {
        node_t* current = root;
        while (current != nullptr) {
            if (key < current->key) {
                current = current->left;
                out_type = LEFT;
            } else if(key > current->key) {
                current = current->right;
                out_type = RIGHT;
            } else {
                break;
            }
        }
        return current;
    }

    int height(const node_t* n) const {
        if(!n) return 0;
        return 1 + std::max(height(n->left), height (n->right));
    }

public:
    struct iterator {
        const node_t* node = 0;

        iterator(const node_t* n)
            : node(n) {}

        const node_t* leftmost(const node_t* node) const {
            while (node->left) {
                node = node->left;
            }
            return node;
        }

        const node_t* inorder_successor(const node_t* node) {
            if (node->right) {
                return leftmost(node->right);
            }
            const node_t* p = node->parent;
            while (p && node == p->right) {
                node = p;
                p = p->parent;
            }
            return p;
        }

        const std::string& operator*() const {
            return node->key;
        }
        iterator& operator++() {
            node = inorder_successor(node);
            return *this;
        }
        bool operator==(const iterator& other) const {
            return node == other.node;
        }
        bool operator!=(const iterator& other) const {
            return node != other.node;
        }
    };

    bool empty() const {
        return !root;
    }

    int balance() const {
        if(!root) return 0;
        return height(root->left) - height(root->right);
    }

    node_t* insert(const std::string& key) {
        if (!root) {
            root = new node_t;
            root->key = key;
            return fix_insert(root);
        }

        node_t* current = root;
        node_t* parent = nullptr;
        while (current != nullptr) {
            parent = current;
            if (key < current->key) {
                current = current->left;
            } else if(key > current->key) {
                current = current->right;
            } else {
                current = nullptr;
            }
        }

        assert(parent);
        node_t* n = 0;
        if (key < parent->key) {
            n = new node_t;
            n->key = key;
            n->parent = parent;
            parent->left = n;
        } else if (key > parent->key) {
            n = new node_t;
            n->key = key;
            n->parent = parent;
            parent->right = n;
        } else {
            n = parent;
            parent->key = key;
        }

        return fix_insert(n);
    }

    void erase(const std::string& key) {
        NODE_TYPE t = NONE;
        node_t* n = find_node(key, t);
        if(!n) return;
        
        erase_node(n);
    }

    iterator begin() const {
        iterator it(find_min(root));
        return it;
    }
    iterator end() const {
        iterator it(nullptr);
        return it;
    }
};

