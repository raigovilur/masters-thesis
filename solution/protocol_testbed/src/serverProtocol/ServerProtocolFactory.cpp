#include "ServerProtocolFactory.h"

#include "ServerMvfstProtocolQUIC.h"
#include "ServerOpenSSLProtocolTLS.h"
#include "ServerOpenSSLProtocolDTLS.h"
#include "ServerOSProtocolTCP.h"
#include "ServerOSProtocolUDP.h"

ServerProto::ServerPtr ServerProto::ServerProtocolFactory::getInstance(Protocol::ProtocolType type) {
    switch (type) {
        case Protocol::MVFST_QUIC:
            return std::make_shared<ServerMvfstProtocolQUIC>();
        case Protocol::OpenSSL_TLS:
            return std::make_shared<ServerOpenSSLProtocolTLS>();
        case Protocol::OpenSSL_DTLS1_2:
            return std::make_shared<ServerOpenSSLProtocolDTLS>();
        case Protocol::TCP:
            return std::make_shared<ServerOSProtocolTCP>();
        case Protocol::UDP:
            return std::make_shared<ServerOSProtocolUDP>();
        default:
            return nullptr;
    }
}
