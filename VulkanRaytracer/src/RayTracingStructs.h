#pragma once
#include <glm/glm.hpp>

struct FrameData
{
    glm::mat4 cameraInverseProjection;
    glm::mat4 cameraInverseView;
    glm::vec4 cameraPos;
    glm::vec4 cameraDirection;
    glm::vec2 window;
    unsigned int raysPerPixel;
    unsigned int maxBouceLimit;
    glm::vec4 skyColorHorizon;
    glm::vec4 skyColorZenith;
    glm::vec4 sunLightDirection;
    glm::vec4 groundColor;
    float sunFocus;
    float sunIntensity;
    unsigned int frameIndex;
    unsigned int sphereNumber;
    unsigned int meshNumber;
};

struct Material
{
    glm::vec3 color;
    float light;
    float smoothness;
    float pad1;
    float pad2;
    float pad3;
};

struct Sphere
{
    glm::vec3 center;
    float radius;
    Material material;
};

struct Mesh
{
    int startTriangle;
    int numTriangles;
    float pad1;
    float pad2;
    glm::vec4 boundingPoint1;
    glm::vec4 boundingPoint2;
    Material material;
};

struct Triangle
{
    glm::vec4 p1;
    glm::vec4 p2;
    glm::vec4 p3;
    glm::vec4 n1;
    glm::vec4 n2;
    glm::vec4 n3;
};