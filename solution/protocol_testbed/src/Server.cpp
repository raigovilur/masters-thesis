#include "Server.h"

#include <folly/String.h>
#include <iostream>
#include <sstream>
#include <iomanip>

#include "serverProtocol/ServerProtocolFactory.h"
#include "appProto/FileSendHeader.h"
#include "appProto/utils.h"

namespace {
    uint64_t convertToUint64_t(const std::vector<unsigned char>& data, size_t dataStart, int byteLength) {
        uint64_t output = 0;
        for (int i = 0; i < byteLength; ++i) {
            output = output << 8;
            output = output | data[dataStart + i];
        }
        return output;
    }
}

bool Server::processDataStream(ClientData& client, unsigned char *bytes, size_t processStart, size_t packetLength) {
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

    // We don't want to read more than we should
    if (bytesToRead > client.fileSize-client.receivedFileSize) {
        bytesToRead = client.fileSize-client.receivedFileSize;
    }

    client.outputStream->write((char*) (bytes + processStart), bytesToRead);
    client.receivedFileSize += bytesToRead;

    if (client.fileReceived()) {
        client.outputStream->close();
    }
    return true;
}


bool Server::consume(std::string client, unsigned char *bytes, size_t packetLength) {
    // This is just for debugging
    std::cout << std::string((char*) bytes, packetLength) << std::endl;

    // FIRST BYTES are header
    size_t numberOfBytesProcessed = 0; // Amount we have consumed already from the packet
    if (_clientHeaders.find(client) == _clientHeaders.end())
    {
        // No header has been received
        if (packetLength >= ISE_HEADER_SIZE) {
            auto headerBytes = std::vector<unsigned char>(bytes, bytes + ISE_HEADER_SIZE);
            _clientHeaders.emplace(std::pair<std::string,ClientData>(client, headerBytes));
            numberOfBytesProcessed = ISE_HEADER_SIZE;
            processHeader(_clientHeaders.at(client));
        } else {
            auto headerBytes = std::vector<unsigned char>(bytes, bytes + packetLength);
            _clientHeaders.emplace(std::pair<std::string, ClientData>(client, headerBytes));
            // We need more bytes to complete the header, further processing not necessary
            return true;
        }
    } else if (!_clientHeaders.at(client).headerProcessed) {
        // some header has been received
        size_t bytesNeeded = ISE_HEADER_SIZE - _clientHeaders.at(client).header.size();
        if (bytesNeeded < 0) {
            // Sanity check:
            std::cout << "Error: saved header needs negative bytes" << std::endl;
        } else if (bytesNeeded > 0){
            // Some more bytes are needed
            if (packetLength < bytesNeeded) {
                std::copy(bytes, bytes + packetLength, std::back_inserter(_clientHeaders.at(client).header));
                // We need more bytes still, cannot process further
                return true;
            } else {
                numberOfBytesProcessed = bytesNeeded;
                std::copy(bytes, bytes + packetLength, std::back_inserter(_clientHeaders.at(client).header));
            }
        }

        // We have a complete header
        processHeader(_clientHeaders.at(client));
    }

    if (numberOfBytesProcessed == packetLength) {
        // The packet has been consumed entirely
        return true;
    }
    if (!_clientHeaders.at(client).headerProcessed) {
        // Sanity check
        assert(false && "Header should be processed by this time");
    }

    size_t bytesToProcess = packetLength - numberOfBytesProcessed;

    if (!processDataStream(_clientHeaders.at(client), bytes, numberOfBytesProcessed, bytesToProcess)) {
        _clientHeaders.erase(_clientHeaders.find(client));
    }

    if (_clientHeaders.at(client).fileReceived()) {
        // TODO compare checksum and then reply OK.
        auto* hash = new char[SHA256_DIGEST_LENGTH] {0};

        std::cout << "Calculating checksum ..." << std::endl;

        if (!Utils::calculateSha256(_clientHeaders.at(client).fileName, (unsigned char*) hash)) {
            std::cout << "Unable to calculate hash for file: " << _clientHeaders.at(client).fileName << std::endl;
            return false;
        }
        std::stringstream hashHexStrStream;
        hashHexStrStream << std::hex;

        for( int i(0) ; i < SHA256_DIGEST_LENGTH; ++i )
            hashHexStrStream << std::setw(2) << std::setfill('0') << (int)hash[i];

        std::cout << "Checksum calculated: " << hashHexStrStream.str() << std::endl;
        auto clientChecksum = _clientHeaders.at(client).checksum;

        if (!Utils::compareBytes(clientChecksum,
                            0,
                                 clientChecksum.size(),
                            std::vector<unsigned char>(hash, hash + SHA256_DIGEST_LENGTH))) {
            std::stringstream receivedHashHexStrStream;
            receivedHashHexStrStream << std::hex;

            for( int i(0) ; i < SHA256_DIGEST_LENGTH; ++i )
                receivedHashHexStrStream << std::setw(2) << std::setfill('0') << (int)clientChecksum[i];
            std::cerr << "Checksums differ! Received checksum: " << receivedHashHexStrStream.str() << " calculated: "  << hashHexStrStream.str() << std::endl;
        }

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

    assert(Utils::compareBytes(header, ISE_PROTOCOL_OFFSET, ISE_PROTOCOL_SIZE, {0x00, 0x00}));
    assert(Utils::compareBytes(header, ISE_PROTOCOL_VERSION_OFFSET, ISE_PROTOCOL_VERSION_SIZE, {0x00, 0x00}));

    data.checksum = std::vector<unsigned char>(header.begin() + ISE_CHECKSUM_OFFSET, header.begin() + ISE_CHECKSUM_OFFSET + ISE_CHECKSUM_SIZE);

    std::string name(header.begin() + ISE_FILE_NAME_OFFSET, header.begin() + ISE_FILE_NAME_OFFSET + ISE_FILE_NAME_SIZE);
    data.fileName = name;

    data.fileSize = convertToUint64_t(header, ISE_FILE_SIZE_OFFSET, ISE_FILE_SIZE);
    std::cout << "Expected file size: " << data.fileSize << std::endl;

    data.headerProcessed = true;

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
