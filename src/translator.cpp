#include "../include/translator.h"
#include "../include/licensing.h"
#include "../include/obfuscation.h"
#include <windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")

#include <unordered_set>
#include <algorithm>
#include <regex>
#include <sstream>
#include <iostream>
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

static const std::unordered_set<std::string> INDONESIAN_MARKERS = {
    "apa", "apakah", "siapa", "dimana", "kapan", "mengapa", "kenapa", "bagaimana",
    "gimana", "kamu", "saya", "aku", "dia", "mereka", "kita", "kami", "anda", "ini",
    "itu", "yang", "dan", "atau", "tidak", "gak", "nggak", "ngga", "ga", "tak", "bukan",
    "ada", "bisa", "bila", "jika", "kalau", "sudah", "udah", "belum", "akan", "mau",
    "ingin", "harus", "adalah", "lagi", "sedang", "dapat", "sama", "dengan", "ke",
    "di", "dari", "untuk", "pada", "kabar", "baik", "tolong", "makasih", "terima",
    "kasih", "mas", "mbak", "gan", "min", "halo", "selamat", "pagi", "siang", "malam",
    "sore", "iya", "ya", "enggak", "gua", "gue", "lu", "sampe", "sampai", "bener",
    "benar", "sih", "dong", "kan", "lah", "deh", "kok", "noh", "tuh", "nih", "nanti",
    "kemarin", "besok", "mana", "sini", "situ", "sana", "brapa", "berapa", "bang",
    "orang", "kerja", "jalan", "makan", "minum", "beli", "jual", "rumah", "mobil", "motor",
    "sepertinya", "kehabisan", "bensin", "kayaknya", "rasanya", "pasti", "bikin", "buat",
    "lihat", "pergi", "datang", "naik", "turun", "bawa", "polisi", "senjata", "peluru",
    "buka", "tutup", "mati", "hidup", "rusak", "bakar", "hilang", "cari", "temu", "tarik",
    "dorong", "pukul", "tendang", "lari", "duduk", "tidur", "bangun", "serang", "kabur",
    "kelakuan", "mu", "sungguh", "memalukan", "parah", "banget", "parahbanget", "anjir",
    "anjg", "jir", "jirrr", "panteq", "pantek", "goblok", "tolol", "bego", "kontol",
    "memek", "peler", "bgst", "asli", "wkwk", "wkwkwk", "woi", "woii", "bro", "bray",
    "jangan", "berbuat", "sial", "keterlaluan", "bahaya", "diri", "aduh", "kasihan", "kasian",
    "artinya", "penggunaan", "melarang", "modifikasi", "berbasis", "aplikasi", "sistem",
    "fitur", "tombol", "halaman", "pesan", "folder", "file", "baca", "tulis", "simpan",
    "hapus", "kirim", "terima", "pilihan", "pilih", "tambah", "ubah", "ganti", "bantu",
    "tentang", "cara", "main", "mainkan", "pakai", "pake", "server", "bebas", "luas",
    "banyak", "sedikit", "semua", "setiap", "beberapa", "apapun", "siapapun", "manapun",
    "tetap", "selalu", "sering", "kadang", "jarang", "pernah", "dulu", "sekarang", "nantinya",
    "tinggal", "tekan", "kolom", "chat", "menempelkan", "hasil", "terjemahannya", "mirip",
    "kode", "struktur", "membaca", "tanpa"
};

static const std::unordered_set<std::string> ENGLISH_MARKERS = {
    "the", "be", "to", "of", "and", "a", "in", "that", "have", "i", "it", "for", "not",
    "on", "with", "he", "as", "you", "do", "at", "this", "but", "his", "by", "from",
    "they", "we", "say", "her", "she", "or", "an", "will", "my", "one", "all", "would",
    "there", "their", "what", "so", "up", "out", "if", "about", "who", "get", "which",
    "go", "me", "when", "make", "can", "like", "time", "no", "just", "him", "know",
    "take", "people", "into", "year", "your", "good", "some", "could", "them", "see",
    "other", "than", "then", "now", "look", "only", "come", "its", "over", "think",
    "also", "back", "after", "use", "two", "how", "our", "work", "first", "well",
    "way", "even", "new", "want", "because", "any", "these", "give", "day", "most",
    "us", "gonna", "wanna", "gotta", "tryna", "finna", "bruh", "homie", "fool", "fuck",
    "fucking", "shit", "bitch", "ass", "asshole", "damn", "motherfucker", "nigga",
    "cops", "police", "car", "gun", "drop", "hands", "freeze", "stop", "move", "deadass",
    "straight", "useless", "damn", "meet", "met", "cellphone", "says", "shouts", "whispers",
    "mad", "happy", "feeling", "feel", "fam", "bro", "brother", "sis", "sister", "dawg",
    "dog", "man", "guy", "dude", "cuz", "trippin", "aight", "nah", "yeah", "yep", "nope"
};

