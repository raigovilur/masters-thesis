//
// Created by raigo on 07.03.21.
//

#include "DummyProtocol.h"

#include <iostream>
#include <thread>
#include <random>

Protocol::DummyProtocol::DummyProtocol() {
    std::cout << "Dummy protocol object created" << std::endl;
}

Protocol::DummyProtocol::~DummyProtocol() {

}

bool Protocol::DummyProtocol::send(const char *buffer, size_t bufferSize) {
    std::cout << "Sending " << bufferSize << " bytes" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    // Sometimes this dummy will randomly drop connection:
    if (_dropsConnection) {
        std::random_device dev;
        std::mt19937 rng(dev());
        std::uniform_int_distribution<std::mt19937::result_type> distribution(0,100);
        if (distribution(rng) < _probabilityOfConnectionLoss) {
            std::cout << "Oops, dummy dropped connection!" << std::endl;
            return false;
        }
    }
    return true;
}

bool Protocol::DummyProtocol::openProtocol(std::string address, uint port) {
    std::cout << "Connecting to " << address << ":" << port << " via dummy interface" << std::endl;
    return true;
}

bool Protocol::DummyProtocol::closeProtocol() {
    std::cout << "Closing connection via dummy interface" << std::endl;
    return true;
}

bool Protocol::DummyProtocol::openProtocolServer(uint port) {
    return false;
}
