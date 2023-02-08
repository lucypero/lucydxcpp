#pragma comment(lib, "user32")
#pragma comment(lib, "d3d11")

//system headers
#include <stdint.h>
#define WIN32_LEAN_AND_MEAN
#include <d3d11_1.h>
#include <windows.h>

// libraries
#include "Effects11\d3dx11effect.h"
#include "imgui\imgui.h"
#include "imgui\backends\imgui_impl_win32.h"
#include "imgui\backends\imgui_impl_dx11.h"

#define TITLE "lucydxcpp"

// project headers
#include "lucytypes.h"
#include "proj_types.h"
#include "lucy_math.h"
#include "utils.h"
#include "demo_light.h"
#include "renderer.h"

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
    on_resize(ctx);
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

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {

    // allocate all memory for the whole game
    void *base_mem = VirtualAlloc(0, TOTAL_MEM, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    assert(base_mem != 0);

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

    // initting window
    rctx.client_width = WINDOW_WIDTH;
    rctx.client_height = WINDOW_HEIGHT;

    WNDCLASSEXA wndClassEx = {sizeof(wndClassEx)};
    wndClassEx.lpfnWndProc = WndProc;
    wndClassEx.lpszClassName = TITLE;

    RegisterClassExA(&wndClassEx);

    RECT initialRect = {
            .right = rctx.client_width,
            .bottom = rctx.client_height,
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

    rctx.window = window;

    SetWindowLongPtrA(window, GWLP_USERDATA, (LONG_PTR)&rctx);

    lres = render_context_init(&big_arena, &rctx);
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