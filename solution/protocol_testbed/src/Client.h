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

    void send(std::ifstream& fileStream, size_t bufferSize, uint retryCount);
    void printStatistics();

private:
    bool sendWithRetries(const char* buffer, size_t bufferSize, uint retryCount, Protocol::ProtocolPtr& protocol);

    Protocol::ProtocolType _type;
    std::string _address;
    uint _port;

    std::chrono::duration<double> _elapsedSeconds{};
    std::streampos _fileSize = 0;
    uint _connectionDrops = 0;
};


#endif //PROTOCOL_TESTBED_CLIENT_H