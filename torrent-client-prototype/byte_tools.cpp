#include "byte_tools.h"
#include <openssl/sha.h>
#include <vector>

int BytesToInt(std::string_view bytes) {
    uint32_t res = 0;
    for (auto &&i : bytes)
    {
        uint8_t byte = i;
        res <<= 8;
        res += static_cast<uint32_t>(byte);
    }

    return res;
}

std::string IntToBytes(int number){
    std::string result(4, 0);
    uint8_t mask = ((1 << 8) - 1);
    for (size_t i = 0; i < 4; i++)
    {
        uint8_t byte = number & mask;
        number >>= 8;
        result[3 - i] = byte;
    }
    
    return result;
}


std::string CalculateSHA1(const std::string& msg) {
    unsigned char hash[20];
    SHA1((unsigned char*)msg.c_str(), msg.size(), hash);

    std::string result;
    for (int i = 0; i < 20; i++) {

        result += hash[i];
    }
    return result;
}

std::string HexEncode(const std::string& input){
    return "huy16hex";
}
