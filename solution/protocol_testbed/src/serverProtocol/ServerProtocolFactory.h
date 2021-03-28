#ifndef PROTOCOL_TESTBED_SERVERPROTOCOLFACTORY_H
#define PROTOCOL_TESTBED_SERVERPROTOCOLFACTORY_H

#include "../appProto/ProtocolType.h"
#include "ServerInterface.h"

namespace ServerProto {
    class ServerProtocolFactory {
    public:
        static ServerPtr getInstance(Protocol::ProtocolType type);
    };
}


#endif //PROTOCOL_TESTBED_SERVERPROTOCOLFACTORY_H
