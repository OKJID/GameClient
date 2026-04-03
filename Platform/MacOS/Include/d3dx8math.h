#pragma once
#ifdef __APPLE__
#include <d3d8.h>
#include <cmath>

#ifndef D3DX_PI
#define D3DX_PI 3.14159265358979323846f
#endif

struct D3DXMATRIX : public D3DMATRIX {
  D3DXMATRIX() { memset(m, 0, sizeof(m)); }
  D3DXMATRIX(const D3DMATRIX& rhs) { memcpy(m, rhs.m, sizeof(m)); }
  D3DXMATRIX(
    float m00, float m01, float m02, float m03,
    float m10, float m11, float m12, float m13,
    float m20, float m21, float m22, float m23,
    float m30, float m31, float m32, float m33)
  {
    m[0][0]=m00; m[0][1]=m01; m[0][2]=m02; m[0][3]=m03;
    m[1][0]=m10; m[1][1]=m11; m[1][2]=m12; m[1][3]=m13;
    m[2][0]=m20; m[2][1]=m21; m[2][2]=m22; m[2][3]=m23;
    m[3][0]=m30; m[3][1]=m31; m[3][2]=m32; m[3][3]=m33;
  }
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
  operator float*() { return &x; }
  operator const float*() const { return &x; }
};

