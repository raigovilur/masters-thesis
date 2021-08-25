#include "OpenSSLProtocolDTLS.h"
#include <openssl/ssl.h>
#include <openssl/conf.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <arpa/inet.h>
#include <iostream>
#include <resolv.h>
#include <cstring>
#include <unistd.h>
#include <algorithm>
#include <thread>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <fcntl.h>
#include <chrono>

#include "../appProto/utils.h"

#define PACKET_SIZE_LEN 2
#define SEQ_NO_LEN 4

bool Protocol::OpenSSLProtocolDTLS::openProtocol(std::string address, uint port, Options options) {
    _options = options;
    SSL_load_error_strings();
    ERR_load_crypto_strings();

    OpenSSL_add_all_algorithms();
    SSL_library_init();

    _socket = socket(AF_INET, SOCK_DGRAM, 0);

    int flags = fcntl(_socket, F_GETFL, 0);
    flags = flags|O_NONBLOCK;
    fcntl(_socket, F_SETFL, flags);

    struct timeval read_timeout{};
    read_timeout.tv_sec = 0;
    read_timeout.tv_usec = 10;
    setsockopt(_socket, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof read_timeout);

    sockaddr_in saiServerAddress{};
    memset(&saiServerAddress, 0, sizeof(saiServerAddress));
    saiServerAddress.sin_family = AF_INET;
    saiServerAddress.sin_addr.s_addr = inet_addr(address.c_str());
    saiServerAddress.sin_port = htons(port);

    if (connect(_socket, (struct sockaddr *)&saiServerAddress, sizeof(saiServerAddress))) {
        std::cerr << "Error connecting to server " << address << std::endl;
        return false;
    }
    _socketInitialized = true;



    SSL_library_init();
    SSLeay_add_ssl_algorithms();
    SSL_load_error_strings();
    const SSL_METHOD *meth = DTLS_client_method();
    SSL_CTX *ctx = SSL_CTX_new(meth);

    if (ctx == nullptr)
    {
        std::cout << "Unable to get context" << std::endl;
        return false;
    }
    SSL_CTX_set_options(ctx, SSL_OP_NO_DTLSv1); // We are only interested in DTLS 1.2
    //SSL_CTX_set_ciphersuites(ctx, "TLS_CHACHA20_POLY1305_SHA256");
    //SSL_CTX_set_ciphersuites(ctx, "TLS_CHACHA20_POLY1305_SHA256");
    _ssl = SSL_new(ctx);



    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    BIO_ctrl(SSL_get_rbio(_ssl), BIO_CTRL_DGRAM_SET_RECV_TIMEOUT, 0, &timeout);
    if (!_ssl) {
        printf("Error creating SSL.\n");
        return -1;
    }
    _sockFd = SSL_get_fd(_ssl);
    SSL_set_fd(_ssl, _socket);
    bool isConnected = false;
    while (!isConnected) {
        int err = SSL_connect(_ssl);
        if (err <= 0) {
            int errorReturned;
            switch (SSL_get_error(_ssl, err)) {
                case SSL_ERROR_NONE:
                    isConnected = true;
                    break;
                case SSL_ERROR_WANT_READ:
                    //std::cout << "Want more read on connect  err=" << err << std::endl;
                    break;
                case SSL_ERROR_WANT_X509_LOOKUP:
                case SSL_ERROR_WANT_WRITE:
                case SSL_ERROR_ZERO_RETURN:
                case SSL_ERROR_SSL:
                case SSL_ERROR_SYSCALL:
                    std::cout << "Error creating SSL connection.  err=" << err << std::endl;
                    fflush(stdout);
                    return -1;
            }
        }
        else {
            isConnected = true;
        }
    }

    _initialized = true;
    return true;
}

