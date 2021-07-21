#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <iostream>
#include <arpa/inet.h>
#include <sys/unistd.h>
#include "ServerOSProtocolTCP.h"

bool ServerProto::ServerOSProtocolTCP::serverListen(const std::string &address, ushort port) {
    _socket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(_socket, (struct sockaddr*)&addr, sizeof(addr)) != 0 )
    {
        perror("can't bind port");
        abort();
    }

    if (listen(_socket, 5) != 0 )
    {
        perror("Can't configure listening port");
        abort();
    }
    while (true)
    {
        struct sockaddr_in addr;
        socklen_t len = sizeof(addr);
        int client = accept(_socket, (struct sockaddr*)&addr, &len);  /* accept connection as usual */
        if (client < 0) {
            std::cout << "Error accepting connection: " <<  strerror(errno) << std::endl;
            continue;
        }
        std::cout << "Connection: " << inet_ntoa(addr.sin_addr) << ":" << ntohs(addr.sin_port) << std::endl;
        unsigned char buf[1024] = {0};
        size_t readAmount = 1;
        while (readAmount > 0)
        {
             readAmount = read(client, buf, sizeof(buf));
            _callback->consume(std::string(inet_ntoa(addr.sin_addr)), buf, readAmount);
        }
        close(client);
    }
    close(_socket);

    return true;
}

ServerProto::ServerOSProtocolTCP::~ServerOSProtocolTCP() {

}

bool ServerProto::ServerOSProtocolTCP::setCertificate(const char *certificate, size_t len) {
    return true;
}

bool ServerProto::ServerOSProtocolTCP::setPrivateKey(const char *pKey, size_t len) {
    return true;
}
