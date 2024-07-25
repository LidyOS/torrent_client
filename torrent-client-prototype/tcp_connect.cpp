#include "tcp_connect.h"
#include "byte_tools.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdexcept>
#include <cstring>
#include <iostream>
#include <chrono>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/poll.h>
#include <limits>
#include <utility>

const std::string &TcpConnect::GetIp() const {
    return ip_;
}

int TcpConnect::GetPort() const {
    return port_;
}

TcpConnect::TcpConnect(const std::string ip, int port, std::chrono::milliseconds connectTimeout, std::chrono::milliseconds readTimeout) 
    : ip_(ip)
    , port_(port)
    , connectTimeout_(connectTimeout)
    , readTimeout_(readTimeout){
        //ToDo soket creat
    }

void TcpConnect::CloseConnection(){
    been_closed = true;
    if (close(sock_) != 0){
        perror("close");
    }
}

TcpConnect::~TcpConnect(){
    if(!been_closed){
        if (close(sock_) != 0){
            perror("close");
        }
    }
}

void TcpConnect::EstablishConnection(){
    sock_ = socket(PF_INET, SOCK_STREAM, 0);
    if (sock_ == -1) {
        perror("Socket");
        throw std::runtime_error("Wrong Soket");
    }
    been_closed = false;
    // std::cerr << "Socket is created in EstablishConnect tcp_connect" << std::endl;


    int flags = fcntl(sock_,F_GETFL,0);
    if(fcntl(sock_,F_SETFL, flags | O_NONBLOCK) < 0){
        perror("fcntl");
        throw std::runtime_error("fcntl");
    }
    struct sockaddr_in address_to_connect;

    memset(&address_to_connect, 0, sizeof(struct sockaddr_in));

    address_to_connect.sin_family = PF_INET;
    address_to_connect.sin_addr.s_addr = inet_addr(ip_.c_str());
    address_to_connect.sin_port = htons(port_);

    if(connect(sock_, (struct sockaddr *) &address_to_connect, sizeof(address_to_connect)) == -1){
        if(errno != EINPROGRESS){
            perror("connect");
            throw std::runtime_error("connect");
            return;
        }
    }
    // std::cerr << "try to connect in EstablishConnect tcp_connect" << std::endl;

    struct timeval timing;
    timing.tv_sec = connectTimeout_.count() / 1000;
    timing.tv_usec = (connectTimeout_.count() % 1000) * 1000;
    fd_set fds;

    FD_ZERO(&fds);
    FD_SET(sock_, &fds);

    int return_val = select(sock_ + 1, NULL, &fds,  NULL, &timing);
    if (return_val > 0){
        // std::cerr << "Succes connect to socket in EstablishConnect tcp_connect"<< std::endl;
        flags = fcntl(sock_, F_GETFL, 0);
        if(fcntl(sock_, F_SETFL, flags & ~O_NONBLOCK) < 0){
            perror("fcntl");
            throw std::runtime_error("fcntl");
        }
    }
    else if(return_val == 0){
        throw std::runtime_error("Timeout to connect in EstablishConnect tcp_connect");
        
    } else{
        throw std::runtime_error("undefined behavior");
    }
}

void TcpConnect::SendData(const std::string& data) const{
    if(been_closed){
        throw std::runtime_error("Socket was closed");
    }
    const char* buffer = data.c_str();
    ssize_t bytesToSend = data.size();
    ssize_t sendedBytes = 0;

    while (sendedBytes < bytesToSend )
    {
        buffer += sendedBytes;
        bytesToSend -= sendedBytes;
        if ((sendedBytes = send(sock_, buffer, bytesToSend, 0)) == -1) {
            perror("send");
            throw std::runtime_error("send");
            return;
        }
    }
}


std::string TcpConnect::ReceiveData(size_t bufferSize) const{
    {
        struct pollfd pofd[1];
        pofd[0].fd = sock_;
        pofd[0].events = POLLIN;

        switch (poll(pofd, 1, readTimeout_.count()))
        {
        case 0:
            throw std::runtime_error("Timeout in ReceiveData tcp_connect");
        case -1:
            perror("poll");
            throw std::runtime_error("poll error in ReceiveData tcp_connect");
        default:
            // std::cerr << "there is data to receive" << std::endl;
            break;
        }
    }

    size_t size_message;
    if(bufferSize > 0){
        size_message = bufferSize;
    } else{
        char buf[4];
        std::string size_(4, 0);
        int pass = 0, allpass = 0;
        while (allpass < 4)
        {
            if((pass = recv(sock_, &buf, 4, 0)) == -1){
                throw std::runtime_error("recv");
            }
            for (size_t i = 0; i < pass; i++)
            {
                size_[i] = buf[i];
            }
            allpass += pass;
            // std::cerr << "pass to find size: " << pass << std::endl;
            if(pass == 0){
                throw std::runtime_error("error count bytes in lenth of message to recieve");
            }
        }
    
        size_message = BytesToInt(size_);
    }
    // std::cerr << "must receive " << size_message << " bytes" << std::endl;

    std::string result = "";
    ssize_t getBytes = 0;
    char buffer[size_message];
    while (getBytes < size_message)
    {
        size_message -= getBytes;
        if ((getBytes = recv(sock_, buffer, size_message, 0)) == -1) {
            perror("recv");
            throw std::runtime_error("recv");
        }
        for (size_t i = 0; i < getBytes; i++)
        {
            result.push_back(buffer[i]);
        }
        // std::cerr << "was received " << getBytes << " bytes" << std::endl;
        if(getBytes == 0){
            if(size_message == 0){
                return result;
            } else{
                throw std::runtime_error("error count bytes");
            }
        }
    }
    return result;
}
