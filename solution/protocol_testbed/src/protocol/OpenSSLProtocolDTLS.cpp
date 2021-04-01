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

bool Protocol::OpenSSLProtocolDTLS::openProtocol(std::string address, uint port) {
    SSL_load_error_strings();
    ERR_load_crypto_strings();

    OpenSSL_add_all_algorithms();
    SSL_library_init();

    _socket = socket(AF_INET, SOCK_DGRAM, 0);

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
    timeout.tv_sec = 5;
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

    _initialized = true;
    return true;
}

bool Protocol::OpenSSLProtocolDTLS::send(const char *buffer, size_t bufferSize, bool eof) {
    if (!_initialized) {
        return false;
    }

    int len = SSL_write(_ssl, buffer, bufferSize);
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

    // TODO figure out what the server should reply with
    // Technically it doesn't need to reply anything
    // But a signal that the data is okay would be nice

//    char buf[1000000];
//    do {
//        len=SSL_read(_ssl, buf, 100);
//        buf[len]=0;
//    } while (len > 0);
//    if (len < 0) {
//        int err = SSL_get_error(_ssl, len);
//        switch (err) {
//            case SSL_ERROR_WANT_READ:
//            case SSL_ERROR_WANT_WRITE:
//            case SSL_ERROR_ZERO_RETURN:
//            case SSL_ERROR_SYSCALL:
//            case SSL_ERROR_SSL:
//            default:S
//                return false;
//        }
//    }
    return true;
}

bool Protocol::OpenSSLProtocolDTLS::closeProtocol() {
    cleanUp();
}

Protocol::OpenSSLProtocolDTLS::~OpenSSLProtocolDTLS() {
    cleanUp();
}

bool Protocol::OpenSSLProtocolDTLS::cleanUp() {
    if (_socketInitialized)
    {
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
