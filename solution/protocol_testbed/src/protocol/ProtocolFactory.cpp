#include "ProtocolFactory.h"
#include "DummyProtocol.h"
#include "OpenSSLProtocolTLS.h"
#include "OpenSSLProtocolDTLS.h"
#include "MvfstProtocolQUIC.h"
#include "OSProtocolTCP.h"
#include "OSProtocolUDP.h"

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
        case ProtocolType::TCP:
            return std::make_shared<OSProtocolTCP>();
        case ProtocolType::UDP:
            return std::make_shared<OSProtocolUDP>();
        default:
            return nullptr;
    }
}
