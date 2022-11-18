#pragma comment(lib, "user32")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d3dcompiler")

///////////////////////////////////////////////////////////////////////////////////////////////////

#define WIN32_LEAN_AND_MEAN
#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <windows.h>

#include <cmath>// sin, cos for rotation

#include <cassert>

#define TITLE "lucydxcpp"

void log(const char *str) {
    OutputDebugStringA(str);
}

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

    log("hello\n");

    WNDCLASSEXA wndClassEx = {sizeof(wndClassEx)};
    wndClassEx.lpfnWndProc = WndProc;
    wndClassEx.lpszClassName = TITLE;

    RegisterClassExA(&wndClassEx);

    RECT initialRect = {0, 0, 1024, 768};
    AdjustWindowRectEx(&initialRect, WS_OVERLAPPEDWINDOW, FALSE, WS_EX_OVERLAPPEDWINDOW);
    LONG initialWidth = initialRect.right - initialRect.left;
    LONG initialHeight = initialRect.bottom - initialRect.top;

    HWND window = CreateWindowExA(WS_EX_OVERLAPPEDWINDOW,
                                  TITLE, TITLE,
                                  WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                  CW_USEDEFAULT, CW_USEDEFAULT,
                                  initialWidth,
                                  initialHeight,
                                  nullptr, nullptr, hInstance, nullptr);

    if (!window) {
        MessageBoxA(nullptr, "CreateWindowEx failed", "Fatal Error", MB_OK);
        return GetLastError();
    }

    // dx 11 creation
    ID3D11Device *device = nullptr;
    ID3D11DeviceContext *device_context = nullptr;
    IDXGISwapChain *swap_chain = nullptr;
    ID3D11RenderTargetView *render_target_view = nullptr;

    DXGI_SWAP_CHAIN_DESC swap_chain_descr = {0};
    swap_chain_descr.BufferDesc.RefreshRate.Numerator = 0;
    swap_chain_descr.BufferDesc.RefreshRate.Denominator = 1;
    swap_chain_descr.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    swap_chain_descr.SampleDesc.Count = 1;
    swap_chain_descr.SampleDesc.Quality = 0;
    swap_chain_descr.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_chain_descr.BufferCount = 1;
    swap_chain_descr.OutputWindow = window;
    swap_chain_descr.Windowed = true;

    D3D_FEATURE_LEVEL feature_level;
    UINT flags = D3D11_CREATE_DEVICE_SINGLETHREADED;
#ifdef DEBUG
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            flags,
            nullptr,
            0,
            D3D11_SDK_VERSION,
            &swap_chain_descr,
            &swap_chain,
            &device,
            &feature_level,
            &device_context);
    assert(hr == 0 && swap_chain && device && device_context);
    ID3D11Texture2D *framebuffer;
    hr = swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void **) &framebuffer);
    assert(SUCCEEDED(hr));

    hr = device->CreateRenderTargetView(framebuffer, nullptr, &render_target_view);
    assert(SUCCEEDED(hr));
    framebuffer->Release();

    flags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef DEBUG
    flags |= D3DCOMPILE_DEBUG;// add more debug output
