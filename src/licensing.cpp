#include "../include/licensing.h"
#include "../include/obfuscation.h"
#include <windows.h>
#include <bcrypt.h>
#include <iphlpapi.h>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <algorithm>
#include <ctime>
#include <cctype>

#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "iphlpapi.lib")

namespace SARPLinggo {

static std::string get_secret_key() {
    return XOR_STR("SARP_LINGGO_SECRET_HMAC_KEY_2026");
}

// Simple SHA-256 helper using Win32 BCrypt API
std::string LicenseManager::hmac_sha256_simple(const std::string& key, const std::string& data) {
    BCRYPT_ALG_HANDLE hAlg = NULL;
    BCRYPT_HASH_HANDLE hHash = NULL;
    NTSTATUS status = 0;
    DWORD cbHash = 0, cbHashObject = 0;
    PBYTE pbHashObject = NULL;
    PBYTE pbHash = NULL;
    std::string result = "";

    if (BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, NULL, BCRYPT_ALG_HANDLE_HMAC_FLAG) >= 0) {
        if (BCryptGetProperty(hAlg, BCRYPT_OBJECT_LENGTH, (PBYTE)&cbHashObject, sizeof(DWORD), &cbHash, 0) >= 0) {
            pbHashObject = (PBYTE)HeapAlloc(GetProcessHeap(), 0, cbHashObject);
            if (BCryptGetProperty(hAlg, BCRYPT_HASH_LENGTH, (PBYTE)&cbHash, sizeof(DWORD), &cbHash, 0) >= 0) {
                pbHash = (PBYTE)HeapAlloc(GetProcessHeap(), 0, cbHash);
                if (BCryptCreateHash(hAlg, &hHash, pbHashObject, cbHashObject, (PUCHAR)key.c_str(), (ULONG)key.length(), 0) >= 0) {
                    BCryptHashData(hHash, (PUCHAR)data.c_str(), (ULONG)data.length(), 0);
                    BCryptFinishHash(hHash, pbHash, cbHash, 0);

                    std::stringstream ss;
                    for (DWORD i = 0; i < cbHash; ++i) {
                        ss << std::hex << std::setw(2) << std::setfill('0') << (int)pbHash[i];
                    }
                    result = ss.str();
                    std::transform(result.begin(), result.end(), result.begin(), ::toupper);
                    BCryptDestroyHash(hHash);
                }
            }
        }
        if (pbHashObject) HeapFree(GetProcessHeap(), 0, pbHashObject);
        if (pbHash) HeapFree(GetProcessHeap(), 0, pbHash);
        BCryptCloseAlgorithmProvider(hAlg, 0);
    }
    return result;
}

std::string LicenseManager::get_local_hwid() {
    std::string machine_guid = "";
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Cryptography", 0, KEY_READ | KEY_WOW64_64KEY, &hKey) == ERROR_SUCCESS) {
        char buffer[256] = {0};
        DWORD dwSize = sizeof(buffer);
        if (RegQueryValueExA(hKey, "MachineGuid", NULL, NULL, (LPBYTE)buffer, &dwSize) == ERROR_SUCCESS) {
            machine_guid = buffer;
        }
        RegCloseKey(hKey);
    }

    char computerName[MAX_COMPUTERNAME_LENGTH + 1] = {0};
    DWORD compSize = sizeof(computerName);
    GetComputerNameA(computerName, &compSize);

    std::string raw_info = machine_guid + "-" + std::string(computerName);
    
    // Quick Hash into 4 hex chars
    unsigned long hash = 5381;
    for (char c : raw_info) {
        hash = ((hash << 5) + hash) + c;
    }

    std::stringstream ss;
    ss << std::hex << std::setw(4) << std::setfill('0') << (hash & 0xFFFF);
    std::string res = ss.str();
    std::transform(res.begin(), res.end(), res.begin(), ::toupper);
    return res;
}

LicenseManager::LicenseManager(const std::string& storage_file) : m_storage_file(storage_file) {}

