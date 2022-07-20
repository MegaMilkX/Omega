#include "skeleton_bone.hpp"

#include "skeleton_editable.hpp"


sklBone* sklBone::createChild(const char* name) {
    sklBone* ch = new sklBone(owner, this, name);
    children.push_back(ch);
    
    owner->rebuildBoneArray();
    return ch;
}
bool sklBone::removeChild(sklBone* bone) {
    for (int i = 0; i < children.size(); ++i) {
        if (children[i] == bone) {
            children.erase(children.begin() + i);
            return true;
        }
    }
    owner->rebuildBoneArray();
    return true;
}