#include "Window.h"
#include <iostream>

LRESULT CALLBACK Window::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_CLOSE) {
        PostQuitMessage(0);
        return 0;
    }
    if (uMsg == WM_KEYDOWN && wParam == VK_ESCAPE) {
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

Window::Window(int width, int height, const std::string& title) 
    : m_width(width), m_height(height), m_title(title), m_hwnd(nullptr)
{
    m_hInstance = GetModuleHandle(nullptr);
    SetProcessDPIAware(); 

    WNDCLASS wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = m_hInstance;
    wc.lpszClassName = "GPUStressClass";
    RegisterClass(&wc);

    RECT wr = {0, 0, m_width, m_height};
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

    m_hwnd = CreateWindowEx(
        0, "GPUStressClass", m_title.c_str(),
        WS_OVERLAPPEDWINDOW | WS_VISIBLE, 
        CW_USEDEFAULT, CW_USEDEFAULT, 
        wr.right - wr.left, wr.bottom - wr.top,
        nullptr, nullptr, m_hInstance, nullptr);
        
    ShowWindow(m_hwnd, SW_SHOW);
    SetForegroundWindow(m_hwnd);
    SetFocus(m_hwnd);
}

Window::~Window() {
    if (m_hwnd) {
        DestroyWindow(m_hwnd);
    }
    UnregisterClass("GPUStressClass", m_hInstance);
}

bool Window::ProcessMessages() {
    MSG msg;
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            return false;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return true;
}
