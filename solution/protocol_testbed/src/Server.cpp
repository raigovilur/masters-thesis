#include "Server.h"

#include <folly/String.h>
#include <iostream>
#include <sstream>
#include <iomanip>

#include "serverProtocol/ServerProtocolFactory.h"
#include "appProto/FileSendHeader.h"
#include "appProto/utils.h"
#include <thread>

#include<nlohmann/json.hpp>

bool Server::processDataStream(ClientData& client, const unsigned char *bytes, size_t processStart, size_t packetLength) {
    std::string s(bytes, bytes + packetLength);
    nlohmann::json j;
    try {
        j = nlohmann::json::parse(s);
    } catch (std::exception &ex) {
        std::cout << "JSON Parse Error" << std::endl;
        return true;
    }

    auto epoch = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch());
    long now = epoch.count();
    auto latency = now - long(j["timestamp"]);
    std::cout << "Id: " << j["id"] << " Latency: " << latency/1000.0 << " milliseconds" << std::endl;

    //TODO : Write to Database or cache
    /*
    if (client.outputStream == nullptr)
    {
        client.outputStream = std::make_unique<std::ofstream>(client.fileName, std::ios::binary);
        if (!client.outputStream->is_open()) {
            std::cout << "Unable to open file stream for file: " << client.fileName << std::endl;
            client.outputStream.reset();
            return false;
        }
    }

    size_t bytesToRead = packetLength - processStart;
    client.outputStream->write((char*) (bytes + processStart), bytesToRead);
    */

    size_t bytesToRead = packetLength - processStart;
    client.receivedBytes += bytesToRead;
    client.receivedNumOfRecords += 1;
    //client.printPercentageStatsIfFullPercent();

    return true;
}
bool Server::consume(std::string client, const unsigned char *bytes, size_t packetLength) {
    return consumeInternal(client, bytes, packetLength);
}

bool Server::consumeInternal(const std::string& client, const unsigned char *bytes, size_t packetLength) {
    // FIRST BYTES are header
    size_t numberOfBytesProcessed = 0; // Amount we have consumed already from the packet
    
    if (_clientHeaders.find(client) == _clientHeaders.end()){
        auto headerBytes = std::vector<unsigned char>(bytes, bytes + packetLength);
        _clientHeaders.emplace(std::pair<std::string, ClientData>(client, headerBytes));
        processHeader(_clientHeaders.at(client));
        return true;
    }

    size_t bytesToProcess = packetLength - numberOfBytesProcessed;
    
    if (!processDataStream(_clientHeaders.at(client), bytes, numberOfBytesProcessed, bytesToProcess)) {
        _clientHeaders.erase(_clientHeaders.find(client));
    }

    return true;
}

bool Server::start(Protocol::ProtocolType type, const std::string& listenAddress, ushort port) {
    ServerProto::ServerPtr server = ServerProto::ServerProtocolFactory::getInstance(type);

    server->setCallback(this);
    server->setCertificate(_cert.data(), _cert.size());
    server->setPrivateKey(_privKey.data(), _privKey.size());

    std::cout << "Server is starting." << std::endl;

    server->serverListen(listenAddress, port);


    return true;
}

bool Server::processHeader(Server::ClientData& data) {
    assert(!data.headerProcessed);

    const std::vector<unsigned char> header = data.header;
    
    //data.fileName = "sent_packets.log";
    data.numOfRecords = Utils::convertToUint64_t(header, ISE_FILE_SIZE_OFFSET, ISE_FILE_SIZE);
    std::cout << "Expected Number of Records: " << data.numOfRecords << std::endl;
    data.headerProcessed = true;
    data.numOfRecordsForOnePercent = data.numOfRecords / 100;
    data.startTime = std::chrono::high_resolution_clock::now();
    data.lastPercentStartTime = data.startTime;

    return true;
}

bool Server::readCertificate(const std::string &privKeyPath, const std::string &certPath) {

    std::ifstream privKeyIfStream(privKeyPath);
    _privKey = std::string((std::istreambuf_iterator<char>(privKeyIfStream) ),
                           (std::istreambuf_iterator<char>()    ) );

    std::ifstream certIfStream(certPath);
    _cert = std::string((std::istreambuf_iterator<char>(certIfStream) ),
                        (std::istreambuf_iterator<char>()    ) );

    return true;
}
