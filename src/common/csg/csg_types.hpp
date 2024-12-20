#pragma once


enum CSG_VOLUME_TYPE {
    CSG_VOLUME_EMPTY,
    CSG_VOLUME_SOLID
};

enum CSG_RELATION_TYPE {
    CSG_RELATION_FRONT,
    CSG_RELATION_OUTSIDE = CSG_RELATION_FRONT,
    CSG_RELATION_BACK,
    CSG_RELATION_INSIDE = CSG_RELATION_BACK,
    CSG_RELATION_ALIGNED,
    CSG_RELATION_REVERSE_ALIGNED,
    CSG_RELATION_SPLIT
};
