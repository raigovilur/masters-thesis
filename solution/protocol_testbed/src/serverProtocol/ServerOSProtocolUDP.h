#ifndef PROTOCOL_TESTBED_SERVEROSPROTOCOLUDP_H
#define PROTOCOL_TESTBED_SERVEROSPROTOCOLUDP_H

#include "ServerInterface.h"

namespace ServerProto {
    class ServerOSProtocolUDP : public ServerInterface {
    public:
        ~ServerOSProtocolUDP();

        bool serverListen(const std::string &address, ushort port) override;

        bool setCertificate(const char *certificate, size_t len) override;

        bool setPrivateKey(const char *pKey, size_t len) override;

    private:
        int _socket;
    };
};


#endif //PROTOCOL_TESTBED_SERVEROSPROTOCOLUDP_H
