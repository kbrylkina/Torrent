#include "byte_tools.h"
#include "piece.h"
#include <iostream>
#include <algorithm>

namespace {
constexpr size_t BLOCK_SIZE = 1 << 14;
}

Piece::Piece(size_t index, size_t length, std::string hash)
    : index_(index), length_(length), hash_(hash) {
    size_t blocks_size = (length + BLOCK_SIZE - 1) / BLOCK_SIZE;
    for (size_t i = 0; i < blocks_size; ++i) {
        blocks_.emplace_back(index, i * BLOCK_SIZE, std::min(BLOCK_SIZE, length - i * BLOCK_SIZE), Block::Status::Missing);
    }
    std::cout <<"blocks_size " << blocks_.size() << '\n';
}

bool Piece::HashMatches() const {
    return CalculateSHA1(GetData()) == hash_;
}

Block* Piece::FirstMissingBlock() {
    auto it = std::find_if(blocks_.begin(), blocks_.end(), [](const Block& block) {
        return block.status == Block::Status::Missing;
    });
    if (it != blocks_.end()){
        std::cout << "Piece::FirstMissingBlock found block with offset " << it->offset << '\n';
    }
    else {
        std::cout << "Piece::FirstMissingBlock NOT FOUND BLOCK" << '\n';
    }
    return it != blocks_.end() ? &(*it) : nullptr;
}

size_t Piece::GetIndex() const {
    return index_;
}

void Piece::SaveBlock(size_t blockOffset, std::string data) {
    for (auto& block : blocks_) {
        if (block.offset == blockOffset && block.status != Block::Status::Retrieved) {
            std::cout<<"Piece::SaveBlock Saved block.offset " << block.offset<<"\n";
            block.data = data;
            block.status = Block::Status::Retrieved;
            break;
        }
    }
    // if (blocks_[blockOffset / BLOCK_SIZE].status != Block::Status::Pending) {
    //     std::cerr << "Piece::SaveBlock Block is not pending\n";
    //     throw std::runtime_error("Block is not pending");
    // }
    // std::cout<<"Piece::SaveBlock Saved block.offset " << blocks_[blockOffset / BLOCK_SIZE].offset <<"\n";
    // blocks_[blockOffset / BLOCK_SIZE].data = data;
    // blocks_[blockOffset / BLOCK_SIZE].status = Block::Status::Retrieved;
}
    
bool Piece::AllBlocksRetrieved() const {
    int cnt = 0;
    for (auto& block : blocks_) {
        cnt++;
        if(block.status != Block::Status::Retrieved) {
            std::cout<<"Piece::AllBlocksRetrieved false "<<cnt<<"\n";
            return false;
        }
    }
    std::cout<<"Piece::AllBlocksRetrieved true\n";
    return true;
}

std::string Piece::GetData() const {
    std::string data;
    for (const auto& block : blocks_) {
        if (block.status == Block::Retrieved) {
            data += block.data;
        }
    }
    return data;
}

std::string Piece::GetDataHash() const {
    return CalculateSHA1(GetData());
}

const std::string& Piece::GetHash() const {
    return hash_;
}

size_t Piece::GetSize() const {
    return length_;
}

void Piece::Reset() {
    for (auto& block : blocks_) {
        block.status = Block::Status::Missing;
        block.data.clear();
    }
}