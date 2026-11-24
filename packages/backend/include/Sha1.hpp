#pragma once
#include <string>
#include <cstring>

typedef unsigned int u32;
typedef unsigned char u8;
typedef unsigned long long u64;

// Compact SHA-1 for WebSocket handshake (RFC 6455)
inline std::string sha1(const std::string& input) {
    u32 h0 = 0x67452301u;
    u32 h1 = 0xEFCDAB89u;
    u32 h2 = 0x98BADCFEu;
    u32 h3 = 0x10325476u;
    u32 h4 = 0xC3D2E1F0u;

    u64 bitLen = static_cast<u64>(input.size()) * 8;
    std::string msg = input;
    msg += static_cast<char>(0x80);
    while ((msg.size() % 64) != 56) msg += static_cast<char>(0x00);
    for (int i = 7; i >= 0; --i) {
        msg += static_cast<char>((bitLen >> (i * 8)) & 0xFF);
    }

    for (std::size_t chunk = 0; chunk < msg.size(); chunk += 64) {
        u32 w[80];
        for (int i = 0; i < 16; ++i) {
            w[i] = (static_cast<u8>(msg[chunk + i * 4]) << 24) |
                   (static_cast<u8>(msg[chunk + i * 4 + 1]) << 16) |
                   (static_cast<u8>(msg[chunk + i * 4 + 2]) << 8) |
                   (static_cast<u8>(msg[chunk + i * 4 + 3]));
        }
        for (int i = 16; i < 80; ++i) {
            u32 v = w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16];
            w[i] = (v << 1) | (v >> 31);
        }

        u32 a = h0, b = h1, c = h2, d = h3, e = h4;
        for (int i = 0; i < 80; ++i) {
            u32 f, k;
            if (i < 20) { f = (b & c) | ((~b) & d); k = 0x5A827999u; }
            else if (i < 40) { f = b ^ c ^ d; k = 0x6ED9EBA1u; }
            else if (i < 60) { f = (b & c) | (b & d) | (c & d); k = 0x8F1BBCDCu; }
            else { f = b ^ c ^ d; k = 0xCA62C1D6u; }

            u32 temp = ((a << 5) | (a >> 27)) + f + e + k + w[i];
            e = d; d = c; c = (b << 30) | (b >> 2); b = a; a = temp;
        }
        h0 += a; h1 += b; h2 += c; h3 += d; h4 += e;
    }

    unsigned char digest[20];
    u32 hs[5] = { h0, h1, h2, h3, h4 };
    for (int i = 0; i < 5; ++i) {
        digest[i * 4]     = static_cast<unsigned char>((hs[i] >> 24) & 0xFF);
        digest[i * 4 + 1] = static_cast<unsigned char>((hs[i] >> 16) & 0xFF);
        digest[i * 4 + 2] = static_cast<unsigned char>((hs[i] >> 8) & 0xFF);
        digest[i * 4 + 3] = static_cast<unsigned char>(hs[i] & 0xFF);
    }
    return std::string(reinterpret_cast<char*>(digest), 20);
}

inline std::string base64Encode(const std::string& data) {
    static const char* tbl =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    int val = 0, valb = -6;
    for (std::size_t i = 0; i < data.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(data[i]);
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            out.push_back(tbl[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) out.push_back(tbl[((val << 8) >> (valb + 8)) & 0x3F]);
    while (out.size() % 4) out.push_back('=');
    return out;
}
