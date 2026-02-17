/*********************************************************************
  \file    pch.h
  \brief   プリコンパイル済みヘッダー
  \author  Ryoto Kikuchi
  \date    2025
 *********************************************************************/
#ifndef PCH_H
#define PCH_H

// Winsock must come first
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

// DirectX
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <wrl/client.h>

// C/C++ Standard
#include <cstdint>
#include <cstring>
#include <cmath>
#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include <functional>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <thread>

#endif // PCH_H
