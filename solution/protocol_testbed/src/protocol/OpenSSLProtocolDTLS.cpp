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

#include "../appProto/utils.h"

#define PACKET_SIZE_LEN 2
#define SEQ_NO_LEN 4

bool Protocol::OpenSSLProtocolDTLS::openProtocol(std::string address, uint port) {
    SSL_load_error_strings();
    ERR_load_crypto_strings();

    OpenSSL_add_all_algorithms();
    SSL_library_init();

    _socket = socket(AF_INET, SOCK_DGRAM, 0);

    int flags = fcntl(_socket,F_GETFL);
    flags |= O_NONBLOCK;

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
    int err = SSL_connect(_ssl);
    if (err <= 0) {
        std::cout << "Error creating SSL connection.  err=" << err << std::endl;
        fflush(stdout);
        return -1;
    }
    //fcntl(_socket, F_SETFL, flags);

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

    ushort sequenceNo = getNextSequence();
    size_t sendPacketSize = PACKET_SIZE_LEN + SEQ_NO_LEN + bufferSize;

    char* sendPacket = new char[sendPacketSize];

    //std::cout << "Sending seq no: " << sequenceNo << std::endl;

    Utils::writeToByteArray(sendPacket, 0, Utils::convertToCharArray(sendPacketSize, PACKET_SIZE_LEN));
    Utils::writeToByteArray(sendPacket, PACKET_SIZE_LEN, Utils::convertToCharArray(sequenceNo, SEQ_NO_LEN));
    memcpy(sendPacket + PACKET_SIZE_LEN + SEQ_NO_LEN, buffer, bufferSize);

    std::vector<unsigned char> rawBytes((unsigned char*)sendPacket,(unsigned char*) (sendPacket + bufferSize));

    int len = SSL_write(_ssl, sendPacket, sendPacketSize);
    if (len < 0) {
        int err = SSL_get_error(_ssl, len);
        switch (err) {
            // TODO proper LOGGING for this:
            case SSL_ERROR_WANT_WRITE:
            case SSL_ERROR_WANT_READ:
            case SSL_ERROR_ZERO_RETURN:
            case SSL_ERROR_SYSCALL:
            case SSL_ERROR_SSL:
            default:
                return false;
        }
    }

    // Receiving reply from server:
    // Check if there's anything in the pipe
    unsigned char buf[100];
    len = SSL_read(_ssl, buf, 100);
    if (len < 0) {
        int err = SSL_get_error(_ssl, len);
        switch (err) {
            case SSL_ERROR_WANT_READ:
            case SSL_ERROR_WANT_WRITE:
            case SSL_ERROR_ZERO_RETURN:
            case SSL_ERROR_SYSCALL:
            case SSL_ERROR_SSL:
            default:
                return false;
        }
    }
    bool matchingSeq = false;
    uint ackSeq;
    if (len > 0) {
        std::vector<unsigned char> ackStream(buf, buf + 100);
        // Response is two byte pairs of ACKed packet seq numbers
        for (uint i = 0; i < len; i += SEQ_NO_LEN) {
            ackSeq = Utils::convertToUnsignedTemplated<ushort>(ackStream, i, SEQ_NO_LEN);
            //std::cout << "Got ack for " << ackSeq << std::endl;
            matchingSeq |= (ackSeq == sequenceNo);
        }
    }

    if (!matchingSeq) {
        std::cerr << "No matching seq no got for " << matchingSeq << ". Last seq arrived: " << ackSeq << std::endl;
        return false;
    }

    delete[] sendPacket;
    return true;
}

bool Protocol::OpenSSLProtocolDTLS::closeProtocol() {
    cleanUp();
    return true;
}

Protocol::OpenSSLProtocolDTLS::~OpenSSLProtocolDTLS() {
    cleanUp();
}

bool Protocol::OpenSSLProtocolDTLS::cleanUp() {
    if (_socketInitialized) {
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
