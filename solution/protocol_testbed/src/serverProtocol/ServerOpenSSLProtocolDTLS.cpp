
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <assuan.h>
#include <evdns.h>
#include <armadillo>
#include <folly/ssl/OpenSSLCertUtils.h>
#include <arpa/inet.h>
#include <netinet/ip.h>

#include "../dtlsCookieVault/ck_secrets_vault.h"
#include "ServerOpenSSLProtocolDTLS.h"

#define COOKIE_SECRET_LENGTH 16

namespace {
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

namespace {
    bool cookieInitialized = false;
}

/* Certificate verification. Returns 1 if trusted, else 0 */
int verifyCert(int ok, X509_STORE_CTX *ctx) {
    return 1;
}


int generateCookie(SSL *ssl, unsigned char *cookie, unsigned int *cookie_len) {
    static bool vaultGenerated = false;

    unsigned char *buffer, result[EVP_MAX_MD_SIZE];
    unsigned int length = 0, resultLength;

    if (!vaultGenerated) {
        ck_secrets_generate(CK_SECRET_MAX);
    }

    struct sockaddr_in clientAddr = {0};

    /* Read peer information */
    BIO_dgram_get_peer(SSL_get_rbio(ssl), &clientAddr);

    /* Create buffer with peer's address and port */
    length =  sizeof(in_addr);
    length += sizeof(in_port_t);
    buffer = (unsigned char*) OPENSSL_malloc(length);

    if (buffer == NULL)
    {
        printf("out of memory\n");
        return 0;
    }

    memcpy(buffer,
           &clientAddr.sin_port,
           sizeof(in_port_t));
    memcpy(buffer + sizeof(clientAddr.sin_port),
           &clientAddr.sin_addr,
           sizeof(struct in_addr));

    buffer = (unsigned char*) OPENSSL_malloc(length);
    HMAC(EVP_sha1(), (const void*) ck_secrets_random(), COOKIE_SECRET_LENGTH,
         (const unsigned char*) buffer, length, result, &resultLength);

    OPENSSL_free(buffer);

    memcpy(cookie, result, resultLength);
    *cookie_len = resultLength;
    return 1;
}

int verifyCookie(SSL *ssl, const unsigned char *cookie, unsigned int cookie_len) {
    unsigned char *buffer, result[EVP_MAX_MD_SIZE];
    unsigned int length = 0, resultLength;
    struct sockaddr_in clientAddr = {0};
    return 1;
    /* Read peer information */
    BIO_dgram_get_peer(SSL_get_rbio(ssl), &clientAddr);

    /* Create buffer with peer's address and port */
    length =  sizeof(in_addr);
    length += sizeof(in_port_t);
    buffer = (unsigned char*) OPENSSL_malloc(length);

    if (buffer == NULL)
    {
        printf("out of memory\n");
        return 0;
    }

    memcpy(buffer,
           &clientAddr.sin_port,
           sizeof(in_port_t));
    memcpy(buffer + sizeof(clientAddr.sin_port),
           &clientAddr.sin_addr,
           sizeof(struct in_addr));

    int returnValue;
    if( ck_secrets_exist(buffer, length, cookie, cookie_len) == 1) {
        returnValue = 1;
    } else {
        returnValue = 0;
    }

    OPENSSL_free(buffer);
    return returnValue;
}
[[noreturn]] bool ServerProto::ServerOpenSSLProtocolDTLS::serverListen(const std::string &address, ushort port) {
    SSL_CTX *ctx;

    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    const SSL_METHOD* method = DTLS_server_method();
    ctx = SSL_CTX_new(method);
    SSL_CTX_set_options(ctx, SSL_OP_NO_DTLSv1); // We are only interested in DTLS1.2
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

    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, verifyCert);
    SSL_CTX_set_cookie_generate_cb(ctx, generateCookie);
    SSL_CTX_set_cookie_verify_cb(ctx, verifyCookie);

