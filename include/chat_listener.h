#pragma once

#include <string>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <vector>

namespace SARPLinggo {

struct ChatItem {
    std::string timestamp;
    std::string type; // "SAYS", "ME", "DO", "OUTBOUND"
    std::string speaker;
    std::string content;
    std::string raw;
    std::string translated = "";
};

class ChatlogListener {
private:
    std::string chatlog_path;
    bool use_codsmp = true;
    std::atomic<bool> running{ false };
    std::thread worker_thread;
    std::function<void(const ChatItem&)> on_new_item_cb;
    std::function<void(const std::string&)> on_status_cb;

    std::string get_active_chatlog_path();
    ChatItem parse_line(const std::string& line);
    void run();

public:
    ChatlogListener() = default;
    ~ChatlogListener();

    void set_path(const std::string& path) { chatlog_path = path; }
    void set_use_codsmp(bool enable) { use_codsmp = enable; }
    void set_on_new_item(std::function<void(const ChatItem&)> cb) { on_new_item_cb = cb; }
    void set_on_status(std::function<void(const std::string&)> cb) { on_status_cb = cb; }

    void start();
    void stop();
};

} // namespace SARPLinggo