static std::string to_lower(const std::string& str) {
    std::string lower = str;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return (char)std::tolower(c); });
    return lower;
}

static std::vector<std::string> extract_words(const std::string& text) {
    std::vector<std::string> words;
    std::regex word_regex("\\b[a-zA-Z]{2,}\\b");
    std::string clean = to_lower(text);
    auto words_begin = std::sregex_iterator(clean.begin(), clean.end(), word_regex);
    auto words_end = std::sregex_iterator();
    for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
        words.push_back(i->str());
    }
    return words;
}

bool is_indonesian_text(const std::string& text) {
    if (text.empty()) return false;
    
    std::string lower = to_lower(text);
    if (lower.rfind("/me", 0) == 0 || lower.rfind("/do", 0) == 0) {
        return true;
    }

    auto words = extract_words(text);
    if (words.empty()) return false;

    size_t id_matches = 0;
    size_t en_matches = 0;

    for (const auto& w : words) {
        if (INDONESIAN_MARKERS.count(w)) id_matches++;
        if (ENGLISH_MARKERS.count(w)) en_matches++;
    }

    if (id_matches >= 1) return true;
    if (en_matches == 0) return true; // Default fallback for Indonesian dialogue without English markers

    return false;
}

bool should_translate_inbound(const std::string& text) {
    if (text.empty()) return false;
    auto words = extract_words(text);
    if (words.empty()) return false;

    size_t english_matches = 0;
    size_t indonesian_matches = 0;
    for (const auto& w : words) {
        if (ENGLISH_MARKERS.count(w)) english_matches++;
        if (INDONESIAN_MARKERS.count(w)) indonesian_matches++;
    }

    // 1. If it contains clear English words or slang markers, translate it!
    if (english_matches >= 1) {
        return true;
    }

    // 2. If it contains Indonesian markers and no English markers, it's native Indonesian (skip)
    if (indonesian_matches >= 1) {
        return false;
    }

    // 3. Foreign text or unknown words (e.g. Spanish, French, etc.) -> translate
    return true;
}

void KeyPoolManager::update_keys(const std::vector<std::string>& new_keys) {
    std::lock_guard<std::mutex> lock(mtx);
    keys = new_keys;
    auto now = std::chrono::system_clock::now();
    for (const auto& k : keys) {
        if (key_states.find(k) == key_states.end()) {
            KeyState st;
            st.key = k;
            st.cooldown_until = now;
            st.status = "ACTIVE";
            key_states[k] = st;
        }
    }
    if (!keys.empty()) {
        current_index = current_index % keys.size();
    } else {
        current_index = 0;
    }
}

bool KeyPoolManager::get_next_working_key(std::string& out_key, size_t& out_idx, std::string& out_masked) {
    std::lock_guard<std::mutex> lock(mtx);
    if (keys.empty()) return false;

    auto now = std::chrono::system_clock::now();
    size_t n = keys.size();

    for (size_t offset = 0; offset < n; ++offset) {
        size_t idx = (current_index + offset) % n;
        const std::string& k = keys[idx];
        auto& st = key_states[k];
        if (now >= st.cooldown_until && st.status != "INVALID") {
            current_index = idx;
            out_key = k;
            out_idx = idx;
            out_masked = (k.length() > 10) ? k.substr(0, 6) + "..." + k.substr(k.length() - 4) : "***";
            return true;
        }
    }

    // Pick key with shortest cooldown if all are on cooldown
    size_t best_idx = 0;
    auto earliest = key_states[keys[0]].cooldown_until;
    for (size_t i = 1; i < n; ++i) {
        if (key_states[keys[i]].cooldown_until < earliest) {
            earliest = key_states[keys[i]].cooldown_until;
            best_idx = i;
        }
    }
    current_index = best_idx;
    out_key = keys[best_idx];
    out_idx = best_idx;
    out_masked = (out_key.length() > 10) ? out_key.substr(0, 6) + "..." + out_key.substr(out_key.length() - 4) : "***";
    return true;
}

