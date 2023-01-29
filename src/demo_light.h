#pragma once

#include "lucytypes.h"
#include "proj_types.h"

// lucy's first light demo

// camera orbits one object and there are 3 dir lights and 1 point light.
// u can change all light parameters and the obj model in the gui.

struct LightDemo
{
    // basic shader is in the render ctx so it won't be here.

    // object related stuff (the model that we see)
    ID3D11Buffer *vb;
    ID3D11Buffer *ib;
    u32 obj_index_count;
    u32 obj_vertex_count;
    XMFLOAT4X4 obj_transform;
    Material obj_material;

    // lights
    DirectionalLight dir_lights[3];
    PointLight point_light;

    XMFLOAT4X4 mView;
	XMFLOAT3 mEyePosW;

    // imgui state
    bool do_wireframe_rs;
    f32 clear_color[4];

    f32 light_x_angle[4];
    f32 light_y_angle[4];

    f32 uv_scale;

    f32 point_light_distance;

    i32 how_many_lights;

    bool disable_point_light;
};

LucyResult demo_init(Arena *arena, RenderContext *rctx, LightDemo *out_demo_state);

void demo_resize(RenderContext *rctx, LightDemo *demo_state);

// update and render (runs every frame)
void demo_update_render(RenderContext *rctx, LightDemo *demo_state, f32 dt);