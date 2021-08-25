#ifndef PROTOCOL_TESTBED_SERVEROSPROTOCOLTCP_H
#define PROTOCOL_TESTBED_OSPROTOCOLTCP_H


#include "ProtocolInterface.h"

namespace Protocol {
    class OSProtocolTCP : public ProtocolInterface {
    public:
        bool openProtocol(std::string address, uint port, uint cipher, Options) override;
        bool openProtocolServer(uint port) override;
        bool send(const char *buffer, size_t bufferSize, bool eof) override;
        bool closeProtocol() override;
        ~OSProtocolTCP();


    private:
        int _socket = 0;
        int _sockFd = 0;
    };
}

#endif //PROTOCOL_TESTBED_SERVEROSPROTOCOLTCP_H
