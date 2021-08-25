
#include <unistd.h>
#include <cstring>
#include <arpa/inet.h>
#include <iostream>
#include <thread>
#include "OSProtocolUDP.h"

bool Protocol::OSProtocolUDP::openProtocol(std::string address, uint port, uint cipher, Protocol::Options options) {
    _options = options;
    _socket = socket(AF_INET, SOCK_DGRAM, 0);
    memset(&saiServerAddress, 0, sizeof(saiServerAddress));
    saiServerAddress.sin_family = AF_INET;
    saiServerAddress.sin_addr.s_addr = inet_addr(address.c_str());
    saiServerAddress.sin_port = htons(port);
    return true;
}

bool Protocol::OSProtocolUDP::openProtocolServer(uint port) {
    return false;
}

bool Protocol::OSProtocolUDP::send(const char *buffer, size_t bufferSize, bool eof) {
    double target = _options.UDPtarget;
    int microsNeededForTarget = ((double) bufferSize * 8) / target / 2;

    if (!_firstPacket) {
        // do congestion control
        int microsecondsTakenToGetHere = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - _lastSendTimestamp).count();
        int diff = microsNeededForTarget - microsecondsTakenToGetHere;
        if (diff > 0) {
            std::this_thread::sleep_for(std::chrono::microseconds(diff));
        }
    } else {
        _firstPacket = false;
    }

    _lastSendTimestamp = std::chrono::system_clock::now();

    sendto(_socket, buffer, bufferSize, MSG_CONFIRM, (const struct sockaddr *) &saiServerAddress, sizeof(saiServerAddress));
    return true;
}

bool Protocol::OSProtocolUDP::closeProtocol() {
    close(_socket);
    return true;
}

Protocol::OSProtocolUDP::~OSProtocolUDP() {

}
