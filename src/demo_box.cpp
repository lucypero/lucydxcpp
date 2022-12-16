// BOX DEMO

struct DemoState {
    ID3DX11EffectTechnique *tech;
    ID3DX11EffectMatrixVariable *wvp_mat_var;
    ID3D11InputLayout *input_layout;

    ID3D11Buffer *box_vb;
    ID3D11Buffer *box_ib;
    u32 index_count = 36;

    XMFLOAT4X4 mat_world;
    XMFLOAT4X4 mat_view;
    XMFLOAT4X4 mat_proj;

    f32 clear_color[4];
    f32 cube_color[4];
};

fn LucyResult demo_init(Arena *arena, RenderContext *rctx, DemoState *out_demo_state) {

    // VARIABLES like WVP matrix and stuff ------------

    XMFLOAT4X4 mat_world;
    XMFLOAT4X4 mat_view;
    XMFLOAT4X4 mat_proj;

    XMMATRIX I = XMMatrixIdentity();
    XMStoreFloat4x4(&mat_world, I);
    XMStoreFloat4x4(&mat_view, I);
    XMStoreFloat4x4(&mat_proj, I);

    // VARIABLES /end ------------------------------

    // SHADER LOADING ------------------------

    u64 checkpoint = arena_save(arena);

    Buf color_fx_buf;
    LucyResult lres = read_whole_file(arena, "build\\color.fxo", &color_fx_buf);
    assert(lres == LRES_OK);

    ID3DX11Effect *effect;

    HRESULT hres = D3DX11CreateEffectFromMemory(
            color_fx_buf.buf,
            color_fx_buf.size,
            0, rctx->device,
            &effect);
    assert(hres == 0);

    //getting tech and WVP matrix from effect
    ID3DX11EffectTechnique *tech = effect->GetTechniqueByName("ColorTech");
    assert(tech->IsValid());

    ID3DX11EffectMatrixVariable *wvp_mat_var = effect->GetVariableByName("gWorldViewProj")->AsMatrix();
    assert(wvp_mat_var->IsValid());

    // shader input layout

    ID3D11InputLayout *input_layout = nullptr;
    D3D11_INPUT_ELEMENT_DESC inputElementDesc[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    D3DX11_PASS_DESC pass_desc;
    tech->GetPassByIndex(0)->GetDesc(&pass_desc);

    hres = rctx->device->CreateInputLayout(
            inputElementDesc,
            arrsize(inputElementDesc),
            pass_desc.pIAInputSignature,
            pass_desc.IAInputSignatureSize,
            &input_layout);
    assert(hres == 0);

    // don't need the shader buffer anymore. color_fx_buf is invalid now.
    arena_restore(arena, checkpoint);

    // SHADER LOADING /END ------------------------

    // INITIALIZING BUFFERS -----------------

    // create vertex buffer
    Vertex vertices[] = {
            {XMFLOAT3(-1.0f, -1.0f, -1.0f), Colors::White},
            {XMFLOAT3(-1.0f, +1.0f, -1.0f), Colors::Black},
            {XMFLOAT3(+1.0f, +1.0f, -1.0f), Colors::Red},
            {XMFLOAT3(+1.0f, -1.0f, -1.0f), Colors::Green},
            {XMFLOAT3(-1.0f, -1.0f, +1.0f), Colors::Blue},
            {XMFLOAT3(-1.0f, +1.0f, +1.0f), Colors::Yellow},
            {XMFLOAT3(+1.0f, +1.0f, +1.0f), Colors::Cyan},
            {XMFLOAT3(+1.0f, -1.0f, +1.0f), Colors::Magenta},
    };

    u32 vert_count = 8;

    ID3D11Buffer *box_vb;

    D3D11_BUFFER_DESC vbd = {};
    vbd.Usage = D3D11_USAGE_DYNAMIC;
    vbd.ByteWidth = sizeof(Vertex) * vert_count;
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    D3D11_SUBRESOURCE_DATA vinit_data = {};
    vinit_data.pSysMem = vertices;
    hres = rctx->device->CreateBuffer(&vbd, &vinit_data, &box_vb);
    assert(hres == 0);

    // create index buffer
    u32 indices[] = {
            //front face
            0, 1, 2,
            0, 2, 3,

            //back face
            4, 6, 5,
            4, 7, 6,

            // left face
            4, 5, 1,
            4, 1, 0,

            // right face
            3, 2, 6,
            3, 6, 7,

            // top face
            1, 5, 6,
            1, 6, 2,

            //bottom face
            4, 0, 3,
            4, 3, 7,
    };

    u32 index_count = 36;

    ID3D11Buffer *box_ib;

    D3D11_BUFFER_DESC ibd = {};
    ibd.Usage = D3D11_USAGE_IMMUTABLE;
    ibd.ByteWidth = sizeof(u32) * index_count;
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    D3D11_SUBRESOURCE_DATA iinit_data = {};
    iinit_data.pSysMem = indices;
    hres = rctx->device->CreateBuffer(&ibd, &iinit_data, &box_ib);
    assert(hres == 0);

    // INITIALIZING BUFFERS - /END -----------------

    // setting matrixes that don't need to be set every frame...
    // (proj matrix)
    //  TODO: this has to be set when u resize too
    XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * math::PI, WINDOW_ASPECT_RATIO, 1.0f, 1000.0f);
    XMStoreFloat4x4(&mat_proj, P);

    // setting demo state
    out_demo_state->tech = tech;
    out_demo_state->wvp_mat_var = wvp_mat_var;
    out_demo_state->input_layout = input_layout;
    out_demo_state->box_vb = box_vb;
    out_demo_state->box_ib = box_ib;
    out_demo_state->index_count = index_count;
    out_demo_state->mat_world = mat_world;
    out_demo_state->mat_view = mat_view;
    out_demo_state->mat_proj = mat_proj;

    f32 clear_color[4] = { 0.4f, 0.7f, 0.0f, 1.0f };
    memcpy(out_demo_state->clear_color, clear_color, sizeof(clear_color));

    f32 cube_color[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
    memcpy(out_demo_state->cube_color, cube_color, sizeof(cube_color));

    return LRES_OK;
}

// update and render (runs every frame)
fn void demo_update_render(RenderContext *rctx, DemoState *demo_state) {
    ImGui::ColorEdit4("clear color", demo_state->clear_color);
    ImGui::ColorEdit4("cube color", demo_state->cube_color);

    XMMATRIX cam_rot_mat = XMMatrixRotationQuaternion(XMQuaternionRotationRollPitchYaw(cam_pitch, cam_yaw, 0.0f));
    XMVECTOR cam_pos_start = XMVectorSet(0.0f, 0.0f, -1.0f * cam_radius, 1.0f);

    XMVECTOR cam_pos = XMVector3Transform(cam_pos_start, cam_rot_mat);
    XMVECTOR cam_target = XMVectorZero();
    XMVECTOR cam_up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    XMMATRIX view_mat = XMMatrixLookAtLH(cam_pos, cam_target, cam_up);
    XMStoreFloat4x4(&demo_state->mat_view, view_mat);

    // Update program state /end ---------------

    // Draw ---------------
//        device_context->ClearRenderTargetView(render_target_view, reinterpret_cast<const f32 *>(&Colors::Blue));
    rctx->device_context->ClearRenderTargetView(rctx->render_target_view, demo_state->clear_color);

    rctx->device_context->ClearDepthStencilView(rctx->depth_stencil_view, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    rctx->device_context->IASetInputLayout(demo_state->input_layout);
    rctx->device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    u32 stride = sizeof(Vertex);
    u32 offset = 0;
    rctx->device_context->IASetVertexBuffers(0, 1, &demo_state->box_vb, &stride, &offset);
    rctx->device_context->IASetIndexBuffer(demo_state->box_ib, DXGI_FORMAT_R32_UINT, 0);

    //Set constants
    XMMATRIX world = XMLoadFloat4x4(&demo_state->mat_world);
    XMMATRIX view = XMLoadFloat4x4(&demo_state->mat_view);
    XMMATRIX proj = XMLoadFloat4x4(&demo_state->mat_proj);
    XMMATRIX wvp = world * view * proj;
    demo_state->wvp_mat_var->SetMatrix(reinterpret_cast<float *>(&wvp));

    // change vertex colors based on imgui picked color
    D3D11_MAPPED_SUBRESOURCE mapped_resource = {};

    // create vertex buffer
    Vertex new_vertices[] = {
            {XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(demo_state->cube_color)},
            {XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT4(demo_state->cube_color)},
            {XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT4(demo_state->cube_color)},
            {XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(demo_state->cube_color)},
            {XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(demo_state->cube_color)},
            {XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT4(demo_state->cube_color)},
            {XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT4(demo_state->cube_color)},
            {XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(demo_state->cube_color)},
    };

    rctx->device_context->Map(demo_state->box_vb, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource);
    memcpy(mapped_resource.pData, new_vertices, sizeof(new_vertices));
    rctx->device_context->Unmap(demo_state->box_vb, 0);

    // Drawing indexes
    D3DX11_TECHNIQUE_DESC tech_desc;
    demo_state->tech->GetDesc(&tech_desc);
    for (u32 p = 0; p < tech_desc.Passes; ++p) {
        demo_state->tech->GetPassByIndex(p)->Apply(0, rctx->device_context);
        rctx->device_context->DrawIndexed(demo_state->index_count, 0, 0);
    }
}