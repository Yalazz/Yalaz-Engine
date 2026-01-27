#pragma once

#include <string>

class VulkanEngine;

// Save entire engine state to a JSON file
void saveEngineState(VulkanEngine& engine, const std::string& filepath);

// Load engine state from a JSON file
void loadEngineState(VulkanEngine& engine, const std::string& filepath);

// Reset engine state to defaults
void resetEngineState(VulkanEngine& engine);
