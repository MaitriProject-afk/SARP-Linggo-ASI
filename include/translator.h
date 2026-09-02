#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <mutex>
#include <chrono>

namespace SARPLinggo {

class LicenseManager;

struct KeyState {
    std::string key;
    std::chrono::system_clock::time_point cooldown_until;
    int rpd_remaining = -1;
    int rpd_limit = -1;
    std::string rpd_reset = "";
    std::string status = "ACTIVE"; // ACTIVE, COOLDOWN_MINUTE, EXHAUSTED_DAILY, INVALID
    int failure_count = 0;
};

class KeyPoolManager {
private:
    std::vector<std::string> keys;
    std::map<std::string, KeyState> key_states;
    size_t current_index = 0;
    std::mutex mtx;

public:
    void update_keys(const std::vector<std::string>& new_keys);
    bool get_next_working_key(std::string& out_key, size_t& out_idx, std::string& out_masked);
    void mark_rate_limited(const std::string& key, int status_code, const std::string& response_text = "");
    void update_key_metrics(const std::string& key, int rpd_rem, int rpd_lim, const std::string& rpd_rst);
    size_t total_keys();
    std::string get_pool_summary();
    std::vector<std::string> get_keys() { return keys; }
};

bool is_indonesian_text(const std::string& text);
bool should_translate_inbound(const std::string& text);

class GroqTranslator {
private:
    KeyPoolManager key_pool;
    LicenseManager* license_mgr = nullptr;
    std::string model = "openai/gpt-oss-20b";
    std::string target_lang = "Indonesian";
    bool developer_mode = false;
    std::mutex mtx;

    int last_rpd_remaining = -1;
    int last_rpd_limit = -1;
    std::string last_rpd_reset = "";
    std::string last_error_detail = "";

    int last_prompt_tokens = 0;
    int last_completion_tokens = 0;
    int last_total_tokens = 0;

    std::string send_winhttp_request(const std::string& key, const std::string& payload_json, int& out_status_code, std::string& out_headers);
    std::string clean_translation_output(const std::string& raw_output);

public:
    GroqTranslator() = default;
    
    void update_api_keys(const std::vector<std::string>& keys);
    void set_model(const std::string& m) { model = m; }
    void set_target_lang(const std::string& lang) { target_lang = lang; }
    void set_developer_mode(bool dev) { developer_mode = dev; }
    void set_license_manager(LicenseManager* lm) { license_mgr = lm; }

    int get_last_rpd_remaining() const { return last_rpd_remaining; }
    int get_last_rpd_limit() const { return last_rpd_limit; }
    int get_last_total_tokens() const { return last_total_tokens; }
    std::string get_last_error_detail() const { return last_error_detail; }
    std::string get_pool_summary() { return key_pool.get_pool_summary(); }

    std::string translate_inbound(const std::string& text, const std::string& chat_type = "SAYS");
    std::string translate_outbound(const std::string& text, const std::string& style = "Standard English");
    
    bool check_rpd_quota(std::string& out_summary);
};

} // namespace SARPLinggo
