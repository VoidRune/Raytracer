#pragma once
#include <glm/glm.hpp>
#include <vector>
#include "RayTracingStructs.h"

void LoadModel(const char* filePath,
    std::vector<Triangle>& triangles,
    std::vector<Mesh>& meshes,
    const Material& material,
    glm::vec3 translate = {0.0, 0.0, 0.0},
    glm::vec3 scale = { 1.0, 1.0, 1.0 });

glm::vec3 toRGB(int R, int G, int B);

glm::vec4 LerpM(const std::vector<glm::vec4> values, float t);
float LerpM(const std::vector<float> values, float t);