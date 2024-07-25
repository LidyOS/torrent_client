#include "torrent_tracker.h"
#include "bencode.h"
#include "byte_tools.h"
#include <cpr/cpr.h>
#include <iostream>

const std::vector<Peer> &TorrentTracker::GetPeers() const {
    return peers_;
}
void TorrentTracker::UpdatePeers(const TorrentFile& tf, std::string peerId, int port){
    cpr::Response res = cpr::Get(
        cpr::Url{tf.announce},
        cpr::Parameters {
                {"info_hash", tf.infoHash},
                {"peer_id", peerId},
                {"port", std::to_string(port)},
                {"uploaded", std::to_string(0)},
                {"downloaded", std::to_string(0)},
                {"left", std::to_string(tf.length)},
                {"compact", std::to_string(1)}
        },
        cpr::Timeout{20000}
    );
    size_t ind = 0;
    std::any result;
    try
    {
        result = Bencode::parsing(res.text, ind);
    }
    catch (const std::runtime_error& e)
    {
        std::cerr << "error in parsing file: " << e.what() << std::endl;
        throw std::runtime_error("it's impossible to get peers");
    }

    std::map<std::string, std::any> data = std::any_cast<std::map<std::string, std::any>>(result);

    std::string peers = std::any_cast<std::string>(data["peers"]);
    for (size_t i = 0; i < peers.size(); i += 6)
    {
        peers_.push_back({.ip = MakeIp(peers, i),
            .port = (((uint16_t((uint8_t) peers[i+4])) << 8) + uint16_t((uint8_t)peers[i+5]))});
    }
}


std::string TorrentTracker::MakeIp(std::string& peers, int beg){
    std::string ip;
    ip += std::to_string((uint8_t) peers[beg]);
    ip += '.';
    ip += std::to_string((uint8_t) peers[beg+1]);
    ip += '.';
    ip += std::to_string((uint8_t) peers[beg+2]);
    ip += '.';
    ip += std::to_string((uint8_t) peers[beg+3]);
    return ip;
}
