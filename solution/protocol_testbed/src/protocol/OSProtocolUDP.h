#ifndef PROTOCOL_TESTBED_OSPROTOCOLUDP_H
#define PROTOCOL_TESTBED_OSPROTOCOLUDP_H


#include <chrono>
#include "ProtocolInterface.h"
#include <netinet/in.h>

namespace Protocol {
    class OSProtocolUDP : public ProtocolInterface {
    public:
        bool openProtocol(std::string address, uint port, uint cipher, Options) override;
        bool openProtocolServer(uint port) override;
        bool send(const char *buffer, size_t bufferSize, bool eof) override;
        bool closeProtocol() override;
        ~OSProtocolUDP();


    private:
        sockaddr_in saiServerAddress{};
        Options _options;
        bool _firstPacket = true;
        std::chrono::time_point<std::chrono::system_clock> _lastSendTimestamp;
        int _socket = 0;
    };
}

#endif //PROTOCOL_TESTBED_OSPROTOCOLUDP_H
