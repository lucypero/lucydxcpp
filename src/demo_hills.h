#pragma once

#include "lucy_math.h"
#include "Effects11\d3dx11effect.h"
#include "utils.h"

#define DEMOSTRUCT HillsDemo

struct HillsDemo {
    ID3DX11EffectTechnique *tech;
    ID3DX11EffectMatrixVariable *wvp_mat_var;
    ID3D11InputLayout *input_layout;

    XMFLOAT4X4 mat_world;
    XMFLOAT4X4 mat_view;
    XMFLOAT4X4 mat_proj;

    //hills
    GeometryGenerator::MeshData grid;

    u32 grid_index_count;
    ID3D11Buffer *hills_vb;
    ID3D11Buffer *hills_ib;

    //imgui controls
    f32 height_cofactor;
};

f32 get_height(f32 x, f32 z);

void regen_vertices(const HillsDemo *demo_state, const GeometryGenerator::MeshData *grid, std::vector<ColorVertex> *vertices);

LucyResult demo_init(Arena *arena, RenderContext *rctx, HillsDemo *out_demo_state);

// update and render (runs every frame)
void demo_update_render(RenderContext *rctx, HillsDemo *demo_state);