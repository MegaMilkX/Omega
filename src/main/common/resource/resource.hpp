#pragma once

class gpuTexture2d;
gpuTexture2d* getTexture2d(const char* name);

class gpuShaderProgram;
gpuShaderProgram* getShaderProgram(const char* name);

class gpuMaterial;
gpuMaterial* getRenderMaterial(const char* name);