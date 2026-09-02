#pragma once

#include <string>
#include <array>

namespace SARPLinggo {

template <size_t N, char K = 0x5A>
class XorString {
private:
    std::array<char, N> m_data;

    constexpr char encrypt_char(char c, size_t idx) const {
        return c ^ static_cast<char>(K + (idx * 7));
    }

public:
    template <size_t... Is>
    constexpr XorString(const char(&str)[N], std::index_sequence<Is...>)
        : m_data{ encrypt_char(str[Is], Is)... } {}

    constexpr XorString(const char(&str)[N])
        : XorString(str, std::make_index_sequence<N>{}) {}

    std::string decrypt() const {
        std::string result;
        result.reserve(N);
        for (size_t i = 0; i < N - 1; ++i) {
            result.push_back(m_data[i] ^ static_cast<char>(K + (i * 7)));
        }
        return result;
    }
};

} // namespace SARPLinggo

#define XOR_STR(str) (SARPLinggo::XorString<sizeof(str)>(str).decrypt())
