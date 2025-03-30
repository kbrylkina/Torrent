#include "tcp_connect.h"
#include "byte_tools.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdexcept>
#include <cstring>
#include <iostream>
#include <chrono>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/poll.h>
#include <limits>
#include <utility>

TcpConnect::TcpConnect(std::string ip, int port, std::chrono::milliseconds connectTimeout, std::chrono::milliseconds readTimeout)
    : ip_(ip), port_(port), connectTimeout_(connectTimeout), readTimeout_(readTimeout), sock_(-1) {
        sock_ = socket(AF_INET, SOCK_STREAM, 0);
}


TcpConnect::~TcpConnect() {
    CloseConnection();
}

void TcpConnect::EstablishConnection() {
    std::cout<<"TcpConnect::EstablishConnection start\n";
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    addr.sin_addr.s_addr = inet_addr(ip_.c_str());

    int flags = fcntl(sock_, F_GETFL, 0);
    fcntl(sock_, F_SETFL, flags | O_NONBLOCK);

    int result = connect(sock_, (struct sockaddr *)&addr, sizeof(addr));
    if (result < 0) {
        struct pollfd pol;
        pol.fd = sock_;
        pol.events = POLLOUT;
        if (poll(&pol, 1, connectTimeout_.count()) <= 0) {
            throw std::runtime_error("TcpConnect::EstablishConnection Timeout");
        }
    }

    flags = fcntl(sock_, F_GETFL, 0);
    fcntl(sock_, F_SETFL, flags & ~O_NONBLOCK);
    std::cout<<"TcpConnect::EstablishConnection completed\n";
}

void TcpConnect::SendData(const std::string& data) const {
    send(sock_, data.c_str(), data.size(), 0);
}

std::string TcpConnect::ReceiveData(size_t bufferSize) const {
    std::cout<<"TcpConnect::ReceiveData start\n";
    std::string data;
    size_t toRead = bufferSize;
    if (bufferSize == 0) {
        uint32_t length;
        recv(sock_, &length, 4, MSG_WAITALL);
        std::string_view string_v(reinterpret_cast<char*>(&length), sizeof(length));
        toRead = BytesToInt(string_v);  
    }

    data.resize(toRead);
    recv(sock_, &data[0], toRead, MSG_WAITALL);
    std::cout<<"TcpConnect::ReceiveData completed\n";
    return data;
}

void TcpConnect::CloseConnection() {
    if (sock_ != -1) {
        close(sock_);
        sock_ = -1;
    }
}

const std::string &TcpConnect::GetIp() const {
    return ip_;
}

int TcpConnect::GetPort() const {
    return port_;
}