#endif
    ID3DBlob *vs_blob_ptr = nullptr, *ps_blob_ptr = nullptr, *error_blob = nullptr;

    // COMPILE VERTEX SHADER
    hr = D3DCompileFromFile(
            L"shaders.hlsl",
            nullptr,
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            "vs_main",
            "vs_5_0",
            flags,
            0,
            &vs_blob_ptr,
            &error_blob);
    if (FAILED(hr)) {
        if (error_blob) {
            OutputDebugStringA((char *) error_blob->GetBufferPointer());
            error_blob->Release();
        }
        if (vs_blob_ptr) { vs_blob_ptr->Release(); }
        assert(false);
    }

    // COMPILE PIXEL SHADER
    hr = D3DCompileFromFile(
            L"shaders.hlsl",
            nullptr,
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            "ps_main",
            "ps_5_0",
            flags,
            0,
            &ps_blob_ptr,
            &error_blob);
    if (FAILED(hr)) {
        if (error_blob) {
            OutputDebugStringA((char *) error_blob->GetBufferPointer());
            error_blob->Release();
        }
        if (ps_blob_ptr) { ps_blob_ptr->Release(); }
        assert(false);
    }

    // compiling shaders

    ID3D11VertexShader *vertex_shader_ptr = nullptr;
    ID3D11PixelShader *pixel_shader_ptr = nullptr;

    hr = device->CreateVertexShader(
            vs_blob_ptr->GetBufferPointer(),
            vs_blob_ptr->GetBufferSize(),
            nullptr,
            &vertex_shader_ptr);
    assert(SUCCEEDED(hr));

    hr = device->CreatePixelShader(
            ps_blob_ptr->GetBufferPointer(),
            ps_blob_ptr->GetBufferSize(),
            nullptr,
            &pixel_shader_ptr);
    assert(SUCCEEDED(hr));

    // input layout
    ID3D11InputLayout *input_layout_ptr = nullptr;
    D3D11_INPUT_ELEMENT_DESC inputElementDesc[] = {
            {"POS", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
            { "COL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            /*
  { "NOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
  { "TEX", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
  */
    };
    hr = device->CreateInputLayout(
            inputElementDesc,
            ARRAYSIZE(inputElementDesc),
            vs_blob_ptr->GetBufferPointer(),
            vs_blob_ptr->GetBufferSize(),
            &input_layout_ptr);
    assert(SUCCEEDED(hr));

    //vertex buffer
    float vertex_data_array[] = {
            0.0f, 0.5f, 0.0f, 1.f, 0.f, 0.f,  // point at top
            0.5f, -0.5f, 0.0f, 0.f, 1.f, 0.f,  // point at bottom-right
            -0.5f, -0.5f, 0.0f, 0.f, 0.f, 1.f// point at bottom-left
    };
    UINT vertex_stride = 6 * sizeof(float);
    UINT vertex_offset = 0;
    UINT vertex_count = 3;

    ID3D11Buffer *vertex_buffer_ptr = nullptr;
    { /*** load mesh data into vertex buffer **/
        D3D11_BUFFER_DESC vertex_buff_descr = {};
        vertex_buff_descr.ByteWidth = sizeof(vertex_data_array);
        vertex_buff_descr.Usage = D3D11_USAGE_DEFAULT;
        vertex_buff_descr.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        D3D11_SUBRESOURCE_DATA sr_data = {};
        sr_data.pSysMem = vertex_data_array;
        hr = device->CreateBuffer(
                &vertex_buff_descr,
                &sr_data,
                &vertex_buffer_ptr);
        assert(SUCCEEDED(hr));
    }

    // main loop
    bool is_running = true;

    while (is_running) {
        MSG msg;

        while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT || msg.message == WM_KEYDOWN)
                is_running = false;
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }

        // render a frame here w dx11
        /* clear the back buffer to cornflower blue for the new frame */
        float background_colour[4] = {0x64 / 255.0f, 0x95 / 255.0f, 0xED / 255.0f, 1.0f};
        device_context->ClearRenderTargetView(render_target_view, background_colour);

        /**** Rasteriser state - set viewport area *****/
        RECT winRect;
        GetClientRect(window, &winRect);
        D3D11_VIEWPORT viewport = {0.0f, 0.0f, (FLOAT) (winRect.right - winRect.left), (FLOAT) (winRect.bottom - winRect.top), 0.0f, 1.0f};
        device_context->RSSetViewports(1, &viewport);

        /**** Output Merger *****/
        device_context->OMSetRenderTargets(1, &render_target_view, nullptr);

        /***** Input Assembler (map how the vertex shader inputs should be read from vertex buffer) ******/
        device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        device_context->IASetInputLayout(input_layout_ptr);
        device_context->IASetVertexBuffers(0, 1, &vertex_buffer_ptr, &vertex_stride, &vertex_offset);

        /*** set vertex shader to use and pixel shader to use, and constant buffers for each ***/
        device_context->VSSetShader(vertex_shader_ptr, nullptr, 0);
        device_context->PSSetShader(pixel_shader_ptr, nullptr, 0);

        /*** draw the vertex buffer with the shaders ****/
        device_context->Draw(vertex_count, 0);

        /**** swap the back and front buffers (show the frame we just drew) ****/
        swap_chain->Present(1, 0);
    }

    return 0;
}