bool Protocol::OpenSSLProtocolDTLS::send(const char *buffer, size_t bufferSize, bool eof) {
    if (!_initialized) {
        return false;
    }

    static bool sizeWarningDisplayed = false;
    if (!sizeWarningDisplayed && bufferSize > 50000) {
        std::cout << "Exceeds recommended maximum buffer size" << std::endl;
    }
    sizeWarningDisplayed = true;

//    if (_notAckedPackets.size() > 15) {
//        static const int maxTime = 1000;
//        auto waitingTime = std::chrono::system_clock::now();
//        while (_notAckedPackets.size() > 5) {
//            getAcks();
//            if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - waitingTime).count() > maxTime) {
//                std::cout << "Waited for "<< maxTime <<" milliseconds. Acks still here:  " << _notAckedPackets.size() << std::endl;
//                break;
//            }
//        }
//    }

    //Flow control:
    // We are trying to keep to 30 mb/s speed, because that's the maximum our link can handle
    // So don't send out more than that
    double target = _options.UDPtarget;
    /*
        1024 * 8 / (1000 * 30mb/s) 
        = 1024 * 8(bit) / 30000 kb/s
        = 1024 * 8(bit) / 30kb/ms
        = 1024 * 8(bit) / 30000bit/ms
        = 0.273 ms
    */
    uint millisNeededForTarget = ((double) bufferSize * 8) / 1000 / target; //1024 * 8 / (1000 * 30mb/s) = 1024 * 8(bit) / 30000 kb/s
    
    /*
    if (!_firstPacket) {
        // do congestion control
        uint millisecondsTakenToGetHere = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - _lastSendTimestamp).count();
        int diff = millisNeededForTarget - millisecondsTakenToGetHere;
        //std::cout << "diff:" << diff << std::endl;
        if (diff > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(diff));
        }
    } else {
        _firstPacket = false;
    }
    */

    _lastSendTimestamp = std::chrono::system_clock::now();

    //ushort sequenceNo = getNextSequence();
    //size_t sendPacketSize = PACKET_SIZE_LEN + SEQ_NO_LEN + bufferSize;
//    _notAckedPackets.push_back(sequenceNo);
//    _timeStamps.emplace(
//            std::pair<ushort, std::chrono::time_point<std::chrono::system_clock>>(
//                    sequenceNo, std::chrono::system_clock::now()
//                    ));

    //char* sendPacket = new char[sendPacketSize];

    //std::cout << "Sending seq no: " << sequenceNo << std::endl;

    //Utils::writeToByteArray(sendPacket, 0, Utils::convertToCharArray(sendPacketSize, PACKET_SIZE_LEN));
    //Utils::writeToByteArray(sendPacket, PACKET_SIZE_LEN, Utils::convertToCharArray(sequenceNo, SEQ_NO_LEN));
    //memcpy(sendPacket + PACKET_SIZE_LEN + SEQ_NO_LEN, buffer, bufferSize);

    //std::vector<unsigned char> rawBytes((unsigned char*)sendPacket,(unsigned char*) (sendPacket + bufferSize));

    while (true) {
        int len = SSL_write(_ssl, buffer, bufferSize);
        if (len < 0) {
            int err = SSL_get_error(_ssl, len);
            switch (err) {
                // TODO proper LOGGING for this:
                case SSL_ERROR_WANT_WRITE:
                    break;
                case SSL_ERROR_WANT_READ:
                case SSL_ERROR_ZERO_RETURN:
                case SSL_ERROR_SYSCALL:
                case SSL_ERROR_SSL:
                default:
                    return false;
            }
        } else {
            break;
        }
    }

    getAcks();
    // Receiving reply from server:
    // Check if there's anything in the pipe
//    unsigned char buf[100];
//    len = SSL_read(_ssl, buf, 100);
//    if (len < 0) {
//        int err = SSL_get_error(_ssl, len);
//        switch (err) {
//            case SSL_ERROR_WANT_READ:
//                // Nothing to read.
//                return true;
//            case SSL_ERROR_WANT_WRITE:
//            case SSL_ERROR_ZERO_RETURN:
//            case SSL_ERROR_SYSCALL:
//            case SSL_ERROR_SSL:
//            default:
//                return true;
//        }
//    }
//    bool matchingSeq = false;
//    uint ackSeq;
//    if (len > 0) {
//        std::vector<unsigned char> ackStream(buf, buf + 100);
//        // Response is two byte pairs of ACKed packet seq numbers
//        for (uint i = 0; i < len; i += SEQ_NO_LEN) {
//            ackSeq = Utils::convertToUnsignedTemplated<ushort>(ackStream, i, SEQ_NO_LEN);
//            //std::cout << "Got ack for " << ackSeq << std::endl;
//            matchingSeq |= (ackSeq == sequenceNo);
//        }
//    }
//
//    if (!matchingSeq) {
//        std::cerr << "No matching seq no got for " << matchingSeq << ". Last seq arrived: " << ackSeq << std::endl;
//        return false;
//    }

    //delete[] sendPacket;
    return true;
}

