#pragma once

#include "mesh3d.hpp"


void meshGenerateCube(Mesh3d* out, float width = 1.0f, float height = 1.0f, float depth = 1.0f);
void meshGenerateSphereCubic(Mesh3d* out, float radius = 1.0f, int detail = 1);
void meshGenerateCheckerPlane(Mesh3d* out, float width = 10.0f, float depth = 10.0f, int checker_density = 10);

void meshGenerateVoxelField(Mesh3d* out, float offset_x, float offset_y, float offset_z);