#pragma once
#include "ModelLoader.h"
//#define TINYOBJLOADER_IMPLEMENTATION
#include "../dependencies/tiny_obj_loader/tiny_obj_loader.h"


glm::vec3 toRGB(int R, int G, int B)
{
    return { R / 255.0f, G / 255.0f, B / 255.0f };
}

glm::vec4 LerpM(const std::vector<glm::vec4> values, float t)
{
    int s = values.size() - 1;
    float m = (s * t);
    int i = (int)m;
    float r = m - (int)m;

    return (values[i] * (1.0f - r)) + (values[i + 1] * r);
}
float LerpM(const std::vector<float> values, float t)
{
    int s = values.size() - 1;
    float m = (s * t);
    int i = (int)m;
    float r = m - (int)m;

    return (values[i] * (1.0f - r)) + (values[i + 1] * r);
}

void LoadModel(const char* filePath,
    std::vector<Triangle>& triangles,
    std::vector<Mesh>& meshes,
    const Material& material,
    glm::vec3 translate,
    glm::vec3 scale)
{
    tinyobj::ObjReaderConfig reader_config;
    reader_config.mtl_search_path = ""; // Path to material files

    tinyobj::ObjReader reader;

    if (!reader.ParseFromFile(filePath, reader_config)) {
        return;
    }

    auto& attrib = reader.GetAttrib();
    auto& shapes = reader.GetShapes();
    auto& materials = reader.GetMaterials();

    glm::vec4 bp1 = glm::vec4(1024, 1024, 1024, 0);
    glm::vec4 bp2 = glm::vec4(-1024, -1024, -1024, 0);

    uint32_t startTriangleIndex = triangles.size();
    uint32_t numTriangles = 0;

    // Loop over shapes
    for (size_t s = 0; s < shapes.size(); s++) {
        // Loop over faces(polygon)
        size_t index_offset = 0;
        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
            size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);
            Triangle tri;

            tinyobj::index_t idx = shapes[s].mesh.indices[index_offset];
            tri.p1.x = attrib.vertices[3 * size_t(idx.vertex_index) + 0] * scale.x + translate.x;
            tri.p1.y = attrib.vertices[3 * size_t(idx.vertex_index) + 1] * scale.y + translate.y;
            tri.p1.z = attrib.vertices[3 * size_t(idx.vertex_index) + 2] * scale.z + translate.z;
            tri.n1.x = attrib.normals[3 * size_t(idx.normal_index) + 0];
            tri.n1.y = attrib.normals[3 * size_t(idx.normal_index) + 1];
            tri.n1.z = attrib.normals[3 * size_t(idx.normal_index) + 2];
            idx = shapes[s].mesh.indices[index_offset + 1];
            tri.p2.x = attrib.vertices[3 * size_t(idx.vertex_index) + 0] * scale.x + translate.x;
            tri.p2.y = attrib.vertices[3 * size_t(idx.vertex_index) + 1] * scale.y + translate.y;
            tri.p2.z = attrib.vertices[3 * size_t(idx.vertex_index) + 2] * scale.z + translate.z;
            tri.n2.x = attrib.normals[3 * size_t(idx.normal_index) + 0];
            tri.n2.y = attrib.normals[3 * size_t(idx.normal_index) + 1];
            tri.n2.z = attrib.normals[3 * size_t(idx.normal_index) + 2];
            idx = shapes[s].mesh.indices[index_offset + 2];
            tri.p3.x = attrib.vertices[3 * size_t(idx.vertex_index) + 0] * scale.x + translate.x;
            tri.p3.y = attrib.vertices[3 * size_t(idx.vertex_index) + 1] * scale.y + translate.y;
            tri.p3.z = attrib.vertices[3 * size_t(idx.vertex_index) + 2] * scale.z + translate.z;
            tri.n3.x = attrib.normals[3 * size_t(idx.normal_index) + 0];
            tri.n3.y = attrib.normals[3 * size_t(idx.normal_index) + 1];
            tri.n3.z = attrib.normals[3 * size_t(idx.normal_index) + 2];
            triangles.push_back(tri);
            index_offset += fv;

            if (tri.p1.x < bp1.x)
                bp1.x = tri.p1.x;
            if (tri.p1.y < bp1.y)
                bp1.y = tri.p1.y;
            if (tri.p1.z < bp1.z)
                bp1.z = tri.p1.z;

            if (tri.p2.x < bp1.x)
                bp1.x = tri.p2.x;
            if (tri.p2.y < bp1.y)
                bp1.y = tri.p2.y;
            if (tri.p2.z < bp1.z)
                bp1.z = tri.p2.z;

            if (tri.p3.x < bp1.x)
                bp1.x = tri.p3.x;
            if (tri.p3.y < bp1.y)
                bp1.y = tri.p3.y;
            if (tri.p3.z < bp1.z)
                bp1.z = tri.p3.z;

            if (tri.p1.x > bp2.x)
                bp2.x = tri.p1.x;
            if (tri.p1.y > bp2.y)
                bp2.y = tri.p1.y;
            if (tri.p1.z > bp2.z)
                bp2.z = tri.p1.z;

            if (tri.p2.x > bp2.x)
                bp2.x = tri.p2.x;
            if (tri.p2.y > bp2.y)
                bp2.y = tri.p2.y;
            if (tri.p2.z > bp2.z)
                bp2.z = tri.p2.z;

            if (tri.p3.x > bp2.x)
                bp2.x = tri.p3.x;
            if (tri.p3.y > bp2.y)
                bp2.y = tri.p3.y;
            if (tri.p3.z > bp2.z)
                bp2.z = tri.p3.z;

            numTriangles++;
        }
    }

    Mesh mesh;
    mesh.startTriangle = startTriangleIndex;
    mesh.numTriangles = numTriangles;
    mesh.boundingPoint1 = bp1;
    mesh.boundingPoint2 = bp2;
    mesh.material = material;
    meshes.push_back(mesh);
}