void KeyPoolManager::mark_rate_limited(const std::string& key, int status_code, const std::string& response_text) {
    std::lock_guard<std::mutex> lock(mtx);
    if (key_states.find(key) == key_states.end()) return;
    auto& st = key_states[key];
    auto now = std::chrono::system_clock::now();

    if (status_code == 401) {
        st.status = "INVALID";
        st.cooldown_until = now + std::chrono::hours(24);
    } else if (status_code == 429) {
        if (response_text.find("daily") != std::string::npos) {
            st.status = "EXHAUSTED_DAILY";
            st.cooldown_until = now + std::chrono::hours(1);
        } else {
            st.status = "COOLDOWN_MINUTE";
            st.cooldown_until = now + std::chrono::seconds(60);
        }
    }
    st.failure_count++;
    if (!keys.empty()) {
        current_index = (current_index + 1) % keys.size();
    }
}

void KeyPoolManager::update_key_metrics(const std::string& key, int rpd_rem, int rpd_lim, const std::string& rpd_rst) {
    std::lock_guard<std::mutex> lock(mtx);
    if (key_states.find(key) == key_states.end()) return;
    auto& st = key_states[key];
    st.status = "ACTIVE";
    st.rpd_remaining = rpd_rem;
    st.rpd_limit = rpd_lim;
    st.rpd_reset = rpd_rst;
    st.failure_count = 0;
    st.cooldown_until = std::chrono::system_clock::now();
}

size_t KeyPoolManager::total_keys() {
    std::lock_guard<std::mutex> lock(mtx);
    return keys.size();
}

std::string KeyPoolManager::get_pool_summary() {
    std::lock_guard<std::mutex> lock(mtx);
    size_t total = keys.size();
    if (total == 0) return "Tidak ada API key terdaftar";

    auto now = std::chrono::system_clock::now();
    size_t ready_count = 0;
    for (const auto& k : keys) {
        if (key_states.find(k) != key_states.end()) {
            if (now >= key_states[k].cooldown_until && key_states[k].status != "INVALID") {
                ready_count++;
            }
        }
    }
    size_t curr = total > 0 ? (current_index + 1) : 0;
    return std::to_string(total) + " Token Terdaftar | Rolling Pointer: Key #" + std::to_string(curr) + " (" + std::to_string(ready_count) + "/" + std::to_string(total) + " Siap)";
}

void GroqTranslator::update_api_keys(const std::vector<std::string>& keys) {
    key_pool.update_keys(keys);
}

static std::string escape_json(const std::string& s) {
    std::ostringstream o;
    for (char c : s) {
        switch (c) {
        case '"': o << "\\\""; break;
        case '\\': o << "\\\\"; break;
        case '\b': o << "\\b"; break;
        case '\f': o << "\\f"; break;
        case '\n': o << "\\n"; break;
        case '\r': o << "\\r"; break;
        case '\t': o << "\\t"; break;
        default:
            if ('\x00' <= c && c <= '\x1f') {
                o << "\\u" << std::hex << (int)c;
            } else {
                o << c;
            }
        }
    }
    return o.str();
}

