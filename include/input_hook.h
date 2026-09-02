#pragma once
#include <windows.h>

namespace SARPLinggo {

class InputHook {
public:
    static bool init(HWND hwnd);
    static void shutdown();
    static bool is_ready() { return m_ready; }
    
    static HWND get_hwnd() { return m_hwnd; }

private:
    static bool m_ready;
    static HWND m_hwnd;
    static WNDPROC m_original_wndproc;
    
    static LRESULT CALLBACK WndProcHook(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

} // namespace SARPLinggo
