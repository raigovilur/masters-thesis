#ifndef PROTOCOL_TESTBED_MVFSTPROTOCOLQUIC_H
#define PROTOCOL_TESTBED_MVFSTPROTOCOLQUIC_H

#include <map>
#include "ProtocolInterface.h"

class MvFstConnector;

namespace Protocol {
    class MvfstProtocolQUIC : public ProtocolInterface {
    public:
        ~MvfstProtocolQUIC();

        bool openProtocol(std::string address, uint port) override;
        bool openProtocolServer(uint port) override;

        bool send(const char *buffer, size_t bufferSize) override;
        bool closeProtocol() override;

    private:
        MvFstConnector* _mvfst;
    };
}



#endif //PROTOCOL_TESTBED_MVFSTPROTOCOLQUIC_H
