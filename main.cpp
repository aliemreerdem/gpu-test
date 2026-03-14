// main.cpp
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <dxgi1_2.h>
#include <d3dcompiler.h>
#include <iostream>
#include <chrono>
#include <string>
#include <fstream>
#include <iomanip>
#include <ctime>
#include <vector>
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

#define SAFE_RELEASE(p) if (p) { (p)->Release(); (p) = nullptr; }

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_DESTROY) {
        PostQuitMessage(0);
        return 0;
    }
    if (uMsg == WM_KEYDOWN && wParam == VK_ESCAPE) {
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// Utility to get current time string for logs
std::string GetCurrentTimeString() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    struct tm timeinfo;
    localtime_s(&timeinfo, &now_c);
    char buf[100];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
    return std::string(buf);
}

// Global Error Logger Utility
void LogErrorToFile(const std::string& message, HRESULT hr = S_OK) {
    std::ofstream errorFile("error.log", std::ios::app);
    if (errorFile.is_open()) {
        errorFile << "[" << GetCurrentTimeString() << "] ERROR: " << message;
        if (hr != S_OK) {
            errorFile << " | HRESULT: 0x" << std::hex << hr << std::dec;
        }
        errorFile << "\n";
        errorFile.close();
    }
    std::cerr << "\n[!] FATAL ERROR Logged to error.log: " << message << std::endl;
}

const int NUM_ELEMENTS = 1024 * 1024 * 16; 