std::string GroqTranslator::send_winhttp_request(const std::string& key, const std::string& payload_json, int& out_status_code, std::string& out_headers) {
    out_status_code = 0;
    out_headers = "";

    std::string masked = (key.length() > 10) ? key.substr(0, 6) + "..." + key.substr(key.length() - 4) : "***";

    HINTERNET hSession = WinHttpOpen(L"SA-RP-Linggo-ASI/1.3",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) {
        LogDebugLocal("[WinHttp Error] WinHttpOpen failed!");
        return "";
    }

    HINTERNET hConnect = WinHttpConnect(hSession, L"api.groq.com", INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) {
        LogDebugLocal("[WinHttp Error] WinHttpConnect failed!");
        WinHttpCloseHandle(hSession);
        return "";
    }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", L"/openai/v1/chat/completions",
        NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);

    if (!hRequest) {
        LogDebugLocal("[WinHttp Error] WinHttpOpenRequest failed!");
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return "";
    }

    std::wstring headers = L"Authorization: Bearer " + std::wstring(key.begin(), key.end()) + L"\r\nContent-Type: application/json\r\n";

    BOOL bResults = WinHttpSendRequest(hRequest,
        headers.c_str(), (DWORD)headers.length(),
        (LPVOID)payload_json.c_str(), (DWORD)payload_json.length(),
        (DWORD)payload_json.length(), 0);

    std::string response_data = "";
    if (bResults) {
        bResults = WinHttpReceiveResponse(hRequest, NULL);
    } else {
        LogDebugLocal("[WinHttp Error] WinHttpSendRequest failed with error code: " + std::to_string(GetLastError()));
    }

    if (bResults) {
        DWORD dwStatusCode = 0;
        DWORD dwSize = sizeof(dwStatusCode);
        WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX, &dwStatusCode, &dwSize, WINHTTP_NO_HEADER_INDEX);

        out_status_code = (int)dwStatusCode;

        // Query Raw Headers for RPD metric tracking
        DWORD dwHeaderSize = 0;
        WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_RAW_HEADERS_CRLF, WINHTTP_HEADER_NAME_BY_INDEX, NULL, &dwHeaderSize, WINHTTP_NO_HEADER_INDEX);
        if (dwHeaderSize > 0) {
            std::vector<wchar_t> header_buf(dwHeaderSize / sizeof(wchar_t) + 1);
            if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_RAW_HEADERS_CRLF, WINHTTP_HEADER_NAME_BY_INDEX, header_buf.data(), &dwHeaderSize, WINHTTP_NO_HEADER_INDEX)) {
                std::wstring w_hdrs(header_buf.data());
                out_headers = std::string(w_hdrs.begin(), w_hdrs.end());
            }
        }

        DWORD dwDownloaded = 0;
        do {
            dwSize = 0;
            if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) break;
            if (dwSize == 0) break;

            std::vector<char> buf(dwSize + 1);
            if (WinHttpReadData(hRequest, (LPVOID)buf.data(), dwSize, &dwDownloaded)) {
                response_data.append(buf.data(), dwDownloaded);
            }
        } while (dwSize > 0);

        // Parse token usage metrics from JSON body
        std::regex r_p_tok("\"prompt_tokens\":\\s*(\\d+)");
        std::regex r_c_tok("\"completion_tokens\":\\s*(\\d+)");
        std::regex r_t_tok("\"total_tokens\":\\s*(\\d+)");
        std::smatch m_tok;

        if (std::regex_search(response_data, m_tok, r_p_tok)) last_prompt_tokens = std::stoi(m_tok[1].str());
        if (std::regex_search(response_data, m_tok, r_c_tok)) last_completion_tokens = std::stoi(m_tok[1].str());
        if (std::regex_search(response_data, m_tok, r_t_tok)) last_total_tokens = std::stoi(m_tok[1].str());

        LogDebugLocal("[Token Usage] Prompt: " + std::to_string(last_prompt_tokens) + " | Completion: " + std::to_string(last_completion_tokens) + " | Total: " + std::to_string(last_total_tokens) + " tokens (Key: " + masked + ")");

        if (response_data.find("\"finish_reason\":\"length\"") != std::string::npos) {
            LogDebugLocal("[Groq Warning] Response output was truncated because it reached max_tokens limit!");
        }

        // Parse remaining requests from header if present
        std::regex r_rem("x-ratelimit-remaining-requests:\\s*(\\d+)", std::regex::icase);
        std::regex r_lim("x-ratelimit-limit-requests:\\s*(\\d+)", std::regex::icase);
        std::regex r_rst("x-ratelimit-reset-requests:\\s*([^\\r\\n]+)", std::regex::icase);
        std::smatch m;

        if (std::regex_search(out_headers, m, r_rem)) {
            last_rpd_remaining = std::stoi(m[1].str());
        }
        if (std::regex_search(out_headers, m, r_lim)) {
            last_rpd_limit = std::stoi(m[1].str());
        }
        if (std::regex_search(out_headers, m, r_rst)) {
            last_rpd_reset = m[1].str();
        }

        if (dwStatusCode == 200) {
            key_pool.update_key_metrics(key, last_rpd_remaining, last_rpd_limit, last_rpd_reset);
        } else {
            LogDebugLocal("[Groq Error] HTTP Status Code: " + std::to_string(dwStatusCode) + " | Response Body: " + response_data);
            key_pool.mark_rate_limited(key, (int)dwStatusCode, response_data);
        }
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    return response_data;
}

