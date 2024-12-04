#pragma once

#include "nlohmann/json.hpp"


nlohmann::json jsonLoad(const char* path);
nlohmann::json jsonLoadWithExtensions(const char* path);
nlohmann::json jsonPreprocessExtensions(const nlohmann::json& json);
