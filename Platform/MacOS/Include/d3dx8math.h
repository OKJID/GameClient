#pragma once
#ifdef __APPLE__
#include <Utility/d3d8_compat.h>

struct D3DXMATRIX : public D3DMATRIX {
  D3DXMATRIX() { memset(m, 0, sizeof(m)); }
  D3DXMATRIX(const D3DMATRIX& rhs) { memcpy(m, rhs.m, sizeof(m)); }
  D3DXMATRIX& operator=(const D3DMATRIX& rhs) { memcpy(m, rhs.m, sizeof(m)); return *this; }
  D3DXMATRIX operator*(const D3DXMATRIX& rhs) const {
    D3DXMATRIX out;
    for (int i = 0; i < 4; ++i)
      for (int j = 0; j < 4; ++j) {
        out.m[i][j] = 0;
        for (int k = 0; k < 4; ++k)
          out.m[i][j] += m[i][k] * rhs.m[k][j];
      }
    return out;
  }
};

struct D3DXVECTOR3 {
  float x, y, z;
  D3DXVECTOR3() : x(0), y(0), z(0) {}
  D3DXVECTOR3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
};

struct D3DXVECTOR4 {
  float x, y, z, w;
  D3DXVECTOR4() : x(0), y(0), z(0), w(0) {}
  float& operator[](int i) { return (&x)[i]; }
  float operator[](int i) const { return (&x)[i]; }
};

inline D3DXVECTOR4* D3DXVec3Transform(D3DXVECTOR4 *pOut, const D3DXVECTOR3 *pV, const D3DXMATRIX *pM) {
  float x = pV->x, y = pV->y, z = pV->z;
  pOut->x = x * pM->m[0][0] + y * pM->m[1][0] + z * pM->m[2][0] + pM->m[3][0];
  pOut->y = x * pM->m[0][1] + y * pM->m[1][1] + z * pM->m[2][1] + pM->m[3][1];
  pOut->z = x * pM->m[0][2] + y * pM->m[1][2] + z * pM->m[2][2] + pM->m[3][2];
  pOut->w = x * pM->m[0][3] + y * pM->m[1][3] + z * pM->m[2][3] + pM->m[3][3];
  return pOut;
}

inline DWORD D3DXGetFVFVertexSize(DWORD FVF) {
  DWORD size = 0;
  if (FVF & 0x002) size += 12;
  if (FVF & 0x004) size += 16;
  if (FVF & 0x010) size += 12;
  if (FVF & 0x040) size += 4;
  if (FVF & 0x080) size += 4;
  DWORD numTex = (FVF >> 8) & 0xF;
  size += numTex * 8;
  return size;
}

#endif
