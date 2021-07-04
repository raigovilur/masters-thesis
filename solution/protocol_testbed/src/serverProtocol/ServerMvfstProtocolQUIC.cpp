#include "ServerMvfstProtocolQUIC.h"

#include <glog/logging.h>

#include <quic/samples/echo/LogQuicStats.h>
#include <quic/server/QuicServer.h>
#include <quic/server/QuicServerTransport.h>
#include <quic/server/QuicSharedUDPSocketFactory.h>
#include <quic/fizz/handshake/QuicFizzFactory.h>


#include <quic/api/QuicSocket.h>

#include <folly/io/async/EventBase.h>

#include <utility>
#include <iostream>
#include <fizz/protocol/OpenSSLFactory.h>

#include <fizz/crypto/Utils.h>

// These are here because GCC couldn't find them in Fizz
constexpr folly::StringPiece fizz::Sha384::BlankHash;
constexpr folly::StringPiece fizz::Sha256::BlankHash;

namespace quic {

    class ServersideCertificateVerifier : public fizz::CertificateVerifier {
    public:
        ~ServersideCertificateVerifier() override = default;

        void verify(const std::vector<std::shared_ptr<const fizz::PeerCert>>&)
        const override {
            return;
        }

        std::vector<fizz::Extension> getCertificateRequestExtensions()
        const override {
            return std::vector<fizz::Extension>();
        }
    };

