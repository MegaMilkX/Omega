#pragma once

#include "math/gfxm.hpp"
#include "gpu/vertex_format.hpp"
#include "gpu/gpu_buffer.hpp"

typedef int HTRANSFORM;
typedef int HMESH;
typedef int HSKIN;
typedef int HMESH_INSTANCE;
typedef int HSKIN_INSTANCE;

// Transform node
HTRANSFORM      transformCreate();
void            transformDestroy(HTRANSFORM t);

void            transformSetTransform(HTRANSFORM t, const gfxm::mat4& m);
void            transformSetTranslation(HTRANSFORM t, const gfxm::vec3& tr);
void            transformSetRotation(HTRANSFORM t, const gfxm::quat& rot);
void            transformSetScale(HTRANSFORM t, const gfxm::vec3& scl);


// Mesh data
HMESH           meshCreate();
void            meshDestroy(HMESH mesh);


// Skin data
HSKIN           skinCreate();
void            skinDestroy(HSKIN skin);

// Mesh instance
HMESH_INSTANCE  meshInstanceCreate(HMESH mesh);
void            meshInstanceDestroy(HMESH_INSTANCE inst);