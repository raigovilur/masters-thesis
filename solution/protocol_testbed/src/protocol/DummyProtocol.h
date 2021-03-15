#ifndef PROTOCOL_TESTBED_DUMMYPROTOCOL_H
#define PROTOCOL_TESTBED_DUMMYPROTOCOL_H

#include <string>
#include "ProtocolInterface.h"

namespace Protocol {
    class DummyProtocol : public ProtocolInterface {
    public:
        DummyProtocol();
        ~DummyProtocol();

        bool openProtocol(std::string address, uint port) override;

        bool send(const char *buffer, size_t bufferSize) override;
        bool closeProtocol() override;

    private:
        bool _dropsConnection = true;
        uint _probabilityOfConnectionLoss = 30;
    };
}

#endif //PROTOCOL_TESTBED_DUMMYPROTOCOL_H
