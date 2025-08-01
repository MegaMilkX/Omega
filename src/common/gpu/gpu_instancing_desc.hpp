#pragma once

#include <algorithm>
#include <vector>

#include "gpu/vertex_format.hpp"
#include "gpu/gpu_buffer.hpp"

class gpuInstancingDesc {
public:
    struct AttribDesc {
        VFMT::GUID guid;
        const gpuBuffer* buffer;
        int stride;
        int offset;
    };

private:
    std::vector<AttribDesc> attribs;
    int instance_count = 0;

    int findAttribDesc(VFMT::GUID guid) const {
        if (attribs.empty()) {
            return -1;
        }

        for (int i = 0; i < attribs.size(); ++i) {
            if (attribs[i].guid == guid) {
                return i;
            }
        }
        return -1;
    }

public:
    void setInstanceCount(int count) {
        instance_count = count;
    }
    int getInstanceCount() const {
        return instance_count;
    }

    void setInstanceAttribArray(VFMT::GUID attrib_guid, const gpuBuffer* buffer, int stride = 0, int offset = 0) {
        int found_idx = findAttribDesc(attrib_guid);
        if(found_idx == -1) {
            AttribDesc desc;
            desc.guid = attrib_guid;
            desc.buffer = buffer;
            desc.stride = stride;
            desc.offset = offset;
            attribs.push_back(desc);
            std::sort(attribs.begin(), attribs.end(), [](const AttribDesc& a, const AttribDesc& b) -> bool {
                return a.guid < b.guid;
            });
        } else {
            AttribDesc& desc = attribs[found_idx];
            desc.guid = attrib_guid;
            desc.buffer = buffer;
            desc.stride = stride;
            desc.offset = offset;
        }
    }

    int getLocalInstanceAttribId(VFMT::GUID attrib_guid) const {
        int id = findAttribDesc(attrib_guid);
        return id;
    }
    const AttribDesc& getLocalInstanceAttribDesc(int id) const {
        return attribs[id];
    }
};

