#pragma comment(lib, "user32")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d3dcompiler")

//system headers
#include <stdint.h>
#define WIN32_LEAN_AND_MEAN
#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <stdio.h>
#include <windows.h>
#include <string>
#include <unordered_map>
#include <vector>

// libraries
#include "Effects11\d3dx11effect.h"
#include "DirectXTK\DDSTextureLoader.h"
#include "DirectXTK\WICTextureLoader.h"
#include "imgui\imgui.h"
#include "imgui\backends\imgui_impl_win32.h"
#include "imgui\backends\imgui_impl_dx11.h"

#define TITLE "lucydxcpp"
#define ENABLE_MSAA true

// project headers
#include "lucytypes.h"
#include "proj_types.h"
#include "lucy_math.h"
#include "utils.h"
#include "obj_loader.h"
#include "exercises.h"
#include "demo_box.h"
#include "demo_hills.h"
#include "demo_shapes.h"
#include "demo_light.h"

// Define the demo struct number to run here

// 0: Box Demo
// 1: Hills Demo
// 2: Shapes Demo
#define DEMO_TO_RUN 3

#if DEMO_TO_RUN == 0
#define DEMOSTRUCT BoxDemo
#elif DEMO_TO_RUN == 1
#define DEMOSTRUCT HillsDemo
#elif DEMO_TO_RUN == 2
#define DEMOSTRUCT ShapesDemo
#elif DEMO_TO_RUN == 3
#define DEMOSTRUCT LightDemo
#endif

fn void OnMouseWheel(WPARAM w_delta, RenderContext *rctx) {
    i32 delta = (i32)w_delta;

    rctx->mouse_wheel_delta = delta;

    // negative is scrolling towards user
    rctx->cam_radius -= (f32)delta * 0.01f;
    rctx->cam_radius = math::clamp(rctx->cam_radius, 3.0f, 200.0f);
}

fn void OnMouseDown(WPARAM btnState, i32 x, i32 y, RenderContext *ctx) {
    ctx->last_mouse_pos.x = x;
    ctx->last_mouse_pos.y = y;
    SetCapture(ctx->window);
}

fn void OnMouseUp(WPARAM btnState, i32 x, i32 y, RenderContext *ctx) {
    ReleaseCapture();
}

fn void OnMouseMove(WPARAM btnState, i32 x, i32 y, RenderContext *ctx) {
    if ((btnState & MK_LBUTTON) != 0) {
        // Make each pixel correspond to a quarter of a degree.
        float dx = XMConvertToRadians(
                0.25f * static_cast<f32>(x - ctx->last_mouse_pos.x));
        float dy = XMConvertToRadians(
                0.25f * static_cast<f32>(y - ctx->last_mouse_pos.y));

        ctx->cam_yaw += dx;
        ctx->cam_pitch -= dy;

        //limiting pitch
        ctx->cam_pitch = math::clamp(ctx->cam_pitch, -1.4f, 1.4f);

    } else if ((btnState & MK_RBUTTON) != 0) {
        float dx = 0.005f * static_cast<f32>(x - ctx->last_mouse_pos.x);
        float dy = 0.005f * static_cast<f32>(y - ctx->last_mouse_pos.y);
        ctx->cam_radius += dx - dy;
        ctx->cam_radius = math::clamp(ctx->cam_radius, 3.0f, 200.0f);
    }
    ctx->last_mouse_pos.x = x;
    ctx->last_mouse_pos.y = y;
}

