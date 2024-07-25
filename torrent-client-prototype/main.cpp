#include "torrent_tracker.h"
#include "piece_storage.h"
#include "peer_connect.h"
#include "byte_tools.h"
#include <cassert>
#include <iostream>
#include <filesystem>
#include <random>
#include <thread>
#include <algorithm>
#include <map>

namespace fs = std::filesystem;

std::mutex cerrMutex, coutMutex;



const std::string PeerId = "TESTAPPDONTWORRYMYID";
size_t PiecesToDownload;

bool RunDownloadMultithread(PieceStorage& pieces, const TorrentFile& torrentFile, const std::string& ourId, const TorrentTracker& tracker) {
    using namespace std::chrono_literals;

    std::vector<std::thread> peerThreads;
    std::vector<std::shared_ptr<PeerConnect>> peerConnections;

    for (const Peer& peer : tracker.GetPeers()) {
        peerConnections.emplace_back(std::make_shared<PeerConnect>(peer, torrentFile, ourId, pieces));
    }

    peerThreads.reserve(peerConnections.size());

    for (auto& peerConnectPtr : peerConnections) {
        peerThreads.emplace_back(
                [peerConnectPtr] () {
                    bool tryAgain = true;
                    int attempts = 0;
                    do {
                        try {
                            ++attempts;
                            peerConnectPtr->Run();
                        } catch (const std::runtime_error& e) {
                            std::lock_guard<std::mutex> cerrLock(cerrMutex);
                            std::cerr << "Runtime error: " << e.what() << std::endl;
                        } catch (const std::exception& e) {
                            std::lock_guard<std::mutex> cerrLock(cerrMutex);
                            std::cerr << "Exception: " << e.what() << std::endl;
                        } catch (...) {
                            std::lock_guard<std::mutex> cerrLock(cerrMutex);
                            std::cerr << "Unknown error" << std::endl;
                        }
                        tryAgain = peerConnectPtr->Failed() && attempts < 10;
                        std::this_thread::sleep_for(1s);
                    } while (tryAgain);
                }
        );
    }

    {
        std::lock_guard<std::mutex> coutLock(coutMutex);
        std::cout << "Started " << peerThreads.size() << " threads for peers" << std::endl;
    }

    std::this_thread::sleep_for(10s);
    while (pieces.PiecesSavedToDiscCount() < PiecesToDownload) {
        std::lock_guard<std::mutex> coutLock(coutMutex);
        std::cout << std::setprecision(2) << (double) pieces.PiecesSavedToDiscCount() / (double) PiecesToDownload * 100  << "% download..."
         << std::endl;
        if (pieces.PiecesInProgressCount() == 0) {
            {
                std::lock_guard<std::mutex> coutLock(coutMutex);
                std::cout
                        << "Want to download more pieces but all peer connections are not working. Let's request new peers"
                        << std::endl;
            }

            for (auto& peerConnectPtr : peerConnections) {
                peerConnectPtr->Terminate();
            }
            for (std::thread& thread : peerThreads) {
                thread.join();
            }
            return true;
        }
        std::this_thread::sleep_for(1s);
    }

    {
        std::lock_guard<std::mutex> coutLock(coutMutex);
        std::cout << "Terminating all peer connections" << std::endl;
    }
    for (auto& peerConnectPtr : peerConnections) {
        peerConnectPtr->Terminate();
    }

    for (std::thread& thread : peerThreads) {
        thread.join();
    }

    return false;
}

void StartDownload(const fs::path& file, int needPercent, fs::path &outputDirectory) {
    TorrentFile torrentFile;
    
    try {
        torrentFile = LoadTorrentFile(file);
        std::cout << "Loaded torrent file " << file << ". " << torrentFile.comment << std::endl;
    } catch (const std::invalid_argument& e) {
        std::cerr << e.what() << std::endl;
        return;
    }


    PiecesToDownload = torrentFile.pieceHashes.size() * needPercent / 100;
    std::cout << "piece to dowland " << PiecesToDownload << std::endl;
    PieceStorage pieces(torrentFile, outputDirectory, PiecesToDownload);

    std::cout << "Connecting to tracker " << torrentFile.announce << std::endl;
    TorrentTracker tracker(torrentFile.announce);
    bool requestMorePeers = false;
    do {
        try
        {
            tracker.UpdatePeers(torrentFile, PeerId, 12345);
            
        }
        catch (const std::runtime_error& e)
        {
            std::cerr << "bad connection: " << e.what() << std::endl;
            return;
        }

        if (tracker.GetPeers().empty()) {
            std::cerr << "No peers found. Cannot download a file" << std::endl;
        }

        std::cout << "Found " << tracker.GetPeers().size() << " peers" << std::endl;
        for (const Peer& peer : tracker.GetPeers()) {
            std::cout << "Found peer " << peer.ip << ":" << peer.port << std::endl;
        }

        requestMorePeers = RunDownloadMultithread(pieces, torrentFile, PeerId, tracker);
    } while (requestMorePeers);

}

int main(int argc, char const *argv[])
{
    std::map<std::string, std::string> attributes;
    fs::path torrentDirectory;
    for (size_t i = 1; i < argc; i++)
    {
        if(std::string(argv[i]) == "-p" || std::string(argv[i]) == "-d"){
            attributes[std::string(argv[i])] = std::string(argv[i + 1]);
            ++i;
        }
        torrentDirectory = std::string(argv[i]);
    }

    std::cout << "dir: " << attributes["-d"] << std::endl;
    std::cout << "torrent file path: " << torrentDirectory << std::endl;

    fs::path outputDirectory = attributes["-d"];
    std::filesystem::create_directories(outputDirectory);
    StartDownload(torrentDirectory, std::stoi(attributes["-p"]), outputDirectory);
    
    return 0;
}

