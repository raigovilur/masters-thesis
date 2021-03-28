#include "ServerProtocolFactory.h"

#include "ServerMvfstProtocolQUIC.h"

ServerProto::ServerPtr ServerProto::ServerProtocolFactory::getInstance(Protocol::ProtocolType type) {
    switch (type) {
        case Protocol::MVFST_QUIC:
            return std::make_shared<ServerMvfstProtocolQUIC>();
        default:
            return nullptr;
    }
}
