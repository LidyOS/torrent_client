#include "byte_tools.h"
#include "piece.h"
#include <iostream>
#include <algorithm>

namespace {
constexpr size_t BLOCK_SIZE = 1 << 14;
    
}

Piece::Piece(size_t index, size_t length, std::string hash)
: index_(index), length_(length), hash_(hash)
{
    for (size_t offset = 0; offset < length_; offset += BLOCK_SIZE)
    {
        blocks_.emplace_back();
        blocks_.back().offset = offset;
        blocks_.back().piece = index;
        blocks_.back().length = std::min(BLOCK_SIZE, length - offset);
        blocks_.back().status = Block::Status::Missing;
        blocks_.back().data = "";
    }
}

bool Piece::HashMatches() const{
    return hash_ == GetDataHash();
}

Block* Piece::FirstMissingBlock(){
    for (auto &&i : blocks_)
    {
        if(i.status == Block::Status::Missing){
            return &i;
        }
    }
    return nullptr;
}

size_t Piece::GetIndex() const{
    return index_;
}

void Piece::SaveBlock(size_t blockOffset, std::string data){
    blocks_[blockOffset / BLOCK_SIZE].data = data;
    blocks_[blockOffset / BLOCK_SIZE].status = Block::Status::Retrieved;
}

bool Piece::AllBlocksRetrieved() const{
    for (auto &&i : blocks_)
    {
        if(i.status != Block::Status::Retrieved){
            return false;
        }
    }
    return true;
}

std::string Piece::GetData() const{
    std::string result;

    for (auto &&i : blocks_)
    {
        result += i.data;
    }
    return result;
}

std::string Piece::GetDataHash() const{
    std::string data = GetData();
    return CalculateSHA1(data);
}

const std::string& Piece::GetHash() const{
    return hash_;
}

void Piece::Reset(){
    for (auto &&i : blocks_)
    {
        i.status = Block::Status::Missing;
        i.data.clear();
    }
    
}