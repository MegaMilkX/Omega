#pragma once

#include <nlohmann/json.hpp>
#include "animation/animation.hpp"
#include "animation/animation_uaf.hpp"


bool readAnimationJson(nlohmann::json& json, Animation* anim);
bool writeAnimationJson(nlohmann::json& json, Animation* anim);

bool readAnimationBytes(const void* data, size_t sz, Animation* anim);
bool writeAnimationBytes(std::vector<unsigned char>& out, Animation* anim);
