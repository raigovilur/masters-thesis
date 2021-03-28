#ifndef PROTOCOL_TESTBED_SERVER_H
#define PROTOCOL_TESTBED_SERVER_H

#include <cstddef>
#include "serverProtocol/ServerInterface.h"
#include "appProto/ProtocolType.h"

class Server : public ServerProto::ServerReceiveCallback {
public:
    bool start(Protocol::ProtocolType type, std::string listenAddress, ushort port);

    bool consume(char *bytes, size_t length) override;

public:



};


#endif //PROTOCOL_TESTBED_SERVER_H
