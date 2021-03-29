#include "Server.h"

#include <folly/String.h>
#include <iostream>

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

// THESE are anyway test certificates/keys, so it doesn't really matter that they end up in git
constexpr folly::StringPiece certificate = R"(
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIUB62ZpZCtt36bbY0wK8Xc8f+MJs0wDQYJKoZIhvcNAQEL
BQAwRTELMAkGA1UEBhMCRUUxEzARBgNVBAgMClNvbWUtU3RhdGUxITAfBgNVBAoM
GEludGVybmV0IFdpZGdpdHMgUHR5IEx0ZDAeFw0yMTAzMjQwODIzMDJaFw0yMjAz
MjQwODIzMDJaMEUxCzAJBgNVBAYTAkVFMRMwEQYDVQQIDApTb21lLVN0YXRlMSEw
HwYDVQQKDBhJbnRlcm5ldCBXaWRnaXRzIFB0eSBMdGQwggIiMA0GCSqGSIb3DQEB
AQUAA4ICDwAwggIKAoICAQCjvRZy31MDYGFEfRWUVsCv06hyiEb3wJwbEMIooJVO
jie3Xwzfx2Xatdk7Yoxl6OlFNkdr8ux8ZbTQrzY2JNb/xPALP60yPyXfiAtnhbga
caWdhJDnka4bvtTp3YbXJDXT4AxdCfviFEIc+S73qhULqRzFH3J9dQfAkzfV4ep9
8wTbg//EPIbHExIHKN+7Q5ah0shEoFKRnOWW8x5HjxWCT/yDfi5j7tsXbwSbzYUW
7t1FbwZ9PwglThtUUq7nYNDr9dvlePLycdTARaQQsMK4gRWB+A/5KgITVsurx/v/
IuOeBITE8+1RKa/iZcyykepLWhgLT9kK5aB+exXPf+4ykzUe5KVOFmMSu2Sj8MRX
F11H16VErlBCCFCInJQ0tDeHdH0uV0to9NGuy5LMuEWCVkRLf2Oz94uEnRYPzbf2
/d4M/Ke+bDEnPED8DeNNPHa20uZNXDYVCGV59v5Gvd4vMohBAZ78etoZbA/8md7+
F7XA8l5b8LrHv4LVhg7RviWEvEF/6siEG+KdH4qJXqFCEssypg8Lo+iPxjv4JuV3
/WGto40tV99lgyuGApNR6RUDKVMa7MXLUbyd7YYecjUBlrss19R7lZWRAfB3sRmG
D+vT7F7vSy2UKihptZTBYrnFKnOQyzNrZyx3615GWRN+8pgSLhLU0oWbdrGjn/io
wwIDAQABo1MwUTAdBgNVHQ4EFgQUY2GHni5BAh5JnEn9A021L16cXx4wHwYDVR0j
BBgwFoAUY2GHni5BAh5JnEn9A021L16cXx4wDwYDVR0TAQH/BAUwAwEB/zANBgkq
hkiG9w0BAQsFAAOCAgEAW0wPUI3zkEaUPdw6VI+9B1sS6QRKqqp4KuJl8V1Q9Ywt
n6uqw+n1P7O8Fj1etOAfW2mBeAprmf/XjNDxQ61tNy8k/XnlrL/UHUkanwv0n0M9
4LUPlp2PVTXrKEpjuukUPbRAKrWV0p29LnWnO9GlBefNjWvwMhjwAdX8srwbw2/0
/ofNPOftjzPvtPgyzj4579TbkY55DhJe9YKiatkVOr3Ab9Q1WpNeX7+eS1H/f0nh
e9d8WIgv9Z+AC0QRvsnoewLDbRhGavoLNvW30XEStIPmUrRzOIF79/PjkM9Ny0a1
/fvQPesnkPsLeonE6TimXJsn8dF3Czt0ehgxO4IQ20FXDXHgehSR2H4Wg3TVSZ74
z5hOWRHYrvXrDy/b4n/sP7AO/3WeSmsD2LrQsippmgsnqpevn0192YDaB358f4XX
BbYguoy73tHu41DcauwgPwx2PJlDgv4V06TQu7Jk49Y8nmripRE6Ne+wqJ3gDDN/
r+POTDFaDGGv8bjQ7P/BmOa5erYOZmDzDnvOii0A/OYYN2LC0xptx+QmWSWTzQ+3
bxMe8FGvGo7nWtXH1+HeF0/raGWzt8LsF0NXPQX3eR1CzdLAtSyxeHLvn65naMJT
o/Y3t3jZrODQQ7jUQjE/XtfuKFYKINBR5CKkrY7XQHahc/GJr2GZsrLqPF8FWu0=
-----END CERTIFICATE-----
)";

constexpr folly::StringPiece privKey = R"(
-----BEGIN PRIVATE KEY-----
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
Sd/H4YHoc8qOIZUOXaVW3YlTShhbeZIx
-----END PRIVATE KEY-----
)";

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
        assert(false && "Header should be processeed by this time");
    }

    size_t bytesToProcess = packetLength - numberOfBytesProcessed;

    if (!processDataStream(_clientHeaders.at(client), bytes, numberOfBytesProcessed, bytesToProcess)) {
        _clientHeaders.erase(_clientHeaders.find(client));
    }

    if (_clientHeaders.at(client).fileReceived()) {
        // TODO Calculate checksum, compare and then reply OK.
        _clientHeaders.erase(_clientHeaders.find(client));
    }

    return true;
}

bool Server::start(Protocol::ProtocolType type, const std::string& listenAddress, ushort port) {
    ServerProto::ServerPtr server = ServerProto::ServerProtocolFactory::getInstance(type);

    server->setCallback(this);
    server->setCertificate(certificate.data(), certificate.size());
    server->setPrivateKey(privKey.data(), privKey.size());

    server->listen(listenAddress, port);


    return true;
}

bool Server::processHeader(Server::ClientData& data) {
    assert(!data.headerProcessed);

    const std::vector<unsigned char> header = data.header;

    assert(Utils::compareBytes(header, ISE_PROTOCOL_OFFSET, ISE_PROTOCOL_SIZE, {0x00, 0x00}));
    assert(Utils::compareBytes(header, ISE_PROTOCOL_VERSION_OFFSET, ISE_PROTOCOL_VERSION_SIZE, {0x00, 0x00}));

    // TODO checksum
    // data.checksum = std::copy from ISE_CHECKSUM_OFFSET to OFFSET + LEN
    std::string name(header.begin() + ISE_FILE_NAME_OFFSET, header.begin() + ISE_FILE_NAME_OFFSET + ISE_FILE_NAME_SIZE);
    data.fileName = name;

    data.fileSize = convertToUint64_t(header, ISE_FILE_SIZE_OFFSET, ISE_FILE_SIZE);
    std::cout << "Expected file size: " << data.fileSize << std::endl;

    data.headerProcessed = true;

    return true;
}
