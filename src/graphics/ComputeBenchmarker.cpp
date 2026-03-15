#include "ComputeBenchmarker.h"
#include "Renderer.h"
#include "../core/Logger.h"
#include <fstream>
#include <vector>

#define SAFE_RELEASE(p) if ((p)) { (p)->Release(); (p) = nullptr; }

ComputeBenchmarker::ComputeBenchmarker(Renderer* renderer)
    : m_renderer(renderer), m_pComputeShader(nullptr), 
      m_pBufferInA(nullptr), m_pBufferInB(nullptr), m_pBufferOutC(nullptr),
      m_pSRV_A(nullptr), m_pSRV_B(nullptr), m_pUAV_C(nullptr)
{
}

ComputeBenchmarker::~ComputeBenchmarker() {
    SAFE_RELEASE(m_pComputeShader);
    SAFE_RELEASE(m_pBufferInA);
    SAFE_RELEASE(m_pBufferInB);
    SAFE_RELEASE(m_pBufferOutC);
    SAFE_RELEASE(m_pSRV_A);
    SAFE_RELEASE(m_pSRV_B);
    SAFE_RELEASE(m_pUAV_C);
}

bool ComputeBenchmarker::LoadKernel(int testMode) {
    const char* kernelName = "CSMath";
    if (testMode == 2) kernelName = "CSMemory";
    if (testMode == 3) kernelName = "CSGame";
    if (testMode == 4) kernelName = "CSCrypto";
    if (testMode == 5) kernelName = "CSRayTrace";
    if (testMode == 6) kernelName = "CSMemoryMassive"; 
    if (testMode == 7) kernelName = "CSParticles";
    if (testMode == 8) kernelName = "CSCache";
    if (testMode == 9) kernelName = "CSDeferred";
    if (testMode == 10) kernelName = "CSRedDead2";
    if (testMode == 11) kernelName = "CSDiablo4";

    std::string shaderPath = std::string("shaders\\") + std::string(kernelName) + ".cso";
    std::ifstream file(shaderPath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        Logger::LogError("Failed to open shader file: " + shaderPath);
        return false;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<char> shaderData(size);
    if (!file.read(shaderData.data(), size)) {
        Logger::LogError("Failed to read shader file: " + shaderPath);
        return false;
    }

    HRESULT hr = m_renderer->GetDevice()->CreateComputeShader(shaderData.data(), size, nullptr, &m_pComputeShader);
    if (FAILED(hr)) {
        Logger::LogError("Failed to create Compute Shader: " + std::string(kernelName), hr);
        return false;
    }
    
    return true;
}

bool ComputeBenchmarker::InitializeBuffers(int elements) {
    ID3D11Device* pDevice = m_renderer->GetDevice();
    size_t bufferSize = elements * sizeof(float);

    D3D11_BUFFER_DESC descBase = {0};
    descBase.Usage = D3D11_USAGE_DEFAULT;
    descBase.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    descBase.CPUAccessFlags = 0;
    descBase.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
    descBase.ByteWidth = (UINT)bufferSize;

    HRESULT hr;
    hr = pDevice->CreateBuffer(&descBase, nullptr, &m_pBufferInA);
    if (FAILED(hr)) return false;
    
    hr = pDevice->CreateBuffer(&descBase, nullptr, &m_pBufferInB);
    if (FAILED(hr)) return false;

    D3D11_BUFFER_DESC descOut = descBase;
    descOut.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
    hr = pDevice->CreateBuffer(&descOut, nullptr, &m_pBufferOutC);
    if (FAILED(hr)) return false;

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
    srvDesc.BufferEx.FirstElement = 0;
    srvDesc.BufferEx.NumElements = elements;
    srvDesc.BufferEx.Flags = D3D11_BUFFEREX_SRV_FLAG_RAW;
    pDevice->CreateShaderResourceView(m_pBufferInA, &srvDesc, &m_pSRV_A);
    pDevice->CreateShaderResourceView(m_pBufferInB, &srvDesc, &m_pSRV_B);

    D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
    uavDesc.Format = DXGI_FORMAT_R32_TYPELESS;
    uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.FirstElement = 0;
    uavDesc.Buffer.NumElements = elements;
    uavDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_RAW;
    pDevice->CreateUnorderedAccessView(m_pBufferOutC, &uavDesc, &m_pUAV_C);

    return true;
}

void ComputeBenchmarker::Dispatch(int dispatchSize) {
    auto ctx = m_renderer->GetContext();
    ctx->CSSetShader(m_pComputeShader, nullptr, 0);

    ID3D11ShaderResourceView* srvs[] = { m_pSRV_A, m_pSRV_B };
    ctx->CSSetShaderResources(0, 2, srvs);
    ctx->CSSetUnorderedAccessViews(0, 1, &m_pUAV_C, nullptr);
    
    // Each thread group covers 256 threads (configured in hlsl).
    UINT threadGroupsX = (dispatchSize + 255) / 256;
    ctx->Dispatch(threadGroupsX, 1, 1);

    // V27 Sync/Flush pattern
    ctx->Flush();

    // Cleanup resources for next frame
    ID3D11ShaderResourceView* nullSRV[] = { nullptr, nullptr };
    ctx->CSSetShaderResources(0, 2, nullSRV);
    ID3D11UnorderedAccessView* nullUAV[] = { nullptr };
    ctx->CSSetUnorderedAccessViews(0, 1, nullUAV, nullptr);
}
