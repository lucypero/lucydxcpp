#pragma comment(lib, "user32")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d3dcompiler")

///////////////////////////////////////////////////////////////////////////////////////////////////

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11_1.h>
#include <d3dcompiler.h>

#include <math.h> // sin, cos for rotation

#include "data.h" // example 3d model (the 'data.h' source file is provided below, along with 'shaders.hlsl')

///////////////////////////////////////////////////////////////////////////////////////////////////

#define TITLE "Minimal D3D11 by d7samurai"

///////////////////////////////////////////////////////////////////////////////////////////////////

struct float3 { float x, y, z; };
struct matrix { float m[4][4]; };

matrix operator*(const matrix& m1, const matrix& m2);

///////////////////////////////////////////////////////////////////////////////////////////////////

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{

    WNDCLASSEXA wndClassEx = { sizeof(wndClassEx) };
    wndClassEx.lpfnWndProc   = DefWindowProcA;
    wndClassEx.lpszClassName = TITLE;

    RegisterClassExA(&wndClassEx);

    HWND window = CreateWindowExA(0, TITLE, TITLE, WS_POPUP | WS_MAXIMIZE | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr, nullptr, nullptr);

    ///////////////////////////////////////////////////////////////////////////////////////////////

    D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };

    ID3D11Device* baseDevice;
    ID3D11DeviceContext* baseDeviceContext;

    D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT, featureLevels, ARRAYSIZE(featureLevels), D3D11_SDK_VERSION, &baseDevice, nullptr, &baseDeviceContext);

    ///////////////////////////////////////////////////////////////////////////////////////////////

    ID3D11Device1* device;

    baseDevice->QueryInterface(__uuidof(ID3D11Device1), reinterpret_cast<void**>(&device));

    ID3D11DeviceContext1* deviceContext;

    baseDeviceContext->QueryInterface(__uuidof(ID3D11DeviceContext1), reinterpret_cast<void**>(&deviceContext));

    ///////////////////////////////////////////////////////////////////////////////////////////////

    IDXGIDevice1* dxgiDevice;

    device->QueryInterface(__uuidof(IDXGIDevice1), reinterpret_cast<void**>(&dxgiDevice));

    IDXGIAdapter* dxgiAdapter;

    dxgiDevice->GetAdapter(&dxgiAdapter);

    IDXGIFactory2* dxgiFactory;

    dxgiAdapter->GetParent(__uuidof(IDXGIFactory2), reinterpret_cast<void**>(&dxgiFactory));

    ///////////////////////////////////////////////////////////////////////////////////////////////

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
    swapChainDesc.Width              = 0; // use window width
    swapChainDesc.Height             = 0; // use window height
    swapChainDesc.Format             = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
    swapChainDesc.Stereo             = FALSE;
    swapChainDesc.SampleDesc.Count   = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferUsage        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount        = 2;
    swapChainDesc.Scaling            = DXGI_SCALING_STRETCH;
    swapChainDesc.SwapEffect         = DXGI_SWAP_EFFECT_DISCARD;
    swapChainDesc.AlphaMode          = DXGI_ALPHA_MODE_UNSPECIFIED;
    swapChainDesc.Flags              = 0;

    IDXGISwapChain1* swapChain;

    dxgiFactory->CreateSwapChainForHwnd(device, window, &swapChainDesc, nullptr, nullptr, &swapChain);

    ///////////////////////////////////////////////////////////////////////////////////////////////

    ID3D11Texture2D* frameBuffer;

    swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&frameBuffer));

    ID3D11RenderTargetView* frameBufferView;

    device->CreateRenderTargetView(frameBuffer, nullptr, &frameBufferView);

    ///////////////////////////////////////////////////////////////////////////////////////////////

    D3D11_TEXTURE2D_DESC depthBufferDesc;

    frameBuffer->GetDesc(&depthBufferDesc); // base on framebuffer properties

    depthBufferDesc.Format    = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    ID3D11Texture2D* depthBuffer;

    device->CreateTexture2D(&depthBufferDesc, nullptr, &depthBuffer);

    ID3D11DepthStencilView* depthBufferView;

    device->CreateDepthStencilView(depthBuffer, nullptr, &depthBufferView);

    ///////////////////////////////////////////////////////////////////////////////////////////////

    ID3DBlob* vsBlob;

    D3DCompileFromFile(L"shaders.hlsl", nullptr, nullptr, "vs_main", "vs_5_0", 0, 0, &vsBlob, nullptr);

    ID3D11VertexShader* vertexShader;

    device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &vertexShader);

    D3D11_INPUT_ELEMENT_DESC inputElementDesc[] = // float3 position, float3 normal, float2 texcoord, float3 color
            {
                    { "POS", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,                            0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                    { "NOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                    { "TEX", 0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                    { "COL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            };

    ID3D11InputLayout* inputLayout;

    device->CreateInputLayout(inputElementDesc, ARRAYSIZE(inputElementDesc), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &inputLayout);

    ///////////////////////////////////////////////////////////////////////////////////////////////

    ID3DBlob* psBlob;

    D3DCompileFromFile(L"d7_shaders.hlsl", nullptr, nullptr, "ps_main", "ps_5_0", 0, 0, &psBlob, nullptr);

    ID3D11PixelShader* pixelShader;

    device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &pixelShader);

    ///////////////////////////////////////////////////////////////////////////////////////////////

    D3D11_RASTERIZER_DESC1 rasterizerDesc = {};
    rasterizerDesc.FillMode = D3D11_FILL_SOLID;
    rasterizerDesc.CullMode = D3D11_CULL_BACK;

    ID3D11RasterizerState1* rasterizerState;

    device->CreateRasterizerState1(&rasterizerDesc, &rasterizerState);

    ///////////////////////////////////////////////////////////////////////////////////////////////

    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter         = D3D11_FILTER_MIN_MAG_MIP_POINT;
    samplerDesc.AddressU       = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressV       = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressW       = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;

    ID3D11SamplerState* samplerState;

    device->CreateSamplerState(&samplerDesc, &samplerState);

    ///////////////////////////////////////////////////////////////////////////////////////////////

    D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
    depthStencilDesc.DepthEnable    = TRUE;
    depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    depthStencilDesc.DepthFunc      = D3D11_COMPARISON_LESS;

    ID3D11DepthStencilState* depthStencilState;

    device->CreateDepthStencilState(&depthStencilDesc, &depthStencilState);

    ///////////////////////////////////////////////////////////////////////////////////////////////

    struct Constants
    {
        matrix Transform;
        matrix Projection;
        float3 LightVector;
    };

    D3D11_BUFFER_DESC constantBufferDesc = {};
    constantBufferDesc.ByteWidth      = sizeof(Constants) + 0xf & 0xfffffff0; // round constant buffer size to 16 byte boundary
    constantBufferDesc.Usage          = D3D11_USAGE_DYNAMIC;
    constantBufferDesc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
    constantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    ID3D11Buffer* constantBuffer;

    device->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);

    ///////////////////////////////////////////////////////////////////////////////////////////////

    D3D11_BUFFER_DESC vertexBufferDesc = {};
    vertexBufferDesc.ByteWidth = sizeof(VertexData);
    vertexBufferDesc.Usage     = D3D11_USAGE_IMMUTABLE;
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA vertexData = { VertexData };

    ID3D11Buffer* vertexBuffer;

    device->CreateBuffer(&vertexBufferDesc, &vertexData, &vertexBuffer);

    ///////////////////////////////////////////////////////////////////////////////////////////////

    D3D11_BUFFER_DESC indexBufferDesc = {};
    indexBufferDesc.ByteWidth = sizeof(IndexData);
    indexBufferDesc.Usage     = D3D11_USAGE_IMMUTABLE;
    indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA indexData = { IndexData };

    ID3D11Buffer* indexBuffer;

    device->CreateBuffer(&indexBufferDesc, &indexData, &indexBuffer);

    ///////////////////////////////////////////////////////////////////////////////////////////////

    D3D11_TEXTURE2D_DESC textureDesc = {};
    textureDesc.Width              = TEXTURE_WIDTH;  // in data.h
    textureDesc.Height             = TEXTURE_HEIGHT; // in data.h
    textureDesc.MipLevels          = 1;
    textureDesc.ArraySize          = 1;
    textureDesc.Format             = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    textureDesc.SampleDesc.Count   = 1;
    textureDesc.Usage              = D3D11_USAGE_IMMUTABLE;
    textureDesc.BindFlags          = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA textureData = {};
    textureData.pSysMem            = TextureData;
    textureData.SysMemPitch        = TEXTURE_WIDTH * 4; // 4 bytes per pixel

    ID3D11Texture2D* texture;

    device->CreateTexture2D(&textureDesc, &textureData, &texture);

    ID3D11ShaderResourceView* textureView;

    device->CreateShaderResourceView(texture, nullptr, &textureView);

    ///////////////////////////////////////////////////////////////////////////////////////////////

    FLOAT backgroundColor[4] = { 0.025f, 0.025f, 0.025f, 1.0f };

    UINT stride = 11 * 4; // vertex size (11 floats: float3 position, float3 normal, float2 texcoord, float3 color)
    UINT offset = 0;

    D3D11_VIEWPORT viewport = { 0.0f, 0.0f, static_cast<float>(depthBufferDesc.Width), static_cast<float>(depthBufferDesc.Height), 0.0f, 1.0f };

    ///////////////////////////////////////////////////////////////////////////////////////////////

    float w = viewport.Width / viewport.Height; // width (aspect ratio)
    float h = 1.0f;                             // height
    float n = 1.0f;                             // near
    float f = 9.0f;                             // far

    float3 modelRotation    = { 0.0f, 0.0f, 0.0f };
    float3 modelScale       = { 1.0f, 1.0f, 1.0f };
    float3 modelTranslation = { 0.0f, 0.0f, 4.0f };

    ///////////////////////////////////////////////////////////////////////////////////////////////

    while (true)
    {
        MSG msg;

        while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_KEYDOWN) return 0;
            DispatchMessageA(&msg);
        }

        ///////////////////////////////////////////////////////////////////////////////////////////

        matrix rotateX   = { 1, 0, 0, 0, 0, static_cast<float>(cos(modelRotation.x)), -static_cast<float>(sin(modelRotation.x)), 0, 0, static_cast<float>(sin(modelRotation.x)), static_cast<float>(cos(modelRotation.x)), 0, 0, 0, 0, 1 };
        matrix rotateY   = { static_cast<float>(cos(modelRotation.y)), 0, static_cast<float>(sin(modelRotation.y)), 0, 0, 1, 0, 0, -static_cast<float>(sin(modelRotation.y)), 0, static_cast<float>(cos(modelRotation.y)), 0, 0, 0, 0, 1 };
        matrix rotateZ   = { static_cast<float>(cos(modelRotation.z)), -static_cast<float>(sin(modelRotation.z)), 0, 0, static_cast<float>(sin(modelRotation.z)), static_cast<float>(cos(modelRotation.z)), 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
        matrix scale     = { modelScale.x, 0, 0, 0, 0, modelScale.y, 0, 0, 0, 0, modelScale.z, 0, 0, 0, 0, 1 };
        matrix translate = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, modelTranslation.x, modelTranslation.y, modelTranslation.z, 1 };

        modelRotation.x += 0.005f;
        modelRotation.y += 0.009f;
        modelRotation.z += 0.001f;

        ///////////////////////////////////////////////////////////////////////////////////////////

        D3D11_MAPPED_SUBRESOURCE mappedSubresource;

        deviceContext->Map(constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubresource);

        auto constants = reinterpret_cast<Constants*>(mappedSubresource.pData);

        constants->Transform   = rotateX * rotateY * rotateZ * scale * translate;
        constants->Projection  = { 2 * n / w, 0, 0, 0, 0, 2 * n / h, 0, 0, 0, 0, f / (f - n), 1, 0, 0, n * f / (n - f), 0 };
        constants->LightVector = { 1.0f, -1.0f, 1.0f };

        deviceContext->Unmap(constantBuffer, 0);

        ///////////////////////////////////////////////////////////////////////////////////////////

        deviceContext->ClearRenderTargetView(frameBufferView, backgroundColor);
        deviceContext->ClearDepthStencilView(depthBufferView, D3D11_CLEAR_DEPTH, 1.0f, 0);

        deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        deviceContext->IASetInputLayout(inputLayout);
        deviceContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
        deviceContext->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);

        deviceContext->VSSetShader(vertexShader, nullptr, 0);
        deviceContext->VSSetConstantBuffers(0, 1, &constantBuffer);

        deviceContext->RSSetViewports(1, &viewport);
        deviceContext->RSSetState(rasterizerState);

        deviceContext->PSSetShader(pixelShader, nullptr, 0);
        deviceContext->PSSetShaderResources(0, 1, &textureView);
        deviceContext->PSSetSamplers(0, 1, &samplerState);

        deviceContext->OMSetRenderTargets(1, &frameBufferView, depthBufferView);
        deviceContext->OMSetDepthStencilState(depthStencilState, 0);
        deviceContext->OMSetBlendState(nullptr, nullptr, 0xffffffff); // use default blend mode (i.e. disable)

        ///////////////////////////////////////////////////////////////////////////////////////////

        deviceContext->DrawIndexed(ARRAYSIZE(IndexData), 0, 0);

        ///////////////////////////////////////////////////////////////////////////////////////////

        swapChain->Present(1, 0);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

