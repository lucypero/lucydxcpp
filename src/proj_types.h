#pragma once

#include <d3d11_1.h>
#include "Effects11\d3dx11effect.h"

#include "lucy_math.h"

const u32 WINDOW_WIDTH = 1280;
const u32 WINDOW_HEIGHT = 720;
const f32 WINDOW_ASPECT_RATIO = (f32) WINDOW_WIDTH / (f32) WINDOW_HEIGHT;

struct ColorVertex {
    XMFLOAT3 Pos;
    XMFLOAT4 Color;
};

struct Vertex {
    XMFLOAT3 Pos;
    XMFLOAT3 Normal;
    XMFLOAT2 Tex;
};

// shader and lighting structs

// Note: Make sure structure alignment agrees with HLSL structure padding rules. 
//   Elements are packed into 4D vectors with the restriction that an element
//   cannot straddle a 4D vector boundary.

struct DirectionalLight
{
	XMFLOAT4 Ambient;
	XMFLOAT4 Diffuse;
	XMFLOAT4 Specular;
	XMFLOAT3 Direction;
	f32 Pad; // Pad the last float so we can set an array of lights if we wanted.
};

struct PointLight
{
	XMFLOAT4 Ambient;
	XMFLOAT4 Diffuse;
	XMFLOAT4 Specular;

	// Packed into 4D vector: (Position, Range)
	XMFLOAT3 Position;
	f32 Range;

	// Packed into 4D vector: (A0, A1, A2, Pad)
	XMFLOAT3 Att;
	f32 Pad; // Pad the last float so we can set an array of lights if we wanted.
};

struct SpotLight
{
	XMFLOAT4 Ambient;
	XMFLOAT4 Diffuse;
	XMFLOAT4 Specular;

	// Packed into 4D vector: (Position, Range)
	XMFLOAT3 Position;
	float Range;

	// Packed into 4D vector: (Direction, Spot)
	XMFLOAT3 Direction;
	float Spot;

	// Packed into 4D vector: (Att, Pad)
	XMFLOAT3 Att;
	float Pad; // Pad the last float so we can set an array of lights if we wanted.
};

struct Material
{
	XMFLOAT4 Ambient;
	XMFLOAT4 Diffuse;
	XMFLOAT4 Specular; // w = SpecPower
	XMFLOAT4 Reflect;
};

// dx effect (3 dir lights, one point light)
// one texture
struct BasicEffect {

    ID3DX11Effect *effect;

    // directional lights (how many to compile, 1 to 3)
	ID3DX11EffectTechnique* Light1Tech;
	ID3DX11EffectTechnique* Light2Tech;
	ID3DX11EffectTechnique* Light3Tech;

	// cb per frame
	ID3DX11EffectVectorVariable* EyePosW;
	ID3DX11EffectVariable* DirLights;
	ID3DX11EffectVariable* point_light;

	// cb per object
	ID3DX11EffectMatrixVariable* World;
	ID3DX11EffectMatrixVariable* WorldInvTranspose;
	ID3DX11EffectMatrixVariable* WorldViewProj;
	ID3DX11EffectVariable* Mat;
	ID3DX11EffectMatrixVariable* TexTransform;

	// texture
	ID3DX11EffectShaderResourceVariable *texture;
	ID3DX11EffectShaderResourceVariable *texture_specular;

	void SetWorldViewProj(CXMMATRIX M)                  { WorldViewProj->SetMatrix(reinterpret_cast<const float*>(&M)); }
	void SetWorld(CXMMATRIX M)                          { World->SetMatrix(reinterpret_cast<const float*>(&M)); }
	void SetWorldInvTranspose(CXMMATRIX M)              { WorldInvTranspose->SetMatrix(reinterpret_cast<const float*>(&M)); }
	void SetTexTransform(CXMMATRIX M)              		{ TexTransform->SetMatrix(reinterpret_cast<const float*>(&M)); }
	void SetEyePosW(const XMFLOAT3& v)                  { EyePosW->SetRawValue(&v, 0, sizeof(XMFLOAT3)); }
	void SetDirLights(const DirectionalLight *lights)   { DirLights->SetRawValue(lights, 0, 3*sizeof(DirectionalLight)); }
	void SetPointLight(const PointLight *light)         { point_light->SetRawValue(light, 0, sizeof(PointLight)); }
	void SetMaterial(const Material *mat)               { Mat->SetRawValue(mat, 0, sizeof(Material)); }
	void SetTexture(ID3D11ShaderResourceView *tex)      { texture->SetResource(tex);}
	void SetTextureSpecular(ID3D11ShaderResourceView *tex)      { texture_specular->SetResource(tex);}
};

struct RenderContext {
    HWND window;
	i32 client_width;
	i32 client_height;

    ID3D11Device1 *device;
    ID3D11DeviceContext1 *device_context;
    IDXGISwapChain1 *swapchain;
    ID3D11RenderTargetView *render_target_view;
    ID3D11DepthStencilView *depth_stencil_view;
	ID3D11Texture2D *depth_stencil_buffer;
    POINT last_mouse_pos;
    f32 cam_yaw;
    f32 cam_pitch;
    f32 cam_radius;
	bool minimized;
	bool maximized;
	bool resizing;

    XMFLOAT4X4 mProj;

    // the effect
    BasicEffect basic_effect;

    // input layouts
    ID3D11InputLayout *il_pos_normal_uv; //POS and NORMAL and UVs

	// textures
	ID3D11ShaderResourceView *srv_diffuse;
	ID3D11ShaderResourceView *srv_specular;

    // rasterizer states
    ID3D11RasterizerState *mWireframeRS; // wireframe
    ID3D11RasterizerState *mSolidRS; // solid

	i32 mouse_wheel_delta;
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


// really basic shader
struct Shader {
    ID3DX11EffectTechnique *tech;
    ID3D11InputLayout *mInputLayout;
    ID3DX11EffectMatrixVariable *wvp_mat_var;
    ID3DX11EffectScalarVariable *time_var;
};

// total memory allocated
#define TOTAL_MEM (100 * 1000000) // 100 MB

#define GET_X_LPARAM(lp) ((int) (short) LOWORD(lp))
#define GET_Y_LPARAM(lp) ((int) (short) HIWORD(lp))

enum class ShaderFile {
    Color,
    ColorTrippy
};