void OnResize(RenderContext *ctx) {
    log("onresize(). new res: %i x %i", ctx->client_width, ctx->client_height);

	// Release the old views, as they hold references to the buffers we
	// will be destroying.  Also release the old depth/stencil buffer.

	ReleaseCOM(ctx->render_target_view);
	ReleaseCOM(ctx->depth_stencil_view);
	ReleaseCOM(ctx->depth_stencil_buffer);
    
	// ReleaseCOM(ctx.de);

	// Resize the swap chain and recreate the render target view.

	HR(ctx->swapchain->ResizeBuffers(1, ctx->client_width, ctx->client_height, DXGI_FORMAT_R8G8B8A8_UNORM, 0));
	ID3D11Texture2D* backBuffer;
	HR(ctx->swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer)));
	HR(ctx->device->CreateRenderTargetView(backBuffer, 0, &ctx->render_target_view));
	ReleaseCOM(backBuffer);


    // checking for msaa support
    UINT msaa_4_quality;
    ctx->device->CheckMultisampleQualityLevels(
            DXGI_FORMAT_R8G8B8A8_UNORM, 4, &msaa_4_quality);
    assert(msaa_4_quality > 0);

	// Create the depth/stencil buffer and view.

	D3D11_TEXTURE2D_DESC depthStencilDesc;

	depthStencilDesc.Width     = ctx->client_width;
	depthStencilDesc.Height    = ctx->client_height;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.ArraySize = 1;
	depthStencilDesc.Format    = DXGI_FORMAT_D24_UNORM_S8_UINT;
#if ENABLE_MSAA
    depthStencilDesc.SampleDesc.Count   = 4;
    depthStencilDesc.SampleDesc.Quality = msaa_4_quality-1;
#else
    depthStencilDesc.SampleDesc.Count   = 1;
    depthStencilDesc.SampleDesc.Quality = 0;
#endif
	depthStencilDesc.Usage          = D3D11_USAGE_DEFAULT;
	depthStencilDesc.BindFlags      = D3D11_BIND_DEPTH_STENCIL;
	depthStencilDesc.CPUAccessFlags = 0; 
	depthStencilDesc.MiscFlags      = 0;

    HR(ctx->device->CreateTexture2D(&depthStencilDesc, 0, &ctx->depth_stencil_buffer));
    HR(ctx->device->CreateDepthStencilView(ctx->depth_stencil_buffer, 0, &ctx->depth_stencil_view));

	// Bind the render target view and depth/stencil view to the pipeline.

	ctx->device_context->OMSetRenderTargets(1, &ctx->render_target_view, ctx->depth_stencil_view);

	// Set the viewport transform.

    D3D11_VIEWPORT vp = {};
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	vp.Width    = static_cast<float>(ctx->client_width);
	vp.Height   = static_cast<float>(ctx->client_height);
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;

	ctx->device_context->RSSetViewports(1, &vp);

    // resizing proj matrix (frustrum aspect ratio)
    XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * math::PI, aspect_ratio(ctx), 1.0f, 1000.0f);
    XMStoreFloat4x4(&ctx->mProj, P);
}

