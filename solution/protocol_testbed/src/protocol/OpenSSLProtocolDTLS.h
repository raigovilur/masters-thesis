#ifndef PROTOCOL_TESTBED_OPENSSLPROTOCOLDTLS_H
#define PROTOCOL_TESTBED_OPENSSLPROTOCOLDTLS_H

#include "ProtocolInterface.h"
#include <openssl/ossl_typ.h>
#include <map>
#include <vector>
#include <chrono>

namespace Protocol {

    class OpenSSLProtocolDTLS : public ProtocolInterface {
    public:
        bool openProtocol(std::string address, uint port, uint cipher, Options) override;
        bool openProtocolServer(uint port) override;
        bool send(const char *buffer, size_t bufferSize, bool eof) override;
        bool closeProtocol() override;
        ~OpenSSLProtocolDTLS();

    private:
        static uint getNextSequence();
        void getAcks();
        void cleanStaleAcks();

    public:
        bool isAllSent() override;

    private:

        bool cleanUp();

    private:
        bool _initialized = false;
        bool _socketInitialized = false;
        std::map<unsigned short, std::vector<ushort>> _waitingAck;

        SSL* _ssl = nullptr;
        int _socket = 0;
        int _sockFd = 0;

        std::vector<ushort> _notAckedPackets;
        std::map<ushort, std::chrono::time_point<std::chrono::system_clock>> _timeStamps;

        bool _firstPacket = true;
        std::chrono::time_point<std::chrono::system_clock> _lastSendTimestamp;
        Options _options;
    };
};


#endif //PROTOCOL_TESTBED_OPENSSLPROTOCOLDTLS_H
