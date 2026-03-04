#pragma once

#include "gpu/param_block/param_block.hpp"


class gpuCommonBlockManager;
class gpuCommonBlock : public gpuParamBlock {
	friend gpuCommonBlockManager;

	gfxm::mat4 matProjection = gfxm::mat4(1.f);
	gfxm::mat4 matView = gfxm::mat4(1.f);
	gfxm::mat4 matView_prev = gfxm::mat4(1.f);
	gfxm::vec3 cameraPosition = gfxm::vec3(0, 0, 0);
	float time = .0f; 
	gfxm::vec2 viewportSize = gfxm::vec2(1.f, 1.f);
	float zNear = .01f;
	float zFar = 100.f;
	gfxm::vec4 vp_rect_ratio = gfxm::vec4(.0f, .0f, 1.f, 1.f);

	float gamma = 2.2f;
	float exposure = .1f;
public:
	void setProjection(gfxm::mat4 matProjection) {
		this->matProjection = matProjection;
		markDirty();
	}
	void setView(gfxm::mat4 matView) {
		this->matView = matView;
		markDirty();
	}
	void setViewPrev(gfxm::mat4 matView_prev) {
		this->matView_prev = matView_prev;
		markDirty();
	}
	void setCamPos(gfxm::vec3 cameraPosition) {
		this->cameraPosition = cameraPosition;
		markDirty();
	}
	void setTime(float time) {
		this->time = time;
		markDirty();
	}
	void setViewportSize(gfxm::vec2 viewportSize) {
		this->viewportSize = viewportSize;
		this->viewportSize.x = gfxm::_max(1.f, this->viewportSize.x);
		this->viewportSize.y = gfxm::_max(1.f, this->viewportSize.y);
		markDirty();
	}
	void setZNear(float zNear) {
		this->zNear = zNear;
		markDirty();
	}
	void setZFar(float zFar) {
		this->zFar = zFar;
		markDirty();
	}
	void setVpRectRatio(gfxm::vec4 vp_rect_ratio) {
		this->vp_rect_ratio = vp_rect_ratio;
		markDirty();
	}
	void setGamma(float gamma) {
		this->gamma = gamma;
		markDirty();
	}
	void setExposure(float exposure) {
		this->exposure = exposure;
		markDirty();
	}
};

