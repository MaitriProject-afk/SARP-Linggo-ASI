#include <windows.h>
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>

#include "../include/config.h"
#include "../include/translator.h"
#include "../include/chat_listener.h"
#include "../include/clipboard_listener.h"
#include "../include/voice_listener.h"
#include "../include/licensing.h"
#include "../include/gui.h"
#include "../include/d3d9_hook.h"
#include "../include/input_hook.h"
#include "../imgui/imgui.h"

namespace SARPLinggo {

std::string g_log_file_path = Config::get_game_directory() + "\\SARPLinggo_debug.log";

OverlayGUI g_gui_instance;
OverlayGUI* g_gui = &g_gui_instance;

Config g_config;
GroqTranslator g_translator;
LicenseManager g_license(Config::get_game_directory() + "\\SARPLinggo_license.json");
ChatlogListener g_chat_listener;
ClipboardListener g_clipboard_listener;
VoiceListener g_voice_listener;

static void LogDebugMain(const std::string& msg) {
    try {
        std::ofstream log(g_log_file_path, std::ios::app);
        if (log.is_open()) {
            log << "[SA-RP Linggo] " << msg << std::endl;
        }
    } catch (...) {}
}

static LONG WINAPI CrashFilter(EXCEPTION_POINTERS* pExceptionInfo) {
    if (pExceptionInfo && pExceptionInfo->ExceptionRecord) {
        DWORD code = pExceptionInfo->ExceptionRecord->ExceptionCode;
        PVOID addr = pExceptionInfo->ExceptionRecord->ExceptionAddress;

        char mod_name[MAX_PATH] = "Unknown Module";
        HMODULE hMod = NULL;
        if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)addr, &hMod)) {
            GetModuleFileNameA(hMod, mod_name, sizeof(mod_name));
        }

        std::stringstream ss;
        ss << "[CRASH DETECTED] Exception Code: 0x" << std::hex << code << " at Address: 0x" << (DWORD)addr << " (" << mod_name << ")";
        LogDebugMain(ss.str());
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

static bool IsValidReadPtr(const void* p, size_t size) {
    if (!p) return false;
    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQuery(p, &mbi, sizeof(mbi)) == 0) return false;
    if (mbi.State != MEM_COMMIT) return false;
    if (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)) return false;
    return true;
}

static bool IsSAMPChatInputOpen() {
    try {
        HMODULE hSAMP = GetModuleHandleA("samp.dll");
        if (!hSAMP) return false;

        DWORD dwSAMP = (DWORD)hSAMP;

        // SAMP offsets for CInput pointer across SAMP 0.3.DL, 0.3.7 R1, R3, R4, R5
        DWORD offsets[] = { 0x2A1470, 0x21A0E8, 0x26E8CC, 0x26E9C8, 0x2A1468 };
        for (DWORD offset : offsets) {
            DWORD pInputAddr = dwSAMP + offset;
            if (!IsValidReadPtr((void*)pInputAddr, sizeof(DWORD))) continue;

            DWORD pInput = *(DWORD*)pInputAddr;
            if (pInput && IsValidReadPtr((void*)pInput, 16)) {
                int enabled1 = *(int*)(pInput + 0x4);
                int enabled2 = *(int*)(pInput + 0x8);
                if (enabled1 == 1 || enabled2 == 1) {
                    return true;
                }
            }
        }
    } catch (...) {}
    return false;
}

