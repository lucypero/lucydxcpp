
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

fn LucyResult demo_init(Arena *arena, RenderContext *rctx, ShapesDemo *out_demo_state) {

    rctx->cam_radius = 30.0f;


    // loading obj
    ObjFile skull_obj = {};
    
    LucyResult res = load_obj(arena, "Models\\skull\\skull.obj", &skull_obj);
    // LucyResult res = load_obj(arena, "Models\\cube.obj", &skull_obj);
    assert(res == LRES_OK);

    //printing obj
    // for(XMFLOAT3 &p: skull_obj.positions) {
    //     log("pos: %f %f %f", p.x, p.y, p.z);
    // }

    // for(XMFLOAT3 &p: skull_obj.normals) {
    //     log("normals: %f %f %f", p.x, p.y, p.z);
    // }

    // for(XMFLOAT3 &p: skull_obj.uvs) {
    //     log("uvs: %f %f", p.x, p.y);
    // }

    // for(i64 &p: skull_obj.position_indices) {
    //     log("pos indices: %i", p);
    // }

    XMMATRIX I = XMMatrixIdentity();
    XMStoreFloat4x4(&out_demo_state->mGridWorld, I);
    XMStoreFloat4x4(&out_demo_state->mView, I);
    XMStoreFloat4x4(&out_demo_state->mProj, I);

    // Define transformations from local space to world space.
    XMStoreFloat4x4(&out_demo_state->mGridWorld, I);
    XMMATRIX boxScale = XMMatrixScaling(2.0f, 1.0f, 2.0f);
    XMMATRIX boxOffset = XMMatrixTranslation(0.0f, 0.5f, 0.0f);
    XMStoreFloat4x4(&out_demo_state->mBoxWorld, XMMatrixMultiply(boxScale, boxOffset));
    XMMATRIX centerSphereScale = XMMatrixScaling(2.0f, 2.0f, 2.0f);
    XMMATRIX centerSphereOffset = XMMatrixTranslation(0.0f, 2.0f, 0.0f);
    XMStoreFloat4x4(&out_demo_state->mCenterSphere, XMMatrixMultiply(centerSphereScale, centerSphereOffset));
    // We create 5 rows of 2 cylinders and spheres per row.
    for (int i = 0; i < 5; ++i) {
        XMStoreFloat4x4(&out_demo_state->mCylWorld[i * 2 + 0],
                        XMMatrixTranslation(-5.0f, 1.5f, -10.0f + i * 5.0f));
        XMStoreFloat4x4(&out_demo_state->mCylWorld[i * 2 + 1],
                        XMMatrixTranslation(+5.0f, 1.5f, -10.0f + i * 5.0f));
        XMStoreFloat4x4(&out_demo_state->mSphereWorld[i * 2 + 0],
                        XMMatrixTranslation(-5.0f, 3.5f, -10.0f + i * 5.0f));
        XMStoreFloat4x4(&out_demo_state->mSphereWorld[i * 2 + 1],
                        XMMatrixTranslation(+5.0f, 3.5f, -10.0f + i * 5.0f));
    }

    // obj transform
    XMMATRIX obj_scale = XMMatrixScaling(0.3f, 0.3f, 0.3f);
    XMMATRIX obj_rot = XMMatrixRotationX(-math::PI / 2.0f);
    XMMATRIX obj_offset = XMMatrixTranslation(0.0f, 3.0f, 0.0f);
    XMStoreFloat4x4(&out_demo_state->obj_transform, XMMatrixMultiply(obj_rot, XMMatrixMultiply(obj_scale, obj_offset)));

    // BuildGeometryBuffers()
    GeometryGenerator::MeshData box;
    GeometryGenerator::MeshData grid;
    GeometryGenerator::MeshData sphere;
    GeometryGenerator::MeshData cylinder;

    GeometryGenerator::create_box(1.0f, 1.0f, 1.0f, &box);
    GeometryGenerator::create_grid(20.0f, 30.0f, 60, 40, &grid);
//     GeometryGenerator::create_sphere(0.5f, 20, 20, &sphere);
    GeometryGenerator::create_geosphere(0.5f, 2, &sphere);
    GeometryGenerator::create_cylinder(0.5f, 0.3f, 3.0f, 20, 20, &cylinder);


    // Cache the vertex offsets to each object in the concatenated
    // vertex buffer.
    out_demo_state->mBoxVertexOffset = 0;
    out_demo_state->mGridVertexOffset = (i32) box.Vertices.size();
    out_demo_state->mSphereVertexOffset = (i32) out_demo_state->mGridVertexOffset + (i32) grid.Vertices.size();
    out_demo_state->mCylinderVertexOffset = (i32) out_demo_state->mSphereVertexOffset + (i32) sphere.Vertices.size();

    // Cache the index count of each object.
    out_demo_state->mBoxIndexCount = (i32) box.Indices.size();
    out_demo_state->mGridIndexCount = (i32) grid.Indices.size();
    out_demo_state->mSphereIndexCount = (i32) sphere.Indices.size();
    out_demo_state->mCylinderIndexCount = (i32) cylinder.Indices.size();
    // Cache the starting index for each object in the concatenated
    // index buffer.
    out_demo_state->mBoxIndexOffset = 0;
    out_demo_state->mGridIndexOffset = out_demo_state->mBoxIndexCount;
    out_demo_state->mSphereIndexOffset = out_demo_state->mGridIndexOffset + out_demo_state->mGridIndexCount;
    out_demo_state->mCylinderIndexOffset = out_demo_state->mSphereIndexOffset + out_demo_state->mSphereIndexCount;
    UINT totalVertexCount = (u32) (box.Vertices.size() +
                                   grid.Vertices.size() +
                                   sphere.Vertices.size() +
                                   cylinder.Vertices.size());
    UINT totalIndexCount =
            out_demo_state->mBoxIndexCount +
            out_demo_state->mGridIndexCount +
            out_demo_state->mSphereIndexCount +
            out_demo_state->mCylinderIndexCount;
    //
    // Extract the vertex elements we are interested in and pack the
    // vertices of all the meshes into one vertex buffer.
    //
    std::vector<ColorVertex> vertices(totalVertexCount);
    XMFLOAT4 black(0.0f, 0.0f, 0.0f, 1.0f);
    UINT k = 0;
    for (size_t i = 0; i < box.Vertices.size(); ++i, ++k) {
        vertices[k].Pos = box.Vertices[i].Position;
        vertices[k].Color = black;
    }
    for (size_t i = 0; i < grid.Vertices.size(); ++i, ++k) {
        vertices[k].Pos = grid.Vertices[i].Position;
        vertices[k].Color = black;
    }
    for (size_t i = 0; i < sphere.Vertices.size(); ++i, ++k) {
        vertices[k].Pos = sphere.Vertices[i].Position;
        vertices[k].Color = black;
    }
    for (size_t i = 0; i < cylinder.Vertices.size(); ++i, ++k) {
        vertices[k].Pos = cylinder.Vertices[i].Position;
        vertices[k].Color = black;
    }
    D3D11_BUFFER_DESC vbd;
    vbd.Usage = D3D11_USAGE_IMMUTABLE;
    vbd.ByteWidth = sizeof(ColorVertex) * totalVertexCount;
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = 0;
    vbd.MiscFlags = 0;
    D3D11_SUBRESOURCE_DATA vinitData;
    vinitData.pSysMem = &vertices[0];
    HRESULT hres = rctx->device->CreateBuffer(&vbd, &vinitData, &out_demo_state->mVB);
    assert(hres == 0);
    //
    // Pack the indices of all the meshes into one index buffer.
    //
    std::vector<UINT> indices;
    indices.insert(indices.end(), box.Indices.begin(), box.Indices.end());
    indices.insert(indices.end(), grid.Indices.begin(), grid.Indices.end());
    indices.insert(indices.end(), sphere.Indices.begin(),
                   sphere.Indices.end());
    indices.insert(indices.end(), cylinder.Indices.begin(),
                   cylinder.Indices.end());
    D3D11_BUFFER_DESC ibd;
    ibd.Usage = D3D11_USAGE_IMMUTABLE;
    ibd.ByteWidth = sizeof(UINT) * totalIndexCount;
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibd.CPUAccessFlags = 0;
    ibd.MiscFlags = 0;
    D3D11_SUBRESOURCE_DATA iinitData;
    iinitData.pSysMem = &indices[0];
    hres = rctx->device->CreateBuffer(&ibd, &iinitData, &out_demo_state->mIB);
    assert(hres == 0);


    // creating buffers for custom object

    // ID3D11Buffer *obj_vb;
    // ID3D11Buffer *obj_ib;
    // u32 obj_index_count;

    // for now i will just use the position
    std::vector<ColorVertex> obj_vertices(skull_obj.positions.size());

    for(u32 i = 0; i<skull_obj.positions.size(); ++i) {
        obj_vertices[i].Pos = skull_obj.positions[i];
        obj_vertices[i].Color = black;
    }

    vbd = {};
    vbd.Usage = D3D11_USAGE_IMMUTABLE;
    vbd.ByteWidth = sizeof(ColorVertex) * (u32)skull_obj.positions.size();
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = 0;
    vbd.MiscFlags = 0;
    vinitData = {};
    vinitData.pSysMem = &obj_vertices[0];
    hres = rctx->device->CreateBuffer(&vbd, &vinitData, &out_demo_state->obj_vb);
    assert(hres == 0);

    std::vector<u32> obj_indices(skull_obj.position_indices.size());

    for(u32 i = 0; i<skull_obj.position_indices.size(); ++i) {
        assert(skull_obj.position_indices[i] >= 0);
        obj_indices[i] = (u32)skull_obj.position_indices[i] - 1;
    }

    ibd = {};
    ibd.Usage = D3D11_USAGE_IMMUTABLE;
    ibd.ByteWidth = sizeof(u32) * (u32)skull_obj.position_indices.size();
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibd.CPUAccessFlags = 0;
    ibd.MiscFlags = 0;
    iinitData = {};
    iinitData.pSysMem = &obj_indices[0];
    hres = rctx->device->CreateBuffer(&ibd, &iinitData, &out_demo_state->obj_ib);
    assert(hres == 0);

    out_demo_state->obj_index_count = (u32)skull_obj.position_indices.size();

    // SHADER LOADING ------------------------
    hres = setup_color_shader(arena, rctx, ShaderFile::Color, &out_demo_state->shader);
    assert(hres == LRES_OK);

    // SHADER LOADING /END ------------------------


    //creating rasterizer state

    D3D11_RASTERIZER_DESC wireframeDesc = {};
    wireframeDesc.FillMode = D3D11_FILL_SOLID;
    // wireframeDesc.FillMode = D3D11_FILL_WIREFRAME;
    wireframeDesc.CullMode = D3D11_CULL_BACK;
    wireframeDesc.FrontCounterClockwise = false;
    wireframeDesc.DepthClipEnable = true;

    HR(rctx->device->CreateRasterizerState(&wireframeDesc, &out_demo_state->mWireframeRS));

    // setting matrixes that don't need to be set every frame...
    // (proj matrix)
    //  TODO: this has to be set when u resize too
    XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * math::PI, WINDOW_ASPECT_RATIO, 1.0f, 1000.0f);
    XMStoreFloat4x4(&out_demo_state->mProj, P);

    return LRES_OK;
}

