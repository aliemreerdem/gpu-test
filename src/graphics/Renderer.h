#pragma once
#include <d3d11.h>
#include <dxgi1_2.h>
#include <string>
#include <vector>

class Renderer {
public:
    Renderer(HWND hwnd, int width, int height);
    ~Renderer();

    bool Initialize(int selectedGpuIndex);
    
    ID3D11Device* GetDevice() const { return m_pDevice; }
    ID3D11DeviceContext* GetContext() const { return m_pContext; }

    static std::vector<IDXGIAdapter*> EnumerateAdapters();
    
    // Core dummy states for AAA signature
    ID3D11DepthStencilState* GetDepthStencilState() const { return m_pDepthStencilState; }
    ID3D11RenderTargetView* GetRTV() const { return m_pRenderTargetView; }
    ID3D11DepthStencilView* GetDSV() const { return m_pDepthStencilView; }

    void Present();

private:
    bool CreateDeviceAndSwapChain(IDXGIAdapter* pAdapter);
    bool CreateRenderTargets();
    bool CreateDummyShaders();

    HWND m_hwnd;
    int m_width;
    int m_height;

    // DX Core
    ID3D11Device* m_pDevice;
    ID3D11DeviceContext* m_pContext;
    IDXGISwapChain* m_pSwapChain;
    ID3D11RenderTargetView* m_pRenderTargetView;
    ID3D11DepthStencilView* m_pDepthStencilView;
    ID3D11DepthStencilState* m_pDepthStencilState;

    // V14 AMD Overlay Bypasses
    ID3D11Buffer* m_pConstantBuffer;
    ID3D11VertexShader* m_pDummyVS;
    ID3D11PixelShader* m_pDummyPS;
};
