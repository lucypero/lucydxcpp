#pragma once

#include "lucy_math.h"
#include "utils.h"

struct ShapesDemo {

    // shader vars
    Shader shader;

    // rest

    ID3D11Buffer *mVB;
    ID3D11Buffer *mIB;


    ID3D11Buffer *obj_vb;
    ID3D11Buffer *obj_ib;
    u32 obj_index_count;

    ID3DX11Effect *mFX;

    ID3D11RasterizerState *mWireframeRS;

    // Define transformations from local spaces to world space.
    XMFLOAT4X4 mSphereWorld[10];
    XMFLOAT4X4 mCylWorld[10];
    XMFLOAT4X4 mBoxWorld;
    XMFLOAT4X4 mGridWorld;
    XMFLOAT4X4 mCenterSphere;

    XMFLOAT4X4 obj_transform;

    XMFLOAT4X4 mView;
    XMFLOAT4X4 mProj;

    int mBoxVertexOffset;
    int mGridVertexOffset;
    int mSphereVertexOffset;
    int mCylinderVertexOffset;

    UINT mBoxIndexOffset;
    UINT mGridIndexOffset;
    UINT mSphereIndexOffset;
    UINT mCylinderIndexOffset;

    UINT mBoxIndexCount;
    UINT mGridIndexCount;
    UINT mSphereIndexCount;
    UINT mCylinderIndexCount;
};

LucyResult demo_init(Arena *arena, RenderContext *rctx, ShapesDemo *out_demo_state);

// update and render (runs every frame)
void demo_update_render(RenderContext *rctx, ShapesDemo *demo_state, f32 dt);