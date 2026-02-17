#include "pch.h"
#include "mesh_factory.h"
#include <cmath>

namespace Engine {
    std::shared_ptr<Mesh> MeshFactory::CreateBox(float sizeX, float sizeY, float sizeZ) {
        auto pMesh = std::make_shared<Mesh>();

        float hx = sizeX * 0.5f;
        float hy = sizeY * 0.5f;
        float hz = sizeZ * 0.5f;

        std::vector<Vertex3D> vertices = {
            // 前面 (Z = -hz)
            {{ -hx,  hy, -hz }, { 0, 0, -1 }, { 1, 1, 1, 1 }, { 0, 0 }},
            {{  hx,  hy, -hz }, { 0, 0, -1 }, { 1, 1, 1, 1 }, { 1, 0 }},
            {{ -hx, -hy, -hz }, { 0, 0, -1 }, { 1, 1, 1, 1 }, { 0, 1 }},
            {{  hx, -hy, -hz }, { 0, 0, -1 }, { 1, 1, 1, 1 }, { 1, 1 }},
            // 背面 (Z = +hz)
            {{  hx,  hy,  hz }, { 0, 0, 1 }, { 1, 1, 1, 1 }, { 0, 0 }},
            {{ -hx,  hy,  hz }, { 0, 0, 1 }, { 1, 1, 1, 1 }, { 1, 0 }},
            {{  hx, -hy,  hz }, { 0, 0, 1 }, { 1, 1, 1, 1 }, { 0, 1 }},
            {{ -hx, -hy,  hz }, { 0, 0, 1 }, { 1, 1, 1, 1 }, { 1, 1 }},
            // 左面 (X = -hx)
            {{ -hx,  hy,  hz }, { -1, 0, 0 }, { 1, 1, 1, 1 }, { 0, 0 }},
            {{ -hx,  hy, -hz }, { -1, 0, 0 }, { 1, 1, 1, 1 }, { 1, 0 }},
            {{ -hx, -hy,  hz }, { -1, 0, 0 }, { 1, 1, 1, 1 }, { 0, 1 }},
            {{ -hx, -hy, -hz }, { -1, 0, 0 }, { 1, 1, 1, 1 }, { 1, 1 }},
            // 右面 (X = +hx)
            {{  hx,  hy, -hz }, { 1, 0, 0 }, { 1, 1, 1, 1 }, { 0, 0 }},
            {{  hx,  hy,  hz }, { 1, 0, 0 }, { 1, 1, 1, 1 }, { 1, 0 }},
            {{  hx, -hy, -hz }, { 1, 0, 0 }, { 1, 1, 1, 1 }, { 0, 1 }},
            {{  hx, -hy,  hz }, { 1, 0, 0 }, { 1, 1, 1, 1 }, { 1, 1 }},
            // 上面 (Y = +hy)
            {{ -hx,  hy,  hz }, { 0, 1, 0 }, { 1, 1, 1, 1 }, { 0, 0 }},
            {{  hx,  hy,  hz }, { 0, 1, 0 }, { 1, 1, 1, 1 }, { 1, 0 }},
            {{ -hx,  hy, -hz }, { 0, 1, 0 }, { 1, 1, 1, 1 }, { 0, 1 }},
            {{  hx,  hy, -hz }, { 0, 1, 0 }, { 1, 1, 1, 1 }, { 1, 1 }},
            // 下面 (Y = -hy)
            {{ -hx, -hy, -hz }, { 0, -1, 0 }, { 1, 1, 1, 1 }, { 0, 0 }},
            {{  hx, -hy, -hz }, { 0, -1, 0 }, { 1, 1, 1, 1 }, { 1, 0 }},
            {{ -hx, -hy,  hz }, { 0, -1, 0 }, { 1, 1, 1, 1 }, { 0, 1 }},
            {{  hx, -hy,  hz }, { 0, -1, 0 }, { 1, 1, 1, 1 }, { 1, 1 }},
        };

        std::vector<uint32_t> indices = {
            // 前面
            0, 1, 2, 2, 1, 3,
            // 背面
            4, 5, 6, 6, 5, 7,
            // 左面
            8, 9, 10, 10, 9, 11,
            // 右面
            12, 13, 14, 14, 13, 15,
            // 上面
            16, 17, 18, 18, 17, 19,
            // 下面
            20, 21, 22, 22, 21, 23,
        };

        pMesh->SetVertices(std::move(vertices));
        pMesh->SetIndices(std::move(indices));

        return pMesh;
    }

    std::shared_ptr<Mesh> MeshFactory::CreatePlane(float width, float height) {
        auto pMesh = std::make_shared<Mesh>();

        float hw = width * 0.5f;
        float hh = height * 0.5f;

        std::vector<Vertex3D> vertices = {
            {{ -hw, 0,  hh }, { 0, 1, 0 }, { 1, 1, 1, 1 }, { 0, 0 }},
            {{  hw, 0,  hh }, { 0, 1, 0 }, { 1, 1, 1, 1 }, { 1, 0 }},
            {{ -hw, 0, -hh }, { 0, 1, 0 }, { 1, 1, 1, 1 }, { 0, 1 }},
            {{  hw, 0, -hh }, { 0, 1, 0 }, { 1, 1, 1, 1 }, { 1, 1 }},
        };

        std::vector<uint32_t> indices = { 0, 1, 2, 2, 1, 3 };

        pMesh->SetVertices(std::move(vertices));
        pMesh->SetIndices(std::move(indices));

        return pMesh;
    }

    std::shared_ptr<Mesh> MeshFactory::CreateSphere(float radius, uint32_t slices, uint32_t stacks) {
        auto pMesh = std::make_shared<Mesh>();

        std::vector<Vertex3D> vertices;
        std::vector<uint32_t> indices;

        constexpr float PI = 3.14159265358979323846f;

        // 頂点生成
        for (uint32_t stack = 0; stack <= stacks; ++stack) {
            float phi = PI * static_cast<float>(stack) / static_cast<float>(stacks);
            float sinPhi = std::sin(phi);
            float cosPhi = std::cos(phi);

            for (uint32_t slice = 0; slice <= slices; ++slice) {
                float theta = 2.0f * PI * static_cast<float>(slice) / static_cast<float>(slices);
                float sinTheta = std::sin(theta);
                float cosTheta = std::cos(theta);

                Vertex3D v;
                v.normal = { cosTheta * sinPhi, cosPhi, sinTheta * sinPhi };
                v.position = { v.normal.x * radius, v.normal.y * radius, v.normal.z * radius };
                v.texCoord = { static_cast<float>(slice) / slices, static_cast<float>(stack) / stacks };
                v.color = { 1, 1, 1, 1 };

                vertices.push_back(v);
            }
        }

        // インデックス生成
        for (uint32_t stack = 0; stack < stacks; ++stack) {
            for (uint32_t slice = 0; slice < slices; ++slice) {
                uint32_t first = stack * (slices + 1) + slice;
                uint32_t second = first + slices + 1;

                indices.push_back(first);
                indices.push_back(second);
                indices.push_back(first + 1);

                indices.push_back(second);
                indices.push_back(second + 1);
                indices.push_back(first + 1);
            }
        }

        pMesh->SetVertices(std::move(vertices));
        pMesh->SetIndices(std::move(indices));

        return pMesh;
    }

    std::shared_ptr<Mesh> MeshFactory::CreateFromFile(const std::string& filePath) {
        // TODO: OBJ/FBX読み込み実装
        return nullptr;
    }
}
