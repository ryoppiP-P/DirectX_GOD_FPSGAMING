/*********************************************************************
  \file    共通ヘッダー [main.h]
  
  \Author  Ryoto Kikuchi
  \data    2025/9/29
 *********************************************************************/
#pragma once

#pragma warning(push)
#pragma warning(disable:4005)

#define _CRT_SECURE_NO_WARNINGS			// scanf のwarning防止
#include <stdio.h>

// Ensure winsock2 is included before any windows.h to avoid conflicts with winsock.h
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>

#include <d3d11.h>
#include <d3dcompiler.h>

#define DIRECTINPUT_VERSION 0x0800		// 警告対処
#include "dinput.h"
#include "mmsystem.h"

#pragma warning(pop)

#include <DirectXMath.h>
using namespace DirectX;


//*****************************************************************************
// マクロ定義
//*****************************************************************************
static constexpr int SCREEN_WIDTH (1920);			// ウインドウの幅
static constexpr int SCREEN_HEIGHT (1080);				// ウインドウの高さ


//*****************************************************************************
// プロトタイプ宣言
//*****************************************************************************

