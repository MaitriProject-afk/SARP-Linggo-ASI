#include "../include/clipboard_listener.h"
#include "../include/translator.h"
#include <windows.h>
#include <chrono>
#include <algorithm>
#include <fstream>

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

ClipboardListener::~ClipboardListener() {
    stop();
}

void ClipboardListener::init(GroqTranslator* trans, const std::string& style) {
    translator = trans;
    outbound_style = style;
}

void ClipboardListener::start() {
    if (running) return;
    running = true;
    worker_thread = std::thread(&ClipboardListener::run, this);
}

void ClipboardListener::stop() {
    if (!running) return;
    running = false;
    if (worker_thread.joinable()) {
        worker_thread.join();
    }
}

std::string ClipboardListener::get_clipboard_text() {
    try {
        int retries = 5;
        while (!OpenClipboard(NULL) && retries > 0) {
            retries--;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        if (retries == 0 && !OpenClipboard(NULL)) return "";

        std::string result = "";
        HANDLE hDataUnicode = GetClipboardData(CF_UNICODETEXT);
        if (hDataUnicode) {
            wchar_t* pwszText = static_cast<wchar_t*>(GlobalLock(hDataUnicode));
            if (pwszText) {
                int len = WideCharToMultiByte(CP_UTF8, 0, pwszText, -1, NULL, 0, NULL, NULL);
                if (len > 0) {
                    std::string text(len - 1, '\0');
                    WideCharToMultiByte(CP_UTF8, 0, pwszText, -1, &text[0], len, NULL, NULL);
                    result = text;
                }
                GlobalUnlock(hDataUnicode);
            }
        } else {
            HANDLE hData = GetClipboardData(CF_TEXT);
            if (hData) {
                char* pszText = static_cast<char*>(GlobalLock(hData));
                if (pszText) {
                    result = std::string(pszText);
                    GlobalUnlock(hData);
                }
            }
        }

        CloseClipboard();
        return result;
    } catch (...) {
        return "";
    }
}

bool ClipboardListener::set_clipboard_text(const std::string& text) {
    try {
        int retries = 10;
        while (!OpenClipboard(NULL) && retries > 0) {
            retries--;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        if (retries == 0 && !OpenClipboard(NULL)) return false;

        EmptyClipboard();

        // 1. Set CF_UNICODETEXT (Standard UTF-16 for modern Windows & SAMP chat input)
        int wlen = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, NULL, 0);
        if (wlen > 0) {
            HGLOBAL hGlob = GlobalAlloc(GMEM_MOVEABLE, wlen * sizeof(wchar_t));
            if (hGlob) {
                wchar_t* pBuf = static_cast<wchar_t*>(GlobalLock(hGlob));
                if (pBuf) {
                    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, pBuf, wlen);
                    GlobalUnlock(hGlob);
                    SetClipboardData(CF_UNICODETEXT, hGlob);
                } else {
                    GlobalFree(hGlob);
                }
            }
        }

        // 2. Set CF_TEXT (Legacy ANSI fallback)
        HGLOBAL hGlobText = GlobalAlloc(GMEM_MOVEABLE, text.length() + 1);
        if (hGlobText) {
            char* pBufText = static_cast<char*>(GlobalLock(hGlobText));
            if (pBufText) {
                memcpy(pBufText, text.c_str(), text.length() + 1);
                GlobalUnlock(hGlobText);
                SetClipboardData(CF_TEXT, hGlobText);
            } else {
                GlobalFree(hGlobText);
            }
        }

        CloseClipboard();
        return true;
    } catch (...) {
        return false;
    }
}

static std::string to_lower(const std::string& str) {
    std::string lower = str;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return (char)std::tolower(c); });
    return lower;
}

void ClipboardListener::run() {
    while (running) {
        try {
            if (!enabled || !translator) {
                std::this_thread::sleep_for(std::chrono::milliseconds(300));
                continue;
            }

            std::string text = get_clipboard_text();

            // Trim whitespace
            size_t first = text.find_first_not_of(" \t\r\n");
            if (first != std::string::npos) {
                size_t last = text.find_last_not_of(" \t\r\n");
                text = text.substr(first, (last - first + 1));
            } else {
                text = "";
            }

            if (text.length() >= 2) {
                std::lock_guard<std::mutex> lock(mtx);
                if (text != last_processed_text && text != last_translated_text) {
                    std::string lower_text = to_lower(text);

                    // Filter out URLs, paths, system commands
                    bool skip = false;
                    std::string skip_reason = "";
                    if (lower_text.rfind("http://", 0) == 0 || lower_text.rfind("https://", 0) == 0 || lower_text.rfind("c:\\", 0) == 0 || lower_text.rfind("d:\\", 0) == 0) {
                        skip = true;
                        skip_reason = "URL atau Path File";
                    }
                    if (lower_text.rfind("/", 0) == 0 && !(lower_text.rfind("/me", 0) == 0 || lower_text.rfind("/do", 0) == 0)) {
                        skip = true;
                        skip_reason = "Perintah / Command Game";
                    }

                    if (skip) {
                        last_processed_text = text;
                        LogDebugLocal("[ClipboardIgnored] Teks diabaikan (" + skip_reason + "): \"" + text + "\"");
                    } else {
                        bool is_id = is_indonesian_text(text);
                        LogDebugLocal("[ClipboardCaptured] Teks terdeteksi di Clipboard: \"" + text + "\" (Terdeteksi ID: " + (is_id ? "YA" : "TIDAK") + ")");

                        if (is_id) {
                            last_processed_text = text;
                            LogDebugLocal("[ClipboardOutbound] Mengirim terjemahan outbound (Style: " + outbound_style + "): \"" + text + "\"");

                            std::string translated = translator->translate_outbound(text, outbound_style);

                            if (!translated.empty() && translated != text) {
                                last_translated_text = translated;
                                if (set_clipboard_text(translated)) {
                                    LogDebugLocal("[ClipboardSuccess] Terjemahan outbound berhasil & disalin ke clipboard: \"" + translated + "\"");
                                } else {
                                    LogDebugLocal("[ClipboardError] Gagal menulis hasil terjemahan ke Win32 Clipboard!");
                                }

                                if (on_translated_cb) {
                                    on_translated_cb(text, translated);
                                }
                            } else {
                                LogDebugLocal("[ClipboardError] Terjemahan outbound gagal atau menghasilkan teks kosong/sama persis.");
                            }
                        } else {
                            last_processed_text = text;
                            LogDebugLocal("[ClipboardIgnored] Teks diabaikan karena tidak terdeteksi sebagai Bahasa Indonesia: \"" + text + "\"");
                        }
                    }
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(300));
        } catch (const std::exception& e) {
            LogDebugLocal("[ClipboardException] " + std::string(e.what()));
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
        } catch (...) {
            LogDebugLocal("[ClipboardException] Unknown exception in ClipboardListener loop");
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
        }
    }
}

} // namespace SARPLinggo
