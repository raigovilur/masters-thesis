#ifndef PROTOCOL_TESTBED_SERVEROSPROTOCOLTCP_H
#define PROTOCOL_TESTBED_SERVEROSPROTOCOLTCP_H

#include "ServerInterface.h"

namespace ServerProto {
    class ServerOSProtocolTCP : public ServerInterface {
    public:
        ~ServerOSProtocolTCP();

        bool serverListen(const std::string &address, ushort port, uint cipher) override;

        bool setCertificate(const char *certificate, size_t len) override;

        bool setPrivateKey(const char *pKey, size_t len) override;

    private:
        int _socket;
    };
};



#endif //PROTOCOL_TESTBED_SERVEROSPROTOCOLTCP_H
