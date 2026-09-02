#include "../include/config.h"
#include <windows.h>
#include <shlobj.h>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;

namespace SARPLinggo {

static std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

bool Config::load(const std::string& ini_path) {
    try {
        std::error_code ec;
        if (!fs::exists(ini_path, ec)) {
            chatlog_path = detect_chatlog_path();
            groq_api_key = detect_groq_api_key();
            save(ini_path);
            return false;
        }
    } catch (...) {}

    char buffer[2048];

    // Read keys
    GetPrivateProfileStringA("Settings", "GroqAPIKey", "", buffer, sizeof(buffer), ini_path.c_str());
    groq_api_key = trim(buffer);
    if (groq_api_key.empty()) {
        groq_api_key = detect_groq_api_key();
    }

    GetPrivateProfileStringA("Settings", "GroqModel", "openai/gpt-oss-20b", buffer, sizeof(buffer), ini_path.c_str());
    groq_model = trim(buffer);
    if (groq_model.empty()) {
        groq_model = "openai/gpt-oss-20b";
    }

    GetPrivateProfileStringA("Settings", "TargetLanguage", "Indonesian", buffer, sizeof(buffer), ini_path.c_str());
    target_language = trim(buffer);

    GetPrivateProfileStringA("Settings", "OutboundStyle", "Standard English", buffer, sizeof(buffer), ini_path.c_str());
    outbound_style = trim(buffer);

    GetPrivateProfileStringA("Settings", "ChatlogPath", "", buffer, sizeof(buffer), ini_path.c_str());
    chatlog_path = trim(buffer);

    GetPrivateProfileStringA("Settings", "LicenseToken", "", buffer, sizeof(buffer), ini_path.c_str());
    license_token = trim(buffer);

    try {
        std::error_code ec;
        if (chatlog_path.empty() || !fs::exists(chatlog_path, ec)) {
            chatlog_path = detect_chatlog_path();
        }
    } catch (...) {}

    use_codsmp = GetPrivateProfileIntA("Settings", "UseCodSMP", 1, ini_path.c_str()) != 0;
    enable_clipboard_outbound = GetPrivateProfileIntA("Settings", "EnableClipboardOutbound", 1, ini_path.c_str()) != 0;
    enable_inbound_chatlog = GetPrivateProfileIntA("Settings", "EnableInboundChatlog", 1, ini_path.c_str()) != 0;
    auto_translate_ic = GetPrivateProfileIntA("Settings", "AutoTranslateIC", 1, ini_path.c_str()) != 0;
    auto_translate_me_do = GetPrivateProfileIntA("Settings", "AutoTranslateMeDo", 1, ini_path.c_str()) != 0;
    developer_mode = GetPrivateProfileIntA("Settings", "DeveloperMode", 0, ini_path.c_str()) != 0;
    font_size = GetPrivateProfileIntA("Settings", "FontSize", 11, ini_path.c_str());
    toggle_hotkey = GetPrivateProfileIntA("Settings", "ToggleHotkey", VK_F7, ini_path.c_str());

    GetPrivateProfileStringA("Settings", "Opacity", "0.90", buffer, sizeof(buffer), ini_path.c_str());
    try {
        overlay_opacity = std::stof(buffer);
    } catch (...) {
        overlay_opacity = 0.90f;
    }

    click_through = GetPrivateProfileIntA("Settings", "ClickThrough", 0, ini_path.c_str()) != 0;
    max_feed_items = GetPrivateProfileIntA("Settings", "MaxFeedItems", 50, ini_path.c_str());

    return true;
}

bool Config::save(const std::string& ini_path) {
    WritePrivateProfileStringA("Settings", "GroqAPIKey", groq_api_key.c_str(), ini_path.c_str());
    WritePrivateProfileStringA("Settings", "GroqModel", groq_model.c_str(), ini_path.c_str());
    WritePrivateProfileStringA("Settings", "TargetLanguage", target_language.c_str(), ini_path.c_str());
    WritePrivateProfileStringA("Settings", "OutboundStyle", outbound_style.c_str(), ini_path.c_str());
    WritePrivateProfileStringA("Settings", "ChatlogPath", chatlog_path.c_str(), ini_path.c_str());
    WritePrivateProfileStringA("Settings", "LicenseToken", license_token.c_str(), ini_path.c_str());

    WritePrivateProfileStringA("Settings", "UseCodSMP", use_codsmp ? "1" : "0", ini_path.c_str());
    WritePrivateProfileStringA("Settings", "EnableClipboardOutbound", enable_clipboard_outbound ? "1" : "0", ini_path.c_str());
    WritePrivateProfileStringA("Settings", "EnableInboundChatlog", enable_inbound_chatlog ? "1" : "0", ini_path.c_str());
    WritePrivateProfileStringA("Settings", "AutoTranslateIC", auto_translate_ic ? "1" : "0", ini_path.c_str());
    WritePrivateProfileStringA("Settings", "AutoTranslateMeDo", auto_translate_me_do ? "1" : "0", ini_path.c_str());
    WritePrivateProfileStringA("Settings", "DeveloperMode", developer_mode ? "1" : "0", ini_path.c_str());
    WritePrivateProfileStringA("Settings", "FontSize", std::to_string(font_size).c_str(), ini_path.c_str());
    WritePrivateProfileStringA("Settings", "ToggleHotkey", std::to_string(toggle_hotkey).c_str(), ini_path.c_str());
    WritePrivateProfileStringA("Settings", "Opacity", std::to_string(overlay_opacity).c_str(), ini_path.c_str());
    WritePrivateProfileStringA("Settings", "ClickThrough", click_through ? "1" : "0", ini_path.c_str());
    WritePrivateProfileStringA("Settings", "MaxFeedItems", std::to_string(max_feed_items).c_str(), ini_path.c_str());

    return true;
}

std::string Config::get_game_directory() {
    char path[MAX_PATH] = {0};
    if (GetModuleFileNameA(NULL, path, MAX_PATH) > 0) {
        std::string exe_path(path);
        size_t pos = exe_path.find_last_of("\\/");
        if (pos != std::string::npos) {
            return exe_path.substr(0, pos);
        }
    }
    return ".";
}

std::string Config::detect_chatlog_path() {
    try {
        char user_profile[MAX_PATH] = {0};
        if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PROFILE, NULL, 0, user_profile))) {
            std::error_code ec;
            std::string docs = std::string(user_profile) + "\\Documents\\GTA San Andreas User Files\\SAMP\\chatlog.txt";
            if (fs::exists(docs, ec)) return docs;

            std::string onedrive = std::string(user_profile) + "\\OneDrive\\Documents\\GTA San Andreas User Files\\SAMP\\chatlog.txt";
            if (fs::exists(onedrive, ec)) return onedrive;
        }

        char my_docs[MAX_PATH] = {0};
        if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PERSONAL, NULL, 0, my_docs))) {
            std::error_code ec;
            std::string docs = std::string(my_docs) + "\\GTA San Andreas User Files\\SAMP\\chatlog.txt";
            if (fs::exists(docs, ec)) return docs;
        }

        std::error_code ec;
        std::string pub = "C:\\Users\\Public\\Documents\\GTA San Andreas User Files\\SAMP\\chatlog.txt";
        if (fs::exists(pub, ec)) return pub;
    } catch (...) {}

    char user_profile[MAX_PATH] = {0};
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PROFILE, NULL, 0, user_profile))) {
        return std::string(user_profile) + "\\Documents\\GTA San Andreas User Files\\SAMP\\chatlog.txt";
    }
    return "C:\\Users\\Public\\Documents\\GTA San Andreas User Files\\SAMP\\chatlog.txt";
}

