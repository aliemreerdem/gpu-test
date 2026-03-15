#pragma once
#include <string>
#include <windows.h>

class Logger {
public:
    static std::string GetCurrentTimeString();
    static void LogError(const std::string& message, HRESULT hr = S_OK);
    static void LogInfo(const std::string& message);
    static void LogWarning(const std::string& message);
};
