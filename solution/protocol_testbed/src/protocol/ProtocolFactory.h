#ifndef PROTOCOL_TESTBED_SERVERPROTOCOLFACTORY_H
#define PROTOCOL_TESTBED_PROTOCOLFACTORY_H


#include <memory>
#include "ProtocolInterface.h"
#include "appProto/ProtocolType.h"

namespace Protocol{
    class ProtocolFactory {
    public:
        static ProtocolPtr getInstance(ProtocolType type);
    };
}

#endif //PROTOCOL_TESTBED_SERVERPROTOCOLFACTORY_H