std::string GroqTranslator::clean_translation_output(const std::string& raw_output) {
    if (raw_output.empty()) return "";
    std::string text = raw_output;

    // Remove <think>...</think>
    std::regex think_regex("<think>[\\s\\S]*?</think>");
    text = std::regex_replace(text, think_regex, "");

    // Trim quotes and whitespace
    size_t first = text.find_first_not_of(" \t\r\n\"`*");
    if (first == std::string::npos) return "";
    size_t last = text.find_last_not_of(" \t\r\n\"`*");
    text = text.substr(first, (last - first + 1));

    // Restore uncensored profanity if asterisks were outputted
    std::regex r1("motherf[*#@%]+er", std::regex::icase); text = std::regex_replace(text, r1, "motherfucker");
    std::regex r2("f[*#@%]+k", std::regex::icase); text = std::regex_replace(text, r2, "fuck");
    std::regex r3("b[*#@%]+ch", std::regex::icase); text = std::regex_replace(text, r3, "bitch");
    std::regex r4("a[*#@%]+shole", std::regex::icase); text = std::regex_replace(text, r4, "asshole");
    std::regex r5("n[*#@%]+ga", std::regex::icase); text = std::regex_replace(text, r5, "nigga");

    return text;
}

std::string GroqTranslator::translate_inbound(const std::string& text, const std::string& chat_type) {
    if (text.empty()) return text;

    size_t total_attempts = key_pool.total_keys();
    if (total_attempts == 0) {
        last_error_detail = "[Groq API Key Belum Diisi / Token Pool Kosong]";
        LogDebugLocal("[Groq Error] Inbound translation failed: No Groq API key available!");
        return "";
    }

    // Clean formal Indonesian system prompt (Strictly prohibit reasoning monologue or think tags)
    std::string system_prompt =
        "You are a professional translator for GTA SA-MP roleplay.\\n"
        "Translate foreign input text into clear, formal, natural Indonesian (Bahasa Indonesia yang baik, benar, dan natural).\\n\\n"
        "CRITICAL DIRECTIVES:\\n"
        "1. OUTPUT ONLY THE FINAL INDONESIAN TRANSLATION SENTENCE.\\n"
        "2. DO NOT use <think> tags, internal monologue, reasoning, or explanations.\\n"
        "3. Use standard formal Indonesian pronouns ('Saya/Aku', 'Anda/Kamu') instead of street slang ('lu', 'gue').";

    std::string json_payload = "{"
        "\"model\":\"" + model + "\","
        "\"messages\":["
        "{\"role\":\"system\",\"content\":\"" + escape_json(system_prompt) + "\"},"
        "{\"role\":\"user\",\"content\":\"" + escape_json(text) + "\"}"
        "],"
        "\"temperature\":0.1,"
        "\"max_tokens\":1024"
        "}";

    for (size_t attempt = 0; attempt < total_attempts; ++attempt) {
        std::string key, masked;
        size_t key_idx = 0;
        if (!key_pool.get_next_working_key(key, key_idx, masked)) {
            break;
        }

        int status_code = 0;
        std::string out_headers = "";
        std::string res_json = send_winhttp_request(key, json_payload, status_code, out_headers);

        if (status_code == 200 && !res_json.empty()) {
            size_t content_pos = res_json.find("\"content\":");
            if (content_pos != std::string::npos) {
                size_t start_q = res_json.find("\"", content_pos + 10);
                if (start_q != std::string::npos) {
                    size_t end_q = start_q + 1;
                    while (end_q < res_json.length()) {
                        if (res_json[end_q] == '"' && res_json[end_q - 1] != '\\') break;
                        end_q++;
                    }
                    std::string content = res_json.substr(start_q + 1, end_q - start_q - 1);
                    std::string cleaned = clean_translation_output(content);
                    if (cleaned.empty()) {
                        LogDebugLocal("[Groq Error] Raw content extracted but clean_translation_output returned empty! Raw: " + content);
                    } else {
                        last_error_detail = "";
                        return cleaned;
                    }
                }
            }
            LogDebugLocal("[Groq Error] Failed to parse content field from HTTP 200 response: " + res_json);
        } else {
            key_pool.mark_rate_limited(key, status_code, res_json);
            LogDebugLocal("[Rolling Key Rotation] Key #" + std::to_string(key_idx + 1) + " (" + masked + ") hit status " + std::to_string(status_code) + ". Retrying with next token...");
        }
    }

    last_error_detail = "[Semua API Key Rate-Limited / Error]";
    return "";
}

