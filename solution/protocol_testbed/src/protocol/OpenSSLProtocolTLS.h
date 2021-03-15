#ifndef PROTOCOL_TESTBED_OPENSSLPROTOCOLTLS_H
#define PROTOCOL_TESTBED_OPENSSLPROTOCOLTLS_H

#include "ProtocolInterface.h"
#include <openssl/ossl_typ.h>

namespace Protocol {
    class OpenSSLProtocolTLS : public ProtocolInterface {
    public:
        bool openProtocol(std::string address, uint port) override;
        bool send(const char *buffer, size_t bufferSize) override;
        bool closeProtocol() override;
        ~OpenSSLProtocolTLS();

    private:
        bool _initialized = false;
        bool _socketInitialized = false;

        SSL* _ssl = nullptr;
        int _socket = 0;
        int _sockFd = 0;
    };
}



#endif //PROTOCOL_TESTBED_OPENSSLPROTOCOLTLS_H
