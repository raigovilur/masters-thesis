#include "ProtocolFactory.h"
#include "DummyProtocol.h"
#include "OpenSSLProtocolTLS.h"

Protocol::ProtocolPtr Protocol::ProtocolFactory::getInstance(Protocol::ProtocolType type) {
    switch (type) {
        case ProtocolType::Dummy:
            return std::make_shared<DummyProtocol>();
        case ProtocolType::OpenSSL_TLS:
            return std::make_shared<OpenSSLProtocolTLS>();
        default:
            return nullptr;
    }
}
