#include "Client.h"
#include <fstream>
#include <iostream>
#include <cstring>
#include <openssl/sha.h>
#include <vector>
#include <filesystem>

#include "appProto/FileSendHeader.h"
#include "protocol/ProtocolFactory.h"

namespace {
    bool calculateSha256(const std::string& path, unsigned char* hash)
    {
        std::ifstream fp(path, std::ios::binary);

        if (not fp.good()) {
            std::cout << "Cannot open file to checksum: \"" << path << "\": " << std::strerror(errno) << std::endl;
            return false;
        }

        constexpr const std::size_t bufferSize {1 << 12 };
        char buffer[bufferSize];

        SHA256_CTX ctx;
        SHA256_Init(&ctx);

        while (fp.good()) {
            fp.read(buffer, bufferSize);
            SHA256_Update(&ctx, buffer, fp.gcount());
        }

        SHA256_Final(hash, &ctx);
        fp.close();

        return true;
    }

    void writeToByteArray(char* array, size_t offset, const std::vector<char>& whatToWrite) {
        for (size_t i = 0; i < whatToWrite.size(); ++i) {
            array[offset + i] = whatToWrite[i];
        }
    }

    std::vector<char> convertToCharArray(size_t value, ushort charArraySize) {
        std::vector<char> output;
        output.resize(charArraySize);

        for (int i = charArraySize - 1; i >= 0; --i) {
            output[i] = value & 0xFF;
            value = value >> 8;
        }

        return output;
    }
}

Client::Client(Protocol::ProtocolType type, std::string address, uint port)
    : _type(type)
    , _address(std::move(address))
    , _port(port) {

}

Client::~Client() { }


void Client::send(const std::string& path, size_t bufferSize, uint retryCount) {

    auto* hash = new char[SHA256_DIGEST_LENGTH] {0};

    auto startTime = std::chrono::system_clock::now();

    if (!calculateSha256(path, (unsigned char*) hash)) {
        std::cout << "Unable to calculate has for file: " << path << std::endl;
        return;
    }

    //| Protocol ID (2 bytes) | Protocol version (2 bytes) | checksum for the file (64 bytes) | file size (64 bytes) | file name (1024 bytes ending 0 bytes) | file itself... |

    _elapsedHashSeconds = std::chrono::system_clock::now() - startTime;
    std::ifstream fileStream(path, std::fstream::binary | std::fstream::in);

    fileStream.seekg(0, std::ios::end);
    _fileSize = fileStream.tellg();
    fileStream.seekg(0, std::ios::beg);

    std::unique_ptr<char[]> header(new char[ISE_HEADER_SIZE] {0});

    // Composing header
    // Protocol ID is 0
    writeToByteArray(header.get(), ISE_PROTOCOL_OFFSET, {0x00, 0x00});
    // Version is 0
    writeToByteArray(header.get(), ISE_PROTOCOL_VERSION_OFFSET, {0x00, 0x00});
    // SHA-256 file checksum
    writeToByteArray(header.get(), ISE_CHECKSUM_OFFSET, std::vector<char>(hash, hash + SHA256_DIGEST_LENGTH));
    // File size
    writeToByteArray(header.get(), ISE_FILE_SIZE_OFFSET, convertToCharArray(_fileSize, ISE_FILE_SIZE));

    // File Name
    std::string fileName = std::filesystem::path(path).filename();
    writeToByteArray(header.get(), ISE_FILE_NAME_OFFSET, std::vector<char>(fileName.begin(), fileName.end()));

    delete[] hash;

    uint buffersNeeded = _fileSize / bufferSize;
    uint lastPacketSize = _fileSize % bufferSize;
    std::unique_ptr<char[]> buffer(new char[bufferSize]);

    Protocol::ProtocolPtr protocol = Protocol::ProtocolFactory::getInstance(_type);
    startTime = std::chrono::system_clock::now();
    if (!protocol->openProtocol(_address, _port)) {
        protocol->closeProtocol();
        return;
    }

    // Sending header
    if (!sendWithRetries(header.get(), ISE_HEADER_SIZE, retryCount, protocol,
                         buffersNeeded == 0 && lastPacketSize == 0)) {
        std::cout << "Client: Connection failed" << std::endl;
        protocol->closeProtocol();
        _elapsedSeconds = std::chrono::system_clock::now() - startTime;
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
    fileStream.close();
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
    std::cout << "Hashing took " << _elapsedHashSeconds.count() << " seconds " << std::endl;
}