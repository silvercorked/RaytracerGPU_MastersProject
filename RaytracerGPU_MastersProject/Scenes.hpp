#pragma once

#include "utils/PrimitiveTypes.hpp"

#include "VulkanWrapper/RaytraceScene.hpp"

#include <memory>

auto randomSpheres(std::unique_ptr<RaytraceScene>& scene) -> void;

auto cornellMixedScene(std::unique_ptr<RaytraceScene>& scene) -> void;