LRESULT handle_resizing(i32 msg, LPARAM wparam, i32 width, i32 height, RenderContext *rctx) {

    LRESULT res = 0;

    switch (msg) {
        case WM_SIZE:
            rctx->client_width = width;
            rctx->client_height = height;

            if(rctx->client_width == 0) {
                log("why is it 0????????");
                assert(false);
            }

            switch(wparam) {
                case SIZE_MINIMIZED:
                    log("size minimized");
                    rctx->minimized = true;
                    rctx->maximized = false;
                    break;
                case SIZE_MAXIMIZED:
                    log("size maximized");
                    rctx->minimized = false;
                    rctx->maximized = true;
                    OnResize(rctx);
                    break;
                case SIZE_RESTORED:

                    // Restoring from minimized state?
                    if(rctx->minimized) {
                        log("size restored from minimized state");
                        rctx->minimized = false;
                        OnResize(rctx);
                    } 
                    // Restoring from maximized state?
                    else if (rctx->maximized) {
                        log("size restored from maximized state");
                        rctx->maximized = false;
                        OnResize(rctx);
                    } 
                    else if (rctx->resizing) {
                        // If user is dragging the resize bars, we do not resize 
                        // the buffers here because as the user continuously 
                        // drags the resize bars, a stream of WM_SIZE messages are
                        // sent to the window, and it would be pointless (and slow)
                        // to resize for each WM_SIZE message received from dragging
                        // the resize bars.  So instead, we reset after the user is 
                        // done resizing the window and releases the resize bars, which 
                        // sends a WM_EXITSIZEMOVE message.
                    } 
                    else // API call such as SetWindowPos or mSwapChain->SetFullscreenState.
                    {
                        log("size restored from other means");
                        OnResize(rctx);
                    }
                    break;
            }
            break;

        // WM_EXITSIZEMOVE is sent when the user grabs the resize bars.
        case WM_ENTERSIZEMOVE:
            log("enter size move");
            rctx->resizing  = true;
            break;

        // WM_EXITSIZEMOVE is sent when the user releases the resize bars.
        // Here we reset everything based on the new window dimensions.
        case WM_EXITSIZEMOVE:
            rctx->resizing = false;
            log("exit size move");
            OnResize(rctx);
            return 0;
    }

    return res;
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

fn LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {

    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam))
        return true;

    LRESULT result = 0;

    bool do_imgui_handling = false;

    if (ImGui::GetCurrentContext() != nullptr) {
        ImGuiIO &io = ImGui::GetIO();
        if (io.WantCaptureKeyboard || io.WantCaptureMouse) {
            do_imgui_handling = true;
        }
    }

    RenderContext *rctx = (RenderContext*)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
    if(!rctx) {
        return DefWindowProcA(hwnd, msg, wparam, lparam);
    }

    // handling when controlling imgui
    if(do_imgui_handling) {
        switch (msg) {
            case WM_KEYDOWN: {
                if (wparam == VK_ESCAPE)
                    DestroyWindow(hwnd);
                break;
            }
            case WM_DESTROY: {
                PostQuitMessage(0);
                break;
            }
            case WM_LBUTTONDOWN:
            case WM_MBUTTONDOWN:
            case WM_RBUTTONDOWN:
                rctx->last_mouse_pos.x = GET_X_LPARAM(lparam);
                rctx->last_mouse_pos.y = GET_Y_LPARAM(lparam);
                return 0;
            case WM_SIZE:
            case WM_ENTERSIZEMOVE:
            case WM_EXITSIZEMOVE:
                result = handle_resizing(msg, wparam, GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam), rctx);
                break;
            default:
                result = DefWindowProcA(hwnd, msg, wparam, lparam);
        }
    } else {
        // user handling (when not in imgui)
        switch (msg) {
            case WM_CREATE: {
                //SetWindowLongPtrA(window, GWLP_USERDATA, )
                break;
            }
            case WM_KEYDOWN: {
                if (wparam == VK_ESCAPE)
                    DestroyWindow(hwnd);
                break;
            }
            case WM_DESTROY: {
                PostQuitMessage(0);
                break;
            }
            case WM_LBUTTONDOWN:
            case WM_MBUTTONDOWN:
            case WM_RBUTTONDOWN:
                OnMouseDown(wparam, GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam), rctx);
                return 0;
            case WM_LBUTTONUP:
            case WM_MBUTTONUP:
            case WM_RBUTTONUP:
                OnMouseUp(wparam, GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam), rctx);
                return 0;
            case WM_MOUSEMOVE:
                OnMouseMove(wparam, GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam), rctx);
                return 0;
            case WM_MOUSEWHEEL:
                OnMouseWheel(GET_WHEEL_DELTA_WPARAM(wparam), rctx);
                break;
            case WM_SIZE:
            case WM_ENTERSIZEMOVE:
            case WM_EXITSIZEMOVE:
                result = handle_resizing(msg, wparam, GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam), rctx);
                break;
            default:
                result = DefWindowProcA(hwnd, msg, wparam, lparam);
        }
    }

    return result;
}