    struct sockaddr_in serverAddr;
    int serverFd = socket(AF_INET, SOCK_DGRAM, 0);
    int reuse = 1;
    if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
    }
    bzero(&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = INADDR_ANY; // TODO set to address variable
    if (bind(serverFd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) != 0 )
    {
        perror("Can't bind port");
        abort();
    }

    while (true)
    {
        BIO *bio = BIO_new_dgram(serverFd, BIO_NOCLOSE);

        struct timeval timeout;
        timeout.tv_sec = 3;
        timeout.tv_usec = 0;
        BIO_ctrl(bio, BIO_CTRL_DGRAM_SET_RECV_TIMEOUT, 0, &timeout);
        SSL* ssl = SSL_new(ctx);
        SSL_set_bio(ssl, bio, bio);
        SSL_set_options(ssl, SSL_OP_COOKIE_EXCHANGE);

        union {
            struct sockaddr_storage ss;
            struct sockaddr_in s4;
            struct sockaddr_in6 s6;
        } clientAddr;

        memset(&clientAddr, 0, sizeof(clientAddr));

        while (DTLSv1_listen(ssl, (BIO_ADDR *) &clientAddr) <= 0);

        int clientFd = socket(AF_INET, SOCK_DGRAM, 0);
        if (clientFd < 0) {
            std::cout << "Error accepting connection (client socket): " <<  strerror(errno) << std::endl;
            continue;
        }
        if (setsockopt(clientFd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
            perror("setsockopt(SO_REUSEADDR) failed");
        }
        std::cout << "Connection: " << inet_ntoa(clientAddr.s4.sin_addr) << ":" << ntohs(clientAddr.s4.sin_port) << std::endl;
        if (bind(clientFd, (struct sockaddr *) &serverAddr, sizeof(struct sockaddr_in))) {
            perror("bind to clientFd");
            close(clientFd);
            SSL_free(ssl);
            continue;
        }
        if (connect(clientFd, (struct sockaddr *) &clientAddr, sizeof(struct sockaddr_in))) {
            perror("connect");
            close(clientFd);
            SSL_free(ssl);
            continue;
        }

        BIO_set_fd(SSL_get_rbio(ssl), clientFd, BIO_NOCLOSE);
        BIO_ctrl(SSL_get_rbio(ssl), BIO_CTRL_DGRAM_SET_CONNECTED, 0, &clientAddr.s4.sin_addr);

        SSL_set_fd(ssl, clientFd);

        //TODO BIO_ctrl(SSL_get_rbio(ssl), BIO_CTRL_DGRAM_SET_RECV_TIMEOUT, 0, &timeout);


        serviceConnection(clientFd, &clientAddr.s4, ssl);
    }
    close(serverFd);
    SSL_CTX_free(ctx);
}

bool ServerProto::ServerOpenSSLProtocolDTLS::setCertificate(const char *certificate, size_t len) {
    _cert = getCert(certificate, len);
    return true;
}

bool ServerProto::ServerOpenSSLProtocolDTLS::setPrivateKey(const char *pKey, size_t len) {
    _pKey = getPrivateKey(pKey, len);
    return true;
}

ServerProto::ServerOpenSSLProtocolDTLS::~ServerOpenSSLProtocolDTLS() {
    EVP_PKEY_free(_pKey);
    _pKey = nullptr;
    X509_free(_cert);
    _cert = nullptr;
}

bool ServerProto::ServerOpenSSLProtocolDTLS::serviceConnection(int clientFd, const sockaddr_in* addr, SSL* ssl) {
    //BIO *clientBio = SSL_get_rbio(ssl);
    //BIO_set_fd(clientBio, clientFd, BIO_NOCLOSE);
    //BIO_ctrl(clientBio, BIO_CTRL_DGRAM_SET_CONNECTED, 0, &addr);
    SSL_accept(ssl);

    unsigned char buf[1024] = {0};
    int sd = 0, bytes = 0;
    if (SSL_accept(ssl) <= 0)     /* do SSL-protocol accept */
        ERR_print_errors_fp(stderr);
    else {
        while (!(SSL_get_shutdown(ssl) & SSL_RECEIVED_SHUTDOWN)) {
            bytes = SSL_read(ssl, buf, sizeof(buf)); /* get request */
            if ( bytes > 0 ) {
                _callback->consume(std::string(inet_ntoa(addr->sin_addr)), buf, bytes);
            } else {
                ERR_print_errors_fp(stderr);
            }
        }
    }
    sd = SSL_get_fd(ssl);
    SSL_free(ssl);
    close(sd);
    return true;
}