// update and render (runs every frame)
fn void demo_update_render(RenderContext *rctx, ShapesDemo *demo_state, f32 dt) {

    //imgui stuff
    ImGui::DragFloat("cam radius", &rctx->cam_radius, 0.1f, 100.0f, 10.0f);
    ImGui::LabelText("hello", "helloo");
    // /imgui

    // Build the view matrix.
    XMMATRIX cam_rot_mat = XMMatrixRotationQuaternion(XMQuaternionRotationRollPitchYaw(rctx->cam_pitch, rctx->cam_yaw, 0.0f));
    XMVECTOR cam_pos_start = XMVectorSet(0.0f, 0.0f, -1.0f * rctx->cam_radius, 1.0f);

    XMVECTOR cam_pos = XMVector3Transform(cam_pos_start, cam_rot_mat);
    XMVECTOR cam_target = XMVectorZero();
    XMVECTOR cam_up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    XMMATRIX view_mat = XMMatrixLookAtLH(cam_pos, cam_target, cam_up);
    XMStoreFloat4x4(&demo_state->mView, view_mat);

    rctx->device_context->ClearRenderTargetView(rctx->render_target_view,
                                                reinterpret_cast<const float *>(&Colors::Blue));
    rctx->device_context->ClearDepthStencilView(rctx->depth_stencil_view,
                                                D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    Shader *shader = &demo_state->shader;

    rctx->device_context->IASetInputLayout(shader->mInputLayout);
    rctx->device_context->IASetPrimitiveTopology(
            D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    rctx->device_context->RSSetState(demo_state->mWireframeRS);
    UINT stride = sizeof(ColorVertex);
    UINT offset = 0;
    rctx->device_context->IASetVertexBuffers(0, 1, &demo_state->mVB, &stride, &offset);
    rctx->device_context->IASetIndexBuffer(demo_state->mIB, DXGI_FORMAT_R32_UINT, 0);
    // Set constants
    XMMATRIX view = XMLoadFloat4x4(&demo_state->mView);
    XMMATRIX proj = XMLoadFloat4x4(&demo_state->mProj);
    XMMATRIX viewProj = view * proj;
    D3DX11_TECHNIQUE_DESC techDesc;
    XMMATRIX temp = {};
    shader->tech->GetDesc(&techDesc);
    for (UINT p = 0; p < techDesc.Passes; ++p) {
        // Draw the grid.
        XMMATRIX world = XMLoadFloat4x4(&demo_state->mGridWorld);
        temp = world * viewProj;
        shader->wvp_mat_var->SetMatrix(
                reinterpret_cast<float *>(&temp));
        shader->tech->GetPassByIndex(p)->Apply(0, rctx->device_context);
        rctx->device_context->DrawIndexed(
                demo_state->mGridIndexCount, demo_state->mGridIndexOffset, demo_state->mGridVertexOffset);
        // Draw the box.
        world = XMLoadFloat4x4(&demo_state->mBoxWorld);
        temp = world * viewProj;
        shader->wvp_mat_var->SetMatrix(
                reinterpret_cast<float *>(&temp));
        shader->tech ->GetPassByIndex(p)->Apply(0, rctx->device_context);
        rctx->device_context->DrawIndexed(
                demo_state->mBoxIndexCount, demo_state->mBoxIndexOffset, demo_state->mBoxVertexOffset);
        // Draw center sphere.
        // world = XMLoadFloat4x4(&demo_state->mCenterSphere);
        // temp = world * viewProj;
        // shader->wvp_mat_var->SetMatrix(
        //         reinterpret_cast<float *>(&temp));
        // shader->tech->GetPassByIndex(p)->Apply(0, rctx->device_context);
        // rctx->device_context->DrawIndexed(
        //         demo_state->mSphereIndexCount, demo_state->mSphereIndexOffset, demo_state->mSphereVertexOffset);
        // Draw the cylinders.
        for (int i = 0; i < 10; ++i) {
            world = XMLoadFloat4x4(&demo_state->mCylWorld[i]);
            temp = world * viewProj;
            shader->wvp_mat_var->SetMatrix(
                    reinterpret_cast<float *>(&temp));
            shader->tech->GetPassByIndex(p)->Apply(0, rctx->device_context);
            rctx->device_context->DrawIndexed(demo_state->mCylinderIndexCount,
                                              demo_state->mCylinderIndexOffset, demo_state->mCylinderVertexOffset);
        }
        // Draw the spheres.
        for (int i = 0; i < 10; ++i) {
            world = XMLoadFloat4x4(&demo_state->mSphereWorld[i]);
            temp = world * viewProj;
            shader->wvp_mat_var->SetMatrix(
                    reinterpret_cast<float *>(&temp));
            shader->tech->GetPassByIndex(p)->Apply(0, rctx->device_context);
            rctx->device_context->DrawIndexed(demo_state->mSphereIndexCount,
                                              demo_state->mSphereIndexOffset, demo_state->mSphereVertexOffset);
        }

        world = XMLoadFloat4x4(&demo_state->obj_transform);
        temp = world * viewProj;
        shader->wvp_mat_var->SetMatrix(
                reinterpret_cast<float *>(&temp));
        shader->tech->GetPassByIndex(p)->Apply(0, rctx->device_context);

        rctx->device_context->IASetVertexBuffers(0, 1, &demo_state->obj_vb, &stride, &offset);
        rctx->device_context->IASetIndexBuffer(demo_state->obj_ib, DXGI_FORMAT_R32_UINT, 0);
        rctx->device_context->DrawIndexed(demo_state->obj_index_count, 0, 0);
    }
}
