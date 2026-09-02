#include "../include/d3d9_hook.h"
#include "../include/gui.h"
#include "../include/input_hook.h"
#include <windows.h>
#include <d3d9.h>
#include <fstream>
#include <iostream>
#include <MinHook.h>
#include "../imgui/imgui.h"
#include "../imgui/backends/imgui_impl_win32.h"
#include "../imgui/backends/imgui_impl_dx9.h"

namespace SARPLinggo {

extern OverlayGUI* g_gui;
extern std::string g_log_file_path;

static void LogDebugLocal(const std::string& msg) {
    try {
        std::ofstream log(g_log_file_path, std::ios::app);
        if (log.is_open()) {
            log << "[SA-RP Linggo] " << msg << std::endl;
        }
    } catch (...) {}
}

typedef HRESULT(APIENTRY* tPresent)(IDirect3DDevice9* pDevice, const RECT* pSourceRect, const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA* pDirtyRegion);
typedef HRESULT(APIENTRY* tReset)(IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters);
typedef BOOL(WINAPI* tSetCursorPos)(int X, int Y);

static tPresent oPresent = nullptr;
static tReset oReset = nullptr;
static tSetCursorPos oSetCursorPos = nullptr;

static bool imgui_initialized = false;
static IDirect3DDevice9* s_device = nullptr;
bool D3D9Hook::m_ready = false;

BOOL WINAPI hkSetCursorPos(int X, int Y) {
    if (g_gui && g_gui->is_toggled() && g_gui->is_in_cursor_mode()) {
        return TRUE; // Block GTA SA from resetting mouse cursor to screen center while unlocked
    }
    return oSetCursorPos(X, Y);
}

static bool GetD3D9VTable(void** pVTable) {
    // 1. Try fetching existing GTA SA D3D Device at 0xC97C28
    IDirect3DDevice9* pDevice = *(IDirect3DDevice9**)0xC97C28;
    if (pDevice != nullptr) {
        void** vtbl = *(void***)pDevice;
        if (vtbl != nullptr) {
            pVTable[0] = vtbl[17]; // Present
            pVTable[1] = vtbl[16]; // Reset
            LogDebugLocal("GetD3D9VTable: Obtained vtable from 0xC97C28 GTA SA Device");
            return true;
        }
    }

    // 2. Dummy D3D9 Device Creation Fallback
    IDirect3D9* pD3D = Direct3DCreate9(D3D_SDK_VERSION);
    if (!pD3D) {
        LogDebugLocal("GetD3D9VTable: Direct3DCreate9 failed");
        return false;
    }

    HWND hWnd = CreateWindowA("BUTTON", "Dummy", WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, NULL, NULL, NULL, NULL);
    if (!hWnd) {
        pD3D->Release();
        return false;
    }

    D3DPRESENT_PARAMETERS d3dpp = {};
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.hDeviceWindow = hWnd;

    IDirect3DDevice9* pDummyDevice = nullptr;
    HRESULT hr = pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
        D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pDummyDevice);

    if (FAILED(hr) || !pDummyDevice) {
        LogDebugLocal("GetD3D9VTable: CreateDevice dummy failed HRESULT: " + std::to_string(hr));
        DestroyWindow(hWnd);
        pD3D->Release();
        return false;
    }

    void** vtbl = *(void***)pDummyDevice;
    pVTable[0] = vtbl[17]; // Present
    pVTable[1] = vtbl[16]; // Reset

    pDummyDevice->Release();
    DestroyWindow(hWnd);
    pD3D->Release();

    LogDebugLocal("GetD3D9VTable: Obtained vtable from dummy D3D9 device.");
    return true;
}

static HRESULT APIENTRY hkPresent(IDirect3DDevice9* pDevice, const RECT* pSourceRect, const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA* pDirtyRegion) {
    if (!imgui_initialized) {
        s_device = pDevice;
        D3DDEVICE_CREATION_PARAMETERS params;
        HWND hwnd = NULL;
        if (SUCCEEDED(pDevice->GetCreationParameters(&params))) {
            hwnd = params.hFocusWindow;
        }
        if (!hwnd) {
            hwnd = FindWindowA(NULL, "Grand Theft Auto San Andreas");
            if (!hwnd) hwnd = FindWindowA("Grand over San Andreas", NULL);
            if (!hwnd) hwnd = GetForegroundWindow();
        }

        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

        InputHook::init(hwnd);

        ImGui_ImplWin32_Init(hwnd);
        ImGui_ImplDX9_Init(pDevice);

        imgui_initialized = true;
        LogDebugLocal("ImGui initialized successfully with D3DDevice window");
    }

    if (g_gui && g_gui->is_toggled()) {
        if (pDevice->TestCooperativeLevel() == D3D_OK) {
            IDirect3DStateBlock9* pStateBlock = nullptr;
            if (SUCCEEDED(pDevice->CreateStateBlock(D3DSBT_ALL, &pStateBlock)) && pStateBlock) {
                pStateBlock->Capture();
            }

            ImGui_ImplDX9_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();

            g_gui->render();

            ImGui::EndFrame();
            ImGui::Render();
            ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());

            if (pStateBlock) {
                pStateBlock->Apply();
                pStateBlock->Release();
            }
        }
    }

    return oPresent(pDevice, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
}

static HRESULT APIENTRY hkReset(IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters) {
    if (imgui_initialized) {
        ImGui_ImplDX9_InvalidateDeviceObjects();
    }
    HRESULT hr = oReset(pDevice, pPresentationParameters);
    if (SUCCEEDED(hr) && imgui_initialized) {
        ImGui_ImplDX9_CreateDeviceObjects();
    }
    return hr;
}

static void imgui_shutdown() {
    if (imgui_initialized) {
        ImGui_ImplDX9_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        imgui_initialized = false;
    }
}

bool D3D9Hook::init() {
    if (m_ready) return true;

    LogDebugLocal("D3D9Hook::init starting...");

    void* vtable[2] = { nullptr, nullptr };
    if (!GetD3D9VTable(vtable)) {
        LogDebugLocal("GetD3D9VTable failed");
        return false;
    }

    MH_STATUS status = MH_Initialize();
    if (status != MH_OK && status != MH_ERROR_ALREADY_INITIALIZED) {
        LogDebugLocal("MH_Initialize failed Status: " + std::to_string(status));
        return false;
    }

    status = MH_CreateHook(vtable[0], (LPVOID)&hkPresent, (LPVOID*)&oPresent);
    if (status != MH_OK) {
        LogDebugLocal("MH_CreateHook Present failed Status: " + std::to_string(status));
        return false;
    }

    status = MH_CreateHook(vtable[1], (LPVOID)&hkReset, (LPVOID*)&oReset);
    if (status != MH_OK) {
        LogDebugLocal("MH_CreateHook Reset failed Status: " + std::to_string(status));
        return false;
    }

    status = MH_CreateHook((LPVOID)&SetCursorPos, (LPVOID)&hkSetCursorPos, (LPVOID*)&oSetCursorPos);
    if (status != MH_OK) {
        LogDebugLocal("MH_CreateHook SetCursorPos failed Status: " + std::to_string(status));
    }

    status = MH_EnableHook(MH_ALL_HOOKS);
    if (status != MH_OK) {
        LogDebugLocal("MH_EnableHook failed Status: " + std::to_string(status));
        return false;
    }

    m_ready = true;
    LogDebugLocal("D3D9Hook::init completed SUCCESSFULLY");
    return true;
}

void D3D9Hook::shutdown() {
    if (!m_ready) return;

    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();

    InputHook::shutdown();
    imgui_shutdown();

    m_ready = false;
}

} // namespace SARPLinggo