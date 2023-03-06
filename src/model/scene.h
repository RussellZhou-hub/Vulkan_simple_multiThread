#pragma once
#include "../config.h"

class Scene {
public:
    Scene();

    std::vector<glm::vec3> trianglesPositions;

    std::vector<glm::vec3> squarePositions;

	std::vector<glm::vec3> starPositions;
};