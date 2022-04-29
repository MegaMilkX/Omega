#ifndef RECT_PACK_BINARY_HPP
#define RECT_PACK_BINARY_HPP

#include <stdint.h>
#include <algorithm>
#include <assert.h>
#include <vector>
#include <set>
#include "common/math/gfxm.hpp"

class RectPack {
public:
    struct Rect {
        uint32_t id = 0;
        float x = 0, y = 0;
        float w = 0, h = 0;
        Rect() {}
        Rect(uint32_t id, float w, float h)
        : id(id), w(w), h(h) {}
        Rect(uint32_t id, float x, float y, float w, float h)
        : id(id), x(x), y(y), w(w), h(h) {}
        Rect(float w, float h)
        : w(w), h(h) {}
        Rect(float x, float y, float w, float h)
        : x(x), y(y), w(w), h(h) {}
        float max_side() const {
            return gfxm::_max(w, h);
        }
        float get_square() const {
            return w*h;
        }
    };

private:
    struct TreeNode {
        float x = 0;
        float y = 0;
        float w = 0;
        float h = 0;
    };

    Rect*                   rects = 0;
    int                     rect_count = 0;
    std::vector<TreeNode>   nodes;
    std::set<int>           leaves;
    int                     padding = 0;

    void split(int node_id, int& a, int& b, bool horizontal, float offset) {
        assert(leaves.count(node_id));

        a = nodes.size();
        b = nodes.size() + 1;
        nodes.push_back(TreeNode{});
        nodes.push_back(TreeNode{});
        leaves.erase(node_id);
        auto& node = nodes[node_id];
        auto& node_a = nodes[a];
        auto& node_b = nodes[b];
        leaves.insert(a);
        leaves.insert(b);

        if(horizontal) {
            node_a = TreeNode{
                node.x, node.y, node.w, offset
            };
            node_b = TreeNode{
                node.x, node.y + offset, node.w, node.h - offset
            };
        } else {
            node_a = TreeNode{
                node.x, node.y, offset, node.h
            };
            node_b = TreeNode{
                node.x + offset, node.y, node.w - offset, node.h
            };
        }
    }
    bool fit(int node_id, int rect_id) {
        auto& node = nodes[node_id];
        auto& rect = rects[rect_id];
        if (node.w == rect.w && node.h == rect.h) {
            rect.x = node.x;
            rect.y = node.y;
            leaves.erase(node_id);
        } else if (node.w >= rect.w && node.h >= rect.h) {
            int a, b;
            if ((node.w - rect.w) > (node.h - rect.h)) {
                split(node_id, a, b, false, rect.w);
            } else {
                split(node_id, a, b, true, rect.h);
            }
            fit(a, rect_id);
        } else {
            assert(false);
            return false;
        }
        return true;
    }
    // Returns new node after expansion
    int expand(int rect_id) {
        auto& root = nodes[0];
        auto& rect = rects[rect_id];
        bool expandDown = root.w > root.h;
        if(expandDown) {
            if(rect.w > root.w) {
                expandDown = false;
            }
        } else {
            if(rect.h > root.h) {
                expandDown = true;
            }
        }
        TreeNode new_node;
        if(expandDown) {
            new_node = TreeNode{ 0, root.h, root.w, rect.h };
            root.h += rect.h;
        } else {
            new_node = TreeNode{ root.w, 0, rect.w, root.h };
            root.w += rect.w;
        }
        int idx = nodes.size(); 
        leaves.insert(idx);
        nodes.push_back(new_node);
        return idx;
    }
    int findLeaf(float w, float h) {
        for(auto& l : leaves) {
            auto& n = nodes[l];
            if(w <= n.w && h <= n.h) {
                return l;
            }
        }
        return -1;
    }
    void insert(int rect_id) {
        int leaf = findLeaf(rects[rect_id].w, rects[rect_id].h);
        if(leaf == -1) {
            leaf = expand(rect_id);
        }
        assert(leaf != -1);
        fit(leaf, rect_id);
    }

public:
    enum SORT {
        MAXSIDE = 0,
        WIDTH,
        HEIGHT,
        SQUARE
    };
    enum FLAGS {
        POWER_OF_TWO = 0x01
    };

    RectPack() {
        nodes.push_back(TreeNode());
        leaves.insert(0);
    }
    
    static bool RectCmpMaxSide(const Rect& l, const Rect& r)
    {return l.max_side()>r.max_side();}
    static bool RectCmpWidth(const Rect& l, const Rect& r)
    {return l.w>r.w;}
    static bool RectCmpHeight(const Rect& l, const Rect& r)
    {return l.h>r.h;}
    static bool RectCmpSquare(const Rect& l, const Rect& r)
    {return l.get_square()>r.get_square();}

    Rect pack(Rect* rects, int rect_count, int padding, SORT sort, int flags ) {
        assert(rects);
        assert(rect_count);
        this->padding = padding;
        this->rects = rects;
        this->rect_count = rect_count;

        std::vector<int> sorted_rect_indices(rect_count);
        for(int i = 0; i < rect_count; ++i) {
            sorted_rect_indices[i] = i;
        }

        if(sort == MAXSIDE) {
            std::sort(sorted_rect_indices.begin(), sorted_rect_indices.end(), [this, rects](const int& l, const int& r){
                return rects[l].max_side() > rects[r].max_side();
            });
        } else if(sort == WIDTH) {
            std::sort(sorted_rect_indices.begin(), sorted_rect_indices.end(), [this, rects](const int& l, const int& r){
                return rects[l].w > rects[r].w;
            });
        } else if(sort == HEIGHT) {
            std::sort(sorted_rect_indices.begin(), sorted_rect_indices.end(), [this, rects](const int& l, const int& r){
                return rects[l].h > rects[r].h;
            });
        } else if(sort == SQUARE) {
            std::sort(sorted_rect_indices.begin(), sorted_rect_indices.end(), [this, rects](const int& l, const int& r){
                return rects[l].get_square() > rects[r].get_square();
            });
        }

        assert(!leaves.empty());
        auto& rootLeaf = *leaves.begin();

        nodes[rootLeaf].w = rects[sorted_rect_indices[0]].w;
        nodes[rootLeaf].h = rects[sorted_rect_indices[0]].h;
        for(int i = 0; i < rect_count; ++i) {
            insert(sorted_rect_indices[i]);
        }

        // POWER OF TWO

        //
        return Rect(nodes[0].x, nodes[0].y, nodes[0].w, nodes[0].h);
    }

};


#endif
