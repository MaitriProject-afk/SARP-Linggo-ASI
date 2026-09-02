#include "../include/input_hook.h"
#include <imgui.h>
#include "../include/gui.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace SARPLinggo {

bool InputHook::m_ready = false;
HWND InputHook::m_hwnd = nullptr;
WNDPROC InputHook::m_original_wndproc = nullptr;



extern OverlayGUI* g_gui;

LRESULT CALLBACK InputHook::WndProcHook(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (g_gui && g_gui->is_toggled()) {
        if (g_gui->is_in_cursor_mode()) {
            if (msg == WM_SETCURSOR) {
                SetCursor(LoadCursor(NULL, IDC_ARROW));
                return TRUE;
            }

            ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);

            ImGuiIO& io = ImGui::GetIO();

            // When unlocked, swallow mouse messages so GTA SA camera doesn't rotate / shoot while interacting
            if (msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST) {
                return TRUE;
            }

            if (io.WantCaptureKeyboard && (msg >= WM_KEYFIRST && msg <= WM_KEYLAST)) {
                return TRUE;
            }
        }
    }

    return CallWindowProc(m_original_wndproc, hWnd, msg, wParam, lParam);
}

bool InputHook::init(HWND hwnd) {
    if (m_ready) return true;
    m_hwnd = hwnd;

    // WndProc subclass for keyboard / ImGui window messages
    m_original_wndproc = (WNDPROC)SetWindowLongPtr(m_hwnd, GWLP_WNDPROC, (LONG_PTR)WndProcHook);

    m_ready = (m_original_wndproc != nullptr);
    return m_ready;
}

void InputHook::shutdown() {
    if (m_ready && m_hwnd && m_original_wndproc) {
        SetWindowLongPtr(m_hwnd, GWLP_WNDPROC, (LONG_PTR)m_original_wndproc);
        m_ready = false;
    }
}

} // namespace SARPLinggo
