# 🌐 SA-RP Linggo ASI v1.2

> **Advanced Real-Time AI Translation & Slang Converter Overlay for GTA SAMP Roleplay (DirectX 9 Native ASI Plugin)**

![License](https://img.shields.io/badge/License-Proprietary-blue.svg)
![Game](https://img.shields.io/badge/Game-GTA%20San%20Andreas%20%2F%20SA--MP-orange.svg)
![C++](https://img.shields.io/badge/Language-C%2B%2B17-green.svg)
![Architecture](https://img.shields.io/badge/Architecture-x86%20(32--bit)-purple.svg)

**SA-RP Linggo ASI** adalah plugin native C++ (`.asi`) untuk **GTA San Andreas / SA-MP (Jogjgamers / Roleplay Server)** yang membaca *chatlog* game secara real-time dan menampilkan hasil terjemahan Bahasa Inggris ⇄ Indonesia beserta Slang Converter langsung di atas layar game menggunakan **ImGui DirectX 9 Overlay**.

---

## ✨ Fitur Utama

- ⚡ **Inbound Live Chat Translator (IC, /me, /do, Says, Shouts, Whispers)**: Membaca percakapan lawan RP di `chatlog.txt` secara otomatis dan menampilkan terjemahan Bahasa Indonesia di Overlay Game.
- 📋 **Outbound Clipboard Auto-Translate**: Membaca teks Bahasa Indonesia di clipboard (`CTRL+C`) lalu menyalin balik hasil terjemahan Bahasa Inggris ke clipboard (`CTRL+V` di game).
- 🎮 **DirectX 9 ImGui Overlay**: Interface overlay modern yang *lightweight*, responsif, dan tidak membuat FPS game *drop*.
- 🔑 **Dynamic License & HWID Security**: Sistem penguncian token terenkripsi HMAC-SHA256 untuk komunitas Discord.
- 🔒 **Zero-Lag & Thread Safe**: Dibuat menggunakan multi-threading C++17 terpisah sehingga proses terjemahan HTTP tidak mengganggu *looping* rendering utama game.
- 🛡️ **Crash-Resistant Engine**: Dilengkapi proteksi `IDirect3DStateBlock9`, `VirtualQuery Kernel Memory Guard`, serta `SetCursorPos Unfreeze Guard` untuk mencegah crash saat teleport, transisi interior, atau Alt-Tab.

---

## 🎮 Cara Menggunakan di Game

| Hotkey / Perintah | Fungsi |
| :--- | :--- |
| **`Shift + Enter`** | **Unlock Cursor Mode**: Membuka kursor mouse untuk menggeser overlay, menyalin teks, atau mengatur konfigurasi. |
| **`F7`** | **Toggle Overlay**: Menyembunyikan / menampilkan jendela overlay SA-RP Linggo. |
| **`CTRL + C`** | Menyalin teks Bahasa Indonesia ➔ otomatis diterjemahkan ke Bahasa Inggris di clipboard. |

---

## 📥 Panduan Instalasi

1. Pastikan GTA SA Anda sudah terpasang **ASI Loader** (seperti `vorbisHooked.dll` / `d3d9.dll` dari Ultimate ASI Loader atau CLEO 4).
2. Unduh file `SARPLinggo.asi` dari menu **Releases**.
3. Salin / Paste file `SARPLinggo.asi` langsung ke **Folder Utama GTA San Andreas** Anda (sejajar dengan `gta_sa.exe`).
4. Jalankan game GTA SA / SA-MP seperti biasa.

---

## 🔑 Aktivasi Lisensi

1. Tekan tombol **`Shift + Enter`** saat berada di dalam game.
2. Pindah ke tab **Lisensi & Info**.
3. Masukkan token lisensi resmi Anda yang didapatkan dari Server Discord Komunitas.
4. Klik **Aktivasi Lisensi**.

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
File biner `.asi` hasil kompilasi akan berada di `build32/libSARPLinggo.asi`.

---

## 📄 Lisensi & Hak Cipta

© 2026 **SA-RP Linggo Team / MaitriProject**. All Rights Reserved.  
Dikembangkan khusus untuk mendukung komunitas Roleplay GTA SA-MP Indonesia (Jogjgamers Reality Project).
