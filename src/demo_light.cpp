#include "demo_light.h"

#include "obj_loader.h"
#include "imgui\imgui.h"

// lucy's first light demo

LucyResult demo_init(Arena *arena, RenderContext *rctx, LightDemo *out_demo_state) {

    rctx->cam_radius = 7.0f;

    out_demo_state->point_light_distance = 20.0f;
    out_demo_state->uv_scale = 1.0f;

    out_demo_state->how_many_lights = 1;

    out_demo_state->disable_point_light = false;

    // initializing imgui stuff
    f32 clear_color[4] = { 0.4f, 0.4f, 0.4f, 1.0f };
    memcpy(out_demo_state->clear_color, clear_color, sizeof(clear_color));

    // loading obj
    RenderObjects objs = {};
    // LucyResult res = load_obj(arena, "Models\\skull\\skull.obj", &the_obj);
    // LucyResult res = load_obj(arena, "Models\\plane.obj", &the_obj);
    LR(load_obj_for_rendering(arena, rctx->device, "Models\\mill\\my_mill.obj", &objs));
    // LucyResult res = load_obj(arena, "Models\\cube.obj", &the_obj);

    out_demo_state->render_objs = objs;

    // initializing lights lights

    DirectionalLight dir_light = {
        .Ambient = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f),
        .Diffuse = XMFLOAT4(0.2f, 0.2f, 0.2f, 0.2f),
        .Specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 0.5f),
        .Direction = XMFLOAT3(1.0f, 0.0f, 0.0f),
        .Pad = 0.0f,
   };

    out_demo_state->dir_lights[0] = dir_light;

    dir_light.Direction.x = -1.0f;

    out_demo_state->dir_lights[1] = dir_light;
    out_demo_state->dir_lights[2] = dir_light;

    out_demo_state->point_light = {
        .Ambient = XMFLOAT4(0.1f, 0.1f, 0.1f, 0.1f),
        .Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
        .Specular = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
        // what is
        .Position = XMFLOAT3(1.5f, 1.5f, 1.5f),
        .Range = 100.0f,

        .Att = XMFLOAT3(1.0f, 1.0f, 1.0f),
        .Pad = 0.0f,
    };

    out_demo_state->mView = {};
    out_demo_state->mEyePosW = {};

    // initting texture stuff


    return LRES_OK;
}

