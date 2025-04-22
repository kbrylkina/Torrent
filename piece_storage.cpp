#include "piece_storage.h"
#include <iostream>

PieceStorage::PieceStorage(const TorrentFile& tf, const std::filesystem::path& outputDirectory)
    : outputFile_(outputDirectory / tf.name, std::ios::binary | std::ios::out), piecesInProgress_(0) { 
    if (!outputFile_) {
        std::cout<<"PieceStorage::PieceStorage Failed to open output file for writing\n";
        throw std::runtime_error("PieceStorage::PieceStorage Failed to open output file for writing.");
    }
    outputFile_.seekp(tf.length - 1);
    outputFile_.write("\0", 1);
    for (size_t i = 0; i < tf.pieceHashes.size(); ++i) {
        size_t piece_len = (i < tf.pieceHashes.size() - 1) ? tf.pieceLength : (tf.length % tf.pieceLength);
        remainPieces_.push(std::make_shared<Piece>(i, piece_len, tf.pieceHashes[i]));
    }
}

PiecePtr PieceStorage::GetNextPieceToDownload() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!remainPieces_.empty()) {
        PiecePtr nextPiece = remainPieces_.front();
        ++piecesInProgress_;
        remainPieces_.pop();
        return nextPiece;
    }
    return nullptr;
}

void PieceStorage::PieceProcessed(const PiecePtr& piece) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (piece->HashMatches()) {
        SavePieceToDisk(piece);
        savedPiecesIndices_.push_back(piece->GetIndex());
        --piecesInProgress_;
    } else {
        std::cout << "PieceStorage::PieceProcessed hash mismatch for piece index " << piece->GetIndex() << std::endl;
    }
}

bool PieceStorage::QueueIsEmpty() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return remainPieces_.empty();
}

size_t PieceStorage::PiecesSavedToDiscCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return savedPiecesIndices_.size();
}

size_t PieceStorage::TotalPiecesCount() const {
    return savedPiecesIndices_.size() + remainPieces_.size() + piecesInProgress_;
}

void PieceStorage::SavePieceToDisk(const PiecePtr& piece) {
    size_t offset = piece->GetIndex() * piece->GetSize();
    std::cout << "PieceStorage::SavePieceToDisk Writing piece to disk: Index " << piece->GetIndex() << ", Offset " << offset << ", Size " << piece->GetSize() << std::endl;
    outputFile_.seekp(offset);
    outputFile_.write(reinterpret_cast<const char*>(piece->GetData().data()), piece->GetSize());
    if (!outputFile_) {
        std::cout << "PieceStorage::SavePieceToDisk Failed to write to disk for piece index " << piece->GetIndex() << std::endl;
    }
    std::cout << "PieceStorage::SavePieceToDisk Save is ok\n";
}

void PieceStorage::CloseOutputFile() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!outputFile_) {
        std::cout<<"PieceStorage::CloseOutputFile Failed to close the output file properly\n";
        throw std::runtime_error("PieceStorage::CloseOutputFile Failed to close the output file properly.");
    }
    outputFile_.close();
}

const std::vector<size_t>& PieceStorage::GetPiecesSavedToDiscIndices() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return savedPiecesIndices_;
}

size_t PieceStorage::PiecesInProgressCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return piecesInProgress_;
}