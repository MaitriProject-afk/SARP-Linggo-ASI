#include "../include/voice_listener.h"
#include "../include/translator.h"
#include "../include/config.h"

#include <iostream>
#include <fstream>
#include <chrono>
#include <algorithm>

namespace SARPLinggo {

static std::vector<uint8_t> pcm_to_wav(const std::vector<uint8_t>& pcm, uint32_t sample_rate = 16000, uint16_t channels = 1, uint16_t bits_per_sample = 16) {
    uint32_t data_size = (uint32_t)pcm.size();
    uint32_t chunk_size = 36 + data_size;
    uint32_t byte_rate = sample_rate * channels * (bits_per_sample / 8);
    uint16_t block_align = channels * (bits_per_sample / 8);

    std::vector<uint8_t> wav;
    wav.reserve(44 + data_size);

    auto write_32 = [&](uint32_t val) {
        wav.push_back(val & 0xFF);
        wav.push_back((val >> 8) & 0xFF);
        wav.push_back((val >> 16) & 0xFF);
        wav.push_back((val >> 24) & 0xFF);
    };

    auto write_16 = [&](uint16_t val) {
        wav.push_back(val & 0xFF);
        wav.push_back((val >> 8) & 0xFF);
    };

    // RIFF Header
    wav.push_back('R'); wav.push_back('I'); wav.push_back('F'); wav.push_back('F');
    write_32(chunk_size);
    wav.push_back('W'); wav.push_back('A'); wav.push_back('V'); wav.push_back('E');

    // fmt  subchunk
    wav.push_back('f'); wav.push_back('m'); wav.push_back('t'); wav.push_back(' ');
    write_32(16); // Subchunk1Size for PCM
    write_16(1);  // AudioFormat = PCM
    write_16(channels);
    write_32(sample_rate);
    write_32(byte_rate);
    write_16(block_align);
    write_16(bits_per_sample);

    // data subchunk
    wav.push_back('d'); wav.push_back('a'); wav.push_back('t'); wav.push_back('a');
    write_32(data_size);

    wav.insert(wav.end(), pcm.begin(), pcm.end());
    return wav;
}

static void copy_to_clipboard(const std::string& text) {
    if (text.empty()) return;
    if (!OpenClipboard(NULL)) return;
    EmptyClipboard();
    HGLOBAL hGlob = GlobalAlloc(GMEM_MOVEABLE, text.length() + 1);
    if (hGlob) {
        char* pBuf = (char*)GlobalLock(hGlob);
        if (pBuf) {
            memcpy(pBuf, text.c_str(), text.length() + 1);
            GlobalUnlock(hGlob);
            SetClipboardData(CF_TEXT, hGlob);
        }
    }
    CloseClipboard();
}

VoiceListener::~VoiceListener() {
    stop();
}

void VoiceListener::init(GroqTranslator* trans, Config* cfg) {
    translator = trans;
    config = cfg;

    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = 1;
    wfx.nSamplesPerSec = 16000;
    wfx.wBitsPerSample = 16;
    wfx.nBlockAlign = wfx.nChannels * (wfx.wBitsPerSample / 8);
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
    wfx.cbSize = 0;
}

void CALLBACK VoiceListener::WaveInProc(HWAVEIN hwi, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2) {
    if (uMsg == WIM_DATA) {
        VoiceListener* pThis = (VoiceListener*)dwInstance;
        if (pThis) {
            WAVEHDR* pHeader = (WAVEHDR*)dwParam1;
            pThis->handle_wave_data(pHeader);
        }
    }
}

void VoiceListener::handle_wave_data(WAVEHDR* pHeader) {
    if (!pHeader || pHeader->dwBytesRecorded == 0) return;

    std::vector<uint8_t> chunk(pHeader->lpData, pHeader->lpData + pHeader->dwBytesRecorded);

    {
        std::lock_guard<std::mutex> lock(mtx);
        pre_buffer.push_back(chunk);
        while (pre_buffer.size() > 15) { // Keep ~450ms pre-buffer
            pre_buffer.pop_front();
        }

        if (is_recording) {
            recorded_pcm.insert(recorded_pcm.end(), chunk.begin(), chunk.end());
        }
    }

    if (running && hWaveIn) {
        waveInUnprepareHeader(hWaveIn, pHeader, sizeof(WAVEHDR));
        waveInPrepareHeader(hWaveIn, pHeader, sizeof(WAVEHDR));
        waveInAddBuffer(hWaveIn, pHeader, sizeof(WAVEHDR));
    }
}

void VoiceListener::start() {
    if (running) return;
    running = true;

    MMRESULT res = waveInOpen(&hWaveIn, WAVE_MAPPER, &wfx, (DWORD_PTR)WaveInProc, (DWORD_PTR)this, CALLBACK_FUNCTION);
    if (res != MMSYSERR_NOERROR) {
        hWaveIn = NULL;
        return;
    }

    // Allocate 4 recording buffers of 100ms each
    static WAVEHDR headers[4];
    static std::vector<uint8_t> buffers[4];
    DWORD buf_size = wfx.nAvgBytesPerSec / 10; // 100ms buffer

    for (int i = 0; i < 4; ++i) {
        buffers[i].resize(buf_size, 0);
        ZeroMemory(&headers[i], sizeof(WAVEHDR));
        headers[i].lpData = (LPSTR)buffers[i].data();
        headers[i].dwBufferLength = buf_size;

        waveInPrepareHeader(hWaveIn, &headers[i], sizeof(WAVEHDR));
        waveInAddBuffer(hWaveIn, &headers[i], sizeof(WAVEHDR));
    }

    waveInStart(hWaveIn);

    worker_thread = std::thread([this]() {
        bool key_was_down = false;

        while (running) {
            try {
                if (config && config->enable_voice_input) {
                    int vk = config->voice_hotkey_vk;
                    bool key_down = (GetAsyncKeyState(vk) & 0x8000) != 0;

                    if (key_down && !key_was_down) {
                        start_recording();
                    } else if (!key_down && key_was_down) {
                        stop_and_process();
                    }
                    key_was_down = key_down;
                }
            } catch (...) {}
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    });
}

void VoiceListener::stop() {
    running = false;
    if (worker_thread.joinable()) {
        worker_thread.join();
    }

    if (hWaveIn) {
        waveInReset(hWaveIn);
        waveInClose(hWaveIn);
        hWaveIn = NULL;
    }
}

void VoiceListener::start_recording() {
    std::lock_guard<std::mutex> lock(mtx);
    is_recording = true;
    recorded_pcm.clear();

    // Pre-fill recorded_pcm with pre_buffer (~400ms)
    for (const auto& chunk : pre_buffer) {
        recorded_pcm.insert(recorded_pcm.end(), chunk.begin(), chunk.end());
    }

    if (on_status_cb) {
        on_status_cb("🎙️ Recording Voice... (Release Key to Send)", "#EF4444");
    }
}

void VoiceListener::stop_and_process() {
    std::vector<uint8_t> pcm_to_process;
    {
        std::lock_guard<std::mutex> lock(mtx);
        is_recording = false;
        pcm_to_process = recorded_pcm;
        recorded_pcm.clear();
    }

    if (pcm_to_process.empty()) {
        if (on_status_cb) on_status_cb("⚠️ Short Recording (No Audio)", "#F59E0B");
        return;
    }

    float duration = (float)pcm_to_process.size() / (float)wfx.nAvgBytesPerSec;
    if (duration < 0.35f) {
        if (on_status_cb) on_status_cb("⚠️ Hold hotkey longer to talk", "#F59E0B");
        return;
    }

    // Check Audio Volume / Peak Amplitude to filter out silent mic / muted mic
    const int16_t* samples = (const int16_t*)pcm_to_process.data();
    size_t sample_count = pcm_to_process.size() / sizeof(int16_t);
    int16_t max_amp = 0;
    double sum_sq = 0.0;
    for (size_t i = 0; i < sample_count; ++i) {
        int16_t s = abs(samples[i]);
        if (s > max_amp) max_amp = s;
        sum_sq += (double)s * s;
    }
    double rms = (sample_count > 0) ? sqrt(sum_sq / (double)sample_count) : 0.0;

    if (max_amp < 250 || rms < 40.0) {
        if (on_status_cb) on_status_cb("⚠️ Mic Silent / Muted (Cek Volume Mic Windows)", "#F59E0B");
        return;
    }

    std::thread(&VoiceListener::process_audio_buffer, this, pcm_to_process, duration).detach();
}

void VoiceListener::process_audio_buffer(std::vector<uint8_t> pcm_bytes, float duration) {
    try {
        char status_buf[128];
        sprintf_s(status_buf, sizeof(status_buf), "⚡ Transcribing Voice (%.1fs)...", duration);
        if (on_status_cb) on_status_cb(status_buf, "#38BDF8");

        if (!translator) {
            if (on_status_cb) on_status_cb("⚠️ Translator Not Ready", "#EF4444");
            return;
        }

        std::vector<uint8_t> wav_bytes = pcm_to_wav(pcm_bytes, wfx.nSamplesPerSec, wfx.nChannels, wfx.wBitsPerSample);
        
        std::string err = "";
        std::string indonesian_text = translator->transcribe_audio(wav_bytes, err);
        if (!err.empty() || indonesian_text.empty()) {
            if (on_status_cb) on_status_cb(err.empty() ? "⚠️ Speech Not Recognized" : err, "#F59E0B");
            return;
        }

        std::string style = config ? config->outbound_style : "Standard English";
        if (on_status_cb) on_status_cb("⚡ Translating to " + style + "...", "#38BDF8");

        std::string translated_text = translator->translate_outbound(indonesian_text, style);
        if (translated_text.empty()) {
            if (on_status_cb) on_status_cb("⚠️ Translation Failed", "#EF4444");
            return;
        }

        copy_to_clipboard(translated_text);

        if (on_status_cb) on_status_cb("● Voice Outbound Ready! (Press CTRL+V)", "#06B6D4");

        if (on_voice_translated_cb) {
            on_voice_translated_cb("🎙️ " + indonesian_text, translated_text);
        }
    } catch (...) {
        if (on_status_cb) on_status_cb("⚠️ Voice Error Exception", "#EF4444");
    }
}

} // namespace SARPLinggo