DWORD WINAPI PluginMainThread(LPVOID lpParam) {
    SetUnhandledExceptionFilter(CrashFilter);
    g_log_file_path = Config::get_game_directory() + "\\SARPLinggo_debug.log";
    LogDebugMain("PluginMainThread started inside process.");

    std::string ini_path = Config::get_game_directory() + "\\SARPLinggo.ini";
    try {
        g_config.load(ini_path);
        LogDebugMain("Loaded INI Config successfully.");
    } catch (const std::exception& e) {
        LogDebugMain(std::string("Exception loading config: ") + e.what());
    } catch (...) {
        LogDebugMain("Unknown exception loading config.");
    }

    g_gui->set_config(&g_config);
    g_gui->set_translator(&g_translator);
    g_gui->set_license_manager(&g_license);
    g_gui->set_clipboard_listener(&g_clipboard_listener);

    g_translator.set_license_manager(&g_license);
    g_translator.set_developer_mode(g_config.developer_mode);
    g_translator.update_api_keys(g_config.get_api_keys());

    g_chat_listener.set_path(g_config.chatlog_path);
    g_chat_listener.set_use_codsmp(g_config.use_codsmp);

    g_chat_listener.set_on_new_item([](const ChatItem& item) {
        ChatItem item_to_add = item;
        try {
            if (g_config.enable_inbound_chatlog) {
                bool should_tr = false;
                if (item.type == "SAYS" && g_config.auto_translate_ic) should_tr = true;
                if ((item.type == "ME" || item.type == "DO") && g_config.auto_translate_me_do) should_tr = true;

                if (should_tr && should_translate_inbound(item.content)) {
                    std::string res = g_translator.translate_inbound(item.content, item.type);
                    if (!res.empty()) {
                        item_to_add.translated = res;
                    } else {
                        item_to_add.translated = "[Gagal Menerjemahkan]";
                    }
                }
            }
        } catch (const std::exception& e) {
            LogDebugMain(std::string("Exception in on_new_item: ") + e.what());
        } catch (...) {
            LogDebugMain("Unknown exception in on_new_item");
        }
        g_gui->add_chat_card(item_to_add);
    });

    g_chat_listener.start();

    g_clipboard_listener.init(&g_translator, g_config.outbound_style);
    g_clipboard_listener.set_enabled(g_config.enable_clipboard_outbound);
    g_clipboard_listener.set_on_translated([](const std::string& orig, const std::string& trans) {
        std::string status = "Tersalin ke Clipboard: \"" + trans + "\"";
        g_gui->set_status(status);

        // Add Outbound Chat Card to Feed Chat tab in Overlay
        ChatItem card;
        card.speaker = "Clipboard (Outbound)";
        card.type = "OUTBOUND";
        card.content = orig;
        card.translated = trans;

        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        struct tm buf;
        localtime_s(&buf, &in_time_t);
        char time_str[32];
        strftime(time_str, sizeof(time_str), "%H:%M:%S", &buf);
        card.timestamp = std::string(time_str);

        g_gui->add_chat_card(card);
    });
    g_clipboard_listener.start();

    g_voice_listener.init(&g_translator, &g_config);
    g_voice_listener.set_on_status([](const std::string& status_msg, const std::string& color_hex) {
        g_gui->set_status(status_msg);
    });
    g_voice_listener.set_on_voice_translated([](const std::string& orig, const std::string& trans) {
        ChatItem card;
        card.speaker = "MIC (Voice Outbound)";
        card.type = "OUTBOUND_VOICE";
        card.content = orig;
        card.translated = trans;

        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        struct tm buf;
        localtime_s(&buf, &in_time_t);
        char time_str[32];
        strftime(time_str, sizeof(time_str), "%H:%M:%S", &buf);
        card.timestamp = std::string(time_str);

        g_gui->add_chat_card(card);
    });
    g_voice_listener.start();

    // Hook D3D9 for rendering ImGui inside GTA SA
    int hook_attempts = 0;
    while (!D3D9Hook::is_ready() && hook_attempts < 20) {
        hook_attempts++;
        LogDebugMain("Attempting D3D9 Hook injection attempt " + std::to_string(hook_attempts));
        if (D3D9Hook::init()) {
            LogDebugMain("D3D9 Hook injected successfully");
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    LogDebugMain("Entering main polling loop.");

    // Hotkey Polling Loop
    bool shift_h_was_down = false;
    bool shift_enter_was_down = false;

    while (true) {
        try {
            bool shift_down = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
            bool h_down = (GetAsyncKeyState('H') & 0x8000) != 0;
            bool enter_down = (GetAsyncKeyState(VK_RETURN) & 0x8000) != 0;

            bool chat_input_open = IsSAMPChatInputOpen();

            // Shift + H -> Toggle Hide/Show Overlay Window (Only when NOT typing in SA-MP chat)
            if (shift_down && h_down) {
                if (!shift_h_was_down) {
                    if (!chat_input_open) {
                        g_gui->toggle_visibility();
                        LogDebugMain("Hotkey Shift+H toggled overlay visibility. Now visible: " + std::to_string(g_gui->is_toggled()));
                    }
                    shift_h_was_down = true;
                }
            } else {
                shift_h_was_down = false;
            }

            // Shift + Enter -> Toggle Cursor Interaction Mode (Only when NOT typing in SA-MP chat)
            if (shift_down && enter_down) {
                if (!shift_enter_was_down) {
                    if (!chat_input_open) {
                        g_gui->toggle_cursor_mode();
                        LogDebugMain("Hotkey Shift+Enter toggled cursor mode. Now: " + std::to_string(g_gui->is_in_cursor_mode()));
                    }
                    shift_enter_was_down = true;
                }
            } else {
                shift_enter_was_down = false;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        } catch (const std::exception& e) {
            LogDebugMain(std::string("Exception in PluginMainThread: ") + e.what());
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        } catch (...) {
            LogDebugMain("Unknown Exception in PluginMainThread");
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }

    return 0;
}

} // namespace SARPLinggo

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)SARPLinggo::PluginMainThread, NULL, 0, NULL);
    } else if (ul_reason_for_call == DLL_PROCESS_DETACH) {
        SARPLinggo::D3D9Hook::shutdown();
        SARPLinggo::g_chat_listener.stop();
        SARPLinggo::g_clipboard_listener.stop();
    }
    return TRUE;
}