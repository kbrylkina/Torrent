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

/*
* Формируем строку с сообщением, которую можно будет послать пиру в соответствии с протоколом.
* Получается строка вида "<1 + payload length><message id><payload>"
* Секция с длиной сообщения занимает 4 байта и представляет собой целое число в формате big-endian
* id сообщения занимает 1 байт и может принимать значения от 0 до 9 включительно
*/
std::string Message::ToString() const {
    std::string result;
    result.resize(4);
    size_t len = messageLength;

    for (int i = 3; i >= 0; i--) {
        result[i] = static_cast<char>(len & 255);
        len >>= 8;
    }
    return result + static_cast<char>(id) + payload;
}
