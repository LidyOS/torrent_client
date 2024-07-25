#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <variant>
#include <list>
#include <map>
#include <sstream>
#include <any>

namespace Bencode {

    struct TorrentFile {
        std::string announce;
        std::string comment;
        std::vector<std::string> pieceHashes;
        size_t pieceLength;
        size_t length;
        std::string name;
        std::string infoHash;
    };
    std::any parsing(std::string& file, size_t &cur_index);
    void check_index(std::string& file, size_t& cur_index);
    int64_t parsing_integer(std::string& file, size_t& cur_index, const char end);
    std::string parsing_string(std::string& file, size_t& cur_index);
    std::vector<std::any> parsing_list(std::string& file, size_t& cur_index);
    std::map<std::string, std::any> parsing_dictionary(std::string& file, size_t& cur_index);
    std::any parsing(std::string& file, size_t& cur_index);
    std::string ecoding(std::any object);
}
