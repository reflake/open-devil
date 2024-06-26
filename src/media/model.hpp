#pragma once

#include <string>
#include <vector>
#include <glm/ext/vector_float2.hpp>

#include <glm/ext/vector_float3.hpp>
#include "image.hpp"

struct Mesh {

    std::string texturePath;
    std::vector<glm::vec3> vertPositions;
    std::vector<glm::vec2> texCoords;
    std::vector<int> indices;
};

Mesh readModel( const std::string& meshPath );