#pragma once

#include "lucy_math.h"
#include "utils.h"

#define DEMOSTRUCT BoxDemo

// BOX DEMO

struct BoxDemo {
    // shader vars
    Shader shader;

    ID3D11Buffer *box_vb;
    ID3D11Buffer *box_ib;
    u32 index_count = 36;

    XMFLOAT4X4 mat_world;
    XMFLOAT4X4 mat_view;
    XMFLOAT4X4 mat_proj;

    f32 clear_color[4];
    f32 cube_color[4];

    f32 total_time;

    ID3D11RasterizerState *rs_wireframe;
    ID3D11RasterizerState *rs_solid;

    bool enable_wireframe_view;
};

LucyResult demo_init(Arena *arena, RenderContext *rctx, BoxDemo *out_demo_state);

// update and render (runs every frame)
void demo_update_render(RenderContext *rctx, BoxDemo *demo_state, f32 dt);