#include <glog/logging.h>

//#include <quic/common/test/TestUtils.h>
#include <quic/samples/echo/LogQuicStats.h>
#include <quic/server/QuicServer.h>
#include <quic/server/QuicServerTransport.h>
#include <quic/server/QuicSharedUDPSocketFactory.h>
#include <quic/fizz/handshake/QuicFizzFactory.h>


#include <quic/api/QuicSocket.h>

#include <folly/io/async/EventBase.h>
#include <quic/common/BufUtil.h>

#include <utility>
#include <iostream>
#include <fizz/protocol/OpenSSLFactory.h>

#include <fizz/crypto/Utils.h>
#include <folly/init/Init.h>
/*
// THESE are anyway test certificates/keys, so it doesn't really matter that they end up in git
constexpr folly::StringPiece certificate = R"(
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIUB62ZpZCtt36bbY0wK8Xc8f+MJs0wDQYJKoZIhvcNAQEL
BQAwRTELMAkGA1UEBhMCRUUxEzARBgNVBAgMClNvbWUtU3RhdGUxITAfBgNVBAoM
GEludGVybmV0IFdpZGdpdHMgUHR5IEx0ZDAeFw0yMTAzMjQwODIzMDJaFw0yMjAz
MjQwODIzMDJaMEUxCzAJBgNVBAYTAkVFMRMwEQYDVQQIDApTb21lLVN0YXRlMSEw
HwYDVQQKDBhJbnRlcm5ldCBXaWRnaXRzIFB0eSBMdGQwggIiMA0GCSqGSIb3DQEB
AQUAA4ICDwAwggIKAoICAQCjvRZy31MDYGFEfRWUVsCv06hyiEb3wJwbEMIooJVO
jie3Xwzfx2Xatdk7Yoxl6OlFNkdr8ux8ZbTQrzY2JNb/xPALP60yPyXfiAtnhbga
caWdhJDnka4bvtTp3YbXJDXT4AxdCfviFEIc+S73qhULqRzFH3J9dQfAkzfV4ep9
8wTbg//EPIbHExIHKN+7Q5ah0shEoFKRnOWW8x5HjxWCT/yDfi5j7tsXbwSbzYUW
7t1FbwZ9PwglThtUUq7nYNDr9dvlePLycdTARaQQsMK4gRWB+A/5KgITVsurx/v/
IuOeBITE8+1RKa/iZcyykepLWhgLT9kK5aB+exXPf+4ykzUe5KVOFmMSu2Sj8MRX
F11H16VErlBCCFCInJQ0tDeHdH0uV0to9NGuy5LMuEWCVkRLf2Oz94uEnRYPzbf2
/d4M/Ke+bDEnPED8DeNNPHa20uZNXDYVCGV59v5Gvd4vMohBAZ78etoZbA/8md7+
F7XA8l5b8LrHv4LVhg7RviWEvEF/6siEG+KdH4qJXqFCEssypg8Lo+iPxjv4JuV3
/WGto40tV99lgyuGApNR6RUDKVMa7MXLUbyd7YYecjUBlrss19R7lZWRAfB3sRmG
D+vT7F7vSy2UKihptZTBYrnFKnOQyzNrZyx3615GWRN+8pgSLhLU0oWbdrGjn/io
wwIDAQABo1MwUTAdBgNVHQ4EFgQUY2GHni5BAh5JnEn9A021L16cXx4wHwYDVR0j
BBgwFoAUY2GHni5BAh5JnEn9A021L16cXx4wDwYDVR0TAQH/BAUwAwEB/zANBgkq
hkiG9w0BAQsFAAOCAgEAW0wPUI3zkEaUPdw6VI+9B1sS6QRKqqp4KuJl8V1Q9Ywt
n6uqw+n1P7O8Fj1etOAfW2mBeAprmf/XjNDxQ61tNy8k/XnlrL/UHUkanwv0n0M9
4LUPlp2PVTXrKEpjuukUPbRAKrWV0p29LnWnO9GlBefNjWvwMhjwAdX8srwbw2/0
/ofNPOftjzPvtPgyzj4579TbkY55DhJe9YKiatkVOr3Ab9Q1WpNeX7+eS1H/f0nh
e9d8WIgv9Z+AC0QRvsnoewLDbRhGavoLNvW30XEStIPmUrRzOIF79/PjkM9Ny0a1
/fvQPesnkPsLeonE6TimXJsn8dF3Czt0ehgxO4IQ20FXDXHgehSR2H4Wg3TVSZ74
z5hOWRHYrvXrDy/b4n/sP7AO/3WeSmsD2LrQsippmgsnqpevn0192YDaB358f4XX
BbYguoy73tHu41DcauwgPwx2PJlDgv4V06TQu7Jk49Y8nmripRE6Ne+wqJ3gDDN/
r+POTDFaDGGv8bjQ7P/BmOa5erYOZmDzDnvOii0A/OYYN2LC0xptx+QmWSWTzQ+3
bxMe8FGvGo7nWtXH1+HeF0/raGWzt8LsF0NXPQX3eR1CzdLAtSyxeHLvn65naMJT
o/Y3t3jZrODQQ7jUQjE/XtfuKFYKINBR5CKkrY7XQHahc/GJr2GZsrLqPF8FWu0=
-----END CERTIFICATE-----
)";

constexpr folly::StringPiece privKey = R"(
-----BEGIN PRIVATE KEY-----
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
Sd/H4YHoc8qOIZUOXaVW3YlTShhbeZIx
-----END PRIVATE KEY-----
)";

// These are here because GCC couldn't find them in Fizz
constexpr folly::StringPiece fizz::Sha384::BlankHash;
constexpr folly::StringPiece fizz::Sha256::BlankHash;

namespace quic {
    namespace samples {

        folly::ssl::EvpPkeyUniquePtr getPrivateKey(folly::StringPiece key) {
            folly::ssl::BioUniquePtr bio(BIO_new(BIO_s_mem()));
            CHECK(bio);
            CHECK_EQ(BIO_write(bio.get(), key.data(), key.size()), key.size());
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

        folly::ssl::X509UniquePtr getCert(folly::StringPiece cert) {
            folly::ssl::BioUniquePtr bio(BIO_new(BIO_s_mem()));
            CHECK(bio);
            CHECK_EQ(BIO_write(bio.get(), cert.data(), cert.size()), cert.size());
            folly::ssl::X509UniquePtr x509(PEM_read_bio_X509(bio.get(), nullptr, nullptr, nullptr));
            CHECK(x509);
            return x509;
        }

        std::shared_ptr<fizz::SelfCert> readCert() {
            auto certificate = getCert(certificate);
            auto privKey = getPrivateKey(privKey);
            std::vector<folly::ssl::X509UniquePtr> certs;
            certs.emplace_back(std::move(certificate));
            return std::make_shared<fizz::SelfCertImpl<fizz::KeyType::RSA>>(
                    std::move(privKey), std::move(certs));
        }

        std::shared_ptr<fizz::server::FizzServerContext> createServerCtx() {
            auto cert = readCert();
            auto certManager = std::make_unique<fizz::server::CertManager>();
            certManager->addCert(std::move(cert), true);
            auto serverCtx = std::make_shared<fizz::server::FizzServerContext>();
            serverCtx->setFactory(std::make_shared<QuicFizzFactory>());
            serverCtx->setCertManager(std::move(certManager));
            serverCtx->setOmitEarlyRecordLayer(true);
            serverCtx->setClock(std::make_shared<fizz::SystemClock>());
            return serverCtx;
        }

        class CoutHandler : public quic::QuicSocket::ConnectionCallback,
                            public quic::QuicSocket::ReadCallback,
                            public quic::QuicSocket::WriteCallback {
        public:
            using StreamData = std::pair<BufQueue, bool>;

            explicit CoutHandler(folly::EventBase* evbIn) : evb(evbIn) {}

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
//                if (input_.find(id) == input_.end()) {
//                    input_.emplace(id, std::make_pair(BufQueue(), false));
//                }
                quic::Buf data = std::move(res.value().first);
//                bool eof = res.value().second;
//                auto dataLen = (data ? data->computeChainDataLength() : 0);
//                LOG(INFO) << "Got len=" << dataLen << " eof=" << uint32_t(eof)
//                          << " total=" << input_[id].first.chainLength() + dataLen
//                          << " data="
//                          << ((data) ? data->clone()->moveToFbString().toStdString()
//                                     : std::string());
//                if (eof) {
                    print_out(id, data);
//                }
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

            void print_out(quic::StreamId id, quic::Buf& data) {
                auto echoedData = folly::IOBuf::copyBuffer("print_out ");

                std::string stringData((char*) data->data(), data->length());
                std::cout << stringData;
            }

            void onStreamWriteReady(quic::StreamId id, uint64_t maxToSend) noexcept
            override {
//                LOG(INFO) << "socket is write ready with maxToSend=" << maxToSend;
//                print_out(id, input_[id]);
            }

            void onStreamWriteError(
                    quic::StreamId id,
                    std::pair<quic::QuicErrorCode, folly::Optional<folly::StringPiece>>
                    error) noexcept override {
                LOG(ERROR) << "write error with stream=" << id
                           << " error=" << toString(error);
            }

            folly::EventBase* getEventBase() {
                return evb;
            }

            folly::EventBase* evb;
            std::shared_ptr<quic::QuicSocket> sock;

        private:
            std::map<quic::StreamId, StreamData> input_;
        };

        class EchoServerTransportFactory : public quic::QuicServerTransportFactory {
        public:
            ~EchoServerTransportFactory() override {
                while (!coutHandlers.empty()) {
                    auto& handler = coutHandlers.back();
                    handler->getEventBase()->runImmediatelyOrRunInEventBaseThreadAndWait(
                            [this] {
                                // The evb should be performing a sequential consistency atomic
                                // operation already, so we can bank on that to make sure the writes
                                // propagate to all threads.
                                coutHandlers.pop_back();
                            });
                }
            }

            EchoServerTransportFactory() = default;

            quic::QuicServerTransport::Ptr make(
                    folly::EventBase *evb,
                    std::unique_ptr<folly::AsyncUDPSocket> sock,
                    const folly::SocketAddress &,
                    std::shared_ptr<const fizz::server::FizzServerContext> ctx) noexcept
            override {
                CHECK_EQ(evb, sock->getEventBase());
                auto coutHandler = std::make_unique<CoutHandler>(evb);
                auto transport = quic::QuicServerTransport::make(
                        evb, std::move(sock), *coutHandler, ctx);
                coutHandler->setQuicSocket(transport);
                coutHandlers.push_back(std::move(coutHandler));
                return transport;
            }

            std::vector<std::unique_ptr<CoutHandler>> coutHandlers;

        private:
        };

        class EchoServer {
        public:
            explicit EchoServer(std::string host = "0.0.0.0", uint16_t port = 12345)
                    : host_(std::move(host)), port_(port), server_(QuicServer::createQuicServer()) {
                server_->setQuicServerTransportFactory(
                        std::make_unique<EchoServerTransportFactory>());
                server_->setTransportStatsCallbackFactory(
                        std::make_unique<LogQuicStatsFactory>());
                auto serverCtx = createServerCtx();
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
}*/

#include "src/Server.h"
#include "src/appProto/ProtocolType.h"

int main(int argc, char *argv[]) {
    FLAGS_logtostderr = true;
    folly::Init init(&argc, &argv);
    fizz::CryptoUtils::init();

    Server server;
    server.start(Protocol::MVFST_QUIC, "0.0.0.0", 12345);

}