/*********************************************************************
  \file    polygon.h

  Box vertex data and polygon init/uninit functions.
  This header is self-contained and does not depend on System/ headers.
 *********************************************************************/
#pragma once

#include "renderer.h"

// Box vertex data (36 vertices)
extern VERTEX_3D Box[36];

// Primitive init/uninit
void InitPolygon();
void UninitPolygon();

// Default texture
ID3D11ShaderResourceView* GetPolygonTexture();
