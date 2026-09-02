#include "../include/chat_listener.h"
#include <windows.h>
#include <fstream>
#include <sstream>
#include <regex>
#include <chrono>
#include <filesystem>
#include <algorithm>
#include <iostream>

namespace fs = std::filesystem;

namespace SARPLinggo {

extern std::string g_log_file_path;
static void LogDebug(const std::string& msg) {
    try {
        std::ofstream log(g_log_file_path, std::ios::app);
        if (log.is_open()) {
            log << "[ChatListener] " << msg << std::endl;
        }
    } catch (...) {}
}

static const std::vector<std::string> IGNORE_PREFIXES = {
    "PAYCHECK:", "SERVER:", "MOTD:", "MASK:", "Ad:", "Contact Info:",
    "[ADMIN]", "[NEWS]", "[AD]", "[RADIO]", "[R]", "Screenshot",
    "Connecting to", "Connected."
};

ChatlogListener::~ChatlogListener() {
    stop();
}

void ChatlogListener::start() {
    if (running) return;
    running = true;
    worker_thread = std::thread(&ChatlogListener::run, this);
}

void ChatlogListener::stop() {
    if (!running) return;
    running = false;
    if (worker_thread.joinable()) {
        worker_thread.join();
    }
}

// Win32 Non-locking File Content Reader
static std::string ReadFileNonLocking(const std::string& filepath) {
    HANDLE hFile = CreateFileA(filepath.c_str(), GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile == INVALID_HANDLE_VALUE) return "";

    DWORD fileSize = GetFileSize(hFile, NULL);
    if (fileSize == 0 || fileSize == INVALID_FILE_SIZE) {
        CloseHandle(hFile);
        return "";
    }

    std::string buffer;
    buffer.resize(fileSize);

    DWORD bytesRead = 0;
    BOOL bSuccess = ReadFile(hFile, &buffer[0], fileSize, &bytesRead, NULL);
    CloseHandle(hFile);

    if (!bSuccess) return "";
    buffer.resize(bytesRead);
    return buffer;
}

std::string ChatlogListener::get_active_chatlog_path() {
    if (chatlog_path.empty()) return "";
    std::error_code ec;

    fs::path p(chatlog_path);
    fs::path samp_dir = p.parent_path();
    if (samp_dir.filename() == "logs") {
        samp_dir = samp_dir.parent_path();
    }

    fs::path chatlog_txt = samp_dir / "chatlog.txt";

    if (use_codsmp) {
        if (fs::exists(chatlog_txt, ec)) {
            std::string content = ReadFileNonLocking(chatlog_txt.string());
            if (!content.empty()) {
                std::smatch m;
                std::regex re("Your chatlog has been saved to:\\s*(.+?\\.txt)", std::regex::icase);
                if (std::regex_search(content, m, re)) {
                    std::string target = m[1].str();
                    if (fs::exists(target, ec)) return target;
                }
            }
        }

        fs::path logs_dir = samp_dir / "logs";
        if (fs::exists(logs_dir, ec) && fs::is_directory(logs_dir, ec)) {
            fs::path latest_file;
            fs::file_time_type latest_time;
            bool found = false;

            fs::directory_iterator it(logs_dir, ec);
            if (!ec) {
                for (const auto& entry : it) {
                    if (entry.is_regular_file(ec) && entry.path().extension() == ".txt") {
                        auto ftime = entry.last_write_time(ec);
                        if (!found || ftime > latest_time) {
                            latest_time = ftime;
                            latest_file = entry.path();
                            found = true;
                        }
                    }
                }
            }
            if (found && fs::exists(chatlog_txt, ec)) {
                auto chatlog_time = fs::last_write_time(chatlog_txt, ec);
                if (!ec && chatlog_time >= latest_time) {
                    return chatlog_txt.string();
                }
                return latest_file.string();
            }
        }
    }

    return fs::exists(chatlog_txt, ec) ? chatlog_txt.string() : chatlog_path;
}

ChatItem ChatlogListener::parse_line(const std::string& line) {
    ChatItem item;
    if (line.empty()) return item;

    try {
        std::string clean_line = line;
        std::smatch match_ts;
        std::regex ts_regex("^\\[(\\d{2}:\\d{2}:\\d{2})\\]\\s*");
        if (std::regex_search(line, match_ts, ts_regex)) {
            item.timestamp = match_ts[1].str();
            clean_line = line.substr(match_ts.length());
            size_t first = clean_line.find_first_not_of(" \t\r\n");
            if (first != std::string::npos) {
                clean_line = clean_line.substr(first);
            }
        }

        // Strip SAMP embedded hex color codes: {FFFFFF}, {00FFFF}, etc.
        static const std::regex color_regex("\\{[A-Fa-f0-9]{6}\\}");
        clean_line = std::regex_replace(clean_line, color_regex, "");

        // Trim leading/trailing whitespace
        size_t first_ch = clean_line.find_first_not_of(" \t\r\n");
        if (first_ch == std::string::npos) return item;
        size_t last_ch = clean_line.find_last_not_of(" \t\r\n");
        clean_line = clean_line.substr(first_ch, (last_ch - first_ch + 1));

        if (clean_line.rfind("((", 0) == 0 && clean_line.find("))") == std::string::npos) return item;

        for (const auto& prefix : IGNORE_PREFIXES) {
            if (clean_line.rfind(prefix, 0) == 0) return item;
        }

        // 1. /do action check
        if (clean_line.rfind("* ", 0) == 0) {
            std::regex do_regex("^\\*\\s+(.+?)\\s*\\(\\(\\s*(.+?)\\s*\\)\\)$");
            std::smatch match_do;
            if (std::regex_match(clean_line, match_do, do_regex)) {
                item.type = "DO";
                item.content = match_do[1].str();
                item.speaker = match_do[2].str();
                item.raw = line;
                return item;
            }

            // 2. /me action check
            std::string line_body = clean_line.substr(2);
            std::stringstream ss(line_body);
            std::string p1, p2;
            ss >> p1 >> p2;

            std::string speaker = "Unknown";
            std::string content = "";

            if (p1 == "Mask" || p1 == "mask") {
                speaker = p1 + " " + p2;
                size_t pos = line_body.find(p2);
                if (pos != std::string::npos) content = line_body.substr(pos + p2.length());
            } else if (p1.find('_') != std::string::npos) {
                speaker = p1;
                content = line_body.substr(p1.length());
            } else {
                speaker = p1;
                content = line_body.substr(p1.length());
            }

            item.type = "ME";
            item.speaker = speaker;
            item.content = content;
            item.raw = line;
            return item;
        }

        // 3. IC Spoken Chat (says, shouts, whispers)
        static const std::regex says_regex("^(.+?)\\s+(says|shouts|whispers)(?:\\s+\\([^\\)]+\\))?:\\s*(.+)$", std::regex::icase);
        std::smatch match_says;
        if (std::regex_match(clean_line, match_says, says_regex)) {
            item.speaker = match_says[1].str();
            std::string verb = match_says[2].str();
            item.content = match_says[3].str();
            item.type = "SAYS";
            item.raw = line;

            for (const auto& prefix : IGNORE_PREFIXES) {
                if (item.speaker.rfind(prefix, 0) == 0) return ChatItem();
            }

            if (item.content.length() < 2) return ChatItem();
            return item;
        }
    } catch (const std::exception& e) {
        LogDebug("Exception in parse_line: " + std::string(e.what()));
    } catch (...) {
        LogDebug("Unknown exception in parse_line");
    }

    return item;
}

void ChatlogListener::run() {
    try {
        if (on_status_cb) on_status_cb("Waiting for chatlog file...");

        size_t last_offset = 0;

        while (running) {
            std::error_code ec;
            std::string active_path = get_active_chatlog_path();
            if (active_path.empty() || !fs::exists(active_path, ec)) {
                if (on_status_cb) on_status_cb("File not found: " + (chatlog_path.empty() ? "Not configured" : chatlog_path));
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                continue;
            } else {
                static std::string last_reported_path = "";
                if (active_path != last_reported_path) {
                    last_reported_path = active_path;
                    if (on_status_cb) on_status_cb("● Monitoring Chatlog...");
                }
            }

            // Non-locking file read with Win32 CreateFileA (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE)
            HANDLE hFile = CreateFileA(active_path.c_str(), GENERIC_READ,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

            if (hFile == INVALID_HANDLE_VALUE) {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                continue;
            }

            DWORD fileSize = GetFileSize(hFile, NULL);
            if (fileSize == INVALID_FILE_SIZE) {
                CloseHandle(hFile);
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                continue;
            }

            if (last_offset == 0) {
                last_offset = fileSize;
                LogDebug("Initial last_offset set to fileSize: " + std::to_string(last_offset) + " for path: " + active_path);
            } else if (fileSize < last_offset) {
                last_offset = 0;
                LogDebug("File reset detected. Reset last_offset to 0.");
            }

            if (fileSize > last_offset) {
                DWORD bytesToRead = fileSize - last_offset;
                LogDebug("fileSize (" + std::to_string(fileSize) + ") > last_offset (" + std::to_string(last_offset) + "). Reading " + std::to_string(bytesToRead) + " bytes.");
                std::string new_data;
                new_data.resize(bytesToRead);

                SetFilePointer(hFile, last_offset, NULL, FILE_BEGIN);
                DWORD bytesRead = 0;
                BOOL bReadSuccess = ReadFile(hFile, &new_data[0], bytesToRead, &bytesRead, NULL);
                CloseHandle(hFile);

                if (bReadSuccess && bytesRead > 0) {
                    new_data.resize(bytesRead);
                    last_offset += bytesRead;

                    std::stringstream ss(new_data);
                    std::string line;
                    while (running && std::getline(ss, line)) {
                        if (!line.empty() && line.back() == '\r') line.pop_back();
                        ChatItem parsed = parse_line(line);
                        if (!parsed.content.empty()) {
                            LogDebug("PARSED SUCCESS: [" + parsed.speaker + "] " + parsed.content);
                            if (on_new_item_cb) on_new_item_cb(parsed);
                        }
                    }
                }
            } else {
                CloseHandle(hFile);
                std::this_thread::sleep_for(std::chrono::milliseconds(250));
            }
        }
    } catch (...) {}
}

} // namespace SARPLinggo
