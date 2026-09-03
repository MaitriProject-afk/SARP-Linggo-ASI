# 🌐 SA-RP Linggo ASI v1.4.0 (Voice Input & Open-Source Build)

> **Advanced Real-Time AI Translation, Speech-To-Text Voice Input, & American Hood Slang Converter Overlay for GTA SA-MP Roleplay (DirectX 9 Native ASI Plugin)**

![License](https://img.shields.io/badge/License-MIT-green.svg)
![Game](https://img.shields.io/badge/Game-GTA%20San%20Andreas%20%2F%20SA--MP-orange.svg)
![C++](https://img.shields.io/badge/Language-C%2B%2B17-green.svg)
![Architecture](https://img.shields.io/badge/Architecture-x86%20(32--bit)-purple.svg)
![Status](https://img.shields.io/badge/Status-100%25%20Free%20%26%20Open%20Source-blue.svg)

**SA-RP Linggo ASI** adalah plugin native C++ (`.asi`) untuk **GTA San Andreas / SA-MP (Jogjgamers Reality Project / Roleplay Server)** yang membaca *chatlog* game secara real-time dan menampilkan hasil terjemahan Bahasa Inggris ⇄ Indonesia, **Voice Input Speech-to-Text (Groq Whisper API)**, serta **American Hood Slang Converter** langsung di atas layar game menggunakan **ImGui DirectX 9 Overlay**.

Mulai versi 1.3.0+, **SA-RP Linggo telah resmi menjadi proyek 100% Free & Open-Source (MIT License)** di bawah naungan **MaitriProject**. Seluruh sistem lisensi/token telah dihapus penuh — Anda cukup memasukkan Groq API Key milik sendiri (gratis) untuk langsung menggunakannya.

---

## ✨ Fitur Utama v1.4.0

- 🎙️ **Voice Input Push-To-Talk (Groq Whisper STT)**: Bicara langsung ke mikrofon (Default Hotkey `F4`), audio Anda otomatis ditranskrip oleh Groq Whisper API (`whisper-large-v3-turbo`) dengan deteksi kata RP khusus (`/me`, `/do`, `slash me`, `slash do`) dan langsung diterjemahkan ke Bahasa Inggris untuk di-paste (`CTRL+V`) ke dalam game.
- ⚡ **Inbound Live Chat Translator (IC, /me, /do, Says, Shouts, Whispers)**: Membaca percakapan lawan RP di `chatlog.txt` secara otomatis dan menampilkan terjemahan Bahasa Indonesia di Overlay Game.
- 🇺🇸 **American Hood & Ghetto Slang Mode**: Pilihan mode terjemahan gaya bahasa jalanan Amerika (AAVE / Gangster Slang seperti *finna, homie, dawg, trippin, aight, loc, strapped*) untuk roleplay geng yang authentic.
- 📋 **Outbound Clipboard Auto-Translate**: Membaca teks Bahasa Indonesia di clipboard (`CTRL+C`) lalu menyalin balik hasil terjemahan Bahasa Inggris ke clipboard untuk langsung di-paste (`CTRL+V`) di dalam game.
- 🔑 **Rolling Groq API Token Pool**: Dukungan multiple API key Groq dengan rotasi otomatis dan perbaikan deteksi marker Bahasa Inggris kasual 1:1.
- 🎮 **DirectX 9 ImGui Overlay**: Interface overlay modern yang *lightweight*, responsif, dan tidak mempengaruhi FPS game.
- 🔓 **100% Free & Open-Source**: Bebas biaya, tanpa kunci lisensi, tanpa pengumpulan data pribadi/HWID.
- 🔒 **Zero-Lag & Thread Safe**: Dibuat menggunakan multi-threading C++17 terpisah sehingga proses terjemahan HTTP WinHTTP tidak mengganggu *looping* rendering utama game.
- 🛡️ **Crash-Resistant Engine**: Dilengkapi proteksi `IDirect3DStateBlock9`, `VirtualQuery Kernel Memory Guard`, serta `SetCursorPos Unfreeze Guard` untuk mencegah crash saat teleport, transisi interior, atau Alt-Tab.

---

## 🎮 Cara Menggunakan di Game

| Hotkey / Perintah | Fungsi |
| :--- | :--- |
| **`F4` (Hold/Tahan)** | **Push-To-Talk Voice Input**: Tahan tombol `F4` sambil bicara di mic ➔ lepas tombol ➔ otomatis ditranskrip & diterjemahkan ➔ `CTRL+V` ke game! |
| **`Shift + Enter`** | **Unlock Cursor Mode**: Membuka kursor mouse untuk menggeser overlay, menyalin teks, atau mengatur konfigurasi. |
| **`Shift + H`** | **Toggle Hide/Show Overlay**: Menyembunyikan / menampilkan jendela overlay SA-RP Linggo di layar. |
| **`CTRL + C`** | Menyalin teks Bahasa Indonesia ➔ otomatis diterjemahkan ke Bahasa Inggris di clipboard. |

---

## 📥 Panduan Instalasi

1. Pastikan GTA SA Anda sudah terpasang **ASI Loader** (seperti `vorbisHooked.dll` / `d3d9.dll` dari Ultimate ASI Loader atau CLEO 4).
2. Unduh file `SARPLinggo.asi` dari menu **[Releases](https://github.com/MaitriProject-afk/SARP-Linggo-ASI/releases)**.
3. Salin / Paste file `SARPLinggo.asi` langsung ke **Folder Utama GTA San Andreas** Anda (sejajar dengan `gta_sa.exe`).
4. Buka file `SARPLinggo.ini` (atau buat melalui menu **Pengaturan** di overlay `Shift + Enter`) dan masukkan **Groq API Key** Anda (Dapat dibuat gratis di [console.groq.com](https://console.groq.com/keys)).
5. Jalankan game GTA SA / SA-MP seperti biasa.

---

## 🛠️ Cara Build dari Source Code (Khusus Developer)

### Persyaratan Build:
- **CMake 3.15+**
- **MinGW-w64 (32-bit / i686)** atau **LLVM Clang 32-bit**
- **Git**

### Langkah Build:
```bash
# 1. Clone repository
git clone https://github.com/MaitriProject-afk/SARP-Linggo-ASI.git
cd SARP-Linggo-ASI

# 2. Generate build folder 32-bit (x86)
cmake -B build32 -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release

# 3. Compile biner ASI
cmake --build build32 --config Release
```
File biner `.asi` hasil kompilasi akan berada di `build32/libSARPLinggo.asi` (atau `SARPLinggo.asi`).

---

## 📜 Lisensi & Attribution

Proyek ini dilisensikan di bawah **[MIT License](LICENSE)** - 100% Free & Open Source.

Dikembangkan & Dipelihara oleh **[MaitriProject](https://github.com/MaitriProject-afk)** untuk mendukung komunitas Roleplay GTA SA-MP Indonesia.
