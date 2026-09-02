#pragma once

#include <string>
#include <vector>

namespace SARPLinggo {

struct LicenseInfo {
    bool active = false;
    std::string reason;
    int remaining_days = 0;
    std::string expires_at;
    std::string token;
    std::string hwid;
};

class LicenseManager {
public:
    explicit LicenseManager(const std::string& storage_file = "SARPLinggo_license.json");

    static std::string get_local_hwid();
    static bool verify_token(const std::string& token_str, std::string& out_msg, int& out_days, std::string& out_hwid);
    
    bool activate_token(const std::string& token_str, std::string& out_msg);
    LicenseInfo get_license_info();
    bool is_active();

private:
    std::string m_storage_file;
    static std::string hmac_sha256_simple(const std::string& key, const std::string& data);
};

} // namespace SARPLinggo
