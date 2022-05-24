#pragma once

#include <algorithm>
#include <vector>

#include "common/render/vertex_format.hpp"
#include "common/render/gpu_buffer.hpp"

class gpuInstancingDesc {
public:
    struct AttribDesc {
        VFMT::GUID guid;
        const gpuBuffer* buffer;
        int stride;
    };

private:
    std::vector<AttribDesc> attribs;
    int instance_count = 0;

    int findAttribDesc(VFMT::GUID guid) const {
        if (attribs.empty()) {
            return -1;
        }
        int begin = 0;
        int end = attribs.size() - 1;
        while (true) {
            int middle = begin + (end - begin) / 2;
            if(guid == attribs[middle].guid) {
                return middle;
            } else if(begin == end) {
                return -1;
            } else if(guid < attribs[middle].guid) {
                end = middle - 1;
            } else if(guid > attribs[middle].guid) {
                begin = middle + 1;
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

    void setInstanceAttribArray(VFMT::GUID attrib_guid, const gpuBuffer* buffer, int stride = 0) {
        int found_idx = findAttribDesc(attrib_guid);
        if(found_idx == -1) {
            AttribDesc desc;
            desc.guid = attrib_guid;
            desc.buffer = buffer;
            desc.stride = stride;
            attribs.push_back(desc);
            std::sort(attribs.begin(), attribs.end(), [](const AttribDesc& a, const AttribDesc& b) -> bool {
                return a.guid < b.guid;
            });
        } else {
            AttribDesc& desc = attribs[found_idx];
            desc.guid = attrib_guid;
            desc.buffer = buffer;
            desc.stride = stride;
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

