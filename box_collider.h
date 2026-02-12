/*********************************************************************
  \file    ボックスコライダー互換ヘッダー [box_collider.h]
  
  旧システムの BoxCollider インターフェースを提供し、
  内部では System/Collision の新システムを使用する
 *********************************************************************/
#pragma once

#include <DirectXMath.h>

using namespace DirectX;

// 旧システム互換の BoxCollider 構造体
// 旧APIの min/max フィールドと fromCenterAndSize/intersects メソッドを提供
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

    // 中心とサイズから BoxCollider を作成
    static BoxCollider fromCenterAndSize(const XMFLOAT3& center, const XMFLOAT3& size) {
        XMFLOAT3 halfSize(size.x * 0.5f, size.y * 0.5f, size.z * 0.5f);
        return BoxCollider(
            XMFLOAT3(center.x - halfSize.x, center.y - halfSize.y, center.z - halfSize.z),
            XMFLOAT3(center.x + halfSize.x, center.y + halfSize.y, center.z + halfSize.z)
        );
    }

    // AABB同士の衝突判定
    bool intersects(const BoxCollider& other) const {
        return (min.x <= other.max.x && max.x >= other.min.x) &&
               (min.y <= other.max.y && max.y >= other.min.y) &&
               (min.z <= other.max.z && max.z >= other.min.z);
    }
};
