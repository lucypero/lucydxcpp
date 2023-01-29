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
    ObjFile the_obj = {};
    LucyResult res = load_obj(arena, "Models\\skull\\skull.obj", &the_obj);
    // LucyResult res = load_obj(arena, "Models\\plane.obj", &the_obj);
    // LucyResult res = load_obj(arena, "Models\\mill\\my_mill.obj", &the_obj);
    // LucyResult res = load_obj(arena, "Models\\cube.obj", &the_obj);
    assert(res == LRES_OK);

    // initializing material
    out_demo_state->obj_material = {
        .Ambient = XMFLOAT4(0.1f, 0.1f, 0.1f, 0.1f),
        .Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
        .Specular = XMFLOAT4(1.0f, 1.0f, 1.0f, 32.0f),
        // what is this
        .Reflect = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
    };

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

    // creating buffers for custom object

    std::vector<Vertex> obj_vertices(the_obj.position_indices.size());
    std::vector<u32> obj_indices(the_obj.position_indices.size());

    // for(u32 i = 0; i<the_obj.positions.size(); ++i) {
    //     obj_vertices[i].Pos = the_obj.positions[i];
    //     // obj_vertices[i].Color = black;
    // }

    for(u32 i = 0; i<the_obj.position_indices.size(); ++i) {
        assert(the_obj.position_indices[i] >= 0);
        assert(the_obj.normal_indices[i] >= 0);
        assert(the_obj.uv_indices[i] >= 0);
        assert(the_obj.normal_indices.size() == the_obj.position_indices.size());
        assert(the_obj.uv_indices.size() == the_obj.position_indices.size());

        // get pos, get normal, then make vertex and push

        obj_indices[i] = (u32)the_obj.position_indices[i] - 1;

        obj_vertices[i].Pos = the_obj.positions[the_obj.position_indices[i] - 1];
        obj_vertices[i].Normal = the_obj.normals[the_obj.normal_indices[i] - 1];
        obj_vertices[i].Tex = the_obj.uvs[the_obj.uv_indices[i] - 1];

        // log("vertex %i pos %f %f %f", obj_indices[i], obj_vertices[obj_indices[i]].Pos.x, obj_vertices[obj_indices[i]].Pos.y, obj_vertices[obj_indices[i]].Pos.z);
        // log("vertex %i normal %f %f %f", obj_indices[i], obj_vertices[obj_indices[i]].Normal.x, obj_vertices[obj_indices[i]].Normal.y, obj_vertices[obj_indices[i]].Normal.z);
    }

    D3D11_BUFFER_DESC vbd = {};
    vbd.Usage = D3D11_USAGE_IMMUTABLE;
    vbd.ByteWidth = sizeof(Vertex) * (u32)obj_vertices.size();
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = 0;
    vbd.MiscFlags = 0;
    D3D11_SUBRESOURCE_DATA vinitData = {};
    vinitData.pSysMem = &obj_vertices[0];
    HRESULT hres = rctx->device->CreateBuffer(&vbd, &vinitData, &out_demo_state->vb);
    assert(hres == 0);

    D3D11_BUFFER_DESC ibd = {};
    ibd.Usage = D3D11_USAGE_IMMUTABLE;
    ibd.ByteWidth = sizeof(u32) * (u32)the_obj.position_indices.size();
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibd.CPUAccessFlags = 0;
    ibd.MiscFlags = 0;
    D3D11_SUBRESOURCE_DATA iinitData = {};
    iinitData.pSysMem = &obj_indices[0];
    hres = rctx->device->CreateBuffer(&ibd, &iinitData, &out_demo_state->ib);
    assert(hres == 0);

    out_demo_state->obj_index_count = (u32)the_obj.position_indices.size();
    out_demo_state->obj_vertex_count = (u32)obj_vertices.size();



    // transform the obj here

	XMMATRIX obj_scale = XMMatrixScaling(1.0f, 1.0f, 1.0f);
    XMMATRIX obj_rot = XMMatrixRotationX(0.0f);
	XMMATRIX obj_offset = XMMatrixTranslation(0.0f, 0.0f, 0.0f);
	XMStoreFloat4x4(&out_demo_state->obj_transform, XMMatrixMultiply(obj_rot, XMMatrixMultiply(obj_scale, obj_offset)));

    out_demo_state->mView = {};
    out_demo_state->mEyePosW = {};

    // initting texture stuff


    return LRES_OK;
}

// update and render (runs every frame)
fn void demo_update_render(RenderContext *rctx, LightDemo *demo_state, f32 dt) {


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

    if(ImGui::TreeNode("material")) {
        imgui_help::float4_edit("m ambient", &demo_state->obj_material.Ambient);
        imgui_help::float4_edit("m diffuse", &demo_state->obj_material.Diffuse);
        imgui_help::float4_edit("m specular", &demo_state->obj_material.Specular);
        ImGui::InputFloat("m shininess", &demo_state->obj_material.Specular.w);
        ImGui::TreePop();
    }


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

    // todo this should depend on imgui checkbox
    rctx->device_context->RSSetState(rs);
    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    rctx->device_context->IASetVertexBuffers(0, 1, &demo_state->vb, &stride, &offset);
    rctx->device_context->IASetIndexBuffer(demo_state->ib, DXGI_FORMAT_R32_UINT, 0);

	XMMATRIX view  = XMLoadFloat4x4(&demo_state->mView);
	XMMATRIX proj  = XMLoadFloat4x4(&rctx->mProj);
	XMMATRIX viewProj = view*proj;

	// Set per frame constants.
	rctx->basic_effect.SetDirLights(dir_lights);
	rctx->basic_effect.SetPointLight(&point_light);

	rctx->basic_effect.SetEyePosW(demo_state->mEyePosW);

    // todo did u initialize all the techs?? i not sure if it does that by default

    // the tech we will use

    auto tech = rctx->basic_effect.Light1Tech;
    switch(demo_state->how_many_lights) {
        case 2:
            tech = rctx->basic_effect.Light2Tech;
            break;
        case 3:
            tech = rctx->basic_effect.Light3Tech;
            break;
    }

    D3DX11_TECHNIQUE_DESC techDesc;
    tech->GetDesc(&techDesc);

    // this should only be 1 pass so don't get scared by this loop
    for (u32 p = 0; p < techDesc.Passes; ++p) {

        // set all the shader vars
		XMMATRIX world = XMLoadFloat4x4(&demo_state->obj_transform);
		XMMATRIX worldInvTranspose = math::InverseTranspose(world);
		XMMATRIX worldViewProj = world*view*proj;

		rctx->basic_effect.SetWorld(world);
		rctx->basic_effect.SetWorldInvTranspose(worldInvTranspose);
		rctx->basic_effect.SetWorldViewProj(worldViewProj);
		rctx->basic_effect.SetMaterial(&demo_state->obj_material);

        // texture stuff
	    // XMMATRIX I = XMMatrixIdentity();
        f32 s = demo_state->uv_scale;
	    XMMATRIX obj_scale = XMMatrixScaling(s, s, s);
        rctx->basic_effect.SetTexTransform(obj_scale);
        rctx->basic_effect.SetTexture(rctx->srv_diffuse);
        rctx->basic_effect.SetTextureSpecular(rctx->srv_specular);

        // apply
        tech->GetPassByIndex(p)->Apply(0, rctx->device_context);

        //draw
        // rctx->device_context->DrawIndexed(demo_state->obj_index_count, 0, 0);

        // we will not use indexed drwaing bc i still don't know how to parsse obj's.
        rctx->device_context->Draw(demo_state->obj_vertex_count, 0);
    }
}