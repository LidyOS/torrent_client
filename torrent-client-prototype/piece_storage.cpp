#include "piece_storage.h"
#include <iostream>


PieceStorage::PieceStorage(const TorrentFile& tf, const std::filesystem::path& outputDirectory, size_t needDowland)
: file_(outputDirectory/tf.name, std::ios_base::binary ), pieceLength_(tf.pieceLength), piecesCount_(needDowland)
{
    for (size_t i = 0; i < std::min(tf.pieceHashes.size() - 1, needDowland); i++)
    {
        remainPieces_.push(std::make_shared<Piece>(i, tf.pieceLength, tf.pieceHashes[i]));
    }
    if(needDowland == tf.pieceHashes.size()){
        remainPieces_.push(std::make_shared<Piece>(tf.pieceHashes.size() - 1, 
        tf.length % tf.pieceLength, tf.pieceHashes[tf.pieceHashes.size() - 1]));   
    }
    size_t ourLenth = needDowland == tf.pieceHashes.size() ? tf.length : needDowland * tf.pieceLength;
    file_.seekp(ourLenth - 1);
    file_ << '\0';
}

PiecePtr PieceStorage::GetNextPieceToDownload() {
    std::unique_lock lock(mutex_);

    if(remainPieces_.empty()) return nullptr;
    PiecePtr nextPiece = remainPieces_.front();
    remainPieces_.pop();
    return nextPiece;
}

void PieceStorage::PieceProcessed(const PiecePtr& piece) {
    std::unique_lock lock(mutex_);
    if(!piece->HashMatches()){
        throw std::runtime_error("hashes don't match");
    }
    SavePieceToDisk(piece);    
}

bool PieceStorage::QueueIsEmpty() const {
    std::shared_lock lock(mutex_);
    return remainPieces_.empty();
}

size_t PieceStorage::TotalPiecesCount() const {
    return piecesCount_;
}
void PieceStorage::SavePieceToDisk(const PiecePtr& piece) {
    // std::cout << "Downloaded piece " << piece->GetIndex() << std::endl;
    savedToDisk_.push_back(piece->GetIndex());
    file_.seekp(piece->GetIndex() * pieceLength_);
    file_ << piece->GetData();
    // std::cout << "total saved: "<< savedToDisk_.size()<< " pieces" << std::endl;
}
void PieceStorage::BackToQueue(const PiecePtr& piece){
    std::unique_lock lock(mutex_);
    remainPieces_.push(piece);
}


size_t PieceStorage::PiecesSavedToDiscCount() const{
    std::shared_lock lock(mutex_);
    return savedToDisk_.size();
}


void PieceStorage::CloseOutputFile(){
    std::unique_lock lock(mutex_);
    if(file_.is_open()){
        // file_.seekp(std::ios_base::end);
        // file_ << '\0';
        file_.close();
    }
}

const std::vector<size_t>& PieceStorage::GetPiecesSavedToDiscIndices() const{
    return savedToDisk_;
}

size_t PieceStorage::PiecesInProgressCount() const{
    std::shared_lock lock(mutex_);
    // std::cout << "remain: " << remainPieces_.size() << std::endl;
    return piecesCount_ - remainPieces_.size() - savedToDisk_.size();
}
