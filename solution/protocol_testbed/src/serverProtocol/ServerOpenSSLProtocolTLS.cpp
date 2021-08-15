#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
//#include <assuan.h>
#include <evdns.h>
//#include <armadillo>
#include <folly/ssl/OpenSSLCertUtils.h>
#include <arpa/inet.h>
#include <csignal>
#include <iostream>

#include "ServerOpenSSLProtocolTLS.h"

namespace {
#define CONNECTION_POOLING_AMOUNT 10

    EVP_PKEY* getPrivateKey(const char *pKey, size_t pkLen) {
        folly::ssl::BioUniquePtr bio(BIO_new(BIO_s_mem()));
        CHECK(bio);
        CHECK_EQ(BIO_write(bio.get(), pKey, pkLen), pkLen);
        EVP_PKEY* pkey = PEM_read_bio_PrivateKey(bio.get(), nullptr, nullptr, nullptr);
        CHECK(pkey);
        return pkey;
    }

//        EvpPkeyUniquePtr getPublicKey(StringPiece key) {
//            BioUniquePtr bio(BIO_new(BIO_s_mem()));
//            CHECK(bio);
//            CHECK_EQ(BIO_write(bio.get(), key.data(), key.size()), key.size());
//            EvpPkeyUniquePtr pkey(
//                    PEM_read_bio_PUBKEY(bio.get(), nullptr, nullptr, nullptr));
//            CHECK(pkey);
//            return pkey;
//        }

    X509* getCert(const char *certificate, size_t certLen) {
        folly::ssl::BioUniquePtr bio(BIO_new(BIO_s_mem()));
        CHECK(bio);
        CHECK_EQ(BIO_write(bio.get(), certificate, certLen), certLen);
        X509* x509 = PEM_read_bio_X509(bio.get(), nullptr, nullptr, nullptr);
        CHECK(x509);
        return x509;
    }
}


bool ServerProto::ServerOpenSSLProtocolTLS::serverListen(const std::string &address, ushort port) {
    SSL_CTX *ctx;

    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    const SSL_METHOD* method = TLS_server_method();
    ctx = SSL_CTX_new(method);
    SSL_CTX_set_options(ctx, SSL_OP_NO_TLSv1_1 | SSL_OP_NO_TLSv1_2); // We are only interested in TLS1.3
    if ( ctx == NULL )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }

    if ( SSL_CTX_use_certificate(ctx, _cert) <= 0 )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    if ( SSL_CTX_use_PrivateKey(ctx, _pKey) <= 0 )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    /* verify private key */
    if ( !SSL_CTX_check_private_key(ctx) )
    {
        fprintf(stderr, "Private key does not match the public certificate\n");
        abort();
    }
    struct sockaddr_in addr;
    int socketFd = socket(PF_INET, SOCK_STREAM, 0);
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY; // TODO set to address variable
    if (bind(socketFd, (struct sockaddr*)&addr, sizeof(addr)) != 0 )
    {
        perror("can't bind port");
        abort();
    }
    if (listen(socketFd, CONNECTION_POOLING_AMOUNT) != 0 )
    {
        perror("Can't configure listening port");
        abort();
    }

    signal(SIGPIPE, SIG_IGN);
    while (true)
    {
        struct sockaddr_in addr;
        socklen_t len = sizeof(addr);
        SSL *ssl;
        int client = accept(socketFd, (struct sockaddr*)&addr, &len);  /* accept connection as usual */
        if (client < 0) {
            std::cout << "Error accepting connection: " <<  strerror(errno) << std::endl;
            continue;
        }
        std::cout << "Connection: " << inet_ntoa(addr.sin_addr) << ":" << ntohs(addr.sin_port) << std::endl;
        ssl = SSL_new(ctx);              /* get new SSL state with context */
        SSL_set_fd(ssl, client);      /* set connection socket to SSL state */
        serviceConnection(&addr, ssl);         /* service connection */
    }
    close(socketFd);
    SSL_CTX_free(ctx);
}

bool ServerProto::ServerOpenSSLProtocolTLS::setCertificate(const char *certificate, size_t len) {
    _cert = getCert(certificate, len);
    return true;
}

bool ServerProto::ServerOpenSSLProtocolTLS::setPrivateKey(const char *pKey, size_t len) {
    _pKey = getPrivateKey(pKey, len);
    return true;
}

ServerProto::ServerOpenSSLProtocolTLS::~ServerOpenSSLProtocolTLS() {
    EVP_PKEY_free(_pKey);
    _pKey = nullptr;
    X509_free(_cert);
    _cert = nullptr;
}

bool ServerProto::ServerOpenSSLProtocolTLS::serviceConnection(const sockaddr_in* addr, SSL* ssl) {
    unsigned char buf[1024] = {0}; //1024Byte
    int sd = 0, bytes = 0;
    if (SSL_accept(ssl) <= 0)     /* do SSL-protocol accept */
        ERR_print_errors_fp(stderr);
    else {
        while (bytes >= 0) {
            bytes = SSL_read(ssl, buf, sizeof(buf)); /* get request */
            if ( bytes > 0 ) {
                _callback->consume(std::string(inet_ntoa(addr->sin_addr)), buf, bytes);
            } else {
                ERR_print_errors_fp(stderr);
            }
        }
    }
    sd = SSL_get_fd(ssl);       /* get socket connection */
    SSL_free(ssl);         /* release SSL state */
    close(sd);          /* close connection */
    return true;
}
