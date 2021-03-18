#include "ProtocolFactory.h"
#include "DummyProtocol.h"
#include "OpenSSLProtocolTLS.h"
#include "OpenSSLProtocolDTLS.h"
#include "MvfstProtocolQUIC.h"

Protocol::ProtocolPtr Protocol::ProtocolFactory::getInstance(Protocol::ProtocolType type) {
    switch (type) {
        case ProtocolType::Dummy:
            return std::make_shared<DummyProtocol>();
        case ProtocolType::OpenSSL_TLS:
            return std::make_shared<OpenSSLProtocolTLS>();
        case ProtocolType::OpenSSL_DTLS1_2:
            return std::make_shared<OpenSSLProtocolDTLS>();
        case ProtocolType::MVFST_QUIC:
            return std::make_shared<MvfstProtocolQUIC>();
        default:
            return nullptr;
    }
}
