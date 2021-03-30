#ifndef PROTOCOL_TESTBED_SERVERMVFSTPROTOCOLQUIC_H
#define PROTOCOL_TESTBED_SERVERMVFSTPROTOCOLQUIC_H


#include <string>
#include "ServerInterface.h"

namespace ServerProto {
    class ServerMvfstProtocolQUIC : public ServerInterface {
    public:
        bool serverListen(const std::string &address, ushort port) override;

        bool setCertificate(const char *certificate, size_t len) override;

        bool setPrivateKey(const char *pKey, size_t len) override;

    private:
        const char* _certificate{};
        size_t _certLen{};
        const char* _privKey{};
        size_t _privKeyLen{};
    };
}


#endif //PROTOCOL_TESTBED_SERVERMVFSTPROTOCOLQUIC_H
