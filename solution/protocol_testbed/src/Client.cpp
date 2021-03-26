//
// Created by raigo on 07.03.21.
//

#include "Client.h"
#include <fstream>
#include <protocol/ProtocolFactory.h>
#include <iostream>


Client::Client(Protocol::ProtocolType type, std::string address, uint port)
    : _type(type)
    , _address(std::move(address))
    , _port(port) {

}

Client::~Client() {

}

void Client::send(std::ifstream &fileStream, size_t bufferSize, uint retryCount) {

    fileStream.seekg(0, std::ios::end);
    _fileSize = fileStream.tellg();
    fileStream.seekg(0, std::ios::beg);

    uint buffersNeeded = _fileSize / bufferSize;
    uint lastPacketSize = _fileSize % bufferSize;
    std::unique_ptr<char[]> buffer(new char[bufferSize]);

    Protocol::ProtocolPtr protocol = Protocol::ProtocolFactory::getInstance(_type);
    auto startTime = std::chrono::system_clock::now();
    if (!protocol->openProtocol(_address, _port)) {
        protocol->closeProtocol();
        return;
    }

    // Sending full buffer amounts
    for (uint i = 0; i < buffersNeeded; ++i) {
        fileStream.read(buffer.get(), bufferSize);
        if (!sendWithRetries(buffer.get(), bufferSize, retryCount, protocol,
                             i == buffersNeeded - 1 && lastPacketSize == 0)) {
            std::cout << "Client: Connection failed" << std::endl;
            protocol->closeProtocol();
            _elapsedSeconds = std::chrono::system_clock::now() - startTime;
            return;
        }
    }

    // Sending last buffer
    if (lastPacketSize > 0) {
        fileStream.read(buffer.get(), lastPacketSize);
        sendWithRetries(buffer.get(), lastPacketSize, retryCount, protocol, true);
    }
    protocol->closeProtocol();
    _elapsedSeconds = std::chrono::system_clock::now() - startTime;
    std::cout << "File sent" << std::endl;
}

bool Client::sendWithRetries(const char *buffer, size_t bufferSize, uint retryCount, Protocol::ProtocolPtr& protocol, bool eof) {
    for (uint retry = 0; retry < retryCount; ++retry) {
        if (protocol->send(buffer, bufferSize, eof)) {
            return true;
        } else {
            if (retry < retryCount - 1)
            {
                std::cout << "Connection dropped, retrying: (" << retry + 1 << ")"  << std::endl;
                // TODO Should have a wait time here for connection drop
                protocol->closeProtocol();
                protocol->openProtocol(_address, _port);
                ++_connectionDrops;
            }
        }
    }
    return false;
}

void Client::printStatistics() {
    std::cout << "Statistics:" << std::endl;
    std::cout << "Sent " << _fileSize << " bytes" << std::endl;
    std::cout << " over " << _elapsedSeconds.count() << " seconds " << std::endl;
    std::cout << "Connection dropped " << _connectionDrops << " times " << std::endl;
}
