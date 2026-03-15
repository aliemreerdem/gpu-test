#include "Renderer.h"
#include "../core/Logger.h"
#include <fstream>
#include <iostream>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

#define SAFE_RELEASE(p) if (p) { (p)->Release(); (p) = nullptr; }

Renderer::Renderer(HWND hwnd, int width, int height)
    : m_hwnd(hwnd), m_width(width), m_height(height),
      m_pDevice(nullptr), m_pContext(nullptr), m_pSwapChain(nullptr),
      m_pRenderTargetView(nullptr), m_pDepthStencilView(nullptr),
      m_pDepthStencilState(nullptr), m_pConstantBuffer(nullptr),
      m_pDummyVS(nullptr), m_pDummyPS(nullptr)
{
}

Renderer::~Renderer() {
    SAFE_RELEASE(m_pDepthStencilState);
    SAFE_RELEASE(m_pConstantBuffer);
    SAFE_RELEASE(m_pDummyVS);
    SAFE_RELEASE(m_pDummyPS);
    SAFE_RELEASE(m_pDepthStencilView);
    SAFE_RELEASE(m_pRenderTargetView);
    SAFE_RELEASE(m_pSwapChain);
    SAFE_RELEASE(m_pContext);
    SAFE_RELEASE(m_pDevice);
}

std::vector<IDXGIAdapter*> Renderer::EnumerateAdapters() {
    std::vector<IDXGIAdapter*> vAdapters;
    IDXGIFactory* pFactory = nullptr;
    HRESULT hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&pFactory);
    if (FAILED(hr)) {
        Logger::LogError("Failed to create DXGI Factory.", hr);
        return vAdapters;
    }

    IDXGIAdapter* pAdapter = nullptr;
    UINT adapterIndex = 0;
    while (pFactory->EnumAdapters(adapterIndex, &pAdapter) != DXGI_ERROR_NOT_FOUND) {
        vAdapters.push_back(pAdapter);
        ++adapterIndex;
    }
    SAFE_RELEASE(pFactory);
    return vAdapters;
}

bool Renderer::Initialize(int selectedGpuIndex) {
    auto adapters = EnumerateAdapters();
    if (adapters.empty() || selectedGpuIndex >= adapters.size()) return false;
    
    IDXGIAdapter* pSelectedAdapter = adapters[selectedGpuIndex];
    if (!CreateDeviceAndSwapChain(pSelectedAdapter)) return false;
    if (!CreateRenderTargets()) return false;
    if (!CreateDummyShaders()) return false;

    for (auto adapter : adapters) SAFE_RELEASE(adapter);
    return true;
}

bool Renderer::CreateDeviceAndSwapChain(IDXGIAdapter* pAdapter) {
    D3D_FEATURE_LEVEL featureLevel;
    HRESULT hr = D3D11CreateDevice(pAdapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &m_pDevice, &featureLevel, &m_pContext);
    if (FAILED(hr)) { Logger::LogError("D3D11CreateDevice Failed", hr); return false; }

    IDXGIDevice* pDXGIDevice = nullptr;
    m_pDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&pDXGIDevice);
    IDXGIAdapter* pDXGIAdapter = nullptr;
    pDXGIDevice->GetAdapter(&pDXGIAdapter);
    IDXGIFactory* pDXGIFactory = nullptr;
    pDXGIAdapter->GetParent(__uuidof(IDXGIFactory), (void**)&pDXGIFactory);

    DXGI_SWAP_CHAIN_DESC scDesc = {0};
    scDesc.BufferCount = 2; // FLIP_DISCARD mandates 2
    scDesc.BufferDesc.Width = m_width;
    scDesc.BufferDesc.Height = m_height;
    scDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scDesc.OutputWindow = m_hwnd;
    scDesc.SampleDesc.Count = 1;
    scDesc.Windowed = TRUE;
    scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; 
    
    hr = pDXGIFactory->CreateSwapChain(m_pDevice, &scDesc, &m_pSwapChain);
    
    SAFE_RELEASE(pDXGIDevice);
    SAFE_RELEASE(pDXGIAdapter);
    SAFE_RELEASE(pDXGIFactory);

    return SUCCEEDED(hr);
}

