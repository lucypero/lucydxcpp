#include "renderer.h"

#include "DirectXTK\DDSTextureLoader.h"
#include "DirectXTK\WICTextureLoader.h"

LucyResult render_context_init(Arena *arena, RenderContext *out_render_ctx) {

    // INITIALIZING DX ------------------------

    UINT create_device_flags = 0;

#ifdef DEBUG
    create_device_flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    ID3D11Device *base_device;
    ID3D11DeviceContext *base_device_context;

    D3D_FEATURE_LEVEL ret_feature_level;

    HRESULT hres = D3D11CreateDevice(
            0,
            D3D_DRIVER_TYPE_HARDWARE,
            0,
            create_device_flags,
            0,
            0,
            D3D11_SDK_VERSION,
            &base_device,
            &ret_feature_level,
            &base_device_context);
    assert(hres == 0);
    assert(ret_feature_level == D3D_FEATURE_LEVEL_11_0);

    // getting device1, dc1 and factory2
    ID3D11Device1 *device;
    base_device->QueryInterface(__uuidof(ID3D11Device1), reinterpret_cast<void **>(&device));
    ID3D11DeviceContext1 *device_context;
    base_device_context->QueryInterface(__uuidof(ID3D11DeviceContext1), reinterpret_cast<void **>(&device_context));

    IDXGIDevice1 *dxgi_device;
    device->QueryInterface(__uuidof(IDXGIDevice1), reinterpret_cast<void **>(&dxgi_device));
    IDXGIAdapter *dxgi_adapter;
    dxgi_device->GetAdapter(&dxgi_adapter);
    IDXGIFactory2 *dxgi_factory;
    dxgi_adapter->GetParent(__uuidof(IDXGIFactory2), reinterpret_cast<void **>(&dxgi_factory));

    //enum adapters
    // (for exercise)
    //    enum_adapters(&big_arena, dxgi_factory);

    // checking for msaa support
    UINT msaa_4_quality;
    device->CheckMultisampleQualityLevels(
            DXGI_FORMAT_R8G8B8A8_UNORM, 4, &msaa_4_quality);
    assert(msaa_4_quality > 0);

    // creating swapchain
    DXGI_SWAP_CHAIN_DESC1 sd = {};
    sd.Width = 0;
    sd.Height = 0;
    sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.Stereo = false;
#if ENABLE_MSAA
    sd.SampleDesc.Count = 4;
    sd.SampleDesc.Quality = msaa_4_quality - 1;
#else
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
#endif
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = 1;
    sd.Scaling = DXGI_SCALING_STRETCH;
    // this is kinda deprectated but we'll keep using it for now bc the new thing requires manual MSAA
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    sd.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    sd.Flags = 0;

    IDXGISwapChain1 *swapchain;
    hres = dxgi_factory->CreateSwapChainForHwnd(device, out_render_ctx->window, &sd, 0, 0, &swapchain);
    assert(hres == 0);

    // telling dxgi to not mess with the window event queue (basically disable auto alt+enter for fullscreen)
    // dxgi_factory->MakeWindowAssociation(window, DXGI_MWA_NO_WINDOW_CHANGES);

    // creating the view to the back buffer
    ID3D11RenderTargetView *render_target_view;
    ID3D11Texture2D *back_buffer;
    swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void **>(&back_buffer));
    hres = device->CreateRenderTargetView(back_buffer, 0, &render_target_view);
    assert(hres == 0);
    ReleaseCOM(back_buffer);

    // creating depth/stencil buffer

    D3D11_TEXTURE2D_DESC depth_desc = {};
    depth_desc.Width = out_render_ctx->client_width;
    depth_desc.Height = out_render_ctx->client_height;
    depth_desc.MipLevels = 1;
    depth_desc.ArraySize = 1;
    depth_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;

#if ENABLE_MSAA
    depth_desc.SampleDesc.Count = 4;
    depth_desc.SampleDesc.Quality = msaa_4_quality - 1;
#else
    depth_desc.SampleDesc.Count = 1;
    depth_desc.SampleDesc.Quality = 0;
