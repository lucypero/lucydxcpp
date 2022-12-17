// HILLS DEMO

namespace GeometryGenerator {

    struct Vertex {
        XMFLOAT3 pos;
        XMFLOAT3 normal;
        XMFLOAT3 tangent_u;
        XMFLOAT3 tex_c;
    };

    struct MeshData {
        std::vector<Vertex> vertices;
        std::vector<u32> indices;
    };

    // m = amount of vertices along the x axis
    // n = amount of vertices along the z axis
    // total vertices: m * n
    void create_grid(f32 width, f32 depth, u32 m, u32 n, MeshData *mesh_data) {

        u32 vertex_count = m * n;
        // total amount of triangles
        u32 face_count = (m-1) * (n-1) * 2;

        //creating the vertices
        f32 half_width = 0.5f*width;
        f32 half_depth = 0.5f*depth;
        f32 dx = width / (n-1);
        f32 dz = depth / (m-1);
        f32 du = 1.0f / (n-1);
        f32 dv = 1.0f / (m-1);

        mesh_data->vertices.resize(vertex_count);
        for(u32 i = 0; i < m; ++i) {
            float z = half_depth - i * dz;
            for (u32 j = 0; j < n; ++j) {
                float x = -half_width + j * dx;
                mesh_data->vertices[i * n + j].pos = XMFLOAT3(x, 0.0f, z);
                // Ignore for now, used for lighting.
                mesh_data->vertices[i * n + j].normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
                mesh_data->vertices[i * n + j].tangent_u = XMFLOAT3(1.0f, 0.0f, 0.0f);
                // Ignore for now, used for texturing.
                mesh_data->vertices[i * n + j].tex_c.x = j * du;
                mesh_data->vertices[i * n + j].tex_c.y = i * dv;
            }
        }

        mesh_data->indices.resize(face_count*3); // 3 indices per face
        // Iterate over each quad and compute indices.
        u32 k = 0;
        for(u32 i = 0; i < m-1; ++i)
        {
            for(u32 j = 0; j < n-1; ++j)
            {
                mesh_data->indices[k] = i*n+j;
                mesh_data->indices[k+1] = i*n+j+1;
                mesh_data->indices[k+2] = (i+1)*n+j;
                mesh_data->indices[k+3] = (i+1)*n+j;
                mesh_data->indices[k+4] = i*n+j+1;
                mesh_data->indices[k+5] = (i+1)*n+j+1;
                k += 6; // next quad
            }
        }
    }
}