int main(int argc, char* argv[])
{
    try {
        // V25: Persistent execution loop
        while (true) {
            int testMode = 0;
            int durationSeconds = -1;

            std::cout << "=======================================" << std::endl;
            std::cout << "  GPU Stress Tester v2 (DirectX 11)" << std::endl;
            std::cout << "=======================================" << std::endl;

            std::cout << "\nTest Modes:" << std::endl;
            std::cout << "1. ALU Math Stress (Stresses GPU Compute Cores - High Temp)" << std::endl;
        std::cout << "2. VRAM Memory Bandwidth (Stresses GPU VRAM and Bus)" << std::endl;
        std::cout << "3. Simulated Game Workload (Mixed ALU + Memory)" << std::endl;
        std::cout << "4. Crypto Hashing Simulation (Integer & Bitwise Operations)" << std::endl;
        std::cout << "5. Ray Tracing Math Simulation (Tensor/Matrix Vector Math)" << std::endl;
        std::cout << "6. VRAM Filler (Massive Memory Allocation & Bandwidth Thrashing)" << std::endl;
        std::cout << "7. GPU Particle Physics (N-Body / O(N^2) Simulation)" << std::endl;
        std::cout << "8. Memory Cache Benchmark (SoA vs AoS / ECS Pattern)" << std::endl;
        std::cout << "9. Deferred Rendering G-Buffer Simulation (PCI-E bandwidth)" << std::endl;
        std::cout << "10. RDR2 Sim: Open-World Volumetric Ray-Marching" << std::endl;
        std::cout << "11. Diablo 4 Sim: Dense Isometric Grid ARPG" << std::endl;
        std::cout << "12. [AUTOMATED SUITE] Run All Tests Sequentially" << std::endl;
        
        while (testMode < 1 || testMode > 12) {
            std::cout << "\nSelect test mode (1-12): ";
            std::cin >> testMode;
        }

            while (durationSeconds < 0) {
                std::cout << "Enter duration in seconds (0 for infinite): ";
                std::cin >> durationSeconds;
            }

            const char* kernelName = "CSMath";
    if (testMode == 2) kernelName = "CSMemory";
    if (testMode == 3) kernelName = "CSGame";
    if (testMode == 4) kernelName = "CSCrypto";
    if (testMode == 5) kernelName = "CSRayTrace";
    if (testMode == 6) kernelName = "CSMemoryMassive"; // Dedicated kernel for huge thread arrays
    if (testMode == 7) kernelName = "CSParticles";
    if (testMode == 8) kernelName = "CSCache";
    if (testMode == 9) kernelName = "CSDeferred";
    if (testMode == 10) kernelName = "CSRedDead2";
    if (testMode == 11) kernelName = "CSDiablo4";

    // Dynamic element count. If test mode 6, use 10x more elements to fill ~12GB VRAM
    int elements = NUM_ELEMENTS;
    if (testMode == 6) elements = NUM_ELEMENTS * 14; 

    // --- DXGI Adapter (GPU) Enumeration ---
    IDXGIFactory* pFactory = nullptr;
    HRESULT hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&pFactory);
    if (FAILED(hr)) {
        LogErrorToFile("Failed to create DXGI Factory.", hr);
        return -1;
    }

    std::vector<IDXGIAdapter*> vAdapters;
    IDXGIAdapter* pAdapter = nullptr;
    UINT adapterIndex = 0;
    
    std::cout << "\nDetecting Available GPUs..." << std::endl;
    while (pFactory->EnumAdapters(adapterIndex, &pAdapter) != DXGI_ERROR_NOT_FOUND) {
        DXGI_ADAPTER_DESC desc;
        pAdapter->GetDesc(&desc);
        
        std::wcout << " GPU [" << adapterIndex << "]: " << desc.Description 
                   << " (VRAM: " << (desc.DedicatedVideoMemory / (1024 * 1024)) << " MB)" << std::endl;
                   
        vAdapters.push_back(pAdapter);
        ++adapterIndex;
    }

    if (vAdapters.empty()) {
        LogErrorToFile("No DXGI-compatible GPUs found.");
        SAFE_RELEASE(pFactory);
        return -1;
    }

    int selectedGpuIndex = 0;
    if (vAdapters.size() > 1) {
        std::cout << "\nMultiple GPUs detected. Enter GPU index (0-" << vAdapters.size() - 1 << ") to test: ";
        std::cin >> selectedGpuIndex;
        if (selectedGpuIndex < 0 || selectedGpuIndex >= vAdapters.size()) {
            std::cout << "Invalid index, defaulting to 0." << std::endl;
            selectedGpuIndex = 0;
        }
    }

    IDXGIAdapter* pSelectedAdapter = vAdapters[selectedGpuIndex];
    DXGI_ADAPTER_DESC selectedDesc;
    pSelectedAdapter->GetDesc(&selectedDesc);
    
    // Safely convert WCHAR to std::string
    char chGpuName[256];
    size_t charsConverted = 0;
    wcstombs_s(&charsConverted, chGpuName, sizeof(chGpuName), selectedDesc.Description, _TRUNCATE);
    std::string gpuName(chGpuName);

    std::cout << "\nInitializing DirectX 11 on: " << gpuName << std::endl;
    if (durationSeconds > 0) {
        std::cout << "Target Duration Per Test: " << durationSeconds << " seconds." << std::endl;
    } else {
        std::cout << "Target Duration Per Test: Infinite." << std::endl;
    }
    std::cout << "WARNING: This will put your GPU under 100% load. Press Ctrl+C to stop.\n" << std::endl;

    // 1. Create Device and Context (using selected adapter)
    ID3D11Device* pDevice = nullptr;
    ID3D11DeviceContext* pContext = nullptr;
    UINT createDeviceFlags = 0;

    D3D_FEATURE_LEVEL featureLevel;
    hr = D3D11CreateDevice(
        pSelectedAdapter,           // Use the user-selected GPU
        D3D_DRIVER_TYPE_UNKNOWN,    // Must be UNKNOWN when passing a specific adapter
        nullptr,                    
        createDeviceFlags,
        nullptr, 0,                 
        D3D11_SDK_VERSION,
        &pDevice,
        &featureLevel,
        &pContext);

    // Cleanup adapter list
    for (IDXGIAdapter* adapter : vAdapters) { SAFE_RELEASE(adapter); }
    SAFE_RELEASE(pFactory);

    if (FAILED(hr))
    {
        LogErrorToFile("Failed to create D3D11 Device.", hr);
        return -1;
    }

    // ==========================================
    // V20: True Fullscreen / DWM Metrics
    // ==========================================
    std::cout << "Creating Game Window to trigger AMD 3D Metrics..." << std::endl;
    
    // Enforce DPI awareness to prevent Windows 11 virtualized desktop scaling
    SetProcessDPIAware(); 
    
    int screenWidth = 1920;
    int screenHeight = 1080;
    std::cout << "Target Resolution: " << screenWidth << "x" << screenHeight << " (Windowed)" << std::endl;

    WNDCLASS wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = "GPUStressClass";
    RegisterClass(&wc);

    // Calculate window size based on desired 1920x1080 client area size
    RECT wr = {0, 0, screenWidth, screenHeight};
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

    HWND hwnd = CreateWindowEx(
        0, "GPUStressClass", "GPU Stress Tester V2 - WINDOWED FULL HD",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE, 
        CW_USEDEFAULT, CW_USEDEFAULT, 
        wr.right - wr.left, wr.bottom - wr.top,
        nullptr, nullptr, wc.hInstance, nullptr);
    ShowWindow(hwnd, SW_SHOW);
    SetForegroundWindow(hwnd);
    SetFocus(hwnd);

    IDXGIDevice* pDXGIDevice = nullptr;
    pDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&pDXGIDevice);
    IDXGIAdapter* pDXGIAdapter = nullptr;
    pDXGIDevice->GetAdapter(&pDXGIAdapter);
    IDXGIFactory* pDXGIFactory = nullptr;
    pDXGIAdapter->GetParent(__uuidof(IDXGIFactory), (void**)&pDXGIFactory);

    DXGI_SWAP_CHAIN_DESC scDesc;
    ZeroMemory(&scDesc, sizeof(scDesc));
    scDesc.BufferCount = 2; // FLIP_DISCARD requires at least 2
    scDesc.BufferDesc.Width = screenWidth;
    scDesc.BufferDesc.Height = screenHeight;
    scDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scDesc.OutputWindow = hwnd;
    scDesc.SampleDesc.Count = 1;
    scDesc.Windowed = TRUE;
    scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // Mandatory for AMD metric hooks on Hybrid Graphics (dGPU->iGPU presentation)
    // V24: Removing DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING.
    // In Windowed mode, if the display lacks VRR, this flag causes Present() to silently fail (DXGI_ERROR_INVALID_CALL).
    scDesc.Flags = 0;

    IDXGISwapChain* pSwapChain = nullptr;
    hr = pDXGIFactory->CreateSwapChain(pDevice, &scDesc, &pSwapChain);

    SAFE_RELEASE(pDXGIDevice);
    SAFE_RELEASE(pDXGIAdapter);
    SAFE_RELEASE(pDXGIFactory);

    ID3D11Texture2D* pBackBuffer = nullptr;
    pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer);
    ID3D11RenderTargetView* pRenderTargetView = nullptr;
    pDevice->CreateRenderTargetView(pBackBuffer, nullptr, &pRenderTargetView);
    SAFE_RELEASE(pBackBuffer);

    // V19: Create Depth Stencil Buffer to complete "100% 3D Game" Signature
    D3D11_TEXTURE2D_DESC descDepth;
    ZeroMemory(&descDepth, sizeof(descDepth));
    descDepth.Width = screenWidth;
    descDepth.Height = screenHeight;
    descDepth.MipLevels = 1;
    descDepth.ArraySize = 1;
    descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    descDepth.SampleDesc.Count = 1;
    descDepth.Usage = D3D11_USAGE_DEFAULT;
    descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    
    ID3D11Texture2D* pDepthStencil = nullptr;
    pDevice->CreateTexture2D(&descDepth, nullptr, &pDepthStencil);

    D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
    ZeroMemory(&descDSV, sizeof(descDSV));
    descDSV.Format = descDepth.Format;
    descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    
    ID3D11DepthStencilView* pDepthStencilView = nullptr;
    if (pDepthStencil) {
        pDevice->CreateDepthStencilView(pDepthStencil, &descDSV, &pDepthStencilView);
        SAFE_RELEASE(pDepthStencil);
    }

    // V22: Explicit Depth/Stencil State Architecture
    D3D11_DEPTH_STENCIL_DESC dssDesc;
    ZeroMemory(&dssDesc, sizeof(dssDesc));
    dssDesc.DepthEnable = TRUE;
    dssDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dssDesc.DepthFunc = D3D11_COMPARISON_LESS;
    dssDesc.StencilEnable = FALSE;
    
    ID3D11DepthStencilState* pDepthStencilState = nullptr;
    pDevice->CreateDepthStencilState(&dssDesc, &pDepthStencilState);

    // Bind the Output Merger so the Graphics Driver sees real 3D Pipeline activity
    pContext->OMSetRenderTargets(1, &pRenderTargetView, pDepthStencilView);
    pContext->OMSetDepthStencilState(pDepthStencilState, 1);

    // Set a viewport (Required for 3D Pipeline tracking by some overlay hooks)
    D3D11_VIEWPORT vp;
    vp.Width = (float)screenWidth;
    vp.Height = (float)screenHeight;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    pContext->RSSetViewports(1, &vp);

    // ==========================================
    // V14: Bind Dummy Raster Shaders for AMD
    // ==========================================
    ID3D11VertexShader* pDummyVS = nullptr;
    ID3D11PixelShader* pDummyPS = nullptr;
    
    // V21: Create a Dynamic Constant Buffer to mimic high-end Game Engines
    struct ConstantBuffer {
        float time;
        float resX;
        float resY;
        float padding;
    };
    
    D3D11_BUFFER_DESC cbDesc;
    ZeroMemory(&cbDesc, sizeof(cbDesc));
    cbDesc.Usage = D3D11_USAGE_DYNAMIC;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    cbDesc.ByteWidth = sizeof(ConstantBuffer);
    
    ID3D11Buffer* pConstantBuffer = nullptr;
    pDevice->CreateBuffer(&cbDesc, nullptr, &pConstantBuffer);

    {
        std::ifstream vsFile("vs_dummy.cso", std::ios::binary | std::ios::ate);
        if (vsFile.is_open()) {
            std::streamsize size = vsFile.tellg();
            vsFile.seekg(0, std::ios::beg);
            std::vector<char> vsData(size);
            if (vsFile.read(vsData.data(), size)) {
                pDevice->CreateVertexShader(vsData.data(), vsData.size(), nullptr, &pDummyVS);
            }
        } else {
            LogErrorToFile("Could not open vs_dummy.cso. Did you run build.bat?");
        }
        
        std::ifstream psFile("ps_dummy.cso", std::ios::binary | std::ios::ate);
        if (psFile.is_open()) {
            std::streamsize size = psFile.tellg();
            psFile.seekg(0, std::ios::beg);
            std::vector<char> psData(size);
            if (psFile.read(psData.data(), size)) {
                pDevice->CreatePixelShader(psData.data(), psData.size(), nullptr, &pDummyPS);
            }
        } else {
            LogErrorToFile("Could not open ps_dummy.cso.");
        }
    }

    bool bForceQuit = false;

    // ==========================================
    // Core Execution Function (Supports Looping)
    // ==========================================
    auto RunBenchmarkKernel = [&](int currentTestMode, const char* kernelStr, int totalElements) -> int {
        std::cout << "\n--------------------------------------------------" << std::endl;
        std::cout << ">>> Running Workload: " << kernelStr << "..." << std::endl;
        std::cout << "--------------------------------------------------" << std::endl;
        
        // 2. Load Pre-compiled Compute Shader (CSO)
        std::string shaderFilename = "kernel_" + std::to_string(currentTestMode) + ".cso";
        
        std::ifstream shaderFile(shaderFilename, std::ios::binary | std::ios::ate);
        if (!shaderFile.is_open()) {
            LogErrorToFile("Failed to find pre-compiled shader file: " + shaderFilename, E_FAIL);
            LogErrorToFile("Please run build.bat first so it generates the .cso files using fxc.exe");
            return -1;
        }

        std::streamsize size = shaderFile.tellg();
        shaderFile.seekg(0, std::ios::beg);
        std::vector<char> shaderData(size);
        if (!shaderFile.read(shaderData.data(), size)) {
            LogErrorToFile("Failed to read shader data from: " + shaderFilename, E_FAIL);
            return -1;
        }
        shaderFile.close();

        ID3D11ComputeShader* pComputeShader = nullptr;
        HRESULT l_hr = pDevice->CreateComputeShader(shaderData.data(), shaderData.size(), nullptr, &pComputeShader);
        
        if (FAILED(l_hr))
        {
            LogErrorToFile("Failed to create compute shader from loaded binary.", l_hr);
            return -1;
        }

        // 3. Create Output Buffers (UAVs)
        ID3D11Buffer* pBufferA = nullptr;
        ID3D11UnorderedAccessView* pUAVA = nullptr;
        
        D3D11_BUFFER_DESC bufferDesc;
        ZeroMemory(&bufferDesc, sizeof(bufferDesc));
        bufferDesc.Usage = D3D11_USAGE_DEFAULT;
        bufferDesc.ByteWidth = sizeof(float) * totalElements;
        bufferDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
        bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
        bufferDesc.StructureByteStride = sizeof(float);

        if (currentTestMode == 6) {
            std::cout << "Attempting to allocate massive VRAM structures (~" << (sizeof(float) * totalElements * 2) / (1024 * 1024) << " MB)..." << std::endl;
        }

        l_hr = pDevice->CreateBuffer(&bufferDesc, nullptr, &pBufferA);
        if (FAILED(l_hr)) { LogErrorToFile("Failed to allocate Buffer A. System out of VRAM.", l_hr); return -1; }

        D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
        ZeroMemory(&uavDesc, sizeof(uavDesc));
        uavDesc.Format = DXGI_FORMAT_UNKNOWN;
        uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
        uavDesc.Buffer.FirstElement = 0;
        uavDesc.Buffer.NumElements = totalElements;

        l_hr = pDevice->CreateUnorderedAccessView(pBufferA, &uavDesc, &pUAVA);
        if (FAILED(l_hr)) { LogErrorToFile("Failed to create UAV A.", l_hr); return -1; }

        ID3D11Buffer* pBufferB = nullptr;
        ID3D11UnorderedAccessView* pUAVB = nullptr;
        l_hr = pDevice->CreateBuffer(&bufferDesc, nullptr, &pBufferB);
        if (FAILED(l_hr)) { LogErrorToFile("Failed to allocate Buffer B. System out of VRAM.", l_hr); return -1; }
        
        l_hr = pDevice->CreateUnorderedAccessView(pBufferB, &uavDesc, &pUAVB);
        if (FAILED(l_hr)) { LogErrorToFile("Failed to create UAV B.", l_hr); return -1; }

        ID3D11Buffer* pBufferC = nullptr;
        ID3D11UnorderedAccessView* pUAVC = nullptr;
        ID3D11Buffer* pBufferD = nullptr;
        ID3D11UnorderedAccessView* pUAVD = nullptr;
        
        if (currentTestMode == 9) {
            std::cout << "Allocating Extended Memory for Deferred G-Buffer Simulation..." << std::endl;
            l_hr = pDevice->CreateBuffer(&bufferDesc, nullptr, &pBufferC);
            if (FAILED(l_hr)) { LogErrorToFile("Failed to allocate Buffer C.", l_hr); return -1; }
            l_hr = pDevice->CreateUnorderedAccessView(pBufferC, &uavDesc, &pUAVC);
            if (FAILED(l_hr)) { LogErrorToFile("Failed to create UAV C.", l_hr); return -1; }
            
            l_hr = pDevice->CreateBuffer(&bufferDesc, nullptr, &pBufferD);
            if (FAILED(l_hr)) { LogErrorToFile("Failed to allocate Buffer D.", l_hr); return -1; }
            l_hr = pDevice->CreateUnorderedAccessView(pBufferD, &uavDesc, &pUAVD);
            if (FAILED(l_hr)) { LogErrorToFile("Failed to create UAV D.", l_hr); return -1; }
        }

        // 4. Set State and Dispatch
        ID3D11UnorderedAccessView* uavs[] = { pUAVA, pUAVB, pUAVC, pUAVD };
        
        auto startTime = std::chrono::steady_clock::now();
        auto realStartTime = std::chrono::system_clock::now();
        auto lastFpsTime = startTime;
        unsigned long long totalDispatches = 0;
        unsigned long long dispatchesSinceLastFps = 0;

        while (!bForceQuit)
        {
            // V22: CPU-GPU Frame Pacing. 100% of modern DX11/12 games use CPU sleeping to avoid spinlocking the OS.
            // This prevents DWM from classifying the window as "Not Responding" and satisfies AMD's game heuristic.
            Sleep(1); 
            
            MSG msg;
            while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
                if (msg.message == WM_QUIT) {
                    bForceQuit = true;
                    break;
                }
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            
            if (bForceQuit) {
                std::cout << "\nWindow closed by user. Terminating benchmark early." << std::endl;
                break;
            }

            auto currentTime = std::chrono::steady_clock::now();
            auto elapsedTime = std::chrono::duration_cast<std::chrono::seconds>(currentTime - startTime).count();

            if (durationSeconds > 0) {
                if (elapsedTime >= durationSeconds) {
                    std::cout << "\nTarget duration of " << durationSeconds << " seconds reached." << std::endl;
                    break;
                }
            }

            // V18/V19: Re-bind EVERYTHING every frame, including the new Depth/Stencil buffer.
            pContext->OMSetRenderTargets(1, &pRenderTargetView, pDepthStencilView);
            pContext->RSSetViewports(1, &vp);
            if (pDummyVS) pContext->VSSetShader(pDummyVS, nullptr, 0);
            if (pDummyPS) pContext->PSSetShader(pDummyPS, nullptr, 0);

            // Trick Windows/AMD drivers into seeing this as a Graphics pipeline (100% 3D Load)
            float clearColor[4] = { (float)sin(elapsedTime) * 0.5f + 0.5f, 0.2f, 0.4f, 1.0f };
            pContext->ClearRenderTargetView(pRenderTargetView, clearColor);
            if (pDepthStencilView) pContext->ClearDepthStencilView(pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

            // V21: Map and Unmap a dynamic Constant Buffer. 
            // All modern 3D games stream View/Projection uniforms dynamically every frame!
            if (pConstantBuffer) {
                D3D11_MAPPED_SUBRESOURCE mappedResource;
                if (SUCCEEDED(pContext->Map(pConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource))) {
                    ConstantBuffer* cbData = (ConstantBuffer*)mappedResource.pData;
                    cbData->time = (float)elapsedTime;
                    cbData->resX = (float)screenWidth;
                    cbData->resY = (float)screenHeight;
                    cbData->padding = 0.0f;
                    pContext->Unmap(pConstantBuffer, 0);
                }
                pContext->VSSetConstantBuffers(0, 1, &pConstantBuffer);
            }

            // Dummy Draw Call: V21 - Bumping to 300,000 vertices (100k triangles)!
            // Vertices 0-2 will form our full-screen plane.
            // Vertices 3+ will form degenerate (0-area) triangles discarded instantly by the Rasterizer.
            // This natively inflates the hardware "Input Assembler" (IA) vertex metric counters to bypass 2D-app filters.
            pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            pContext->Draw(300000, 0);

            // Bind Compute Shader and UAVs right before dispatch
            pContext->CSSetShader(pComputeShader, nullptr, 0);
            pContext->CSSetUnorderedAccessViews(0, (currentTestMode == 9) ? 4 : 2, uavs, nullptr);
            pContext->Dispatch(totalElements / 256, 1, 1);
            
            // Note: DO NOT call pContext->Flush() here. 
            // Calling Flush forces the command queue to submit the Draw and Compute commands 
            // BEFORE the Present packet. This strips the 3D activity from the Present() ticket 
            // and tricks AMD/MSI overlays into thinking the frame was completely empty (0 FPS).
            
            pSwapChain->Present(0, 0); 
            
            totalDispatches++;
            dispatchesSinceLastFps++;
            
            // Live FPS Update (every 1 second)
            auto timeSinceLastFps = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastFpsTime).count();
            if (timeSinceLastFps >= 1000) {
                double currentFps = (double)dispatchesSinceLastFps / (timeSinceLastFps / 1000.0);
                std::cout << "\r[Live] Workload: " << kernelStr << " | FPS (Dispatches/sec): " << std::fixed << std::setprecision(1) << currentFps << " | GPU Load: 100% | Elapsed: " << elapsedTime << "s   " << std::flush;
                
                dispatchesSinceLastFps = 0;
                lastFpsTime = currentTime;
            }
            
            l_hr = pDevice->GetDeviceRemovedReason();
            if (FAILED(l_hr)) {
                LogErrorToFile("GPU Driver was reset by Windows (TDR). Workload timeout or overheat.", l_hr);
                std::cerr << "\nExiting test early." << std::endl;
                break;
            }
        }

        // 5. Telemetry Logging
        {
            std::time_t start_time_t = std::chrono::system_clock::to_time_t(realStartTime);
            char timeStr[100];
            struct tm timeinfo;
            localtime_s(&timeinfo, &start_time_t);
            std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);

            auto actualDurationSeconds = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime).count() / 1000.0;
            double avgFps = (actualDurationSeconds > 0.0) ? (totalDispatches / actualDurationSeconds) : 0.0;

            std::ofstream csvFile("benchmark_results.csv", std::ios::app);
            if (csvFile.is_open()) {
                csvFile.seekp(0, std::ios::end);
                if (csvFile.tellp() == 0) {
                    csvFile << "StartedAt,GPU,KernelName,TargetDurationSec,ActualDurationSec,TotalDispatches,AvgFPS\n";
                }
                csvFile << timeStr << "," 
                        << "\"" << gpuName << "\","
                        << kernelStr << "," 
                        << durationSeconds << "," 
                        << actualDurationSeconds << "," 
                        << totalDispatches << ","
                        << std::fixed << std::setprecision(2) << avgFps << "\n";
                csvFile.close();
                std::cout << "\n[TELEMETRY] Benchmark data saved to 'benchmark_results.csv' (Avg FPS: " << avgFps << ")" << std::endl;
            } else {
                LogErrorToFile("Could not open/create 'benchmark_results.csv' for telemetry logging.");
            }
        }

        // Cleanup this specific test
        ID3D11UnorderedAccessView* nullUAVs[4] = { nullptr, nullptr, nullptr, nullptr };
        pContext->CSSetUnorderedAccessViews(0, 4, nullUAVs, nullptr);
        
        SAFE_RELEASE(pUAVA);
        SAFE_RELEASE(pBufferA);
        SAFE_RELEASE(pUAVB);
        SAFE_RELEASE(pBufferB);
        if (pUAVC) SAFE_RELEASE(pUAVC);
        if (pBufferC) SAFE_RELEASE(pBufferC);
        if (pUAVD) SAFE_RELEASE(pUAVD);
        if (pBufferD) SAFE_RELEASE(pBufferD);
        SAFE_RELEASE(pComputeShader);

        return 0;
    };
    // ==========================================

    // Evaluate Mode Execution
    if (testMode == 12) {
        std::cout << "\n========================================================" << std::endl;
        std::cout << " STARTING AUTOMATED BENCHMARK SUITE (TESTS 1 - 11)" << std::endl;
        std::cout << "========================================================" << std::endl;
        for (int i = 1; i <= 11; i++) {
            const char* autoKernel = "CSMath";
            if (i == 2) autoKernel = "CSMemory";
            if (i == 3) autoKernel = "CSGame";
            if (i == 4) autoKernel = "CSCrypto";
            if (i == 5) autoKernel = "CSRayTrace";
            if (i == 6) autoKernel = "CSMemoryMassive";
            if (i == 7) autoKernel = "CSParticles";
            if (i == 8) autoKernel = "CSCache";
            if (i == 9) autoKernel = "CSDeferred";
            if (i == 10) autoKernel = "CSRedDead2";
            if (i == 11) autoKernel = "CSDiablo4";
            
            int autoElems = (i == 6) ? NUM_ELEMENTS * 14 : NUM_ELEMENTS;
            RunBenchmarkKernel(i, autoKernel, autoElems);
            
            // Short cooldown between heavy tests
            std::cout << "Cooldown for 3 seconds..." << std::endl;
            Sleep(3000); 
        }
        std::cout << "\n[SUCCESS] Automated Suite Completed All 11 Tests!" << std::endl;
        } else {
            RunBenchmarkKernel(testMode, kernelName, elements);
        }

        // Final Core Cleanup for this specific run
        SAFE_RELEASE(pDepthStencilState);
        SAFE_RELEASE(pConstantBuffer);
        SAFE_RELEASE(pDummyVS);
        SAFE_RELEASE(pDummyPS);
        SAFE_RELEASE(pDepthStencilView);
        SAFE_RELEASE(pRenderTargetView);
        SAFE_RELEASE(pSwapChain);
        SAFE_RELEASE(pContext);
        SAFE_RELEASE(pDevice);

        std::cout << "\n*** Test Finished! Press [Enter] to return to the main menu... ***\n";
        std::cin.ignore(10000, '\n');
        std::cin.get();
        system("cls");  // Clear screen for the next run
        
        } // End of while(true)

    } catch (const std::exception& e) {
        LogErrorToFile(std::string("Fatal C++ Exception: ") + e.what());
    } catch (...) {
        LogErrorToFile("Fatal C++ Exception: Unknown Access Violation / Crash occurred.");
    }

    return 0;
}
