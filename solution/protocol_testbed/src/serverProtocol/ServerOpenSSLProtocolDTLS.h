#ifndef PROTOCOL_TESTBED_SERVEROPENSSLPROTOCOLDTLS_H
#define PROTOCOL_TESTBED_SERVEROPENSSLPROTOCOLDTLS_H

#include <openssl/bio.h>
#include <netinet/in.h>
#include "ServerInterface.h"

namespace ServerProto {
    class ServerOpenSSLProtocolDTLS : public ServerInterface {
    public:
        ~ServerOpenSSLProtocolDTLS();

        [[noreturn]] bool serverListen(const std::string &address, ushort port, uint cipher) override;

        bool setCertificate(const char *certificate, size_t len) override;

        bool setPrivateKey(const char *pKey, size_t len) override;

    private:
        bool serviceConnection(int clinentFd, const sockaddr_in* addr, SSL* ssl);

    private:
        EVP_PKEY* _pKey = nullptr;
        X509* _cert = nullptr;
    };
};


#endif //PROTOCOL_TESTBED_SERVEROPENSSLPROTOCOLDTLS_H
