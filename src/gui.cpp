#include "../include/gui.h"
#include "../include/input_hook.h"
#include <windows.h>
#include <commdlg.h>
#include <algorithm>
#include <iostream>
#include <fstream>
#include "../imgui/imgui.h"

namespace SARPLinggo {

extern std::string g_log_file_path;
static void LogDebugLocal(const std::string& msg) {
    try {
        std::ofstream log(g_log_file_path, std::ios::app);
        if (log.is_open()) {
            log << "[SA-RP Linggo] " << msg << std::endl;
        }
    } catch (...) {}
}

void OverlayGUI::add_chat_card(const ChatItem& item) {
    std::lock_guard<std::mutex> lock(mtx);
    feed_items.push_back(item);

    int max_items = config_ref ? config_ref->max_feed_items : 50;
    if (feed_items.size() > static_cast<size_t>(max_items)) {
        feed_items.erase(feed_items.begin(), feed_items.begin() + (feed_items.size() - max_items));
    }
}

void OverlayGUI::set_status(const std::string& msg) {
    std::lock_guard<std::mutex> lock(mtx);
    status_msg = msg;
}

void OverlayGUI::set_config(Config* config) {
    config_ref = config;
    if (config_ref) {
        strncpy(api_key_buffer, config_ref->groq_api_key.c_str(), sizeof(api_key_buffer) - 1);
        strncpy(chatlog_path_buffer, config_ref->chatlog_path.c_str(), sizeof(chatlog_path_buffer) - 1);
        strncpy(license_token_buffer, config_ref->license_token.c_str(), sizeof(license_token_buffer) - 1);
    }
}

void OverlayGUI::render() {
    if (!is_visible) {
        ImGui::GetIO().MouseDrawCursor = false;
        return;
    }

    // Enable ImGui software cursor drawing when cursor mode is unlocked
    ImGuiIO& io = ImGui::GetIO();
    io.MouseDrawCursor = is_cursor_mode;

    if (is_cursor_mode) {
        POINT pt;
        if (GetCursorPos(&pt) && InputHook::get_hwnd()) {
            ScreenToClient(InputHook::get_hwnd(), &pt);
            io.MousePos = ImVec2((float)pt.x, (float)pt.y);
        }
    }

    // Apply cursor & click-through flags based on cursor mode
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoCollapse;
    if (!is_cursor_mode) {
        window_flags |= ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoMove;
    }

    // Set elegant dark blue styling
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 8.0f;
    style.FrameRounding = 5.0f;
    style.PopupRounding = 5.0f;
    style.ScrollbarRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.TabRounding = 5.0f;

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.10f, 0.14f, config_ref ? config_ref->overlay_opacity : 0.90f);
    colors[ImGuiCol_Header] = ImVec4(0.15f, 0.22f, 0.32f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.22f, 0.34f, 0.50f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.18f, 0.28f, 0.42f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.14f, 0.24f, 0.38f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.22f, 0.38f, 0.60f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.18f, 0.30f, 0.48f, 1.00f);
    colors[ImGuiCol_Tab] = ImVec4(0.12f, 0.18f, 0.26f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.22f, 0.38f, 0.60f, 1.00f);
    colors[ImGuiCol_TabActive] = ImVec4(0.18f, 0.30f, 0.48f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.12f, 0.18f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.12f, 0.18f, 0.28f, 1.00f);

    ImGui::SetNextWindowSize(ImVec2(560, 430), ImGuiCond_FirstUseEver);

    std::string cursor_status = is_cursor_mode ? " Unlocked]" : " Locked]";
    // Fixed window ID ###SARPLinggoMainWindow prevents ImGui from treating locked and unlocked as separate windows with different positions
    std::string window_title = "SA-RP Linggo v1.3  [Shift+H: Hide Overlay | Shift+Enter:" + cursor_status + "###SARPLinggoMainWindow";