    folly::ssl::EvpPkeyUniquePtr getPrivateKey(const char *pKey, size_t pkLen) {
        folly::ssl::BioUniquePtr bio(BIO_new(BIO_s_mem()));
        CHECK(bio);
        CHECK_EQ(BIO_write(bio.get(), pKey, pkLen), pkLen);
        folly::ssl::EvpPkeyUniquePtr pkey(
                PEM_read_bio_PrivateKey(bio.get(), nullptr, nullptr, nullptr));
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

    folly::ssl::X509UniquePtr getCert(const char *certificate, size_t certLen) {
        folly::ssl::BioUniquePtr bio(BIO_new(BIO_s_mem()));
        CHECK(bio);
        CHECK_EQ(BIO_write(bio.get(), certificate, certLen), certLen);
        folly::ssl::X509UniquePtr x509(PEM_read_bio_X509(bio.get(), nullptr, nullptr, nullptr));
        CHECK(x509);
        return x509;
    }

    std::shared_ptr<fizz::SelfCert> readCert(const char *certificateData, size_t certLen,
                                             const char *pKey, size_t pkLen) {
        auto certificate = getCert(certificateData, certLen);
        auto privKey = getPrivateKey(pKey, pkLen);
        std::vector<folly::ssl::X509UniquePtr> certs;
        certs.emplace_back(std::move(certificate));
        return std::make_shared<fizz::SelfCertImpl<fizz::KeyType::RSA>>(
                std::move(privKey), std::move(certs));
    }

    std::shared_ptr<fizz::server::FizzServerContext> createServerCtx(
            const char *certificate, size_t certLen,
            const char *pKey, size_t pkLen) {
        auto cert = readCert(certificate, certLen, pKey, pkLen);
        auto certManager = std::make_unique<fizz::server::CertManager>();
        certManager->addCert(std::move(cert), true);
        auto serverCtx = std::make_shared<fizz::server::FizzServerContext>();
        serverCtx->setFactory(std::make_shared<QuicFizzFactory>());
        serverCtx->setCertManager(std::move(certManager));
        serverCtx->setOmitEarlyRecordLayer(true);
        serverCtx->setClock(std::make_shared<fizz::SystemClock>());
        serverCtx->setClientCertVerifier(std::make_shared<ServersideCertificateVerifier>());
        return serverCtx;
    }

    class RequestHandler : public quic::QuicSocket::ConnectionCallback,
                           public quic::QuicSocket::ReadCallback,
                           public quic::QuicSocket::WriteCallback {
    public:
        explicit RequestHandler(folly::EventBase *evbIn, ServerProto::ServerReceiveCallback* callback)
            : evb(evbIn),
            _callback(callback) {}

        void setQuicSocket(std::shared_ptr<quic::QuicSocket> socket) {
            sock = std::move(socket);
        }

        void onNewBidirectionalStream(quic::StreamId id) noexcept override {
            //LOG(INFO) << "Got bidirectional stream id=" << id;
            sock->setReadCallback(id, this);
        }

        void onNewUnidirectionalStream(quic::StreamId id) noexcept override {
            //LOG(INFO) << "Got unidirectional stream id=" << id;
            sock->setReadCallback(id, this);
        }

        void onStopSending(
                quic::StreamId id,
                quic::ApplicationErrorCode error) noexcept override {
            LOG(INFO) << "Got StopSending stream id=" << id << " error=" << error;
        }

        void onConnectionEnd() noexcept override {
            LOG(INFO) << "Socket closed";
        }

        void onConnectionError(
                std::pair<quic::QuicErrorCode, std::string> error) noexcept override {
            LOG(ERROR) << "Socket error=" << toString(error.first);
        }

        void readAvailable(quic::StreamId id) noexcept override {
            //LOG(INFO) << "read available for stream id=" << id;

            auto res = sock->read(id, 0);
            if (res.hasError()) {
                LOG(ERROR) << "Got error=" << toString(res.error());
                return;
            }
            quic::Buf data = std::move(res.value().first);
            _callback->consume("client", (unsigned char*) data->data(), data->length());
        }

        void readError(
                quic::StreamId id,
                std::pair<quic::QuicErrorCode, folly::Optional<folly::StringPiece>>
                error) noexcept override {
            LOG(ERROR) << "Got read error on stream=" << id
                       << " error=" << toString(error);
            // A read error only terminates the ingress portion of the stream state.
            // Your application should probably terminate the egress portion via
            // resetStream
        }

        void onStreamWriteReady(quic::StreamId id, uint64_t maxToSend) noexcept
        override {
//                LOG(INFO) << "socket is write ready with maxToSend=" << maxToSend;
        }

        void onStreamWriteError(
                quic::StreamId id,
                std::pair<quic::QuicErrorCode, folly::Optional<folly::StringPiece>>
                error) noexcept override {
            LOG(ERROR) << "write error with stream=" << id
                       << " error=" << toString(error);
        }

        folly::EventBase *getEventBase() {
            return evb;
        }

    private:
        folly::EventBase *evb;
        std::shared_ptr<quic::QuicSocket> sock;
        ServerProto::ServerReceiveCallback* _callback;
    };

    class MvFstServerTransportFactory : public quic::QuicServerTransportFactory {
    public:
        MvFstServerTransportFactory(
                ServerProto::ServerReceiveCallback* callback) : _callback(callback) {};
        ~MvFstServerTransportFactory() override {
            while (!requestConsumers.empty()) {
                auto &handler = requestConsumers.back();
                handler->getEventBase()->runImmediatelyOrRunInEventBaseThreadAndWait(
                        [this] {
                            // The evb should be performing a sequential consistency atomic
                            // operation already, so we can bank on that to make sure the writes
                            // propagate to all threads.
                            requestConsumers.pop_back();
                        });
            }
        }

        MvFstServerTransportFactory() = default;

        quic::QuicServerTransport::Ptr make(
                folly::EventBase *evb,
                std::unique_ptr<folly::AsyncUDPSocket> sock,
                const folly::SocketAddress &,
                std::shared_ptr<const fizz::server::FizzServerContext> ctx) noexcept
        override {
            CHECK_EQ(evb, sock->getEventBase());
            auto requestHandler = std::make_unique<RequestHandler>(evb, _callback);
            auto transport = quic::QuicServerTransport::make(
                    evb, std::move(sock), *requestHandler, ctx);
            requestHandler->setQuicSocket(transport);
            requestConsumers.push_back(std::move(requestHandler));
            return transport;
        }

        std::vector<std::unique_ptr<RequestHandler>> requestConsumers;
        ServerProto::ServerReceiveCallback* _callback;

    private:
    };

    class MvFstServer {
    public:
        explicit MvFstServer(ServerProto::ServerReceiveCallback* callback,
                             const char *certificate, size_t certLen,
                             const char *pKey, size_t pkLen,
                             std::string host = "0.0.0.0", uint16_t port = 12345)
                : host_(std::move(host)), port_(port), server_(QuicServer::createQuicServer()) {
            server_->setQuicServerTransportFactory(
                    std::make_unique<MvFstServerTransportFactory>(callback));
//            server_->setTransportStatsCallbackFactory(
//                    std::make_unique<LogQuicStatsFactory>());
            auto serverCtx = createServerCtx(certificate, certLen, pKey, pkLen);
            TransportSettings settings;
            //settings.idleTimeout = std::chrono::milliseconds(1000);
            settings.connectUDP = true;
            settings.defaultCongestionController = quic::CongestionControlType::Copa2;
            //settings.advertisedInitialUniStreamWindowSize = 20971520;
            server_->setTransportSettings(settings);
            server_->setFizzContext(serverCtx);
        }

        void start() {
            // Create a SocketAddress and the default or passed in host.
            folly::SocketAddress addr1(host_.c_str(), port_);
            addr1.setFromHostPort(host_, port_);
            server_->start(addr1, 0);
            LOG(INFO) << "Echo server started at: " << addr1.describe();
            eventbase_.loopForever();
        }

    private:
        std::string host_;
        uint16_t port_;
        folly::EventBase eventbase_;
        std::shared_ptr<quic::QuicServer> server_;
    };
}

bool ServerProto::ServerMvfstProtocolQUIC::serverListen(const std::string &address, ushort port) {
    quic::MvFstServer server(_callback, _certificate, _certLen, _privKey, _privKeyLen, address, port);

    server.start();

    return true;
}

bool ServerProto::ServerMvfstProtocolQUIC::setCertificate(const char *certificate, size_t len) {
    _certificate = certificate;
    _certLen = len;
    return true;
}

bool ServerProto::ServerMvfstProtocolQUIC::setPrivateKey(const char *pKey, size_t len) {
    _privKey = pKey;
    _privKeyLen = len;
    return true;
}