// todo: unify swapchain creation both on init and on resize by putting it here
LucyResult recreate_swapchain(Arena *arena, RenderContext *out_render_ctx) {
    return LRES_OK;
}

LucyResult render_context_init(Arena *arena, HINSTANCE hInstance, RenderContext *out_render_ctx) {

    out_render_ctx->client_width = WINDOW_WIDTH;
    out_render_ctx->client_height = WINDOW_HEIGHT;

    WNDCLASSEXA wndClassEx = {sizeof(wndClassEx)};
    wndClassEx.lpfnWndProc = WndProc;
    wndClassEx.lpszClassName = TITLE;

    RegisterClassExA(&wndClassEx);

    RECT initialRect = {
            .right = out_render_ctx->client_width,
            .bottom = out_render_ctx->client_height,
    };

    AdjustWindowRectEx(&initialRect, WS_OVERLAPPEDWINDOW, FALSE, WS_EX_OVERLAPPEDWINDOW);
    LONG initial_width = initialRect.right - initialRect.left;
    LONG initial_height = initialRect.bottom - initialRect.top;

    HWND window = CreateWindowExA(WS_EX_OVERLAPPEDWINDOW,
                             TITLE, TITLE,
                             WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                             CW_USEDEFAULT, CW_USEDEFAULT,
                             initial_width,
                             initial_height,
                             nullptr, nullptr, hInstance, nullptr);

    if (!window) {
        MessageBoxA(nullptr, "CreateWindowEx failed", "Fatal Error", MB_OK);
        return LRES_FAIL;
    }

    SetWindowLongPtrA(window, GWLP_USERDATA, (LONG_PTR)out_render_ctx);


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
    hres = dxgi_factory->CreateSwapChainForHwnd(device, window, &sd, 0, 0, &swapchain);
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

    out_render_ctx->window = window;
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

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {

    // allocate all memory for the whole game
    void *base_mem = VirtualAlloc(0, TOTAL_MEM, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    assert(base_mem != 0);

    ProgramState program_state = {
            .base_mem = base_mem,
            .total_mem_size = TOTAL_MEM,
    };

    #ifdef DEBUG
    log("u are in debug");
    #else
    log("u are in release");
    #endif

    //now i make an arena..?
    Arena big_arena = {
            .buf = (u8 *) base_mem,
            .offset = 0,
            .size = TOTAL_MEM,
    };

    LucyResult lres;

    RenderContext rctx = {};
    lres = render_context_init(&big_arena, hInstance, &rctx);
    assert(lres == 0);

    // initializing whatever demo we #included
    DEMOSTRUCT demo_state = {};
    lres = demo_init(&big_arena, &rctx, &demo_state);
    assert(lres == LRES_OK);

    //initializing imgui
    // Create a Dear ImGui context, setup some options
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;// Enable some options

    // Initialize Platform + Renderer backends (here: using imgui_impl_win32.cpp + imgui_impl_dx11.cpp)
    ImGui_ImplWin32_Init(rctx.window);
    ImGui_ImplDX11_Init(rctx.device, rctx.device_context);

    // main loop state
    bool is_running = true;

    i64 perf_freq;
    QueryPerformanceFrequency((LARGE_INTEGER *) &perf_freq);
    f64 seconds_per_count = 1.0 / (f64) perf_freq;
    i64 time_last;
    QueryPerformanceCounter((LARGE_INTEGER *) &time_last);

    while (is_running) {
        MSG msg;

        while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT)
                is_running = false;
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }

        i64 time_now;
        QueryPerformanceCounter((LARGE_INTEGER *) &time_now);
        i64 dt = time_now - time_last;
        f32 dt_sec = (f32)((f64)dt * seconds_per_count);
        time_last = time_now;

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        demo_update_render(&rctx, &demo_state, dt_sec);
        ImGui::Render();
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        rctx.swapchain->Present(1, 0);
    }

    return 0;
}