bool Protocol::OpenSSLProtocolDTLS::closeProtocol() {
    sleep(10);
    cleanUp();
    return true;
}

Protocol::OpenSSLProtocolDTLS::~OpenSSLProtocolDTLS() {
    cleanUp();
}

bool Protocol::OpenSSLProtocolDTLS::cleanUp() {
    if (_socketInitialized) {
        sleep(5);
        SSL_free(_ssl);
        _initialized = false;

        _socketInitialized = false;
        OPENSSL_cleanup();
        close(_socket);
        _ssl = nullptr;
    }
    return true;
}

bool Protocol::OpenSSLProtocolDTLS::openProtocolServer(uint port) {
    return false;
}

uint Protocol::OpenSSLProtocolDTLS::getNextSequence() {
    static uint sequence = -1;
    ++sequence;

    return sequence % (1L << (8 * SEQ_NO_LEN)); // Security measure in case unsigned is more than four bytes
}

void Protocol::OpenSSLProtocolDTLS::getAcks() {
    return;
    // Receiving reply from server:
    // Check if there's anything in the pipe
    while(true) {
        unsigned char buf[20];
        int len = SSL_read(_ssl, buf, 20);
        if (len <= 0) {
            int err = SSL_get_error(_ssl, len);
            cleanStaleAcks();
            switch (err) {
                case SSL_ERROR_WANT_READ:
                    //std::cout << "Nothing to read " << std::endl;
                    // Nothing to read.
                    return;
                case SSL_ERROR_WANT_WRITE:
                case SSL_ERROR_ZERO_RETURN:
                case SSL_ERROR_SYSCALL:
                case SSL_ERROR_SSL:
                default:
                    return;
            }
        }
        if (len > 0) {
            std::vector<unsigned char> ackStream(buf, buf + 20);
            // Response is four byte sets of ACKed packet seq numbers
            ushort ackSeq;
            for (uint i = 0; i < len; i += SEQ_NO_LEN) {
                ackSeq = Utils::convertToUnsignedTemplated<ushort>(ackStream, i, SEQ_NO_LEN);
                std::cout << "Got ack for " << ackSeq << ". Currently waiting acks for: " << _notAckedPackets.size()
                          << std::endl;
                auto it = std::find(_notAckedPackets.begin(), _notAckedPackets.end(), ackSeq);
                if (it == _notAckedPackets.end()) {
                    std::cout << "Error: sequence " << ackSeq << " not in list of sequences sent out" << std::endl;
                } else {
                    _notAckedPackets.erase(it);
                    _timeStamps.erase(ackSeq);
                }
            }
        }
    }
}

bool Protocol::OpenSSLProtocolDTLS::isAllSent() {
    return true;
    static const int maxTime = 2;
    auto waitingTime = std::chrono::system_clock::now();
    while (!_notAckedPackets.empty()) {
        getAcks();
        if (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - waitingTime).count() > maxTime) {
            std::cout << "Waited for "<< maxTime <<" seconds. Acks still here:  " << _notAckedPackets.size() << std::endl;
            break;
        }
    }
    return true;
}

void Protocol::OpenSSLProtocolDTLS::cleanStaleAcks() {
    static const int ttlSeconds = 3;
    for (auto element : _timeStamps) {
        if (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - element.second).count() > ttlSeconds) {
            std::cout << "Packet: " << element.first << " has outlived it's max waiting time of " << ttlSeconds << " seconds. " << std::endl;
            auto it = std::find(_notAckedPackets.begin(), _notAckedPackets.end(), element.first);
            if (it == _notAckedPackets.end()) {
                std::cout << "Error cleaning stale packets: sequence " << element.first << " not in list of sequences sent out" << std::endl;
            } else {
                _notAckedPackets.erase(it);
                _timeStamps.erase(element.first);
            }
        }
    }
}