bool Renderer::CreateRenderTargets() {
    ID3D11Texture2D* pBackBuffer = nullptr;
    m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer);
    m_pDevice->CreateRenderTargetView(pBackBuffer, nullptr, &m_pRenderTargetView);
    SAFE_RELEASE(pBackBuffer);

    D3D11_TEXTURE2D_DESC descDepth = {0};
    descDepth.Width = m_width;
    descDepth.Height = m_height;
    descDepth.MipLevels = 1;
    descDepth.ArraySize = 1;
    descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    descDepth.SampleDesc.Count = 1;
    descDepth.Usage = D3D11_USAGE_DEFAULT;
    descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    
    ID3D11Texture2D* pDepthStencil = nullptr;
    m_pDevice->CreateTexture2D(&descDepth, nullptr, &pDepthStencil);

    D3D11_DEPTH_STENCIL_VIEW_DESC descDSV = {0};
    descDSV.Format = descDepth.Format;
    descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    
    if (pDepthStencil) {
        m_pDevice->CreateDepthStencilView(pDepthStencil, &descDSV, &m_pDepthStencilView);
        SAFE_RELEASE(pDepthStencil);
    }

    D3D11_DEPTH_STENCIL_DESC dssDesc = {0};
    dssDesc.DepthEnable = TRUE;
    dssDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dssDesc.DepthFunc = D3D11_COMPARISON_LESS;
    dssDesc.StencilEnable = FALSE;
    m_pDevice->CreateDepthStencilState(&dssDesc, &m_pDepthStencilState);

    m_pContext->OMSetRenderTargets(1, &m_pRenderTargetView, m_pDepthStencilView);
    m_pContext->OMSetDepthStencilState(m_pDepthStencilState, 1);

    D3D11_VIEWPORT vp;
    vp.Width = (float)m_width;
    vp.Height = (float)m_height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    m_pContext->RSSetViewports(1, &vp);

    return true;
}

bool Renderer::CreateDummyShaders() {
    struct ConstantBuffer { float time; float resX; float resY; float pad; };
    D3D11_BUFFER_DESC cbDesc = {0};
    cbDesc.Usage = D3D11_USAGE_DYNAMIC;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    cbDesc.ByteWidth = sizeof(ConstantBuffer);
    m_pDevice->CreateBuffer(&cbDesc, nullptr, &m_pConstantBuffer);

    auto LoadShader = [&](const std::string& path, std::vector<char>& data) -> bool {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open()) return false;
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        data.resize(size);
        return !!file.read(data.data(), size);
    };

    std::vector<char> vsData, psData;
    if (LoadShader("shaders\\vs_dummy.cso", vsData)) 
        m_pDevice->CreateVertexShader(vsData.data(), vsData.size(), nullptr, &m_pDummyVS);
    if (LoadShader("shaders\\ps_dummy.cso", psData)) 
        m_pDevice->CreatePixelShader(psData.data(), psData.size(), nullptr, &m_pDummyPS);
        
    return true;
}

void Renderer::Present() {
    // Fill the heuristic buffer then submit the frame
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    m_pContext->Map(m_pConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    mappedResource.pData = nullptr; // Dummy map
    m_pContext->Unmap(m_pConstantBuffer, 0);

    m_pContext->VSSetShader(m_pDummyVS, nullptr, 0);
    m_pContext->PSSetShader(m_pDummyPS, nullptr, 0);
    m_pContext->VSSetConstantBuffers(0, 1, &m_pConstantBuffer);
    
    m_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
    m_pContext->Draw(300000, 0); // V21 Heuristic - Massive Geometry Draw

    m_pSwapChain->Present(0, 0); // Zero VSync for max frame pacing
}
