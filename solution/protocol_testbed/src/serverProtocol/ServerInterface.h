#ifndef PROTOCOL_TESTBED_SERVERINTERFACE_H
#define PROTOCOL_TESTBED_SERVERINTERFACE_H

#include <memory>

namespace ServerProto {
    class ServerReceiveCallback {
    public:
        virtual bool consume(std::string client, const unsigned char* bytes, size_t length) = 0;
    };

    class ServerInterface {
    public:
        virtual bool serverListen(const std::string &address, ushort port) = 0;
        virtual bool setCertificate(const char* certificate, size_t len) = 0;
        virtual bool setPrivateKey(const char* pKey, size_t len) = 0;

        void setCallback(ServerReceiveCallback* callback) {
            _callback = callback;
        }

    protected:
        ServerReceiveCallback* _callback = nullptr; // Non owning
    };

    typedef std::shared_ptr<ServerInterface> ServerPtr;
}

#endif //PROTOCOL_TESTBED_SERVERINTERFACE_H