    if (ImGui::Begin(window_title.c_str(), &is_visible, window_flags)) {
        if (ImGui::BeginTabBar("MainTabBar")) {

            // TAB 1: LIVE FEED CHAT
            if (ImGui::BeginTabItem("Feed Chat")) {
                ImGui::Spacing();

                std::lock_guard<std::mutex> lock(mtx);
                
                // Status Header Toggle Indicator
                bool inbound_on = config_ref && config_ref->enable_inbound_chatlog;
                bool clipboard_on = config_ref && config_ref->enable_clipboard_outbound;

                ImGui::TextColored(ImVec4(0.70f, 0.75f, 0.82f, 1.0f), "Riwayat terjemahan pesan game:");
                ImGui::SameLine();
                ImGui::TextColored(inbound_on ? ImVec4(0.20f, 0.85f, 0.40f, 1.0f) : ImVec4(0.85f, 0.30f, 0.30f, 1.0f),
                                   "[Inbound: %s]", inbound_on ? "ON" : "OFF");
                ImGui::SameLine();
                ImGui::TextColored(clipboard_on ? ImVec4(0.20f, 0.85f, 0.40f, 1.0f) : ImVec4(0.85f, 0.30f, 0.30f, 1.0f),
                                   "[Clipboard: %s]", clipboard_on ? "ON" : "OFF");

                ImGui::SameLine(ImGui::GetWindowWidth() - 105);
                if (ImGui::Button("Hapus Feed", ImVec2(90, 22))) {
                    feed_items.clear();
                }

                ImGui::Separator();
                ImGui::BeginChild("FeedScrollRegion", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);

                static bool should_scroll_bottom = false;

                for (size_t i = 0; i < feed_items.size(); ++i) {
                    const auto& item = feed_items[i];
                    ImGui::PushID(static_cast<int>(i));

                    ImGui::BeginGroup();

                    // Header color accent per type
                    ImVec4 type_color(0.22f, 0.74f, 0.97f, 1.0f); // Default SAYS (Cyan)
                    if (item.type == "ME") type_color = ImVec4(0.75f, 0.52f, 0.99f, 1.0f);        // Lavender
                    else if (item.type == "DO") type_color = ImVec4(0.66f, 0.33f, 0.97f, 1.0f);   // Purple
                    else if (item.type == "OUTBOUND") type_color = ImVec4(0.02f, 0.71f, 0.83f, 1.0f); // Bright Cyan
                    else if (item.type == "ERROR") type_color = ImVec4(0.94f, 0.27f, 0.27f, 1.0f); // Red

                    // Timestamp & Speaker Header
                    ImGui::TextColored(type_color, "[%s] %s (%s)",
                                       item.timestamp.c_str(), item.speaker.c_str(), item.type.c_str());

                    // Original Line
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.70f, 0.75f, 0.82f, 1.0f));
                    ImGui::TextWrapped("%s", item.content.c_str());
                    ImGui::PopStyleColor();

                    // Translated Line & Copy Button (Only show if translated is not empty)
                    if (!item.translated.empty()) {
                        if (item.translated.rfind("[Gagal", 0) == 0 || item.translated.rfind("[Lisensi", 0) == 0 || item.translated.rfind("[Error", 0) == 0) {
                            ImGui::TextColored(ImVec4(0.94f, 0.27f, 0.27f, 1.0f), "-> %s", item.translated.c_str());
                        } else {
                            ImGui::PushStyleColor(ImGuiCol_Text, type_color);
                            ImGui::TextWrapped("-> %s", item.translated.c_str());
                            ImGui::PopStyleColor();

                            ImGui::SameLine();
                            if (ImGui::SmallButton("[Salin]")) {
                                ImGui::SetClipboardText(item.translated.c_str());
                            }
                        }
                    }

                    ImGui::EndGroup();
                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::PopID();
                }

                if (should_scroll_bottom || ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
                    ImGui::SetScrollHereY(1.0f);
                    should_scroll_bottom = false;
                }

                ImGui::EndChild();
                ImGui::EndTabItem();
            }