// update and render (runs every frame)
void demo_update_render(RenderContext *rctx, LightDemo *demo_state, f32 dt) {

    demo_state->total_time += dt;

    //imgui stuff here
    ImGui::Checkbox("wireframe mode", &demo_state->do_wireframe_rs);
    ImGui::ColorEdit4("clear color", demo_state->clear_color);
    ImGui::DragInt("how many lights", &demo_state->how_many_lights, 0.05f, 1, 3);

    for(i32 i = 0; i< demo_state->how_many_lights; ++i) {

        std::string tree_label{"dir light "};
        tree_label += std::to_string(i);

        std::string number{"dl"};
        number += std::to_string(i);
        number += " ";

        if(ImGui::TreeNode(tree_label.c_str())) {
            std::string tmp;
            tmp = number + "ambient";
            imgui_help::float4_edit(tmp.c_str(), &demo_state->dir_lights[i].Ambient);
            tmp = number + "diffuse";
            imgui_help::float4_edit(tmp.c_str(), &demo_state->dir_lights[i].Diffuse);
            tmp = number + "specular";
            imgui_help::float4_edit(tmp.c_str(), &demo_state->dir_lights[i].Specular);

            // direction
            tmp = number + "light x angle (rad * pi)";
            ImGui::DragFloat(tmp.c_str(), &demo_state->light_x_angle[i], 0.01f, -2.0f, 2.0f);
            tmp = number + "light y angle (rad * pi)";
            ImGui::DragFloat(tmp.c_str(), &demo_state->light_y_angle[i], 0.01f, -2.0f, 2.0f);
            XMMATRIX light_rot = XMMatrixRotationQuaternion(XMQuaternionRotationRollPitchYaw(demo_state->light_y_angle[i] * math::PI, demo_state->light_x_angle[i] * math::PI, 0.0f));
            f32 light_distance = -1.0f;
            XMVECTOR light_pos = XMVectorSet(0.0f, 0.0f, light_distance, 1.0f);
            light_pos = XMVector3Transform(light_pos, light_rot);
            XMStoreFloat3(&demo_state->dir_lights[i].Direction, light_pos);

            ImGui::TreePop();
        }
    }

    if(ImGui::TreeNode("point light")){
        ImGui::Checkbox("disable point light", &demo_state->disable_point_light);
        imgui_help::float4_edit("p ambient", &demo_state->point_light.Ambient);
        imgui_help::float4_edit("p diffuse", &demo_state->point_light.Diffuse);
        imgui_help::float4_edit("p specular", &demo_state->point_light.Specular);

        ImGui::DragFloat("p x angle", &demo_state->light_x_angle[3], 0.01f, -2.0f, 2.0f);
        ImGui::DragFloat("p y angle", &demo_state->light_y_angle[3], 0.01f, -2.0f, 2.0f);
        ImGui::DragFloat("p distance", &demo_state->point_light_distance, 0.02f, 2.0f, 100.0f);

        // imgui_help::float3_edit("p position", &demo_state->point_light.Position);
        XMMATRIX light_rot = XMMatrixRotationQuaternion(XMQuaternionRotationRollPitchYaw(demo_state->light_y_angle[3] * math::PI, demo_state->light_x_angle[3] * math::PI, 0.0f));
        f32 light_distance = -demo_state->point_light_distance;
        XMVECTOR light_pos = XMVectorSet(0.0f, 0.0f, light_distance, 1.0f);
        light_pos = XMVector3Transform(light_pos, light_rot);
        XMStoreFloat3(&demo_state->point_light.Position, light_pos);

        ImGui::InputFloat("p range", &demo_state->point_light.Range);
        imgui_help::float3_edit("p att", &demo_state->point_light.Att);
        ImGui::TreePop();
    }

    // if(ImGui::TreeNode("material")) {
    //     imgui_help::float4_edit("m ambient", &demo_state->obj_material.Ambient);
    //     imgui_help::float4_edit("m diffuse", &demo_state->obj_material.Diffuse);
    //     imgui_help::float4_edit("m specular", &demo_state->obj_material.Specular);
    //     ImGui::InputFloat("m shininess", &demo_state->obj_material.Specular.w);
    //     ImGui::TreePop();
    // }


    ImGui::InputFloat("uv scale", &demo_state->uv_scale);

    // imgui related things
    ID3D11RasterizerState *rs;
    if(demo_state->do_wireframe_rs) {
        rs = rctx->mWireframeRS;
    } else {
        rs = rctx->mSolidRS;
    }

    // lights

    DirectionalLight dir_lights[3];
    memcpy(dir_lights, demo_state->dir_lights, sizeof(DirectionalLight) * 3);

    PointLight point_light = demo_state->point_light;
    if(demo_state->disable_point_light) {
        point_light = {};
    }

    // setting cam vars

    // Build the view matrix.
    XMMATRIX cam_rot_mat = XMMatrixRotationQuaternion(XMQuaternionRotationRollPitchYaw(rctx->cam_pitch, rctx->cam_yaw, 0.0f));
    XMVECTOR cam_pos_start = XMVectorSet(0.0f, 0.0f, -1.0f * rctx->cam_radius, 1.0f);

    XMVECTOR cam_pos = XMVector3Transform(cam_pos_start, cam_rot_mat);
    XMVECTOR cam_target = XMVectorZero();
    XMVECTOR cam_up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    XMStoreFloat3(&demo_state->mEyePosW, cam_pos);

    XMMATRIX view_mat = XMMatrixLookAtLH(cam_pos, cam_target, cam_up);
    XMStoreFloat4x4(&demo_state->mView, view_mat);

    // clearing buffers

    rctx->device_context->ClearRenderTargetView(rctx->render_target_view,
                                                demo_state->clear_color);
    rctx->device_context->ClearDepthStencilView(rctx->depth_stencil_view,
                                                D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    // setting renderer state
    rctx->device_context->IASetInputLayout(rctx->il_pos_normal_uv);
    rctx->device_context->IASetPrimitiveTopology(
            D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    rctx->device_context->RSSetState(rs);

	XMMATRIX view  = XMLoadFloat4x4(&demo_state->mView);
	XMMATRIX proj  = XMLoadFloat4x4(&rctx->mProj);
	// XMMATRIX viewProj = view*proj;

	// Set per frame constants.
	rctx->basic_effect.SetDirLights(dir_lights);
	rctx->basic_effect.SetPointLight(&point_light);

	rctx->basic_effect.SetEyePosW(demo_state->mEyePosW);

    auto tech = rctx->basic_effect.Light1Tech;
    switch(demo_state->how_many_lights) {
        case 2:
            tech = rctx->basic_effect.Light2Tech;
            break;
        case 3:
            tech = rctx->basic_effect.Light3Tech;
            break;
    }

    // texture stuff (so far textures are not handled in render_ojbs)
	// XMMATRIX I = XMMatrixIdentity();
    f32 s = demo_state->uv_scale;
	XMMATRIX obj_scale = XMMatrixScaling(s, s, s);
    rctx->basic_effect.SetTexTransform(obj_scale);
    rctx->basic_effect.SetTexture(rctx->srv_diffuse);
    rctx->basic_effect.SetTextureSpecular(rctx->srv_specular);

    // demo_state->render_objs.draw(tech, view, proj, rctx);

    D3DX11_TECHNIQUE_DESC techDesc;
    tech->GetDesc(&techDesc);

    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    rctx->device_context->IASetVertexBuffers(0, 1, &demo_state->render_objs.vb, &stride, &offset);

    // this should only be 1 pass so don't get scared by this loop
    for (u32 p = 0; p < techDesc.Passes; ++p) {

        // this could be in a render_obj method

        // pick an obj based on time
        i32 obj_i_to_render = (i32)demo_state->total_time % demo_state->render_objs.obj_ranges.size();

        auto const &obj_range = demo_state->render_objs.obj_ranges[obj_i_to_render];

        // set all the shader vars
        XMMATRIX world = XMLoadFloat4x4(&demo_state->render_objs.obj_world_mats[obj_i_to_render]);
        XMMATRIX worldInvTranspose = math::InverseTranspose(world);
        XMMATRIX worldViewProj = world*view*proj;

        rctx->basic_effect.SetWorld(world);
        rctx->basic_effect.SetWorldInvTranspose(worldInvTranspose);
        rctx->basic_effect.SetWorldViewProj(worldViewProj);

        for(auto const &mat_range: demo_state->render_objs.mat_ranges) {

            Material mat = demo_state->render_objs.materials[mat_range.material];
            rctx->basic_effect.SetMaterial(&mat);

            u32 start = math::clamp(mat_range.start, obj_range.start, obj_range.end);
            u32 end = math::clamp(mat_range.end, obj_range.start, obj_range.end);

            // apply
            tech->GetPassByIndex(p)->Apply(0, rctx->device_context);

            // we will not use indexed drawing bc i still don't know how to parsse obj's.
            rctx->device_context->Draw(end - start, start);
        }
    }
}
