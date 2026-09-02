#pragma once

#include <string>
#include <vector>
#include <windows.h>

namespace SARPLinggo {

struct Config {
    std::string groq_api_key = "";
    std::string groq_model = "openai/gpt-oss-20b";
    std::string target_language = "Indonesian";
    std::string outbound_style = "Standard English"; // "Standard English" or "American Hood"
    std::string chatlog_path = "";
    std::string license_token = "";
    bool use_codsmp = true;
    bool enable_clipboard_outbound = true;
    bool enable_inbound_chatlog = true;
    bool auto_translate_ic = true;
    bool auto_translate_me_do = true;
    bool developer_mode = false;
    int font_size = 11;
    int toggle_hotkey = VK_F7; // Default F7
    float overlay_opacity = 0.90f;
    bool click_through = false;
    int max_feed_items = 50;

    // Load configuration from INI file
    bool load(const std::string& ini_path);

    // Save configuration to INI file
    bool save(const std::string& ini_path);

    // Dynamic game directory helper
    static std::string get_game_directory();

    // Auto-detect standard SAMP chatlog.txt path
    static std::string detect_chatlog_path();

    // Auto-detect Groq API key from environment variable or Downloads folder
    static std::string detect_groq_api_key();

    // Helper to parse multiple comma/space separated keys
    std::vector<std::string> get_api_keys() const;
};

} // namespace SARPLinggo
