#include "ServerOSProtocolUDP.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <iostream>
#include <arpa/inet.h>
#include <sys/unistd.h>

ServerProto::ServerOSProtocolUDP::~ServerOSProtocolUDP() {

}

bool ServerProto::ServerOSProtocolUDP::serverListen(const std::string &address, ushort port, uint cipher) {
    _socket = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    int a = 0;
    uint m = sizeof(a);
    getsockopt(_socket, SOL_SOCKET, SO_RCVBUF, (void *) &a, &m);
    std::cout << "Socket receive buffer size " << a << std::endl;
    long sockSize = 100000000;
    setsockopt(_socket, SOL_SOCKET, SO_RCVBUF, (const void *) &sockSize, sizeof(sockSize));
    getsockopt(_socket, SOL_SOCKET, SO_RCVBUF, (void *) &a, &m);
    std::cout << "Socket receive buffer size " << a << std::endl;
    if (bind(_socket, (struct sockaddr*)&addr, sizeof(addr)) != 0 )
    {
        perror("can't bind port");
        abort();
    }
    while (true)
    {
        struct sockaddr_in addr{};
        socklen_t len = sizeof(addr);
        unsigned char buf[102400] = {0};
        size_t readAmount = 1;
        while (readAmount > 0)
        {
            readAmount = recvfrom(_socket, buf, sizeof(buf), 0, (struct sockaddr *) &addr, &len);
            _callback->consume(std::string(inet_ntoa(addr.sin_addr)), buf, readAmount);
        }
    }
    close(_socket);

    return true;
}

bool ServerProto::ServerOSProtocolUDP::setCertificate(const char *certificate, size_t len) {
    return true;
}

bool ServerProto::ServerOSProtocolUDP::setPrivateKey(const char *pKey, size_t len) {
    return true;
}
