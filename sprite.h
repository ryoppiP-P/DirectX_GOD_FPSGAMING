/*********************************************************************
  \file    スプライト互換ヘッダー [sprite.h]
  
  System/Graphics/sprite_2d.h をラップし、旧システムとの互換性を提供する
 *********************************************************************/
#pragma once

#include "System/Graphics/sprite_2d.h"
#include "System/Core/renderer.h"

// 旧システム互換のグローバル関数
inline void InitSprite() {
    Engine::Sprite2D::Initialize(Engine::Renderer::GetInstance().GetDevice());
}

inline void UninitSprite() {
    Engine::Sprite2D::Finalize();
}
