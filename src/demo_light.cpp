
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
    XMFLOAT4X4 obj_transform;
    Material obj_material;

    // lights
    DirectionalLight dir_lights[3];
    PointLight point_light;

    XMFLOAT4X4 mView;
    XMFLOAT4X4 mProj;
	XMFLOAT3 mEyePosW;

    // imgui state

};

fn LucyResult demo_init(Arena *arena, RenderContext *rctx, LightDemo *out_demo_state) {

    rctx->cam_radius = 30.0f;

    // loading obj
    ObjFile the_obj = {};
    // LucyResult res = load_obj(arena, "Models\\skull\\skull.obj", &the_obj);
    LucyResult res = load_obj(arena, "Models\\cube.obj", &the_obj);
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
        .Ambient = XMFLOAT4(0.1f, 0.1f, 0.1f, 0.1f),
        .Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
        .Specular = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
        // what is
        .Direction = XMFLOAT3(1.0f, 1.0f, 1.0f),
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

    std::vector<Vertex> obj_vertices(the_obj.positions.size());
    std::vector<u32> obj_indices(the_obj.position_indices.size());


    // for(u32 i = 0; i<the_obj.positions.size(); ++i) {
    //     obj_vertices[i].Pos = the_obj.positions[i];
    //     // obj_vertices[i].Color = black;
    // }

    for(u32 i = 0; i<the_obj.position_indices.size(); ++i) {
        assert(the_obj.position_indices[i] >= 0);
        assert(the_obj.normal_indices[i] >= 0);

        obj_indices[i] = (u32)the_obj.position_indices[i] - 1;

        log("obj indices[i] is %i", obj_indices[i]);

        // have to find the normal and add it to the right vertex.
        u32 pos_index = (u32)the_obj.position_indices[i] - 1;
        u32 normal_index = (u32)the_obj.normal_indices[i] - 1;

        obj_vertices[obj_indices[i]].Pos = the_obj.positions[pos_index];

        // todo maybe normalize normal here or when u load the obj
        obj_vertices[obj_indices[i]].Normal = the_obj.normals[normal_index];
    }

    // print all the vertices
    for(Vertex &v : obj_vertices) {
        log("pos %f %f %f", v.Pos.x, v.Pos.y, v.Pos.z);
        log("normal %f %f %f", v.Normal.x, v.Normal.y, v.Normal.z);
    }

    D3D11_BUFFER_DESC vbd = {};
    vbd.Usage = D3D11_USAGE_IMMUTABLE;
    vbd.ByteWidth = sizeof(Vertex) * (u32)the_obj.positions.size();
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

    // setting matrixes that don't need to be set every frame...
    // (proj matrix)
    //  TODO: this has to be set when u resize too
    XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * math::PI, WINDOW_ASPECT_RATIO, 1.0f, 1000.0f);
    XMStoreFloat4x4(&out_demo_state->mProj, P);


    // transform the obj here
	// XMMATRIX I = XMMatrixIdentity();
	XMMATRIX obj_scale = XMMatrixScaling(1.0f, 1.0f, 1.0f);
	XMMATRIX obj_offset = XMMatrixTranslation(0.0f, 0.0f, 0.0f);
	XMStoreFloat4x4(&out_demo_state->obj_transform, XMMatrixMultiply(obj_scale, obj_offset));

    out_demo_state->mView = {};
    out_demo_state->mEyePosW = {};

    return LRES_OK;
}

// update and render (runs every frame)
fn void demo_update_render(RenderContext *rctx, LightDemo *demo_state, f32 dt) {

    //imgui stuff here

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
                                                reinterpret_cast<const float *>(&Colors::Blue));
    rctx->device_context->ClearDepthStencilView(rctx->depth_stencil_view,
                                                D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    // setting renderer state
    rctx->device_context->IASetInputLayout(rctx->il_pos_normal);
    rctx->device_context->IASetPrimitiveTopology(
            D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // todo this should depend on imgui checkbox
    rctx->device_context->RSSetState(rctx->mSolidRS);
    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    rctx->device_context->IASetVertexBuffers(0, 1, &demo_state->vb, &stride, &offset);
    rctx->device_context->IASetIndexBuffer(demo_state->ib, DXGI_FORMAT_R32_UINT, 0);


	XMMATRIX view  = XMLoadFloat4x4(&demo_state->mView);
	XMMATRIX proj  = XMLoadFloat4x4(&demo_state->mProj);
	XMMATRIX viewProj = view*proj;


	// Set per frame constants.

	rctx->basic_effect.SetDirLights(demo_state->dir_lights);
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
        rctx->device_context->DrawIndexed(demo_state->obj_index_count, 0, 0);
    }


}