std::string GroqTranslator::translate_outbound(const std::string& text, const std::string& style) {
    if (text.empty()) return text;

    size_t total_attempts = key_pool.total_keys();
    if (total_attempts == 0) {
        last_error_detail = "[Groq API Key Belum Diisi / Token Pool Kosong]";
        LogDebugLocal("[Groq Error] Outbound translation failed: No Groq API key available!");
        return text;
    }

    std::string active_style = (style == "American Hood") ? "American Hood" : "Standard English";
    std::string lower = to_lower(text);

    std::string system_prompt = "";
    if (active_style == "American Hood") {
        if (lower.rfind("/me", 0) == 0) {
            system_prompt = "You are an expert GTA SA-MP roleplay translator. Translate the input action into authentic third-person physical action roleplay for GTA SA-MP. Use vivid, authentic hood/street action descriptions (e.g. 'slides the key into the ignition', 'fires up the bike', 'tucks the piece'). DO NOT append spoken dialogue slang (like 'homie', 'cuz', 'bro') to physical actions unless the input contains actual quoted dialogue. Output MUST start with '/me '. OUTPUT ONLY the final translated '/me [ENGLISH ACTION]'.";
        } else if (lower.rfind("/do", 0) == 0) {
            system_prompt = "You are an expert GTA SA-MP roleplay translator. Translate the environment or state input into a clear third-person environment, situation, or fact description for GTA SA-MP roleplay. DO NOT use spoken dialogue slang. Output MUST start with '/do '. OUTPUT ONLY the final translated '/do [ENGLISH DESCRIPTION]'.";
        } else {
            system_prompt = "You are an expert GTA SA-MP roleplay translator converting Indonesian text into raw, authentic American Hood / Ghetto / Gangster street spoken English (using AAVE terms like 'finna', 'nawmsayin', 'trippin', 'fool', 'homie', 'cuz', 'aight', 'pop', 'strapped', 'loc'). Preserve all swear words and aggression. DO NOT use <think> tags or internal monologue. OUTPUT ONLY final translated English text without quotes.";
        }
    } else {
        if (lower.rfind("/me", 0) == 0) {
            system_prompt = "You are an expert GTA SA-MP roleplay translator. Translate the input action into proper third-person physical action roleplay in Standard English for GTA SA-MP. Output MUST start with '/me '. OUTPUT ONLY final translated '/me [ENGLISH ACTION]'.";
        } else if (lower.rfind("/do", 0) == 0) {
            system_prompt = "You are an expert GTA SA-MP roleplay translator. Translate the environment or state input into a proper third-person environment, situation, or fact description in Standard English for GTA SA-MP roleplay. Output MUST start with '/do '. OUTPUT ONLY final translated '/do [ENGLISH DESCRIPTION]'.";
        } else {
            system_prompt = "You are an expert GTA SA-MP roleplay translator converting Indonesian text into proper, clear Standard English dialogue. Preserve swear words and tone accurately. DO NOT use <think> tags or internal monologue. OUTPUT ONLY final translated English text without quotes.";
        }
    }

    std::string json_payload = "{"
        "\"model\":\"" + model + "\","
        "\"messages\":["
        "{\"role\":\"system\",\"content\":\"" + escape_json(system_prompt) + "\"},"
        "{\"role\":\"user\",\"content\":\"" + escape_json(text) + "\"}"
        "],"
        "\"temperature\":0.3,"
        "\"max_tokens\":1024"
        "}";

    for (size_t attempt = 0; attempt < total_attempts; ++attempt) {
        std::string key, masked;
        size_t key_idx = 0;
        if (!key_pool.get_next_working_key(key, key_idx, masked)) {
            break;
        }

        int status_code = 0;
        std::string out_headers = "";
        std::string res_json = send_winhttp_request(key, json_payload, status_code, out_headers);

        if (status_code == 200 && !res_json.empty()) {
            size_t content_pos = res_json.find("\"content\":");
            if (content_pos != std::string::npos) {
                size_t start_q = res_json.find("\"", content_pos + 10);
                if (start_q != std::string::npos) {
                    size_t end_q = start_q + 1;
                    while (end_q < res_json.length()) {
                        if (res_json[end_q] == '"' && res_json[end_q - 1] != '\\') break;
                        end_q++;
                    }
                    std::string content = res_json.substr(start_q + 1, end_q - start_q - 1);
                    std::string cleaned = clean_translation_output(content);
                    if (cleaned.empty()) {
                        LogDebugLocal("[Groq Error] Raw outbound content extracted but clean_translation_output returned empty! Raw: " + content);
                    } else {
                        last_error_detail = "";
                        return cleaned;
                    }
                }
            }
            LogDebugLocal("[Groq Error] Outbound content field missing from HTTP 200 response: " + res_json);
        } else {
            key_pool.mark_rate_limited(key, status_code, res_json);
            LogDebugLocal("[Rolling Key Rotation] Key #" + std::to_string(key_idx + 1) + " (" + masked + ") failed with status " + std::to_string(status_code) + ". Retrying with next token...");
        }
    }

    last_error_detail = "[Semua API Key Rate-Limited / Error]";
    return text;
}

