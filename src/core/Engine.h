#pragma once
#include <string>

class Window;
class Renderer;
class ComputeBenchmarker;

class Engine {
public:
    Engine(int testMode, int durationSeconds, int elements, int gpuIndex);
    ~Engine();

    bool Initialize();
    void Run();

private:
    int m_testMode;
    int m_durationSeconds;
    int m_elements;
    int m_gpuIndex;
    
    Window* m_window;
    Renderer* m_renderer;
    ComputeBenchmarker* m_benchmarker;
    
    // Telemetry
    double m_totalFps;
    int m_fpsSamples;
};
