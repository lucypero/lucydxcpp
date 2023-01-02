#pragma comment(lib, "user32")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d3dcompiler")

#include <stdint.h>

#define WIN32_LEAN_AND_MEAN
#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <stdio.h>
#include <windows.h>

// libraries
#include "Effects11\d3dx11effect.h"
#include "imgui\imgui.h"
#include "imgui\backends\imgui_impl_win32.h"
#include "imgui\backends\imgui_impl_dx11.h"

#include "DirectXMath.h"
#include "DirectXPackedVector.h"
using namespace DirectX;

#include <cassert>

#include <vector>

#define TITLE "lucydxcpp"
#define ENABLE_MSAA true

// Define the demo struct number to run here

// 0: Box Demo
// 1: Hills Demo
// 2: Shapes Demo
#define DEMO_TO_RUN 2

#if DEMO_TO_RUN == 0
#define DEMOSTRUCT BoxDemo
#elif DEMO_TO_RUN == 1
#define DEMOSTRUCT HillsDemo
#elif DEMO_TO_RUN == 2
#define DEMOSTRUCT ShapesDemo
#endif

#include "lucytypes.cpp"

// Project types
#include "proj_types.cpp"

// project source files
#include "math.cpp"
#include "utils.cpp"
#include "exercises.cpp"

#include "demo_box.cpp"
#include "demo_hills.cpp"
#include "demo_shapes.cpp"

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

            default:
                result = DefWindowProcA(hwnd, msg, wparam, lparam);
        }
    }

    return result;
}

LucyResult render_context_init(HINSTANCE hInstance, RenderContext *out_render_ctx) {

    WNDCLASSEXA wndClassEx = {sizeof(wndClassEx)};
    wndClassEx.lpfnWndProc = WndProc;
    wndClassEx.lpszClassName = TITLE;

    RegisterClassExA(&wndClassEx);

    RECT initialRect = {
            .right = WINDOW_WIDTH,
            .bottom = WINDOW_HEIGHT,
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
    dxgi_factory->MakeWindowAssociation(window, DXGI_MWA_NO_WINDOW_CHANGES);

    // creating the view to the back buffer
    ID3D11RenderTargetView *render_target_view;
    ID3D11Texture2D *back_buffer;
    swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void **>(&back_buffer));
    hres = device->CreateRenderTargetView(back_buffer, 0, &render_target_view);
    assert(hres == 0);

    // creating depth/stencil buffer

    D3D11_TEXTURE2D_DESC depth_desc = {};
    depth_desc.Width = WINDOW_WIDTH;
    depth_desc.Height = WINDOW_HEIGHT;
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
    vp.Width = static_cast<float>(WINDOW_WIDTH);
    vp.Height = static_cast<float>(WINDOW_HEIGHT);

    //try this once you have something rendered..
    //    vp.TopLeftX = 100.0f;
    //    vp.TopLeftY = 100.0f;
    //    vp.Width = 500.0f;
    //    vp.Height = 400.0f;

    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;

    device_context->RSSetViewports(1, &vp);

    // INITIALIZING DX /END ------------------------

    out_render_ctx->window = window;
    out_render_ctx->device = device;
    out_render_ctx->device_context = device_context;
    out_render_ctx->swapchain = swapchain;
    out_render_ctx->render_target_view = render_target_view;
    out_render_ctx->depth_stencil_view = depth_stencil_view;

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

    //now i make an arena..?
    Arena big_arena = {
            .buf = (u8 *) base_mem,
            .offset = 0,
            .size = TOTAL_MEM,
    };

    LucyResult lres;

    RenderContext rctx = {};
    lres = render_context_init(hInstance, &rctx);
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
        time_last = time_now;

        //        i64 dt = time_now - time_last;
        // log("dt is %i, in miliseconds it is: %f", dt, ((f64)dt * seconds_per_count) * 1000);

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        demo_update_render(&rctx, &demo_state);
        ImGui::Render();
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        rctx.swapchain->Present(1, 0);
    }

    return 0;
}