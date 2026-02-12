/*********************************************************************
  \file    box_collider.h

  AABB BoxCollider struct.
  This header is self-contained and does not depend on System/ headers.
 *********************************************************************/
#pragma once

#include <DirectXMath.h>

using namespace DirectX;

// BoxCollider struct
// min/max fields with fromCenterAndSize/intersects methods
struct BoxCollider {
    XMFLOAT3 min;
    XMFLOAT3 max;

    BoxCollider()
        : min(0.0f, 0.0f, 0.0f)
        , max(0.0f, 0.0f, 0.0f)
    {}

    BoxCollider(const XMFLOAT3& minVal, const XMFLOAT3& maxVal)
        : min(minVal)
        , max(maxVal)
    {}

    // Create BoxCollider from center and size
    static BoxCollider fromCenterAndSize(const XMFLOAT3& center, const XMFLOAT3& size) {
        XMFLOAT3 halfSize(size.x * 0.5f, size.y * 0.5f, size.z * 0.5f);
        return BoxCollider(
            XMFLOAT3(center.x - halfSize.x, center.y - halfSize.y, center.z - halfSize.z),
            XMFLOAT3(center.x + halfSize.x, center.y + halfSize.y, center.z + halfSize.z)
        );
    }

    // AABB intersection test
    bool intersects(const BoxCollider& other) const {
        return (min.x <= other.max.x && max.x >= other.min.x) &&
               (min.y <= other.max.y && max.y >= other.min.y) &&
               (min.z <= other.max.z && max.z >= other.min.z);
    }
};
