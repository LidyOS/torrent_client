#include "torrent_file.h"
#include "bencode.h"
#include <vector>
#include <fstream>
#include <variant>
#include <sstream>
#include "byte_tools.h"
#include <iostream>


TorrentFile LoadTorrentFile(const std::string& filename) {
    TorrentFile result;
    std::ifstream file(filename, std::ios::binary);

    std::ostringstream out;           
    out << file.rdbuf();
        
    file.close();
    std::string data(out.str());
    size_t ind = 0;

    std::map<std::string, std::any> dictionary;
    try
    {
        dictionary = Bencode::parsing_dictionary(data, ind);   
    }
    catch (const std::runtime_error& e)
    {
        std::cerr << "error in parsing file: " << e.what() << std::endl;
        throw std::invalid_argument("the torrent file cannot be parsed");
    }

    result.announce = std::any_cast<std::string>(dictionary["announce"]);
    result.comment = std::any_cast<std::string>(dictionary["comment"]);
    result.name = std::any_cast<std::string>(std::any_cast<std::map<std::string, std::any>>(dictionary["info"])["name"]);
    result.length = std::any_cast<int64_t>(std::any_cast<std::map<std::string, std::any>>(dictionary["info"])["length"]);
    result.pieceLength = std::any_cast<int64_t>(std::any_cast<std::map<std::string, std::any>>(dictionary["info"])["piece length"]);
    
    std::string encode_info = Bencode::ecoding(std::any_cast<std::map<std::string, std::any>>(dictionary["info"]));

    result.infoHash = CalculateSHA1(encode_info);

    std::string pieces = std::any_cast<std::string>(std::any_cast<std::map<std::string, std::any>>(dictionary["info"])["pieces"]);
    for (size_t i = 0; i < pieces.size(); i+= 20)
    {
        result.pieceHashes.emplace_back(pieces.begin() + i, pieces.begin() + i + 20);
    }
    return result;
}
