#pragma once

#include <string>


namespace exp {


    class Node {
        std::string name;
        Node* parent = 0;
        Node* first_child = 0;
        Node* next_sibling = 0;
    public:
        Node* createChild(const char* name) {
            auto child = new Node();
            if (first_child == 0) {
                first_child = child;
            } else {
                auto ch = first_child;
                while (ch->next_sibling) {
                    ch = next_sibling;
                }
                ch->next_sibling = child;
            }
            return child;
        }
    };


}