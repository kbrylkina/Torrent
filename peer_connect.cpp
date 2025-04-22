#include "byte_tools.h"
#include "peer_connect.h"
#include "message.h"
#include <iostream>
#include <sstream>
#include <utility>
#include <cassert>

using namespace std::chrono_literals;

PeerConnect::PeerConnect(const Peer& peer, const TorrentFile& tf, std::string selfPeerId, PieceStorage& pieceStorage)
    : tf_(tf), socket_(peer.ip, peer.port, 1500ms, 1500ms), selfPeerId_(selfPeerId), terminated_(false), choked_(true), pieceStorage_(pieceStorage) {
}

void PeerConnect::Run() {
    while (!terminated_) {
        if (EstablishConnection()) {
            std::cout << "Connection established to peer" << std::endl;
            MainLoop();
        } else {
            std::cout << "Cannot establish connection to peer" << std::endl;
            Terminate();
            failed_ = true;
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
    std::cout<<"PeerConnect::PerformHandshake Sent handshake\n";

    std::string response = socket_.ReceiveData(68);
    if(response.substr(1, 19) != "BitTorrent protocol" || response[0] != '\x13') {
        throw std::runtime_error("PeerConnect::PerformHandshake Handshake failed1");
    }
    if (response.substr(28, 20) != tf_.infoHash) {
        throw std::runtime_error("PeerConnect::PerformHandshake Handshake failed2: info_hash mismatch");
    }
}

bool PeerConnect::EstablishConnection() {
    std::cout<<"PeerConnect::EstablishConnection\n";
    try {
        PerformHandshake();
        ReceiveBitfield();
        SendInterested();
        return true;
    } catch (const std::exception& e) {
        std::cout << "Failed to establish connection with peer " << socket_.GetIp() << ":" <<
            socket_.GetPort() << " -- " << e.what() << std::endl;
        return false;
    }
}

void PeerConnect::ReceiveBitfield() {
    std::string bitfieldMessage = socket_.ReceiveData();
    std::cout<<"PeerConnect::ReceiveBitfield Received bitfield\n";
    
    Message message = Message::Parse(bitfieldMessage);
    if (message.id == MessageId::BitField) {
        piecesAvailability_ = PeerPiecesAvailability(message.payload);
    } 
    else if (message.id == MessageId::Unchoke) {
        choked_ = false;
    }
    else {
        throw std::runtime_error("PeerConnect::EstablishConnection wrong massage");
    }
}

void PeerConnect::SendInterested() {
    std::cout<<"PeerConnect::SendInterested\n";
    Message interested = Message::Init(MessageId::Interested, "");
    socket_.SendData(interested.ToString());
}

void PeerConnect::RequestPiece() {
    if (pieceInProgress_ == nullptr) {
        pieceInProgress_ = pieceStorage_.GetNextPieceToDownload();
        if (pieceInProgress_ == nullptr) {
            std::cout << "PeerConnect::RequestPiece no pieces to request" << std::endl;
            return;
        }
    }

    Block* block = pieceInProgress_->FirstMissingBlock();
    if (block == nullptr) {
        std::cout << "PeerConnect::RequestPiece no missing blocks in this piece." << std::endl;
        pieceInProgress_ = nullptr;
        return;
    }
    
    std::cout<<"PeerConnect::RequestPiece Request piece "<<pieceInProgress_->GetIndex()<< " " << block->offset << " " << block->length << "\n";
    std::string payload = IntToBytes(pieceInProgress_->GetIndex()) + 
                          IntToBytes(block->offset) + 
                          IntToBytes(block->length);
    Message request = Message::Init(MessageId::Request, payload);
    socket_.SendData(request.ToString());
    block->status = Block::Status::Pending; 
    pendingBlock_ = true;
    std::cout<<"PeerConnect::RequestPiece Request piece "<<pieceInProgress_->GetIndex()<< " " << block->offset << " " << block->length << " end\n";
}

void PeerConnect::Terminate() {
    std::cout << "Terminate\n";
    terminated_ = true;
}

bool PeerConnect::Failed() const {
    return failed_;
}

void PeerConnect::MainLoop() {
    std::cout<<"PeerConnect::MainLoop begin\n";
    while (!terminated_) {
        if (pieceStorage_.QueueIsEmpty()) {
            std::cout << "PeerConnect::MainLoop QueueIsEmpty Terminate\n";
            Terminate();
            continue;
        }

        std::string messageData = socket_.ReceiveData();
        if (messageData.empty()) {
            std::cout << "PeerConnect::MainLoop No data received, possibly connection lost" << std::endl;
            continue;
        }

        Message message = Message::Parse(messageData);
        if(message.id == MessageId::Piece) {
            std::cout<<"PeerConnect::MainLoop Piece\n";
            size_t index = BytesToInt(message.payload.substr(0, 4));
            size_t offset = BytesToInt(message.payload.substr(4, 4));
            pieceInProgress_->SaveBlock(offset, message.payload.substr(8));
            if (pieceInProgress_->AllBlocksRetrieved()) {
                std::cout<<"PeerConnect::MainLoop AllBlocksRetrieved\n";
                if (pieceInProgress_->HashMatches()) {
                    pieceStorage_.PieceProcessed(pieceInProgress_);
                    std::cout << "PeerConnect::MainLoop Piece " << pieceInProgress_->GetIndex() << " downloaded and verified" << std::endl;
                } else {
                    std::cout << "PeerConnect::MainLoop Hash mismatch, resetting piece" << std::endl;
                    pieceInProgress_->Reset();
                }
                pieceInProgress_ = nullptr;
            }
            pendingBlock_ = false;
        }
        else if(message.id == MessageId::Unchoke) {
            std::cout<<"PeerConnect::MainLoop Unchoke\n";
            choked_ = false;
        }
        else if(message.id == MessageId::Choke) {
            std::cout<<"PeerConnect::MainLoop Choke\n";
            choked_ = true;
            Terminate();
        }
        else if(message.id == MessageId::Have) {
            std::cout<<"PeerConnect::MainLoop Have\n";
            size_t index = BytesToInt(message.payload.substr(0, 4));
            piecesAvailability_.SetPieceAvailability(index);   
        }
        else {
            std::cout << "PeerConnect::MainLoop received unexpected message type: " << static_cast<int>(message.id) << std::endl;
        }

        if (!choked_ && !pendingBlock_) {
            std::cout<<"PeerConnect::MainLoop before RequestPiece\n";
            RequestPiece();
        }
    }
    std::cout<<"PeerConnect::MainLoop end\n";
}