bool LicenseManager::verify_token(const std::string& token_str, std::string& out_msg, int& out_days, std::string& out_hwid) {
    out_days = 0;
    out_hwid = "";

    std::string clean = token_str;
    clean.erase(0, clean.find_first_not_of(" \t\r\n"));
    clean.erase(clean.find_last_not_of(" \t\r\n") + 1);
    std::transform(clean.begin(), clean.end(), clean.begin(), ::toupper);

    std::vector<std::string> parts;
    std::stringstream ss(clean);
    std::string item;
    while (std::getline(ss, item, '-')) {
        if (!item.empty()) parts.push_back(item);
    }

    if (parts.size() != 5 || parts[0] != "SARP") {
        out_msg = "Format token tidak valid! Format: SARP-XXXX-HWID-P1-P2";
        return false;
    }

    std::string days_hex = parts[1];
    std::string hwid_part = parts[2];
    std::string part1 = parts[3];
    std::string part2 = parts[4];

    try {
        out_days = (int)std::strtol(days_hex.c_str(), NULL, 16);
    } catch (...) {
        out_msg = "Format jumlah hari token tidak valid!";
        return false;
    }

    std::string payload = days_hex + "-" + hwid_part;
    std::string sig = hmac_sha256_simple(get_secret_key(), payload);

    if (sig.length() < 8 || sig.substr(0, 4) != part1 || sig.substr(4, 4) != part2) {
        out_msg = "Tanda tangan digital token tidak valid (Token palsu / diubah)!";
        return false;
    }

    std::string local_hwid = get_local_hwid();
    out_hwid = hwid_part;

    if (hwid_part != "GLOB" && hwid_part != local_hwid) {
        out_msg = "Token ini dikunci untuk HWID '" + hwid_part + "', tetapi HWID perangkat ini adalah '" + local_hwid + "'!";
        return false;
    }

    out_msg = "Token lisensi valid!";
    return true;
}

bool LicenseManager::activate_token(const std::string& token_str, std::string& out_msg) {
    int days = 0;
    std::string hwid_part;
    if (!verify_token(token_str, out_msg, days, hwid_part)) {
        return false;
    }

    std::time_t now = std::time(nullptr);
    std::time_t expires_at = now + (days * 86400);

    std::ofstream f(m_storage_file, std::ios::trunc);
    if (!f.is_open()) {
        out_msg = "Gagal menyimpan file lisensi!";
        return false;
    }

    f << "{\n";
    f << "  \"token\": \"" << token_str << "\",\n";
    f << "  \"hwid\": \"" << hwid_part << "\",\n";
    f << "  \"days\": " << days << ",\n";
    f << "  \"activated_at\": " << now << ",\n";
    f << "  \"expires_at\": " << expires_at << "\n";
    f << "}\n";

    out_msg = "Lisensi berhasil diaktifkan! Sisa durasi: " + std::to_string(days) + " Hari.";
    return true;
}

LicenseInfo LicenseManager::get_license_info() {
    LicenseInfo info;
    info.active = false;
    info.reason = "Tidak ada file lisensi terdeteksi";

    std::ifstream f(m_storage_file);
    if (!f.is_open()) return info;

    std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    
    // Parse simple JSON fields
    auto get_str = [&](const std::string& key) -> std::string {
        size_t pos = content.find("\"" + key + "\"");
        if (pos == std::string::npos) return "";
        size_t colon = content.find(":", pos);
        if (colon == std::string::npos) return "";
        size_t q1 = content.find("\"", colon);
        if (q1 == std::string::npos) return "";
        size_t q2 = content.find("\"", q1 + 1);
        if (q2 == std::string::npos) return "";
        return content.substr(q1 + 1, q2 - q1 - 1);
    };

    auto get_long = [&](const std::string& key) -> long long {
        size_t pos = content.find("\"" + key + "\"");
        if (pos == std::string::npos) return 0;
        size_t colon = content.find(":", pos);
        if (colon == std::string::npos) return 0;
        return std::atoll(content.c_str() + colon + 1);
    };

    info.token = get_str("token");
    info.hwid = get_str("hwid");
    long long expires_at = get_long("expires_at");
    long long now = std::time(nullptr);

    if (expires_at <= 0 || now > expires_at) {
        info.reason = "Lisensi telah kadaluarsa!";
        return info;
    }

    std::string local_hwid = get_local_hwid();
    if (!info.hwid.empty() && info.hwid != "GLOB" && info.hwid != local_hwid) {
        info.reason = "HWID tidak cocok (Dikunci untuk " + info.hwid + ")";
        return info;
    }

    info.active = true;
    info.remaining_days = static_cast<int>((expires_at - now) / 86400);
    info.reason = "Lisensi Aktif";

    char buf[64];
    std::tm* tm_info = std::localtime((const time_t*)&expires_at);
    if (tm_info) {
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm_info);
        info.expires_at = buf;
    }

    return info;
}

bool LicenseManager::is_active() {
    return get_license_info().active;
}

} // namespace SARPLinggo
