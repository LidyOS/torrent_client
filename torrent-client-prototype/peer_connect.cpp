#include "byte_tools.h"
#include "peer_connect.h"
#include "message.h"
#include <iostream>
#include <sstream>
#include <utility>
#include <bit>

void PeerConnect::Terminate() {
    std::cerr << "Terminate peer: " << this->socket_.GetIp() << std::endl;
    
    terminated_ = true;
}

bool PeerConnect::Failed() const {
    return failed_;
}


PeerConnect::PeerConnect(const Peer& peer, const TorrentFile& tf, std::string selfPeerId,
 PieceStorage& pieceStorage) 
:tf_(tf)
, socket_(peer.ip, peer.port, std::chrono::milliseconds(500), std::chrono::milliseconds(500))
, selfPeerId_(selfPeerId)
, terminated_(false)
, choked_(true)
, pieceInProgress_(nullptr)
, pieceStorage_(pieceStorage)
, pendingBlock_(false)
{
    peerName = socket_.GetIp() + ":" + std::to_string(socket_.GetPort());
}

void PeerConnect::Run() {
    failed_ = false;
    while (!terminated_) {
        if (EstablishConnection()) {
            std::cout << "Connection established to peer " << socket_.GetIp()<< socket_.GetIp() << ":" <<
            socket_.GetPort()<< std::endl;
            try
            {
                MainLoop();
            }
            catch (const std::exception&)
            {
                if(pendingBlock_ && pieceInProgress_ != nullptr){
                    // std::cerr << "Back to queue: " << pieceInProgress_->GetIndex() << " piece"<< std::endl;
                    pieceStorage_.BackToQueue(pieceInProgress_);
                }
                throw;
            }
        } else {
            std::cerr << "Cannot establish connection to peer " << socket_.GetIp() << std::endl;
            failed_ = true;
            Terminate();
        }
    }
}

void PeerConnect::PerformHandshake() {
    socket_.EstablishConnection();
    
    //send hahdshake
    std::string toSend;
    int pstrlen = 19;
    std::string pstr = "BitTorrent protocol";
    toSend.push_back(pstrlen);
    toSend += pstr;
    toSend += std::string(8, 0);
    toSend += tf_.infoHash;
    toSend += selfPeerId_;

    socket_.SendData(toSend);

    //recieve handshake
    std::string back_handshake = socket_.ReceiveData(68);
    if(back_handshake.empty()){
        throw std::runtime_error("bad handshake back");
    }
    std::string back_hash = std::string(back_handshake.begin() + 28, back_handshake.begin() + 48);
    if(back_hash != tf_.infoHash){
        throw std::runtime_error("wrong hash");
    } else{
        peerId_ = std::string( back_handshake.begin() + 48, back_handshake.end());
        std::cerr << "Good handshake" << std::endl;
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
    std::string receive = socket_.ReceiveData();
    if(receive[0] == 20){
        std::cerr << "Received error message" << std::endl;
        receive = socket_.ReceiveData();
    }
    Message res = Message::Parse(receive);
    
   

    switch (res.id)
    {
    case MessageId::BitField:
        piecesAvailability_ = PeerPiecesAvailability(res.payload);
        break;
    case MessageId::Unchoke:

        choked_ = false;
        break;
    default:
        throw std::runtime_error("unknown message");
    }

}

void PeerConnect::SendInterested() {
    try
    {
        Message toSend = Message::Init(MessageId::Interested, "");
        socket_.SendData(toSend.ToString());
    }
    catch(const std::exception&)
    {
        std::cerr << "error in SendInteresed" << '\n';
        throw;
    }
}

void PeerConnect::RequestPiece() {
    if(pieceInProgress_ == nullptr){
        pieceInProgress_ = pieceStorage_.GetNextPieceToDownload();
        if(pieceInProgress_ == nullptr){
            return;
        }
        else{
            // std::cout << "get " << pieceInProgress_->GetIndex()<< " by " << peerName<< std::endl;
        }
    }
    

    while(!piecesAvailability_.IsPieceAvailable(pieceInProgress_->GetIndex())){


        // std::cerr << "Back to queue" << std::endl;
        pieceStorage_.BackToQueue(pieceInProgress_);
        pieceInProgress_ = pieceStorage_.GetNextPieceToDownload();
        if(pieceInProgress_ == nullptr) return;
        // std::cout << "get " << pieceInProgress_->GetIndex()<< " by " << peerName<< std::endl;
    }

    Block* cur_block = pieceInProgress_->FirstMissingBlock();
    Message message = Message::Init(MessageId::Request, IntToBytes(cur_block->piece) 
    + IntToBytes(cur_block->offset) + IntToBytes(cur_block->length));
    try
    {
        socket_.SendData(message.ToString());
        
    }   
    catch (const std::exception&)
    {
        throw std::runtime_error("failed send request");
    }
    pendingBlock_ = true;
}


void PeerConnect::MainLoop() {
    while (!terminated_) {

        Message mes_recieved;
        try
        {
            mes_recieved = Message::Parse(socket_.ReceiveData());
        }
        catch(std::runtime_error &)
        {
            
            throw;
        }

        switch (mes_recieved.id)
        {
        case MessageId::Choke :
            choked_ = true;
            break;
        case MessageId::Unchoke :

            choked_ = false;
            break;
        case MessageId::Have:

            piecesAvailability_.SetPieceAvailability(BytesToInt(mes_recieved.payload));
            break;
        case MessageId::Piece:
        {   

            uint32_t index = BytesToInt(std::string(mes_recieved.payload.begin(), mes_recieved.payload.begin() + 4));
            uint32_t begin = BytesToInt(std::string(mes_recieved.payload.begin()+4, mes_recieved.payload.begin() + 8));
            std::string block = std::string(mes_recieved.payload.begin()+8, mes_recieved.payload.end());

            if(pieceInProgress_->GetIndex() != index){
                throw std::runtime_error("wrong index of piece");
            }

            pieceInProgress_->SaveBlock(begin, block);
            pendingBlock_ = false;

            if(pieceInProgress_->AllBlocksRetrieved()){
                pieceStorage_.PieceProcessed(pieceInProgress_);
                pieceInProgress_ = nullptr;
            }
            break;
        }
        default:
            throw std::runtime_error("unknow message in mainloop");
            break;
        }

        if (!choked_ && !pendingBlock_) {
            RequestPiece();
            if(pieceInProgress_ == nullptr){
                failed_ = false;
                Terminate();
                return;
            } 
        }
    }
}
void PeerPiecesAvailability::SetPieceAvailability(size_t pieceIndex){
    if(pieceIndex >= bitfield_.size() * 8) 
        throw std::out_of_range("in SetPieceAvailability out of range");

    size_t index = pieceIndex / 8, byte = 7 - (pieceIndex % 8);

    bitfield_[index] |= (1 << byte);
}

bool PeerPiecesAvailability::IsPieceAvailable(size_t pieceIndex) const{
    if(pieceIndex >= bitfield_.size() * 8) return false;

    size_t index = pieceIndex / 8, byte = 7 - (pieceIndex % 8);

    return (bitfield_[index] & (1 << byte)) != 0;
}

size_t PeerPiecesAvailability::Size() const{

    size_t countBit = 0;
    for (auto &&i : bitfield_)
    {
        countBit += std::popcount((uint8_t)i);
    }
    return countBit;
}

PeerPiecesAvailability::PeerPiecesAvailability(std::string bitfield) : bitfield_(bitfield){}

PeerPiecesAvailability::PeerPiecesAvailability(): bitfield_(""){}
