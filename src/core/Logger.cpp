#include "Logger.h"
#include <iostream>
#include <fstream>
#include <chrono>

std::string Logger::GetCurrentTimeString() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    struct tm timeinfo;
    localtime_s(&timeinfo, &now_c);
    char buf[100];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
    return std::string(buf);
}

void Logger::LogError(const std::string& message, HRESULT hr) {
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

void Logger::LogInfo(const std::string& message) {
    std::cout << "[INFO] " << message << std::endl;
}

void Logger::LogWarning(const std::string& message) {
    std::cout << "[WARNING] " << message << std::endl;
}
