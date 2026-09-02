#pragma once

#include <string>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>

namespace SARPLinggo {

class GroqTranslator;

class ClipboardListener {
private:
    GroqTranslator* translator = nullptr;
    std::string outbound_style = "Standard English";
    std::atomic<bool> enabled{ true };
    std::atomic<bool> running{ false };
    std::thread worker_thread;

    std::string last_processed_text;
    std::string last_translated_text;
    std::mutex mtx;

    std::function<void(const std::string&, const std::string&)> on_translated_cb;

    std::string get_clipboard_text();
    bool set_clipboard_text(const std::string& text);
    void run();

public:
    ClipboardListener() = default;
    ~ClipboardListener();

    void init(GroqTranslator* trans, const std::string& style);
    void set_enabled(bool val) { enabled = val; }
    void set_style(const std::string& style) { outbound_style = style; }
    void set_on_translated(std::function<void(const std::string&, const std::string&)> cb) { on_translated_cb = cb; }

    void start();
    void stop();
};

} // namespace SARPLinggo
