#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <d3d9.h>
#include "../include/chat_listener.h"
#include "../include/config.h"
#include "../include/translator.h"
#include "../include/licensing.h"
#include "../include/clipboard_listener.h"

namespace SARPLinggo {

class OverlayGUI {
private:
    bool is_visible = true;       // The overlay window is shown
    bool is_cursor_mode = false;  // Cursor unlocked for interacting with overlay (hotkey toggle)
    std::string status_msg = "SA-RP Linggo ASI Active";
    std::vector<ChatItem> feed_items;
    std::mutex mtx;

    Config* config_ref = nullptr;
    GroqTranslator* translator_ref = nullptr;
    LicenseManager* license_ref = nullptr;
    ClipboardListener* clipboard_ref = nullptr;

    // Outbound manual test fields
    char outbound_input[512] = {0};
    std::string outbound_result = "";
    bool is_translating_outbound = false;

    // Settings fields buffers
    char api_key_buffer[16384] = {0};
    char chatlog_path_buffer[512] = {0};
    char license_token_buffer[128] = {0};
    std::string rpd_status_text = "";
    std::string license_action_msg = "";

public:
    void add_chat_card(const ChatItem& item);
    void set_status(const std::string& msg);
    void set_config(Config* config);
    void set_translator(GroqTranslator* translator) { translator_ref = translator; }
    void set_license_manager(LicenseManager* lm) { license_ref = lm; }
    void set_clipboard_listener(ClipboardListener* cl) { clipboard_ref = cl; }

    // Overlay visibility (window shown or hidden entirely)
    void toggle_visibility() { is_visible = !is_visible; }
    bool is_toggled() const { return is_visible; }

    // Cursor interaction mode
    void toggle_cursor_mode() {
        is_cursor_mode = !is_cursor_mode;
        ShowCursor(is_cursor_mode ? TRUE : FALSE);
    }
    bool is_in_cursor_mode() const { return is_cursor_mode; }

    void render();
};

} // namespace SARPLinggo
