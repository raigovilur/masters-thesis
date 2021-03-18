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
        std::cout << "Error connecting to server" << std::endl;
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
    SSL_CTX_set_options(ctx, SSL_OP_NO_DTLSv1); // We are only interested in DTLS 1.2 //TODO or does this block v 1.xxx all?
    _ssl = SSL_new(ctx);
    if (!_ssl) {
        printf("Error creating SSL.\n");
        //log_ssl();
        return -1;
    }
    _sockFd = SSL_get_fd(_ssl);
    SSL_set_fd(_ssl, _socket);
    int err = SSL_connect(_ssl);
    if (err <= 0) {
        std::cout << "Error creating SSL connection.  err=" << err << std::endl;
        //log_ssl();
        fflush(stdout);
        return -1;
    }

    _initialized = true;
    return true;
}

bool Protocol::OpenSSLProtocolDTLS::send(const char *buffer, size_t bufferSize) {
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
/*
#define BUFFER_SIZE          (1<<16)
#define MAX_STACK            (1<<16)
//#define COOKIE_SECRET_LENGTH 16

struct Info {
    SSL* ssl;
    BIO_ADDR* client_addr;
    BIO_ADDR* server_addr;
};


void* connection_handle(Info *info) {
    ssize_t len;
    char buf[BUFFER_SIZE];
    char addrbuf[INET6_ADDRSTRLEN];
    SSL *ssl = info->ssl;
    int fd, reading = 0, rcvnum, count = 0, rcvcount, activesocks;
    fd_set readsocks;
    struct timeval timeout;
    int ret, maxrecvnum = -1, stackindex = 0;
    int recvstack[MAX_STACK];
    const int on = 1, off = 0;
    int num_timeouts = 0, max_timeouts = 5;

    int clientFamily = BIO_ADDR_family(info->client_addr);
    void* clientNetAddr; size_t clientNetAddrSize;
    BIO_ADDR_rawaddress(info->client_addr, clientNetAddr, &clientNetAddrSize);

    OPENSSL_assert(clientFamily == BIO_ADDR_family(info->server_addr));
    fd = socket(clientFamily, SOCK_DGRAM, 0);
    if (fd < 0) {
        perror("socket");
        goto cleanup;
    }

    switch (clientFamily) {
        case AF_INET:

            connect(fd, (struct sockaddr *) &pinfo->client_addr, sizeof(struct sockaddr_in));
            break;
        case AF_INET6:
            setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&off, sizeof(off));
            bind(fd, (const struct sockaddr *) &pinfo->server_addr, sizeof(struct sockaddr_in6));
            connect(fd, (struct sockaddr *) &pinfo->client_addr, sizeof(struct sockaddr_in6));
            break;
        default:
            OPENSSL_assert(0);
            break;
    }

    // Set new fd and set BIO to connected
    BIO_set_fd(SSL_get_rbio(ssl), fd, BIO_NOCLOSE);
    BIO_ctrl(SSL_get_rbio(ssl), BIO_CTRL_DGRAM_SET_CONNECTED, 0, &pinfo->client_addr.ss);

    // Finish handshake
    do { ret = SSL_accept(ssl); }
    while (ret == 0);
    if (ret < 0) {
        perror("SSL_accept");
        printf("%s\n", ERR_error_string(ERR_get_error(), buf));
        goto cleanup;
    }

    // Set and activate timeouts
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    BIO_ctrl(SSL_get_rbio(ssl), BIO_CTRL_DGRAM_SET_RECV_TIMEOUT, 0, &timeout);

//    if (verbose) {
//        if (pinfo->client_addr.ss.ss_family == AF_INET) {
//            printf ("\nThread %lx: accepted connection from %s:%d\n",
//                    (long) pthread_self(),
//                    inet_ntop(AF_INET, &pinfo->client_addr.s4.sin_addr, addrbuf, INET6_ADDRSTRLEN),
//                    ntohs(pinfo->client_addr.s4.sin_port));
//        } else {
//            printf ("\nThread %lx: accepted connection from %s:%d\n",
//                    (long) pthread_self(),
//                    inet_ntop(AF_INET6, &pinfo->client_addr.s6.sin6_addr, addrbuf, INET6_ADDRSTRLEN),
//                    ntohs(pinfo->client_addr.s6.sin6_port));
//        }
//    }

//    if (veryverbose && SSL_get_peer_certificate(ssl)) {
//        printf ("------------------------------------------------------------\n");
//        X509_NAME_print_ex_fp(stdout, X509_get_subject_name(SSL_get_peer_certificate(ssl)),
//                              1, XN_FLAG_MULTILINE);
//        printf("\n\n Cipher: %s", SSL_CIPHER_get_name(SSL_get_current_cipher(ssl)));
//        printf ("\n------------------------------------------------------------\n\n");
//    }

    count = 0;
    rcvcount = 0;

    while (!(SSL_get_shutdown(ssl) & SSL_RECEIVED_SHUTDOWN) && num_timeouts < max_timeouts) {
        timeout.tv_sec = 0;
        timeout.tv_usec = 1;

        FD_ZERO(&readsocks);
        FD_SET(fd,&readsocks);

        while ((activesocks = select(FD_SETSIZE, &readsocks, NULL, NULL, &timeout)) < 0) {
            if (errno != EINTR) {
                perror("select");
                exit(EXIT_FAILURE);
            }
        }

        count = htonl(count);
        memcpy(buf, &count, sizeof(int));
        count = ntohl(count);

        len = SSL_write(ssl, buf, length);

        switch (SSL_get_error(ssl, len)) {
            case SSL_ERROR_NONE:
//                if (verbose) {
//                    printf("Thread %lx: wrote num %d with %d bytes\n",  (long) pthread_self(),
//                           count, (int) len);
//                }
                count++;
                break;
            case SSL_ERROR_WANT_WRITE:
                // Just try again later
                break;
            case SSL_ERROR_WANT_READ:
                // continue with reading
                break;
            case SSL_ERROR_SYSCALL:
                printf("Socket write error: ");
                if (!handle_socket_error()) goto cleanup;
                reading = 0;
                break;
            case SSL_ERROR_SSL:
                printf("SSL write error: ");
                printf("%s (%d)\n", ERR_error_string(ERR_get_error(), buf), SSL_get_error(ssl, len));
                goto cleanup;
                break;
            default:
                printf("Unexpected error while writing!\n");
                goto cleanup;
                break;
        }

        if (activesocks > 0) {

            if (FD_ISSET(fd,&readsocks)) {
                reading = 1;

                while (reading) {

                    len = SSL_read(ssl, buf, sizeof(buf));

                    switch (SSL_get_error(ssl, len)) {
                        case SSL_ERROR_NONE:
                            memcpy(&rcvnum, buf, sizeof(int));
                            rcvnum = ntohl(rcvnum);
                            if (verbose) {
                                printf("Thread %lx: read num %d with %d bytes\n", (long) pthread_self(), rcvnum, (int) len);
                            }

                            if (rcvnum == maxrecvnum + 1)
                                maxrecvnum++;
                            else {
                                stack_insert(recvstack, &stackindex, rcvnum);
                                stack_update(recvstack, &stackindex, &maxrecvnum);
                            }
                            rcvcount++;
                            reading = 0;
                            break;
                        case SSL_ERROR_WANT_READ:
                            // Handle socket timeouts
                            if (BIO_ctrl(SSL_get_rbio(ssl), BIO_CTRL_DGRAM_GET_RECV_TIMER_EXP, 0, NULL)) {
                                num_timeouts++;
                                reading = 0;
                            }
                            // Just try again
                            break;
                        case SSL_ERROR_ZERO_RETURN:
                            reading = 0;
                            break;
                        case SSL_ERROR_SYSCALL:
                            printf("Socket read error: ");
                            if (!handle_socket_error()) goto cleanup;
                            reading = 0;
                            break;
                        case SSL_ERROR_SSL:
                            printf("SSL read error: ");
                            printf("%s (%d)\n", ERR_error_string(ERR_get_error(), buf), SSL_get_error(ssl, len));
                            goto cleanup;
                            break;
                        default:
                            printf("Unexpected error while reading!\n");
                            goto cleanup;
                            break;
                    }
                }
            }
        }
    }

    SSL_shutdown(ssl);
    if (pinfo->client_addr.ss.ss_family == AF_INET) {
        printf("\nThread %lx: Statistics for %s:\n=========================================================\n",
               (long) pthread_self(), inet_ntop(AF_INET, &pinfo->client_addr.s4.sin_addr, addrbuf, INET6_ADDRSTRLEN));
    } else {
        printf("\nThread %lx: Statistics for %s:\n=========================================================\n",
               (long) pthread_self(), inet_ntop(AF_INET6, &pinfo->client_addr.s6.sin6_addr, addrbuf, INET6_ADDRSTRLEN));
    }
    printf("Thread %lx: Sent messages:                    %6d\n", (long) pthread_self(), count);
    printf("Thread %lx: Received messages:                %6d\n\n", (long) pthread_self(), rcvcount);

    printf("Thread %lx: Messages lost:                    %6d\n", (long) pthread_self(), lost_messages(recvstack, stackindex, maxrecvnum));

    cleanup:
    close(fd);

    free(info);
    SSL_free(ssl);
    ERR_remove_state(0);
    if (verbose)
        printf("Thread %lx: done, connection closed.\n", (long) pthread_self());
    pthread_exit( (void *) NULL );
}
*/
bool Protocol::OpenSSLProtocolDTLS::openProtocolServer(uint port) {
    /*int fd, pid;

    BIO_ADDR* client_addr = BIO_ADDR_new();
    BIO_ADDR* server_addr = BIO_ADDR_new();
    pthread_t tid;
    SSL_CTX *ctx;
    SSL *ssl;
    BIO *bio;
    struct timeval timeout;

    const int on = 1, off = 0;

    server_addr.s6.sin6_family = AF_INET6;
    server_addr.s6.sin6_addr = in6addr_any;
    server_addr.s6.sin6_port = htons(port);

    THREAD_setup();
    OpenSSL_add_ssl_algorithms();
    SSL_load_error_strings();
    ctx = SSL_CTX_new(DTLS_server_method());
    /* We accept all ciphers, including NULL.
     * Not recommended beyond testing and debugging
     *//*
    SSL_CTX_set_cipher_list(ctx, "ALL:NULL:eNULL:aNULL");
    pid = getpid();
    //if( !SSL_CTX_set_session_id_context(ctx, (void*)&pid, sizeof pid) )
    //    perror("SSL_CTX_set_session_id_context");

    if (!SSL_CTX_use_certificate_file(ctx, "certs/server-cert.pem", SSL_FILETYPE_PEM))
        printf("\nERROR: no certificate found!");

    if (!SSL_CTX_use_PrivateKey_file(ctx, "certs/server-key.pem", SSL_FILETYPE_PEM))
        printf("\nERROR: no private key found!");

    if (!SSL_CTX_check_private_key (ctx))
        printf("\nERROR: invalid private key!");

    // Client has to authenticate
    // TODO
    //SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE, dtls_verify_callback);

    SSL_CTX_set_read_ahead(ctx, 1);
    // TODO are they needed?
    //SSL_CTX_set_cookie_generate_cb(ctx, generate_cookie);
    //SSL_CTX_set_cookie_verify_cb(ctx, verify_cookie);
    fd = socket(server_addr.ss.ss_family, SOCK_DGRAM, 0);
    if (fd < 0) {
        perror("socket");
        exit(-1);
    }

    if (server_addr.ss.ss_family == AF_INET) {
        bind(fd, (const struct sockaddr *) &server_addr, sizeof(struct sockaddr_in));
    } else {
        setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&off, sizeof(off));
        bind(fd, (const struct sockaddr *) &server_addr, sizeof(struct sockaddr_in6));
    }
    while (true) {

        // Create BIO
        bio = BIO_new_dgram(fd, BIO_NOCLOSE);

        // Set and activate timeouts
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;
        BIO_ctrl(bio, BIO_CTRL_DGRAM_SET_RECV_TIMEOUT, 0, &timeout);

        ssl = SSL_new(ctx);

        SSL_set_bio(ssl, bio, bio);
        SSL_set_options(ssl, SSL_OP_NO_DTLSv1);

        while (DTLSv1_listen(ssl, client_addr) <= 0);

        if (pthread_create( &tid, NULL, connection_handle, info) != 0) {
            perror("pthread_create");
            exit(-1);
        }
    }

    THREAD_cleanup();
*/
    return false;
}
