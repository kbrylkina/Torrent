#include "byte_tools.h"
#include <openssl/sha.h>
#include <vector>
#include <sstream>
#include <iomanip>

int BytesToInt(std::string_view bytes) {
    int result = 0;
    result |= (static_cast<int>(static_cast<unsigned char>(bytes[0])) << 24)
    | (static_cast<int>(static_cast<unsigned char>(bytes[1])) << 16)
    | (static_cast<int>(static_cast<unsigned char>(bytes[2])) << 8) 
    | (static_cast<int>(static_cast<unsigned char>(bytes[3])));

    return result;
}

std::string IntToBytes(int64_t ints) {
    std::string result;
    result.resize(4);
    size_t len = ints;

    for (int i = 3; i >= 0; i--) {
        result[i] = static_cast<char>(len & 255);
        len >>= 8;
    }
    return result;
}

std::string CalculateSHA1(const std::string& msg) {
    unsigned char hashes[20];
    SHA1(reinterpret_cast<const unsigned char*>(msg.c_str()), msg.size(), hashes);
    std::string str(reinterpret_cast<char*>(hashes), 20);
    return str;
}

std::string HexEncode(const std::string& input) {
    std::ostringstream hexStream;
    hexStream << std::hex << std::setfill('0');
    for (unsigned char c : input) {
        hexStream << std::setw(2) << static_cast<int>(c) << ' ';
    }
    return hexStream.str();
}