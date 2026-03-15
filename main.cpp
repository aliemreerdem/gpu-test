// main.cpp - Entry Point Bootstrapper
#include "src/core/Engine.h"
#include "src/core/Logger.h"
#include "src/graphics/Renderer.h"
#include <iostream>
#include <vector>

const int NUM_ELEMENTS = 1024 * 1024 * 16; 

int main()
{
    try {
        while (true) {
            int testMode = 0;
            int durationSeconds = -1;

            std::cout << "=======================================" << std::endl;
            std::cout << "  GPU Stress Tester v3 (DirectX 11 OOP)" << std::endl;
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
                if (std::cin.fail()) {
                    std::cin.clear();
                    std::cin.ignore(10000, '\n');
                }
            }

            while (durationSeconds < 0) {
                std::cout << "Enter duration in seconds (0 for infinite): ";
                std::cin >> durationSeconds;
                if (std::cin.fail()) {
                    std::cin.clear();
                    std::cin.ignore(10000, '\n');
                }
            }

            std::cout << "\nScanning for compatible GPUs..." << std::endl;
            auto adapters = Renderer::EnumerateAdapters();
            int selectedGpuIndex = 0;
            
            if (adapters.empty()) {
                std::cout << "[ERROR] No DirectX 11 compatible GPUs found!" << std::endl;
                system("pause");
                return -1;
            }

            if (adapters.size() == 1) {
                DXGI_ADAPTER_DESC desc;
                adapters[0]->GetDesc(&desc);
                std::wcout << "[Selected GPU]: " << desc.Description << " (" << (desc.DedicatedVideoMemory / 1024 / 1024) << " MB VRAM)\n";
            } else {
                std::cout << "\nAvailable GPUs:" << std::endl;
                for (size_t i = 0; i < adapters.size(); ++i) {
                    DXGI_ADAPTER_DESC desc;
                    adapters[i]->GetDesc(&desc);
                    std::wcout << "  [" << i << "] " << desc.Description << " (" << (desc.DedicatedVideoMemory / 1024 / 1024) << " MB VRAM)\n";
                }
                
                selectedGpuIndex = -1;
                while (selectedGpuIndex < 0 || selectedGpuIndex >= (int)adapters.size()) {
                    std::cout << "\nSelect target GPU index (0-" << adapters.size() - 1 << "): ";
                    std::cin >> selectedGpuIndex;
                    if (std::cin.fail()) {
                        std::cin.clear();
                        std::cin.ignore(10000, '\n');
                        selectedGpuIndex = -1;
                    }
                }
            }

            for (auto adapter : adapters) {
                adapter->Release();
            }

            if (testMode == 12) {
                std::cout << "\n[AUTOMATED SUITE] Running all tests for " << durationSeconds << " seconds each.\n" << std::endl;
                for (int i = 1; i <= 11; ++i) {
                    std::cout << "\n=== Starting Automated Test " << i << " ===" << std::endl;
                    int elements = (i == 6) ? NUM_ELEMENTS * 14 : NUM_ELEMENTS;
                    Engine engine(i, durationSeconds, elements, selectedGpuIndex);
                    if (engine.Initialize()) {
                        engine.Run();
                    } else {
                        Logger::LogError("Engine failed to initialize for Test " + std::to_string(i));
                    }
                }
                std::cout << "\n[AUTOMATED SUITE] All tests completed." << std::endl;
            } else {
                int elements = (testMode == 6) ? NUM_ELEMENTS * 14 : NUM_ELEMENTS;
                Engine engine(testMode, durationSeconds, elements, selectedGpuIndex);
                if (engine.Initialize()) {
                    engine.Run();
                } else {
                    Logger::LogError("Engine failed to initialize.");
                }
            }

            std::cout << "\nTests finished. Press any key to return to menu..." << std::endl;
            system("pause");
        }
    }
    catch (const std::exception& e) {
        Logger::LogError(std::string("Exception caught in main: ") + e.what());
        std::cerr << "Exception: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}