bool GroqTranslator::check_rpd_quota(std::string& out_summary) {
    auto keys = key_pool.get_keys();
    if (keys.empty()) {
        out_summary = "API Key Kosong";
        return false;
    }

    std::stringstream ss;
    bool overall_success = false;

    for (size_t i = 0; i < keys.size(); ++i) {
        const std::string& key = keys[i];
        std::string masked = (key.length() > 10) ? key.substr(0, 6) + "..." + key.substr(key.length() - 4) : "***";

        std::string payload = "{\"model\":\"" + model + "\",\"messages\":[{\"role\":\"user\",\"content\":\"ping\"}],\"max_tokens\":1}";
        int status_code = 0;
        std::string headers = "";
        send_winhttp_request(key, payload, status_code, headers);

        ss << "Key #" << (i + 1) << " (" << masked << "): ";
        if (status_code == 200) {
            overall_success = true;
            ss << "Sisa RPD " << last_rpd_remaining << "/" << last_rpd_limit << " (Reset: " << last_rpd_reset << ")\n";
        } else if (status_code == 429) {
            ss << "[!] Rate-Limited (429)\n";
        } else if (status_code == 401) {
            ss << "[X] Invalid Key (401)\n";
        } else {
            ss << "Error HTTP " << status_code << "\n";
        }
    }

    out_summary = ss.str();
    return overall_success;
}

std::string GroqTranslator::clean_rp_action(const std::string& text) {
    if (text.empty()) return text;
    std::string result = text;
    try {
        result = std::regex_replace(result, std::regex("^(?:pasar|sar)\\s+(mahluk|manusia|anjing|bangsat|tolol|bego)", std::regex_constants::icase), "dasar $1");
        result = std::regex_replace(result, std::regex("\\bdiuntuk\\b", std::regex_constants::icase), "diuntung");

        if (std::regex_search(result, std::regex("^(?:[a-zA-Z]*(?:sdo|shdo)|(?:slash|selas|seles|slas|sles|proses|perses|plas)\\s*do|do)\\b", std::regex_constants::icase))) {
            return std::regex_replace(result, std::regex("^(?:[a-zA-Z]*(?:sdo|shdo)|(?:slash|selas|seles|slas|sles|proses|perses|plas)\\s*do|do)\\b", std::regex_constants::icase), "/do");
        }
        if (std::regex_search(result, std::regex("^(?:[a-zA-Z]*(?:smi|shmi|sme|shme)|(?:slash|selas|seles|slas|sles|proses|perses|plas)\\s*(?:mi|me)?|me)\\b", std::regex_constants::icase))) {
            return std::regex_replace(result, std::regex("^(?:[a-zA-Z]*(?:smi|shmi|sme|shme)|(?:slash|selas|seles|slas|sles|proses|perses|plas)\\s*(?:mi|me)?|me)\\b", std::regex_constants::icase), "/me");
        }
    } catch (...) {}
    return result;
}