std::string Config::detect_groq_api_key() {
    try {
        // 1. Environment variable
        char env_buf[1024] = {0};
        if (GetEnvironmentVariableA("GROQ_API_KEY", env_buf, sizeof(env_buf)) > 0) {
            std::string env_key = trim(env_buf);
            if (env_key.rfind("gsk_", 0) == 0) return env_key;
        }

        // 2. Check Downloads folder using safe Win32 FindFirstFileA to avoid wide_to_char exception on unicode filenames
        char user_profile[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PROFILE, NULL, 0, user_profile))) {
            std::string search_pattern = std::string(user_profile) + "\\Downloads\\gsk_*.txt";
            WIN32_FIND_DATAA fd;
            HANDLE hFind = FindFirstFileA(search_pattern.c_str(), &fd);
            if (hFind != INVALID_HANDLE_VALUE) {
                do {
                    if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                        std::string full_path = std::string(user_profile) + "\\Downloads\\" + fd.cFileName;
                        std::ifstream f(full_path);
                        if (f.is_open()) {
                            std::string content;
                            std::getline(f, content);
                            content = trim(content);
                            if (content.rfind("gsk_", 0) == 0) {
                                FindClose(hFind);
                                return content;
                            }
                        }
                    }
                } while (FindNextFileA(hFind, &fd));
                FindClose(hFind);
            }
        }
    } catch (...) {}
    return "";
}

std::vector<std::string> Config::get_api_keys() const {
    std::vector<std::string> keys;
    if (groq_api_key.empty()) return keys;

    std::stringstream ss(groq_api_key);
    std::string token;
    while (std::getline(ss, token)) {
        std::stringstream ss_comma(token);
        std::string sub_token;
        while (std::getline(ss_comma, sub_token, ',')) {
            sub_token = trim(sub_token);
            if (!sub_token.empty()) {
                if (sub_token.rfind("gsk_", 0) != 0 && sub_token.length() > 20) {
                    sub_token = "gsk_" + sub_token;
                }
                if (std::find(keys.begin(), keys.end(), sub_token) == keys.end()) {
                    keys.push_back(sub_token);
                }
            }
        }
    }
    return keys;
}

} // namespace SARPLinggo
