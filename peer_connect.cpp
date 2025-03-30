#include "byte_tools.h"
#include "peer_connect.h"
#include "message.h"
#include <iostream>
#include <sstream>
#include <utility>
#include <cassert>

using namespace std::chrono_literals;

PeerConnect::PeerConnect(const Peer& peer, const TorrentFile& tf, std::string selfPeerId)
    : tf_(tf), socket_(peer.ip, peer.port, 1000ms, 1000ms), selfPeerId_(selfPeerId), terminated_(false), choked_(true) {
}

void PeerConnect::Run() {
    while (!terminated_) {
        if (EstablishConnection()) {
            std::cout << "Connection established to peer" << std::endl;
            MainLoop();
        } else {
            std::cerr << "Cannot establish connection to peer" << std::endl;
            Terminate();
        }
    }
}


void PeerConnect::PerformHandshake() {
    socket_.EstablishConnection();
    std::string pstr = "BitTorrent protocol";
    char pstrlen = static_cast<char>(pstr.size());
    std::string reserved(8, '\0');
    std::string handshake = std::string(1, pstrlen) + pstr + reserved + tf_.infoHash + selfPeerId_;

    socket_.SendData(handshake);

    std::string response = socket_.ReceiveData(68);
    if(response.substr(1, 19) != "BitTorrent protocol" || response[0] != '\x13') {
        throw std::runtime_error("PeerConnect::PerformHandshake Handshake failed");
    }
    if (response.substr(28, 20) != tf_.infoHash) {
        throw std::runtime_error("PeerConnect::PerformHandshake Handshake failed: info_hash mismatch");
    }
}

bool PeerConnect::EstablishConnection() {
    try {
        PerformHandshake();
        ReceiveBitfield();
        SendInterested();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to establish connection with peer " << socket_.GetIp() << ":" <<
            socket_.GetPort() << " -- " << e.what() << std::endl;
        return false;
    }
}

void PeerConnect::ReceiveBitfield() {
    std::string bitfieldMessage = socket_.ReceiveData();
    Message message = Message::Parse(bitfieldMessage);
    if (message.id == MessageId::BitField) {
        piecesAvailability_ = PeerPiecesAvailability(message.payload);
    } else if (message.id == MessageId::Unchoke) {
        choked_ = false;
    }
    else {
        throw std::runtime_error("PeerConnect::EstablishConnection wrong massage");
    }
}

void PeerConnect::SendInterested() {
    Message interested = Message::Init(MessageId::Interested, "");
    socket_.SendData(interested.ToString());
}

void PeerConnect::Terminate() {
    std::cerr << "Terminate" << std::endl;
    terminated_ = true;
}

void PeerConnect::MainLoop() {
    //ToDo
    std::cout << "Dummy main loop" << std::endl;
    Terminate();
}
