#include "message.h"
#include "byte_tools.h"
#include <cassert>

std::string Message::ToString() const{
    std::string result = "";
    result += IntToBytes(messageLength + 1);
    result.push_back(static_cast<uint8_t>(id));
    result += payload;
    return result;
}

Message Message::Parse(const std::string& messageString){
    Message result;
    if(messageString.empty()){
        result.id = MessageId::KeepAlive;
        result.payload = "";
        return result;
    }
    uint8_t id = static_cast<uint8_t>(messageString[0]);
    result.id = MessageId(id);
    result.payload = std::string(messageString.begin() + 1, messageString.end());
    result.messageLength = result.payload.size();
    return result;
}

Message Message::Init(MessageId id, const std::string& payload){
    Message result;
    result.id = id;
    result.payload = payload;
    result.messageLength = payload.size();
    return result;
}