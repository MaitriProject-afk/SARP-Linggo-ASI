#pragma once

#include <windows.h>
#include <mmeapi.h>
#include <string>
#include <vector>
#include <deque>
#include <thread>
#include <mutex>
#include <atomic>
#include <functional>

namespace SARPLinggo {

class GroqTranslator;
struct Config;

class VoiceListener {
private:
    GroqTranslator* translator = nullptr;
    Config* config = nullptr;

    std::atomic<bool> running{false};
    std::atomic<bool> is_recording{false};

    HWAVEIN hWaveIn = NULL;
    WAVEFORMATEX wfx;
    std::vector<uint8_t> recorded_pcm;
    std::deque<std::vector<uint8_t>> pre_buffer;
    std::mutex mtx;

    std::thread worker_thread;
    std::function<void(const std::string& status_msg, const std::string& color_hex)> on_status_cb;
    std::function<void(const std::string& orig_text, const std::string& trans_text)> on_voice_translated_cb;

    static void CALLBACK WaveInProc(HWAVEIN hwi, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2);
    void handle_wave_data(WAVEHDR* pHeader);
    void process_audio_buffer(std::vector<uint8_t> pcm_bytes, float duration);

public:
    VoiceListener() = default;
    ~VoiceListener();

    void init(GroqTranslator* trans, Config* cfg);
    void set_on_status(std::function<void(const std::string&, const std::string&)> cb) { on_status_cb = cb; }
    void set_on_voice_translated(std::function<void(const std::string&, const std::string&)> cb) { on_voice_translated_cb = cb; }

    void start();
    void stop();

    void start_recording();
    void stop_and_process();
    bool get_is_recording() const { return is_recording; }
};

} // namespace SARPLinggo
