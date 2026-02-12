#pragma once

#include "mesh.h"
#include <memory>

namespace Engine {
    class MeshFactory {
    public:
        // プリミティブ生成
        static std::shared_ptr<Mesh> CreateBox(float sizeX = 1.0f, float sizeY = 1.0f, float sizeZ = 1.0f);
        static std::shared_ptr<Mesh> CreatePlane(float width = 1.0f, float height = 1.0f);
        static std::shared_ptr<Mesh> CreateSphere(float radius = 0.5f, uint32_t slices = 16, uint32_t stacks = 16);

        // ファイルから読み込み（将来用）
        static std::shared_ptr<Mesh> CreateFromFile(const std::string& filePath);

    private:
        MeshFactory() = delete;
    };
}