matrix operator*(const matrix& m1, const matrix& m2)
{
    return
            {
                    m1.m[0][0] * m2.m[0][0] + m1.m[0][1] * m2.m[1][0] + m1.m[0][2] * m2.m[2][0] + m1.m[0][3] * m2.m[3][0],
                    m1.m[0][0] * m2.m[0][1] + m1.m[0][1] * m2.m[1][1] + m1.m[0][2] * m2.m[2][1] + m1.m[0][3] * m2.m[3][1],
                    m1.m[0][0] * m2.m[0][2] + m1.m[0][1] * m2.m[1][2] + m1.m[0][2] * m2.m[2][2] + m1.m[0][3] * m2.m[3][2],
                    m1.m[0][0] * m2.m[0][3] + m1.m[0][1] * m2.m[1][3] + m1.m[0][2] * m2.m[2][3] + m1.m[0][3] * m2.m[3][3],
                    m1.m[1][0] * m2.m[0][0] + m1.m[1][1] * m2.m[1][0] + m1.m[1][2] * m2.m[2][0] + m1.m[1][3] * m2.m[3][0],
                    m1.m[1][0] * m2.m[0][1] + m1.m[1][1] * m2.m[1][1] + m1.m[1][2] * m2.m[2][1] + m1.m[1][3] * m2.m[3][1],
                    m1.m[1][0] * m2.m[0][2] + m1.m[1][1] * m2.m[1][2] + m1.m[1][2] * m2.m[2][2] + m1.m[1][3] * m2.m[3][2],
                    m1.m[1][0] * m2.m[0][3] + m1.m[1][1] * m2.m[1][3] + m1.m[1][2] * m2.m[2][3] + m1.m[1][3] * m2.m[3][3],
                    m1.m[2][0] * m2.m[0][0] + m1.m[2][1] * m2.m[1][0] + m1.m[2][2] * m2.m[2][0] + m1.m[2][3] * m2.m[3][0],
                    m1.m[2][0] * m2.m[0][1] + m1.m[2][1] * m2.m[1][1] + m1.m[2][2] * m2.m[2][1] + m1.m[2][3] * m2.m[3][1],
                    m1.m[2][0] * m2.m[0][2] + m1.m[2][1] * m2.m[1][2] + m1.m[2][2] * m2.m[2][2] + m1.m[2][3] * m2.m[3][2],
                    m1.m[2][0] * m2.m[0][3] + m1.m[2][1] * m2.m[1][3] + m1.m[2][2] * m2.m[2][3] + m1.m[2][3] * m2.m[3][3],
                    m1.m[3][0] * m2.m[0][0] + m1.m[3][1] * m2.m[1][0] + m1.m[3][2] * m2.m[2][0] + m1.m[3][3] * m2.m[3][0],
                    m1.m[3][0] * m2.m[0][1] + m1.m[3][1] * m2.m[1][1] + m1.m[3][2] * m2.m[2][1] + m1.m[3][3] * m2.m[3][1],
                    m1.m[3][0] * m2.m[0][2] + m1.m[3][1] * m2.m[1][2] + m1.m[3][2] * m2.m[2][2] + m1.m[3][3] * m2.m[3][2],
                    m1.m[3][0] * m2.m[0][3] + m1.m[3][1] * m2.m[1][3] + m1.m[3][2] * m2.m[2][3] + m1.m[3][3] * m2.m[3][3],
            };
}