struct D3DXVECTOR4 {
  float x, y, z, w;
  D3DXVECTOR4() : x(0), y(0), z(0), w(0) {}
  D3DXVECTOR4(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {}
  float& operator[](int i) { return (&x)[i]; }
  float operator[](int i) const { return (&x)[i]; }
  operator float*() { return &x; }
  operator const float*() const { return &x; }
};

inline D3DXVECTOR4* D3DXVec3Transform(D3DXVECTOR4 *pOut, const D3DXVECTOR3 *pV, const D3DXMATRIX *pM) {
  float x = pV->x, y = pV->y, z = pV->z;
  pOut->x = x * pM->m[0][0] + y * pM->m[1][0] + z * pM->m[2][0] + pM->m[3][0];
  pOut->y = x * pM->m[0][1] + y * pM->m[1][1] + z * pM->m[2][1] + pM->m[3][1];
  pOut->z = x * pM->m[0][2] + y * pM->m[1][2] + z * pM->m[2][2] + pM->m[3][2];
  pOut->w = x * pM->m[0][3] + y * pM->m[1][3] + z * pM->m[2][3] + pM->m[3][3];
  return pOut;
}

inline D3DXVECTOR4* D3DXVec4Transform(D3DXVECTOR4 *pOut, const D3DXVECTOR4 *pV, const D3DXMATRIX *pM) {
  float x = pV->x, y = pV->y, z = pV->z, w = pV->w;
  pOut->x = x * pM->m[0][0] + y * pM->m[1][0] + z * pM->m[2][0] + w * pM->m[3][0];
  pOut->y = x * pM->m[0][1] + y * pM->m[1][1] + z * pM->m[2][1] + w * pM->m[3][1];
  pOut->z = x * pM->m[0][2] + y * pM->m[1][2] + z * pM->m[2][2] + w * pM->m[3][2];
  pOut->w = x * pM->m[0][3] + y * pM->m[1][3] + z * pM->m[2][3] + w * pM->m[3][3];
  return pOut;
}

inline float D3DXVec4Dot(const D3DXVECTOR4 *pV1, const D3DXVECTOR4 *pV2) {
  return pV1->x * pV2->x + pV1->y * pV2->y + pV1->z * pV2->z + pV1->w * pV2->w;
}

inline D3DXMATRIX* D3DXMatrixRotationZ(D3DXMATRIX *pOut, float angle) {
  float c = cosf(angle);
  float s = sinf(angle);
  memset(pOut->m, 0, sizeof(pOut->m));
  pOut->m[0][0] = c;  pOut->m[0][1] = s;
  pOut->m[1][0] = -s; pOut->m[1][1] = c;
  pOut->m[2][2] = 1.0f;
  pOut->m[3][3] = 1.0f;
  return pOut;
}

inline D3DXMATRIX* D3DXMatrixMultiply(D3DXMATRIX *pOut, const D3DXMATRIX *pM1, const D3DXMATRIX *pM2) {
  D3DXMATRIX result;
  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 4; ++j) {
      result.m[i][j] = 0;
      for (int k = 0; k < 4; ++k)
        result.m[i][j] += pM1->m[i][k] * pM2->m[k][j];
    }
  *pOut = result;
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

inline D3DXMATRIX* D3DXMatrixIdentity(D3DXMATRIX *pOut) {
  memset(pOut->m, 0, sizeof(pOut->m));
  pOut->m[0][0] = pOut->m[1][1] = pOut->m[2][2] = pOut->m[3][3] = 1.0f;
  return pOut;
}
inline D3DXMATRIX* D3DXMatrixScaling(D3DXMATRIX *pOut, float sx, float sy, float sz) {
  memset(pOut->m, 0, sizeof(pOut->m));
  pOut->m[0][0] = sx;
  pOut->m[1][1] = sy;
  pOut->m[2][2] = sz;
  pOut->m[3][3] = 1.0f;
  return pOut;
}
inline D3DXMATRIX* D3DXMatrixTranslation(D3DXMATRIX *pOut, float x, float y, float z) {
  D3DXMatrixIdentity(pOut);
  pOut->m[3][0] = x;
  pOut->m[3][1] = y;
  pOut->m[3][2] = z;
  return pOut;
}
inline D3DXMATRIX* D3DXMatrixTranspose(D3DXMATRIX *pOut, const D3DXMATRIX *pM) {
  if (pOut == pM) {
    D3DXMATRIX temp = *pM;
    for (int i=0; i<4; i++) for (int j=0; j<4; j++) pOut->m[i][j] = temp.m[j][i];
  } else {
    for (int i=0; i<4; i++) for (int j=0; j<4; j++) pOut->m[i][j] = pM->m[j][i];
  }
  return pOut;
}
inline D3DXMATRIX* D3DXMatrixInverse(D3DXMATRIX *pOut, float *pDet, const D3DXMATRIX *pM) {
    float m00 = pM->m[0][0], m01 = pM->m[0][1], m02 = pM->m[0][2], m03 = pM->m[0][3];
    float m10 = pM->m[1][0], m11 = pM->m[1][1], m12 = pM->m[1][2], m13 = pM->m[1][3];
    float m20 = pM->m[2][0], m21 = pM->m[2][1], m22 = pM->m[2][2], m23 = pM->m[2][3];
    float m30 = pM->m[3][0], m31 = pM->m[3][1], m32 = pM->m[3][2], m33 = pM->m[3][3];

    pOut->m[0][0] = m12*m23*m31 - m13*m22*m31 + m13*m21*m32 - m11*m23*m32 - m12*m21*m33 + m11*m22*m33;
    pOut->m[0][1] = m03*m22*m31 - m02*m23*m31 - m03*m21*m32 + m01*m23*m32 + m02*m21*m33 - m01*m22*m33;
    pOut->m[0][2] = m02*m13*m31 - m03*m12*m31 + m03*m11*m32 - m01*m13*m32 - m02*m11*m33 + m01*m12*m33;
    pOut->m[0][3] = m03*m12*m21 - m02*m13*m21 - m03*m11*m22 + m01*m13*m22 + m02*m11*m23 - m01*m12*m23;
    pOut->m[1][0] = m13*m22*m30 - m12*m23*m30 - m13*m20*m32 + m10*m23*m32 + m12*m20*m33 - m10*m22*m33;
    pOut->m[1][1] = m02*m23*m30 - m03*m22*m30 + m03*m20*m32 - m00*m23*m32 - m02*m20*m33 + m00*m22*m33;
    pOut->m[1][2] = m03*m12*m30 - m02*m13*m30 - m03*m10*m32 + m00*m13*m32 + m02*m10*m33 - m00*m12*m33;
    pOut->m[1][3] = m02*m13*m20 - m03*m12*m20 + m03*m10*m22 - m00*m13*m22 - m02*m10*m23 + m00*m12*m23;
    pOut->m[2][0] = m11*m23*m30 - m13*m21*m30 + m13*m20*m31 - m10*m23*m31 - m11*m20*m33 + m10*m21*m33;
    pOut->m[2][1] = m03*m21*m30 - m01*m23*m30 - m03*m20*m31 + m00*m23*m31 + m01*m20*m33 - m00*m21*m33;
    pOut->m[2][2] = m01*m13*m30 - m03*m11*m30 + m03*m10*m31 - m00*m13*m31 - m01*m10*m33 + m00*m11*m33;
    pOut->m[2][3] = m03*m11*m20 - m01*m13*m20 - m03*m10*m21 + m00*m13*m21 + m01*m10*m23 - m00*m11*m23;
    pOut->m[3][0] = m12*m21*m30 - m11*m22*m30 - m12*m20*m31 + m10*m22*m31 + m11*m20*m32 - m10*m21*m32;
    pOut->m[3][1] = m01*m22*m30 - m02*m21*m30 + m02*m20*m31 - m00*m22*m31 - m01*m20*m32 + m00*m21*m32;
    pOut->m[3][2] = m02*m11*m30 - m01*m12*m30 - m02*m10*m31 + m00*m12*m31 + m01*m10*m32 - m00*m11*m32;
    pOut->m[3][3] = m01*m12*m20 - m02*m11*m20 + m02*m10*m21 - m00*m12*m21 - m01*m10*m22 + m00*m11*m22;

    float det = m00*pOut->m[0][0] + m01*pOut->m[1][0] + m02*pOut->m[2][0] + m03*pOut->m[3][0];
    if (pDet) *pDet = det;
    float invDet = 1.0f / det;
    for(int i=0; i<4; i++) for(int j=0; j<4; j++) pOut->m[i][j] *= invDet;
    return pOut;
}

#endif