            // TAB 2: OUTBOUND TRANSLATOR / MANUAL TEST
            if (ImGui::BeginTabItem("Terjemah Keluar")) {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.22f, 0.74f, 0.97f, 1.0f), "Tulis teks Bahasa Indonesia atau aksi /me /do di bawah:");

                ImGui::InputTextMultiline("##OutboundInput", outbound_input, sizeof(outbound_input), ImVec2(-1, 80));

                ImGui::Spacing();
                ImGui::Text("Gaya Bahasa Inggris (Outbound Style):");
                static int style_idx = (config_ref && config_ref->outbound_style == "American Hood") ? 1 : 0;
                const char* styles[] = { "Standard English", "American Hood" };
                if (ImGui::Combo("##StyleCombo", &style_idx, styles, IM_ARRAYSIZE(styles))) {
                    if (config_ref) config_ref->outbound_style = styles[style_idx];
                    if (clipboard_ref) clipboard_ref->set_style(styles[style_idx]);
                }

                ImGui::Spacing();
                if (ImGui::Button("Terjemahkan & Salin Ke Clipboard (CTRL+V)", ImVec2(-1, 32))) {
                    if (translator_ref && outbound_input[0] != '\0') {
                        is_translating_outbound = true;
                        std::string target_style = styles[style_idx];
                        std::string res = translator_ref->translate_outbound(outbound_input, target_style);
                        outbound_result = res;
                        ImGui::SetClipboardText(res.c_str());
                        is_translating_outbound = false;

                        // Also add card to feed
                        ChatItem card;
                        card.timestamp = "OUTBOUND";
                        card.speaker = "Anda";
                        card.content = outbound_input;
                        card.translated = res;
                        card.type = "OUTBOUND";
                        add_chat_card(card);
                    }
                }

                if (!outbound_result.empty()) {
                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::TextColored(ImVec4(0.10f, 0.73f, 0.51f, 1.0f), "Hasil Terjemahan (Telah disalin otomatis):");
                    ImGui::BeginChild("OutboundResultChild", ImVec2(0, 100), true);
                    ImGui::TextWrapped("%s", outbound_result.c_str());
                    ImGui::EndChild();

                    if (ImGui::Button("[Salin Ulang Hasil]", ImVec2(160, 24))) {
                        ImGui::SetClipboardText(outbound_result.c_str());
                    }
                }

                ImGui::EndTabItem();
            }

            // TAB 3: SETTINGS
            if (ImGui::BeginTabItem("Pengaturan")) {
                ImGui::Spacing();

                // Groq API Key Pool
                ImGui::TextColored(ImVec4(0.66f, 0.33f, 0.97f, 1.0f), "Groq AI API Key Pool (Multi-Token Dynamic Rolling Pool):");
                
                if (translator_ref) {
                    std::string pool_sum = translator_ref->get_pool_summary();
                    ImGui::TextColored(ImVec4(0.10f, 0.73f, 0.51f, 1.0f), "%s", pool_sum.c_str());
                }

                ImGui::InputTextMultiline("##APIKeysInput", api_key_buffer, sizeof(api_key_buffer), ImVec2(-1, 90));
                ImGui::TextDisabled("Masukkan banyak token dipisahkan koma, spasi, atau baris baru (gsk_...)");
                
                if (ImGui::Button("Auto-Detect Key Dari Folder Downloads")) {
                    std::string detected = Config::detect_groq_api_key();
                    if (!detected.empty()) {
                        if (strlen(api_key_buffer) > 0) {
                            strncat(api_key_buffer, "\n", sizeof(api_key_buffer) - strlen(api_key_buffer) - 1);
                            strncat(api_key_buffer, detected.c_str(), sizeof(api_key_buffer) - strlen(api_key_buffer) - 1);
                        } else {
                            strncpy(api_key_buffer, detected.c_str(), sizeof(api_key_buffer) - 1);
                        }
                    }
                }

                ImGui::Spacing();
                ImGui::Separator();

                // Chatlog Path
                ImGui::Text("Path File SAMP chatlog.txt:");
                ImGui::InputText("##ChatlogPathInput", chatlog_path_buffer, sizeof(chatlog_path_buffer));
                ImGui::SameLine();
                if (ImGui::Button("Browse...")) {
                    OPENFILENAMEA ofn;
                    char szFile[MAX_PATH] = {0};
                    ZeroMemory(&ofn, sizeof(ofn));
                    ofn.lStructSize = sizeof(ofn);
                    ofn.hwndOwner = NULL;
                    ofn.lpstrFile = szFile;
                    ofn.nMaxFile = sizeof(szFile);
                    ofn.lpstrFilter = "Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
                    ofn.nFilterIndex = 1;
                    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
                    if (GetOpenFileNameA(&ofn)) {
                        strncpy(chatlog_path_buffer, szFile, sizeof(chatlog_path_buffer) - 1);
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button("Auto-Detect Path")) {
                    std::string detected = Config::detect_chatlog_path();
                    strncpy(chatlog_path_buffer, detected.c_str(), sizeof(chatlog_path_buffer) - 1);
                }

                ImGui::Spacing();
                ImGui::Separator();

                // Toggle Switches
                if (config_ref) {
                    ImGui::TextColored(ImVec4(0.22f, 0.74f, 0.97f, 1.0f), "Saklar Fitur Terjemahan (Enable / Disable):");

                    // 1. Inbound Chatlog Translation Toggle
                    ImGui::Checkbox("Master: Aktifkan Penerjemah Chatlog Game (Inbound)", &config_ref->enable_inbound_chatlog);

                    if (config_ref->enable_inbound_chatlog) {
                        ImGui::Indent(20.0f);
                        ImGui::Checkbox("Terjemahkan Chat In-Character (SAYS)", &config_ref->auto_translate_ic);
                        ImGui::Checkbox("Terjemahkan Roleplay /me dan /do", &config_ref->auto_translate_me_do);
                        ImGui::Unindent(20.0f);
                    }

                    ImGui::Spacing();

                    // 2. Clipboard Outbound Translation Toggle
                    if (ImGui::Checkbox("Master: Aktifkan Penangkap Clipboard (Outbound CTRL+C -> Inggris)", &config_ref->enable_clipboard_outbound)) {
                        if (clipboard_ref) {
                            clipboard_ref->set_enabled(config_ref->enable_clipboard_outbound);
                        }
                    }
                }

                ImGui::Spacing();
                ImGui::Separator();

                if (ImGui::Button("Cek Kuota RPD Semua Token Pool", ImVec2(240, 26))) {
                    if (translator_ref) {
                        std::string summary = "";
                        translator_ref->check_rpd_quota(summary);
                        rpd_status_text = summary;
                    }
                }

                if (!rpd_status_text.empty()) {
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.22f, 0.74f, 0.97f, 1.0f), "Status Token Pool:");
                    ImGui::TextWrapped("%s", rpd_status_text.c_str());
                }

                ImGui::Spacing();
                ImGui::Separator();

                if (ImGui::Button("Simpan Pengaturan Ke INI", ImVec2(-1, 32))) {
                    if (config_ref) {
                        config_ref->groq_api_key = api_key_buffer;
                        config_ref->chatlog_path = chatlog_path_buffer;
                        config_ref->save(Config::get_game_directory() + "\\SARPLinggo.ini");

                        if (translator_ref) {
                            translator_ref->update_api_keys(config_ref->get_api_keys());
                        }

                        LogDebugLocal("[GUI] Config saved to INI successfully.");
                    }
                }

                ImGui::EndTabItem();
            }

            // TAB 4: INFO & ABOUT
            if (ImGui::BeginTabItem("Info Mod")) {
                ImGui::Spacing();

                ImGui::TextColored(ImVec4(0.22f, 0.74f, 0.97f, 1.0f), "=== SA-RP Linggo ASI Mod v1.3 ===");
                ImGui::Text("Pengembang: MaitriProject");
                ImGui::Text("Status Mod: 100%% Free & Open-Source Build");
                ImGui::Text("Engine: DirectX 9 Native ASI Hook + ImGui");
                ImGui::Spacing();
                ImGui::Text("Pintasan Keyboard:");
                ImGui::Text("- Shift + H    : Hide / Show Overlay Window");
                ImGui::Text("- Shift + Enter: Unlock / Lock Cursor Interaksi");
                ImGui::Text("- F7           : Toggle Overlay");

                ImGui::Spacing();
                ImGui::Separator();

                ImGui::TextColored(ImVec4(0.10f, 0.73f, 0.51f, 1.0f), "[+] Lisensi: BEBAS / DIBUKA TOTAL (OPEN SOURCE)");
                ImGui::TextWrapped("Mod ini 100%% Gratis dan Bebas Digunakan tanpa perlu aktivasi lisensi. Cukup masukkan Groq API Key milik Anda sendiri pada Tab Pengaturan untuk langsung menggunakannya.");

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        if (!status_msg.empty()) {
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.20f, 0.85f, 0.40f, 1.00f), "[+] Status: %s", status_msg.c_str());
        }

        ImGui::End();
    }
}

} // namespace SARPLinggo
