#ifndef PROTOCOL_TESTBED_SERVERMVFSTPROTOCOLQUIC_H
#define PROTOCOL_TESTBED_MVFSTPROTOCOLQUIC_H

#include <map>
#include "ProtocolInterface.h"

class MvFstConnector;

namespace Protocol {
    class MvfstProtocolQUIC : public ProtocolInterface {
    public:
        ~MvfstProtocolQUIC();

        bool openProtocol(std::string address, uint port, Options) override;
        bool openProtocolServer(uint port) override;

        bool send(const char *buffer, size_t bufferSize, bool eof) override;
        bool closeProtocol() override;

        bool isAllSent() override;

    private:
        MvFstConnector* _mvfst;
    };
}



#endif //PROTOCOL_TESTBED_SERVERMVFSTPROTOCOLQUIC_H
