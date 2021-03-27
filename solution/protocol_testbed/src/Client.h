#ifndef PROTOCOL_TESTBED_CLIENT_H
#define PROTOCOL_TESTBED_CLIENT_H

#include <string>
#include <protocol/ProtocolInterface.h>
#include "protocol/ProtocolType.h"
#include <chrono>

class Client {
public:
    Client(Protocol::ProtocolType type, std::string address, uint port);
    ~Client();

    void setHeadersForFile(std::string filePath);

    void send(const std::string& path, size_t bufferSize, uint retryCount);
    void printStatistics();

private:
    bool sendWithRetries(const char* buffer, size_t bufferSize, uint retryCount, Protocol::ProtocolPtr& protocol, bool eof);

    Protocol::ProtocolType _type;
    std::string _address;
    uint _port;

    std::chrono::duration<double> _elapsedSeconds{};
    std::chrono::duration<double> _elapsedHashSeconds{};
    std::streampos _fileSize = 0;
    uint _connectionDrops = 0;
};


#endif //PROTOCOL_TESTBED_CLIENT_H
