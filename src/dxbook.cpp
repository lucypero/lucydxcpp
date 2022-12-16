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

// project source files
#include "utils.cpp"
#include "exercises.cpp"
#include "math.cpp"

#define TITLE "lucydxcpp"
#define ENABLE_MSAA true

const u32 WINDOW_WIDTH = 1280;
const u32 WINDOW_HEIGHT = 720;
const f32 WINDOW_ASPECT_RATIO = (f32) WINDOW_WIDTH / (f32) WINDOW_HEIGHT;

struct Vertex {
    XMFLOAT3 Pos;
    XMFLOAT4 Color;
};

namespace Colors {
    XMGLOBALCONST XMFLOAT4 White = {1.0f, 1.0f, 1.0f, 1.0f};
    XMGLOBALCONST XMFLOAT4 Black = {0.0f, 0.0f, 0.0f, 1.0f};
    XMGLOBALCONST XMFLOAT4 Red = {1.0f, 0.0f, 0.0f, 1.0f};
    XMGLOBALCONST XMFLOAT4 Green = {0.0f, 1.0f, 0.0f, 1.0f};
    XMGLOBALCONST XMFLOAT4 Blue = {0.0f, 0.0f, 1.0f, 1.0f};
    XMGLOBALCONST XMFLOAT4 Yellow = {1.0f, 1.0f, 0.0f, 1.0f};
    XMGLOBALCONST XMFLOAT4 Cyan = {0.0f, 1.0f, 1.0f, 1.0f};
    XMGLOBALCONST XMFLOAT4 Magenta = {1.0f, 0.0f, 1.0f, 1.0f};
}// namespace Colors

// total memory allocated
#define TOTAL_MEM (1000 * 1024)

#define GET_X_LPARAM(lp) ((int) (short) LOWORD(lp))
#define GET_Y_LPARAM(lp) ((int) (short) HIWORD(lp))

// Gotta make these globals for now so i can access them from WndProc
POINT last_mouse_pos = {};
HWND window = {};
f32 cam_yaw = 0.0f;
f32 cam_pitch = 0.0f;
f32 cam_radius = 5.0f;

fn void OnMouseDown(WPARAM btnState, i32 x, i32 y) {
    last_mouse_pos.x = x;
    last_mouse_pos.y = y;
    SetCapture(window);
}

fn void OnMouseUp(WPARAM btnState, i32 x, i32 y) {
    ReleaseCapture();
}

fn void OnMouseMove(WPARAM btnState, i32 x, i32 y) {
    if ((btnState & MK_LBUTTON) != 0) {
        // Make each pixel correspond to a quarter of a degree.
        float dx = XMConvertToRadians(
                0.25f * static_cast<f32>(x - last_mouse_pos.x));
        float dy = XMConvertToRadians(
                0.25f * static_cast<f32>(y - last_mouse_pos.y));
        cam_yaw += dx;
        cam_pitch -= dy;

        //limiting pitch
        cam_pitch = math::clamp(cam_pitch, -1.4, 1.4);

    } else if ((btnState & MK_RBUTTON) != 0) {
        float dx = 0.005f * static_cast<f32>(x - last_mouse_pos.x);
        float dy = 0.005f * static_cast<f32>(y - last_mouse_pos.y);
        cam_radius += dx - dy;
        cam_radius = math::clamp(cam_radius, 3.0f, 15.0f);
    }
    last_mouse_pos.x = x;
    last_mouse_pos.y = y;
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

fn LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {

    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam))
        return true;

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
        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN:
            OnMouseDown(wparam, GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));
            return 0;
        case WM_LBUTTONUP:
        case WM_MBUTTONUP:
        case WM_RBUTTONUP:
            OnMouseUp(wparam, GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));
            return 0;
        case WM_MOUSEMOVE:
            OnMouseMove(wparam, GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));
            return 0;

        default:
            result = DefWindowProcA(hwnd, msg, wparam, lparam);
    }
    return result;
}

