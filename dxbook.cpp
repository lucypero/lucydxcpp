#pragma comment(lib, "user32")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d3dcompiler")

#include <stdint.h>

#define WIN32_LEAN_AND_MEAN
#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <windows.h>
#include <stdio.h>

// use this later
#if 0
#include "DirectXMath.h"
#include "DirectXPackedVector.h"
#endif

#include <cassert>

// project source files
#include "utils.cpp"
#include "exercises.cpp"

#define TITLE "lucydxcpp"
#define ENABLE_MSAA true

const u32 WINDOW_WIDTH = 1280;
const u32 WINDOW_HEIGHT = 720;

// total memory allocated
#define TOTAL_MEM (10 * 1024)

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    LRESULT result = 0;
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
        default:
            result = DefWindowProcA(hwnd, msg, wparam, lparam);
    }
    return result;
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
        return GetLastError();
    }

    // initializing DX

    UINT create_device_flags = 0;

#ifdef DEBUG
    create_device_flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    ID3D11Device *base_device;
    ID3D11DeviceContext *base_device_context;

    D3D_FEATURE_LEVEL ret_feature_level;

    HRESULT res = D3D11CreateDevice(
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
    assert(res == 0);
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
    enum_adapters(&big_arena, dxgi_factory);

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
    res = dxgi_factory->CreateSwapChainForHwnd(device, window, &sd, 0, 0, &swapchain);
    assert(res == 0);

    // telling dxgi to not mess with the window event queue (basically disable auto alt+enter for fullscreen)
    dxgi_factory->MakeWindowAssociation(window, DXGI_MWA_NO_WINDOW_CHANGES);

    // creating the view to the back buffer
    ID3D11RenderTargetView *render_target_view;
    ID3D11Texture2D *back_buffer;
    swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void **>(&back_buffer));
    res = device->CreateRenderTargetView(back_buffer, 0, &render_target_view);
    assert(res == 0);

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

    res = device->CreateTexture2D(&depth_desc, 0, &depth_stencil_buffer);
    assert( res == 0);
    res = device->CreateDepthStencilView(depth_stencil_buffer, 0, &depth_stencil_view);
    assert( res == 0);

    device_context->OMSetRenderTargets(1, &render_target_view, depth_stencil_view);

    D3D11_VIEWPORT vp = {};
    vp.TopLeftX = 0.0f;
    vp.TopLeftY = 0.0f;
    vp.Width = static_cast<float>(WINDOW_WIDTH);
    vp.Height = static_cast<float>(WINDOW_HEIGHT);
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;

    device_context->RSSetViewports(1, &vp);


    // main loop
    bool is_running = true;

    i64 perf_freq;
    QueryPerformanceFrequency((LARGE_INTEGER*)&perf_freq);
    f64 seconds_per_count = 1.0 / (f64) perf_freq;
    i64 time_last;
    QueryPerformanceCounter((LARGE_INTEGER*)&time_last);

    while (is_running) {
        MSG msg;

        while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT || msg.message == WM_KEYDOWN)
                is_running = false;
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }

        i64 time_now;
        QueryPerformanceCounter((LARGE_INTEGER*)&time_now);
        i64 dt = time_now - time_last;
        time_last = time_now;

//        Sleep(1000);

//        log("dt is %i, in miliseconds it is: %f", dt, ((f64)dt * seconds_per_count) * 1000);


        // render a frame here w dx11
        /* clear the back buffer to cornflower blue for the new frame */
        float background_colour[4] = {0x64 / 255.0f, 0x95 / 255.0f, 0xED / 255.0f, 1.0f};
        device_context->ClearRenderTargetView(render_target_view, background_colour);

        // update and draw here

        swapchain->Present(1, 0);
    }

    return 0;
}