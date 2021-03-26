#ifndef PROTOCOL_TESTBED_PROTOCOLINTERFACE_H
#define PROTOCOL_TESTBED_PROTOCOLINTERFACE_H

/**
 * This is an interface class for all networking components
 * Networking components implement a connection via a particular
 * library and protocol
 */
#include <memory>

namespace Protocol {
    class ProtocolInterface {

    public:
        virtual bool openProtocol(std::string address, uint port) = 0;
        virtual bool openProtocolServer(uint port) = 0;
        virtual bool send(const char *buffer, size_t bufferSize, bool eof = false) = 0;
        virtual bool closeProtocol() = 0;
    };

    typedef std::shared_ptr<ProtocolInterface> ProtocolPtr;
}


#endif //PROTOCOL_TESTBED_PROTOCOLINTERFACE_H
