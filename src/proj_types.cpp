const u32 WINDOW_WIDTH = 1280;
const u32 WINDOW_HEIGHT = 720;
const f32 WINDOW_ASPECT_RATIO = (f32) WINDOW_WIDTH / (f32) WINDOW_HEIGHT;

struct Vertex {
    XMFLOAT3 Pos;
    XMFLOAT4 Color;
};

struct RenderContext {
    HWND window;
    ID3D11Device1 *device;
    ID3D11DeviceContext1 *device_context;
    IDXGISwapChain1 *swapchain;
    ID3D11RenderTargetView *render_target_view;
    ID3D11DepthStencilView *depth_stencil_view;
    POINT last_mouse_pos;
    f32 cam_yaw;
    f32 cam_pitch;
    f32 cam_radius;
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

struct Shader {
    ID3DX11EffectTechnique *tech;
    ID3D11InputLayout *mInputLayout;
    ID3DX11EffectMatrixVariable *wvp_mat_var;
    ID3DX11EffectScalarVariable *time_var;
};

// total memory allocated
#define TOTAL_MEM (1000 * 1024)

#define GET_X_LPARAM(lp) ((int) (short) LOWORD(lp))
#define GET_Y_LPARAM(lp) ((int) (short) HIWORD(lp))

enum class ShaderFile {
    Color,
    ColorTrippy
};