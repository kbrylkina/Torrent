#include "byte_tools.h"
#include <openssl/sha.h>
#include <vector>

int BytesToInt(std::string_view bytes) {
    int result = 0;
    result |= (static_cast<int>(static_cast<unsigned char>(bytes[0])) << 24)
    | (static_cast<int>(static_cast<unsigned char>(bytes[1])) << 16)
    | (static_cast<int>(static_cast<unsigned char>(bytes[2])) << 8) 
    | (static_cast<int>(static_cast<unsigned char>(bytes[3])));

    return result;
}

std::string CalculateSHA1(const std::string& msg) {
    unsigned char hashes[20];
    SHA1(reinterpret_cast<const unsigned char*>(msg.c_str()), msg.size(), hashes);
    std::string str(reinterpret_cast<char*>(hashes), 20);
    return str;
}