fn int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {

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

    window = CreateWindowExA(WS_EX_OVERLAPPEDWINDOW,
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

    u64 checkpoint = arena_save(&big_arena);

    Buf color_fx_buf;
    LucyResult l_res = read_whole_file(&big_arena, "build\\color.fxo", &color_fx_buf);
    assert(l_res == LRES_OK);

    ID3DX11Effect *effect;

    hres = D3DX11CreateEffectFromMemory(
            color_fx_buf.buf,
            color_fx_buf.size,
            0, device,
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

    hres = device->CreateInputLayout(
            inputElementDesc,
            arrsize(inputElementDesc),
            pass_desc.pIAInputSignature,
            pass_desc.IAInputSignatureSize,
            &input_layout);
    assert(hres == 0);

    // don't need the shader buffer anymore. color_fx_buf is invalid now.
    arena_restore(&big_arena, checkpoint);

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
    vbd.Usage = D3D11_USAGE_IMMUTABLE;
    vbd.ByteWidth = sizeof(Vertex) * vert_count;
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vinit_data = {};
    vinit_data.pSysMem = vertices;
    hres = device->CreateBuffer(&vbd, &vinit_data, &box_vb);
    assert(hres == 0);

    // create index buffer
    u32 indices[] = {
            //front face
            0,
            1,
            2,
            0,
            2,
            3,

            //back face
            4,
            6,
            5,
            4,
            7,
            6,

            // left face
            4,
            5,
            1,
            4,
            1,
            0,

            // right face
            3,
            2,
            6,
            3,
            6,
            7,

            // top face
            1,
            5,
            6,
            1,
            6,
            2,

            //bottom face
            4,
            0,
            3,
            4,
            3,
            7,
    };

    u32 index_count = 36;

    ID3D11Buffer *box_ib;

    D3D11_BUFFER_DESC ibd = {};
    ibd.Usage = D3D11_USAGE_IMMUTABLE;
    ibd.ByteWidth = sizeof(u32) * index_count;
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    D3D11_SUBRESOURCE_DATA iinit_data = {};
    iinit_data.pSysMem = indices;
    hres = device->CreateBuffer(&ibd, &iinit_data, &box_ib);
    assert(hres == 0);

    // INITIALIZING BUFFERS - /END -----------------

    // setting matrixes that don't need to be set every frame...
    // (proj matrix)
    //  TODO: this has to be set when u resize too
    XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * math::PI, WINDOW_ASPECT_RATIO, 1.0f, 1000.0f);
    XMStoreFloat4x4(&mat_proj, P);


    //initializing imgui
    // Create a Dear ImGui context, setup some options
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;// Enable some options

    // Initialize Platform + Renderer backends (here: using imgui_impl_win32.cpp + imgui_impl_dx11.cpp)
    ImGui_ImplWin32_Init(window);
    ImGui_ImplDX11_Init(device, device_context);

    //imgui end

    // main loop
    bool is_running = true;

    i64 perf_freq;
    QueryPerformanceFrequency((LARGE_INTEGER *) &perf_freq);
    f64 seconds_per_count = 1.0 / (f64) perf_freq;
    i64 time_last;
    QueryPerformanceCounter((LARGE_INTEGER *) &time_last);

    bool show_demo_window = true;

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

        // Update program state ---------------

        // Beginning of frame: update Renderer + Platform backend, start Dear ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

//        // Any application code here
//        ImGui::Text("Hello, world!");

//        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        // End of frame: render Dear ImGui
        ImGui::Render();

        XMMATRIX cam_rot_mat = XMMatrixRotationQuaternion(XMQuaternionRotationRollPitchYaw(cam_pitch, cam_yaw, 0.0f));
        XMVECTOR cam_pos_start = XMVectorSet(0.0, 0.0, -1.0 * cam_radius, 1.0);

        XMVECTOR cam_pos = XMVector3Transform(cam_pos_start, cam_rot_mat);
        XMVECTOR cam_target = XMVectorZero();
        XMVECTOR cam_up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

        XMMATRIX view_mat = XMMatrixLookAtLH(cam_pos, cam_target, cam_up);
        XMStoreFloat4x4(&mat_view, view_mat);

        // Update program state /end ---------------


        // Draw ---------------
        device_context->ClearRenderTargetView(render_target_view, reinterpret_cast<const f32 *>(&Colors::Blue));

        device_context->ClearDepthStencilView(depth_stencil_view, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
        device_context->IASetInputLayout(input_layout);
        device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        u32 stride = sizeof(Vertex);
        u32 offset = 0;
        device_context->IASetVertexBuffers(0, 1, &box_vb, &stride, &offset);
        device_context->IASetIndexBuffer(box_ib, DXGI_FORMAT_R32_UINT, 0);

        //Set constants
        XMMATRIX world = XMLoadFloat4x4(&mat_world);
        XMMATRIX view = XMLoadFloat4x4(&mat_view);
        XMMATRIX proj = XMLoadFloat4x4(&mat_proj);
        XMMATRIX wvp = world * view * proj;
        wvp_mat_var->SetMatrix(reinterpret_cast<float *>(&wvp));

        // Drawing indexes
        D3DX11_TECHNIQUE_DESC tech_desc;
        tech->GetDesc(&tech_desc);
        for (u32 p = 0; p < tech_desc.Passes; ++p) {
            tech->GetPassByIndex(p)->Apply(0, device_context);
            device_context->DrawIndexed(index_count, 0, 0);
        }

        //drawing imgui
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        // Draw /end ---------------

        swapchain->Present(1, 0);
    }

    return 0;
}