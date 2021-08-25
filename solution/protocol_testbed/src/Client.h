#ifndef PROTOCOL_TESTBED_CLIENT_H
#define PROTOCOL_TESTBED_CLIENT_H

#include <string>
#include <protocol/ProtocolInterface.h>
#include "appProto/ProtocolType.h"
#include <chrono>

#include "util/TimeRecorder.h"

class Client {
public:
    Client(Protocol::ProtocolType type, std::string address, uint port, Protocol::Options options);
    ~Client();

    void send(const std::string& path, size_t bufferSize, uint retryCount, Utils::TimeRecorder *timeRecorder, uint cipher);
    void printStatistics() const;
    void runSpeedTest(uint port, const std::string& bandwidth) const;

private:
    bool sendWithRetries(const char* buffer, size_t bufferSize, uint retryCount, Protocol::ProtocolPtr& protocol, bool eof, uint cipher);

    Protocol::ProtocolType _type;
    std::string _address;
    uint _port;
    Protocol::Options _options;

    std::chrono::duration<double> _elapsedSeconds{};
    std::chrono::duration<double> _elapsedHashSeconds{};
    std::streampos _fileSize = 0;
    uint _connectionDrops = 0;
};


#endif //PROTOCOL_TESTBED_CLIENT_H
