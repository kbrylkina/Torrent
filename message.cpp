#include "message.h"
#include "byte_tools.h"
#include <cstdint>
#include <string>
#include <vector>
#include <stdexcept>
#include <arpa/inet.h>
#include <cstring>

Message Message::Parse(const std::string& messageString) {
    return {static_cast<MessageId>(messageString[0]), messageString.size(), messageString.substr(1)};
}

Message Message::Init(MessageId id, const std::string& payload) {
    return {id, payload.size() + 1, payload};
}

std::string Message::ToString() const {
    std::string result = IntToBytes(messageLength);
    return result + static_cast<char>(id) + payload;
}
