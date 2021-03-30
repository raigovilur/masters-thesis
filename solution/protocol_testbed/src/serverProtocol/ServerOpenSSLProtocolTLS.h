#ifndef PROTOCOL_TESTBED_SERVEROPENSSLPROTOCOLTLS_H
#define PROTOCOL_TESTBED_SERVEROPENSSLPROTOCOLTLS_H

#include <openssl/bio.h>
#include <netinet/in.h>
#include "ServerInterface.h"

namespace ServerProto {
    class ServerOpenSSLProtocolTLS : public ServerInterface {
    public:
        ~ServerOpenSSLProtocolTLS();

        bool serverListen(const std::string &address, ushort port) override;

        bool setCertificate(const char *certificate, size_t len) override;

        bool setPrivateKey(const char *pKey, size_t len) override;

    private:
        bool serviceConnection(const sockaddr_in* addr, SSL* ssl);

    private:
        EVP_PKEY* _pKey = nullptr;
        X509* _cert = nullptr;
    };
};


#endif //PROTOCOL_TESTBED_SERVEROPENSSLPROTOCOLTLS_H