#endif
    depth_desc.Usage = D3D11_USAGE_DEFAULT;
    depth_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    depth_desc.CPUAccessFlags = 0;
    depth_desc.MiscFlags = 0;

    ID3D11Texture2D *depth_stencil_buffer;
    ID3D11DepthStencilView *depth_stencil_view;

    hres = device->CreateTexture2D(&depth_desc, 0, &depth_stencil_buffer);
    assert(hres == 0);
    hres = device->CreateDepthStencilView(depth_stencil_buffer, 0, &depth_stencil_view);
    assert(hres == 0);

    device_context->OMSetRenderTargets(1, &render_target_view, depth_stencil_view);

    // viewport

    D3D11_VIEWPORT vp = {};
    vp.TopLeftX = 0.0f;
    vp.TopLeftY = 0.0f;
    vp.Width = static_cast<float>(out_render_ctx->client_width);
    vp.Height = static_cast<float>(out_render_ctx->client_height);

    //try this once you have something rendered..
    //    vp.TopLeftX = 100.0f;
    //    vp.TopLeftY = 100.0f;
    //    vp.Width = 500.0f;
    //    vp.Height = 400.0f;

    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;

    device_context->RSSetViewports(1, &vp);

    // initializing everything dx related after dx init

    // initializing basic_effect
    BasicEffect basic_effect = {};

    u64 shader_checkpoint = arena_save(arena);

    Buf basic_fx_buf;
    LucyResult lres; 
    lres = read_whole_file(arena, "build\\basic.fxo", &basic_fx_buf);
    // lres = read_whole_file(arena, "build\\toon.fxo", &basic_fx_buf);

    if(lres != LRES_OK)
        return lres;

    hres = D3DX11CreateEffectFromMemory(
            basic_fx_buf.buf,
            basic_fx_buf.size,
            0, device,
            &basic_effect.effect);
    assert(hres == 0);

    // getting techs and variables
	basic_effect.Light1Tech        = basic_effect.effect->GetTechniqueByName("Light1");
	basic_effect.Light2Tech        = basic_effect.effect->GetTechniqueByName("Light2");
	basic_effect.Light3Tech        = basic_effect.effect->GetTechniqueByName("Light3");
	basic_effect.WorldViewProj     = basic_effect.effect->GetVariableByName("gWorldViewProj")->AsMatrix();
	basic_effect.World             = basic_effect.effect->GetVariableByName("gWorld")->AsMatrix();
	basic_effect.WorldInvTranspose = basic_effect.effect->GetVariableByName("gWorldInvTranspose")->AsMatrix();
	basic_effect.EyePosW           = basic_effect.effect->GetVariableByName("gEyePosW")->AsVector();
	basic_effect.DirLights         = basic_effect.effect->GetVariableByName("gDirLights");
	basic_effect.point_light       = basic_effect.effect->GetVariableByName("gPointLight");
	basic_effect.Mat               = basic_effect.effect->GetVariableByName("gMaterial");
	basic_effect.TexTransform      = basic_effect.effect->GetVariableByName("gTexTransform")->AsMatrix();
	basic_effect.texture           = basic_effect.effect->GetVariableByName("gDiffuseMap")->AsShaderResource();
	basic_effect.texture_specular  = basic_effect.effect->GetVariableByName("gSpecularMap")->AsShaderResource();

    out_render_ctx->basic_effect = basic_effect;

    // initializing input layouts
    D3D11_INPUT_ELEMENT_DESC inputElementDesc[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    D3DX11_PASS_DESC pass_desc;
    basic_effect.Light1Tech->GetPassByIndex(0)->GetDesc(&pass_desc);

    hres = device->CreateInputLayout(
            inputElementDesc,
            arrsize(inputElementDesc),
            pass_desc.pIAInputSignature,
            pass_desc.IAInputSignatureSize,
            &out_render_ctx->il_pos_normal_uv);
    assert(hres == 0);

    arena_restore(arena, shader_checkpoint);

    // initialize textures

    ID3D11Resource *texture;
    ID3D11ShaderResourceView *texture_view;

    // HR(CreateDDSTextureFromFile(device, L"Textures\\darkbrickdxt1.dds", &texture, &texture_view));
    HR(CreateWICTextureFromFile(device, L"Textures\\wood\\wood_albedo.tif", &texture, &texture_view));
    out_render_ctx->srv_diffuse = texture_view;

    HR(CreateWICTextureFromFile(device, L"Textures\\wood\\wood_roughness.tif", &texture, &texture_view));
    out_render_ctx->srv_specular = texture_view;

    // initialize samplers??


    // initialize rasterizer states

    D3D11_RASTERIZER_DESC wireframeDesc = {};
    wireframeDesc.FillMode = D3D11_FILL_WIREFRAME;
    wireframeDesc.CullMode = D3D11_CULL_BACK;
    wireframeDesc.FrontCounterClockwise = false;
    wireframeDesc.DepthClipEnable = true;

    HR(device->CreateRasterizerState(&wireframeDesc, &out_render_ctx->mWireframeRS));
    wireframeDesc.FillMode = D3D11_FILL_SOLID;
    HR(device->CreateRasterizerState(&wireframeDesc, &out_render_ctx->mSolidRS));

    // INITIALIZING DX /END ------------------------


    // setting matrixes that don't need to be set every frame...
    // (proj matrix)
    //  TODO: this has to be set when u resize too
    XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * math::PI, aspect_ratio(out_render_ctx), 1.0f, 1000.0f);
    XMStoreFloat4x4(&out_render_ctx->mProj, P);

    out_render_ctx->device = device;
    out_render_ctx->device_context = device_context;
    out_render_ctx->swapchain = swapchain;
    out_render_ctx->render_target_view = render_target_view;
    out_render_ctx->depth_stencil_view = depth_stencil_view;
    out_render_ctx->depth_stencil_buffer = depth_stencil_buffer;

    out_render_ctx->last_mouse_pos = {};
    out_render_ctx->cam_yaw = 0.0f;
    out_render_ctx->cam_pitch = 0.0f;
    out_render_ctx->cam_radius = 5.0f;

    return LRES_OK;
}

// todo: unify swapchain creation both on init and on resize by putting it here
LucyResult recreate_swapchain(Arena *arena, RenderContext *out_render_ctx) {
    return LRES_OK;
}

f32 aspect_ratio(RenderContext *rctx) {
	return static_cast<float>(rctx->client_width) / rctx->client_height;
}

// todo: this doesn't compile
//  make a substruct w shader state and use that instead of a macro...

LucyResult setup_color_shader(Arena *arena, RenderContext *rctx, ShaderFile shader_file, Shader *out_shader) {

    u64 checkpoint = arena_save(arena);

    Buf color_fx_buf;
    LucyResult lres; 

    switch (shader_file){
        case ShaderFile::Color: {
            lres = read_whole_file(arena, "build\\color.fxo", &color_fx_buf);
        }; break;
        case ShaderFile::ColorTrippy: {
            lres = read_whole_file(arena, "build\\color_trippy.fxo", &color_fx_buf);
        }; break;
    };
    
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

    ID3DX11EffectScalarVariable *time_var = effect->GetVariableByName("gTime")->AsScalar();
    assert(time_var->IsValid());

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

    out_shader->tech = tech;
    out_shader->mInputLayout = input_layout;
    out_shader->wvp_mat_var = wvp_mat_var;
    out_shader->time_var = time_var;

    return LRES_OK;
}