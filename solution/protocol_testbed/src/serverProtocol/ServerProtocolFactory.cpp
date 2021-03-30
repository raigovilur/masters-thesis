#include "ServerProtocolFactory.h"

#include "ServerMvfstProtocolQUIC.h"
#include "ServerOpenSSLProtocolTLS.h"

ServerProto::ServerPtr ServerProto::ServerProtocolFactory::getInstance(Protocol::ProtocolType type) {
    switch (type) {
        case Protocol::MVFST_QUIC:
            return std::make_shared<ServerMvfstProtocolQUIC>();
        case Protocol::OpenSSL_TLS:
            return std::make_shared<ServerOpenSSLProtocolTLS>();
        default:
            return nullptr;
    }
}
