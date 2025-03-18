#include "byte_tools.h"

int BytesToInt(std::string_view bytes) {
    int result = 0;
    result |= (static_cast<int>(static_cast<unsigned char>(bytes[0])) << 24)
    | (static_cast<int>(static_cast<unsigned char>(bytes[1])) << 16)
    | (static_cast<int>(static_cast<unsigned char>(bytes[2])) << 8) 
    | (static_cast<int>(static_cast<unsigned char>(bytes[3])));

    return result;
}