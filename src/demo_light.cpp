
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
    XMFLOAT4X4 mProj;
	XMFLOAT3 mEyePosW;

    // imgui state
    bool do_wireframe_rs;
    f32 clear_color[4];

    bool disable_dir_1_light;
    bool disable_dir_2_light;
    bool disable_dir_3_light;

    bool disable_point_light;
};

fn LucyResult demo_init(Arena *arena, RenderContext *rctx, LightDemo *out_demo_state) {

    rctx->cam_radius = 10.0f;

    out_demo_state->disable_dir_2_light = true;
    out_demo_state->disable_dir_3_light = true;
    out_demo_state->disable_point_light = true;

    // initializing imgui stuff
    f32 clear_color[4] = { 0.4f, 0.4f, 0.4f, 1.0f };
    memcpy(out_demo_state->clear_color, clear_color, sizeof(clear_color));

    // loading obj
    ObjFile the_obj = {};
    LucyResult res = load_obj(arena, "Models\\skull\\skull.obj", &the_obj);
    // LucyResult res = load_obj(arena, "Models\\cube.obj", &the_obj);
    assert(res == LRES_OK);

    // initializing material
    out_demo_state->obj_material = {
        .Ambient = XMFLOAT4(0.1f, 0.1f, 0.1f, 0.1f),
        .Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
        .Specular = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
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
    out_demo_state->dir_lights[1] = dir_light;
    out_demo_state->dir_lights[2] = dir_light;

    out_demo_state->point_light = {
        .Ambient = XMFLOAT4(0.1f, 0.1f, 0.1f, 0.1f),
        .Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
        .Specular = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
        // what is
        .Position = XMFLOAT3(1.0f, 1.0f, 1.0f),
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
        assert(the_obj.normal_indices.size() == the_obj.position_indices.size());

        // get pos, get normal, then make vertex and push

        obj_indices[i] = (u32)the_obj.position_indices[i] - 1;

        obj_vertices[i].Pos = the_obj.positions[the_obj.position_indices[i] - 1];
        obj_vertices[i].Normal = the_obj.normals[the_obj.normal_indices[i] - 1];

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

    // setting matrixes that don't need to be set every frame...
    // (proj matrix)
    //  TODO: this has to be set when u resize too
    XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * math::PI, WINDOW_ASPECT_RATIO, 1.0f, 1000.0f);
    XMStoreFloat4x4(&out_demo_state->mProj, P);


    // transform the obj here
	// XMMATRIX I = XMMatrixIdentity();

	XMMATRIX obj_scale = XMMatrixScaling(1.0f, 1.0f, 1.0f);
    XMMATRIX obj_rot = XMMatrixRotationX(0.0f);
	XMMATRIX obj_offset = XMMatrixTranslation(0.0f, 0.0f, 0.0f);
	XMStoreFloat4x4(&out_demo_state->obj_transform, XMMatrixMultiply(obj_rot, XMMatrixMultiply(obj_scale, obj_offset)));

    out_demo_state->mView = {};
    out_demo_state->mEyePosW = {};

    return LRES_OK;
}

// update and render (runs every frame)
fn void demo_update_render(RenderContext *rctx, LightDemo *demo_state, f32 dt) {

    //imgui stuff here
    ImGui::Checkbox("wireframe mode", &demo_state->do_wireframe_rs);
    ImGui::ColorEdit4("clear color", demo_state->clear_color);

    // lights
    ImGui::Checkbox("disable dir light 1", &demo_state->disable_dir_1_light);
    ImGui::Checkbox("disable dir light 2", &demo_state->disable_dir_2_light);
    ImGui::Checkbox("disable dir light 3", &demo_state->disable_dir_3_light);
    ImGui::Checkbox("disable point light", &demo_state->disable_point_light);

    ImGui::LabelText("", "dir light 1 properties");

    f32 a[4];
    a[0] = demo_state->dir_lights[0].Ambient.x;
    a[1] = demo_state->dir_lights[0].Ambient.y;
    a[2] = demo_state->dir_lights[0].Ambient.z;
    a[3] = demo_state->dir_lights[0].Ambient.w;
    ImGui::ColorEdit4("ambient", a);
    demo_state->dir_lights[0].Ambient = XMFLOAT4(a);

    a[0] = demo_state->dir_lights[0].Diffuse.x;
    a[1] = demo_state->dir_lights[0].Diffuse.y;
    a[2] = demo_state->dir_lights[0].Diffuse.z;
    a[3] = demo_state->dir_lights[0].Diffuse.w;
    ImGui::ColorEdit4("diffuse", a);
    demo_state->dir_lights[0].Diffuse = XMFLOAT4(a);

    a[0] = demo_state->dir_lights[0].Specular.x;
    a[1] = demo_state->dir_lights[0].Specular.y;
    a[2] = demo_state->dir_lights[0].Specular.z;
    a[3] = demo_state->dir_lights[0].Specular.w;
    ImGui::ColorEdit4("specular", a);
    demo_state->dir_lights[0].Specular = XMFLOAT4(a);

    f32 dir[3];

    dir[0] = demo_state->dir_lights[0].Direction.x;
    dir[1] = demo_state->dir_lights[0].Direction.y;
    dir[2] = demo_state->dir_lights[0].Direction.z;
    ImGui::InputFloat3("Direction", dir);
    demo_state->dir_lights[0].Direction = XMFLOAT3(dir);

    ImGui::LabelText("", "material");

    imgui_help::float4_edit("m ambient", &demo_state->obj_material.Ambient);
    imgui_help::float4_edit("m diffuse", &demo_state->obj_material.Diffuse);
    imgui_help::float4_edit("m specular", &demo_state->obj_material.Specular);

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

    if(demo_state->disable_dir_1_light) {
        dir_lights[0] = {};
    }
    if(demo_state->disable_dir_2_light) {
        dir_lights[1] = {};
    }
    if(demo_state->disable_dir_3_light) {
        dir_lights[2] = {};
    }

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
    rctx->device_context->IASetInputLayout(rctx->il_pos_normal);
    rctx->device_context->IASetPrimitiveTopology(
            D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // todo this should depend on imgui checkbox
    rctx->device_context->RSSetState(rs);
    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    rctx->device_context->IASetVertexBuffers(0, 1, &demo_state->vb, &stride, &offset);
    rctx->device_context->IASetIndexBuffer(demo_state->ib, DXGI_FORMAT_R32_UINT, 0);


	XMMATRIX view  = XMLoadFloat4x4(&demo_state->mView);
	XMMATRIX proj  = XMLoadFloat4x4(&demo_state->mProj);
	XMMATRIX viewProj = view*proj;


	// Set per frame constants.
	rctx->basic_effect.SetDirLights(dir_lights);
	rctx->basic_effect.SetPointLight(&point_light);

	rctx->basic_effect.SetEyePosW(demo_state->mEyePosW);

    D3DX11_TECHNIQUE_DESC techDesc;
    // todo did u initialize all the techs?? i not sure if it does that by default
    rctx->basic_effect.Light3Tech->GetDesc(&techDesc);

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

        // apply
        rctx->basic_effect.Light3Tech->GetPassByIndex(p)->Apply(0, rctx->device_context);

        //draw
        // rctx->device_context->DrawIndexed(demo_state->obj_index_count, 0, 0);

        // we will not use indexed drwaing bc i still don't know how to parsse obj's.
        rctx->device_context->Draw(demo_state->obj_vertex_count, 0);
    }


}