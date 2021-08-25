#ifndef PROTOCOL_TESTBED_SERVERMVFSTPROTOCOLQUIC_H
#define PROTOCOL_TESTBED_MVFSTPROTOCOLQUIC_H

#include <map>
#include <chrono>
#include "ProtocolInterface.h"

class MvFstConnector;

namespace Protocol {
    class MvfstProtocolQUIC : public ProtocolInterface {
    public:
        ~MvfstProtocolQUIC();

        bool openProtocol(std::string address, uint port, uint cipher, Options) override;
        bool openProtocolServer(uint port) override;

        bool send(const char *buffer, size_t bufferSize, bool eof) override;
        bool closeProtocol() override;

        bool isAllSent() override;

    private:
        MvFstConnector* _mvfst;
        Options _options;
        bool _firstPacket = true;
        std::chrono::time_point<std::chrono::system_clock> _lastSendTimestamp;
    };
}



#endif //PROTOCOL_TESTBED_SERVERMVFSTPROTOCOLQUIC_H