struct DemoState {
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


fn f32 get_height(f32 x, f32 z) {
    return 0.5f * (z * sinf(0.1f * x) + x * cosf(0.1f * z));
}

void regen_vertices(const DemoState *demo_state, const GeometryGenerator::MeshData *grid, std::vector<Vertex> *vertices) {
    for(size_t i = 0; i < grid->vertices.size(); ++i) {
        XMFLOAT3 p = grid->vertices[i].pos;

        p.y = demo_state->height_cofactor * get_height(p.x, p.z);

        (*vertices)[i].Pos = p;

        // Color the vertex based on its height

        if( p.y < -10.0f) {
            // Sandy beach color
            (*vertices)[i].Color = XMFLOAT4(1.0f, 0.96f, 0.62f, 1.0f);
        } else if(p.y < 5.0f) {
            // Light yellow-green
            (*vertices)[i].Color = XMFLOAT4(0.48f, 0.77f, 0.46f, 1.0f);
        } else if(p.y < 12.0f) {
            // Dark yellow-green
            (*vertices)[i].Color = XMFLOAT4(0.1f, 0.48f, 0.19f, 1.0f);
        } else if(p.y < 20.0f) {
            // Dark brown.
            (*vertices)[i].Color = XMFLOAT4(0.45f, 0.39f, 0.34f, 1.0f);
        } else {
            // White snow.
            (*vertices)[i].Color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
        }
    }
}

fn LucyResult demo_init(Arena *arena, RenderContext *rctx, DemoState *out_demo_state) {

    out_demo_state->height_cofactor = 1.0f;

    //writing to global(sorry, change later!!)
    cam_radius = 300.0f;

    // VARIABLES like WVP matrix and stuff ------------

    XMFLOAT4X4 mat_world;
    XMFLOAT4X4 mat_view;
    XMFLOAT4X4 mat_proj;

    XMMATRIX I = XMMatrixIdentity();
    XMStoreFloat4x4(&mat_world, I);
    XMStoreFloat4x4(&mat_view, I);
    XMStoreFloat4x4(&mat_proj, I);

    // VARIABLES /end ------------------------------

    // CREATING VERTEX GRID! --

    GeometryGenerator::MeshData grid;

    GeometryGenerator::create_grid(160.0f, 160.0f, 500, 500, &grid);

    out_demo_state->grid_index_count = (u32)grid.indices.size();

    //
    // Extract the vertex elements we are interested and apply the
    // height function to each vertex. In addition, color the vertices
    // based on their height so we have sandy looking beaches, grassy low
    // hills, and snow mountain peaks.
    //

    std::vector<Vertex> vertices(grid.vertices.size());

    regen_vertices(out_demo_state, &grid, &vertices);

    D3D11_BUFFER_DESC vbd = {};
    vbd.Usage = D3D11_USAGE_DYNAMIC;
    vbd.ByteWidth = sizeof(Vertex) * (u32)grid.vertices.size();
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    D3D11_SUBRESOURCE_DATA vinit_data = {};
    vinit_data.pSysMem = &vertices[0];
    HRESULT hres = rctx->device->CreateBuffer(&vbd, &vinit_data, &out_demo_state->hills_vb);
    assert(hres == 0);

    // Pack the indices of all the meshes into one index buffer.
    D3D11_BUFFER_DESC ibd = {};
    ibd.Usage = D3D11_USAGE_IMMUTABLE;
    ibd.ByteWidth = sizeof(u32) * out_demo_state->grid_index_count;
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    D3D11_SUBRESOURCE_DATA iinit_data = {};
    iinit_data.pSysMem = &grid.indices[0];
    hres = rctx->device->CreateBuffer(&ibd, &iinit_data, &out_demo_state->hills_ib);
    assert(hres == 0);

    // CREATING VERTEX GRID! /end ---

    // SHADER LOADING ------------------------

    u64 checkpoint = arena_save(arena);

    Buf color_fx_buf;
    LucyResult lres = read_whole_file(arena, "build\\color.fxo", &color_fx_buf);
    assert(lres == LRES_OK);

    ID3DX11Effect *effect;

    hres = D3DX11CreateEffectFromMemory(
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

    // setting matrixes that don't need to be set every frame...
    // (proj matrix)
    //  TODO: this has to be set when u resize too
    XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * math::PI, WINDOW_ASPECT_RATIO, 1.0f, 1000.0f);
    XMStoreFloat4x4(&mat_proj, P);

    // setting demo state
    out_demo_state->tech = tech;
    out_demo_state->wvp_mat_var = wvp_mat_var;
    out_demo_state->input_layout = input_layout;
    out_demo_state->mat_world = mat_world;
    out_demo_state->mat_view = mat_view;
    out_demo_state->mat_proj = mat_proj;
    out_demo_state->grid = grid;

    return LRES_OK;
}

// update and render (runs every frame)
fn void demo_update_render(RenderContext *rctx, DemoState *demo_state) {

    //imgui stuff
    ImGui::DragFloat("height factor", &demo_state->height_cofactor, 0.1f, 0.0f, 10.0f);

    // /imgui


    XMMATRIX cam_rot_mat = XMMatrixRotationQuaternion(XMQuaternionRotationRollPitchYaw(cam_pitch, cam_yaw, 0.0f));
    XMVECTOR cam_pos_start = XMVectorSet(0.0f, 0.0f, -1.0f * cam_radius, 1.0f);

    XMVECTOR cam_pos = XMVector3Transform(cam_pos_start, cam_rot_mat);
    XMVECTOR cam_target = XMVectorZero();
    XMVECTOR cam_up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    XMMATRIX view_mat = XMMatrixLookAtLH(cam_pos, cam_target, cam_up);
    XMStoreFloat4x4(&demo_state->mat_view, view_mat);

    // Update program state /end ---------------

    // Draw ---------------
    rctx->device_context->ClearRenderTargetView(rctx->render_target_view, reinterpret_cast<const f32 *>(&Colors::Blue));
    rctx->device_context->ClearDepthStencilView(rctx->depth_stencil_view, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    rctx->device_context->IASetInputLayout(demo_state->input_layout);
    rctx->device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    u32 stride = sizeof(Vertex);
    u32 offset = 0;
    rctx->device_context->IASetVertexBuffers(0, 1, &demo_state->hills_vb, &stride, &offset);
    rctx->device_context->IASetIndexBuffer(demo_state->hills_ib, DXGI_FORMAT_R32_UINT, 0);

    //Set constants
    XMMATRIX world = XMLoadFloat4x4(&demo_state->mat_world);
    XMMATRIX view = XMLoadFloat4x4(&demo_state->mat_view);
    XMMATRIX proj = XMLoadFloat4x4(&demo_state->mat_proj);
    XMMATRIX wvp = world * view * proj;
    demo_state->wvp_mat_var->SetMatrix(reinterpret_cast<float *>(&wvp));

    // manipulating  vertices
    D3D11_MAPPED_SUBRESOURCE mapped_resource = {};

    std::vector<Vertex> vertices(demo_state->grid.vertices.size());
    regen_vertices(demo_state, &demo_state->grid, &vertices);

    rctx->device_context->Map(demo_state->hills_vb, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource);
    memcpy(mapped_resource.pData, &vertices[0], sizeof(Vertex) * demo_state->grid.vertices.size());
    rctx->device_context->Unmap(demo_state->hills_vb, 0);

    // Drawing indexes
    D3DX11_TECHNIQUE_DESC tech_desc;
    demo_state->tech->GetDesc(&tech_desc);
    for (u32 p = 0; p < tech_desc.Passes; ++p) {
        demo_state->tech->GetPassByIndex(p)->Apply(0, rctx->device_context);
        rctx->device_context->DrawIndexed(demo_state->grid_index_count, 0, 0);
    }
}
