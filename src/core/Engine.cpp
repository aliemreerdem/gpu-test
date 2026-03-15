#include "Engine.h"
#include "Window.h"
#include "Logger.h"
#include "../graphics/Renderer.h"
#include "../graphics/ComputeBenchmarker.h"
#include <iostream>
#include <fstream>
#include <chrono>

Engine::Engine(int testMode, int durationSeconds, int elements, int gpuIndex)
    : m_testMode(testMode), m_durationSeconds(durationSeconds), 
      m_elements(elements), m_gpuIndex(gpuIndex),
      m_window(nullptr), m_renderer(nullptr), m_benchmarker(nullptr),
      m_totalFps(0), m_fpsSamples(0)
{
}

Engine::~Engine() {
    delete m_benchmarker;
    delete m_renderer;
    delete m_window;
}

bool Engine::Initialize() {
    m_window = new Window(1920, 1080, "GPU Stress Tester V2 - WINDOWED FULL HD");
    m_renderer = new Renderer(m_window->GetHandle(), m_window->GetWidth(), m_window->GetHeight());
    
    if (!m_renderer->Initialize(m_gpuIndex)) {
        Logger::LogError("Renderer failed to initialize.");
        return false;
    }

    m_benchmarker = new ComputeBenchmarker(m_renderer);
    if (!m_benchmarker->LoadKernel(m_testMode)) return false;
    if (!m_benchmarker->InitializeBuffers(m_elements)) return false;

    return true;
}

void Engine::Run() {
    Logger::LogInfo("Beginning Benchmark Loop...");

    // V26 AMD Windowed Heuristic Slicing
    int frameNum = 0;
    int dispatchSize = m_elements;
    int divisor = 16;
    if (m_testMode <= 9) dispatchSize = m_elements / divisor;

    std::chrono::time_point<std::chrono::high_resolution_clock> testStartTime = std::chrono::high_resolution_clock::now();
    std::chrono::time_point<std::chrono::high_resolution_clock> tStart = std::chrono::high_resolution_clock::now();
    
    bool bForceQuit = false;

    // Fixed tick message pump handling (Carmack style)
    while (m_window->ProcessMessages() && !bForceQuit) {
        
        // V26: Artificial Thread Delay (AMD Game Pacing Heuristic)
        Sleep(1);
        
        m_benchmarker->Dispatch(dispatchSize);
        m_renderer->Present();

        if (m_testMode <= 9 && frameNum % divisor != (divisor - 1)) {
            frameNum++;
            continue;
        }

        auto tEnd = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration<double, std::milli>(tEnd - tStart).count();
        double fps = 1000.0 / ms;

        m_totalFps += fps;
        m_fpsSamples++;

        std::cout << "\r[Test " << m_testMode << "] Frame Time: " << ms << " ms | FPS: " << fps << "       " << std::flush;

        if (m_durationSeconds > 0) {
            double elapsedTestSeconds = std::chrono::duration<double>(tEnd - testStartTime).count();
            if (elapsedTestSeconds >= m_durationSeconds) {
                bForceQuit = true;
            }
        }

        tStart = std::chrono::high_resolution_clock::now();
        frameNum++;
    }

    std::cout << "\nTest Complete." << std::endl;
    
    double avgFps = m_fpsSamples > 0 ? (m_totalFps / m_fpsSamples) : 0.0;
    std::cout << "-> Average FPS: " << avgFps << "\n" << std::endl;

    std::ofstream csvFile("benchmark_results.csv", std::ios::app);
    if (csvFile.is_open()) {
        csvFile << Logger::GetCurrentTimeString() << "," << m_testMode << "," << m_durationSeconds 
                << "," << avgFps << "," << m_elements << "\n";
        csvFile.close();
    }
}
