#pragma once

#include "mesh3d.hpp"


void meshGenerateCube(Mesh3d* out, float width = 1.0f, float height = 1.0f, float depth = 1.0f);
void meshGenerateSphereCubic(Mesh3d* out, float radius = 1.0f, int detail = 1);
void meshGenerateCheckerPlane(Mesh3d* out, float width = 10.0f, float depth = 10.0f, int checker_density = 10);
void meshGenerateGrid(Mesh3d* out, float width = 20.f, float depth = 20.f, int density = 20);

void meshGenerateVoxelField(Mesh3d* out, float offset_x, float offset_y, float offset_z);