std::string GroqTranslator::transcribe_audio(const std::vector<uint8_t>& wav_bytes, std::string& out_error) {
    if (wav_bytes.empty()) {
        out_error = "Ukuran Audio Kosong";
        return "";
    }

    std::string key, masked;
    size_t key_idx = 0;
    size_t total_attempts = key_pool.total_keys();
    if (total_attempts == 0) {
        out_error = "[Groq API Key Belum Diisi]";
        return "";
    }

    std::string boundary = "----SARPLinggoBoundary98765";
    std::string prompt = "Percakapan Bahasa Indonesia SAMP Roleplay: /me, /do, slash me, slash do, dasar, mahluk, manusia, kamu, lu, gue, bangsat, anjing, kontol, bajingan, tidak tahu diri, tidak tahu diuntung.";

    // Build Multipart Body
    std::vector<uint8_t> body;
    auto add_string = [&](const std::string& str) {
        body.insert(body.end(), str.begin(), str.end());
    };

    // Part 1: model
    add_string("--" + boundary + "\r\nContent-Disposition: form-data; name=\"model\"\r\n\r\nwhisper-large-v3-turbo\r\n");
    // Part 2: language
    add_string("--" + boundary + "\r\nContent-Disposition: form-data; name=\"language\"\r\n\r\nid\r\n");
    // Part 3: prompt
    add_string("--" + boundary + "\r\nContent-Disposition: form-data; name=\"prompt\"\r\n\r\n" + prompt + "\r\n");
    // Part 4: response_format
    add_string("--" + boundary + "\r\nContent-Disposition: form-data; name=\"response_format\"\r\n\r\njson\r\n");
    // Part 5: file
    add_string("--" + boundary + "\r\nContent-Disposition: form-data; name=\"file\"; filename=\"speech.wav\"\r\nContent-Type: audio/wav\r\n\r\n");
    body.insert(body.end(), wav_bytes.begin(), wav_bytes.end());
    add_string("\r\n--" + boundary + "--\r\n");

    for (size_t attempt = 0; attempt < total_attempts; ++attempt) {
        if (!key_pool.get_next_working_key(key, key_idx, masked)) {
            break;
        }

        HINTERNET hSession = WinHttpOpen(L"SA-RP-Linggo/1.3", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
        if (!hSession) continue;

        HINTERNET hConnect = WinHttpConnect(hSession, L"api.groq.com", INTERNET_DEFAULT_HTTPS_PORT, 0);
        if (!hConnect) {
            WinHttpCloseHandle(hSession);
            continue;
        }

        HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", L"/openai/v1/audio/transcriptions", NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
        if (!hRequest) {
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            continue;
        }

        std::string headers = "Authorization: Bearer " + key + "\r\nContent-Type: multipart/form-data; boundary=" + boundary + "\r\n";
        std::wstring wheaders(headers.begin(), headers.end());

        BOOL bResults = WinHttpSendRequest(hRequest, wheaders.c_str(), (DWORD)wheaders.length(), (LPVOID)body.data(), (DWORD)body.size(), (DWORD)body.size(), 0);
        if (bResults) bResults = WinHttpReceiveResponse(hRequest, NULL);

        DWORD status_code = 0;
        DWORD dwSize = sizeof(status_code);
        if (bResults) {
            WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &status_code, &dwSize, WINHTTP_NO_HEADER_INDEX);
        }

        std::string res_text = "";
        if (bResults) {
            DWORD dwDownloaded = 0;
            do {
                dwSize = 0;
                if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) break;
                if (dwSize == 0) break;

                std::vector<char> buffer(dwSize + 1, 0);
                if (WinHttpReadData(hRequest, (LPVOID)buffer.data(), dwSize, &dwDownloaded)) {
                    res_text.append(buffer.data(), dwDownloaded);
                }
            } while (dwSize > 0);
        }

        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);

        if (status_code == 200 && !res_text.empty()) {
            size_t text_pos = res_text.find("\"text\":");
            if (text_pos != std::string::npos) {
                size_t start_q = res_text.find("\"", text_pos + 7);
                if (start_q != std::string::npos) {
                    size_t end_q = start_q + 1;
                    while (end_q < res_text.length()) {
                        if (res_text[end_q] == '"' && res_text[end_q - 1] != '\\') break;
                        end_q++;
                    }
                    std::string raw_text = res_text.substr(start_q + 1, end_q - start_q - 1);
                    std::string cleaned_text = clean_rp_action(raw_text);
                    out_error = "";
                    return cleaned_text;
                }
            }
        } else {
            key_pool.mark_rate_limited(key, (int)status_code, res_text);
        }
    }

    out_error = "[Transkripsi Voice Gagal]";
    return "";
}

} // namespace SARPLinggo
