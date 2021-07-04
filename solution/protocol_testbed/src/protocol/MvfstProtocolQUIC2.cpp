#include "MvfstProtocolQUIC2.h"


#include <iostream>
#include <string>
#include <thread>

#include <glog/logging.h>

#include <folly/fibers/Baton.h>
#include <folly/io/async/ScopedEventBaseThread.h>
#include <fizz/client/FizzClientContext.h>

#include <quic/api/QuicSocket.h>
#include <quic/client/QuicClientTransport.h>
#include <quic/common/BufUtil.h>
#include <quic/fizz/client/handshake/FizzClientQuicHandshakeContext.h>
#include <utility>


#include <quic/samples/echo/LogQuicStats.h>
// A private class that implements MVFST

using namespace quic;
class MvFstConnector : public quic::QuicSocket::ConnectionCallback,
                       public quic::QuicSocket::ReadCallback,
                       public quic::QuicSocket::WriteCallback
{
public:
    class ClientsideCertificateVerifier : public fizz::CertificateVerifier {
    public:
        ~ClientsideCertificateVerifier() override = default;

        void verify(const std::vector<std::shared_ptr<const fizz::PeerCert>>&)
        const override {
            return;
        }

        std::vector<fizz::Extension> getCertificateRequestExtensions()
        const override {
            return std::vector<fizz::Extension>();
        }
    };

    MvFstConnector(std::string  host, uint16_t port)
            : host_(std::move(host)), port_(port), _networkThread("MVFSTConnectorThread"){}
            

    void readAvailable(quic::StreamId streamId) noexcept override {  }

    void readError(
            quic::StreamId streamId,
            std::pair<quic::QuicErrorCode, folly::Optional<folly::StringPiece>>
            error) noexcept override {
        LOG(ERROR) << "MVFSTConnector failed read from stream=" << streamId
                   << ", error=" << toString(error);
        // A read error only terminates the ingress portion of the stream state.
        // Your application should probably terminate the egress portion via
        // resetStream
    }

    void onNewBidirectionalStream(quic::StreamId id) noexcept override {
        LOG(INFO) << "MVFSTConnector: new bidirectional stream=" << id;
        quicClient_->setReadCallback(id, this);
    }

    void onNewUnidirectionalStream(quic::StreamId id) noexcept override {
        LOG(INFO) << "MVFSTConnector: new unidirectional stream=" << id;
        quicClient_->setReadCallback(id, this);
    }

    void onStopSending(
            quic::StreamId id,
            quic::ApplicationErrorCode /*error*/) noexcept override {
        VLOG(10) << "MVFSTConnector got StopSending stream id=" << id;
    }

    void onConnectionEnd() noexcept override {
        LOG(INFO) << "MVFSTConnector connection end";
    }

    void onConnectionError(
            std::pair<quic::QuicErrorCode, std::string> error) noexcept override {
        LOG(ERROR) << "MVFSTConnector error: " << toString(error.first)
                   << "; errStr=" << error.second;
        startDone_.post();
    }

    void onTransportReady() noexcept override {
        startDone_.post();
    }

    void onStreamWriteReady(quic::StreamId id, uint64_t maxToSend) noexcept
    override {
        LOG(INFO) << "MVFSTConnector socket is write ready with maxToSend="
                  << maxToSend;
        sendMessage(id, pendingOutput_, maxToSend);
    }

    void onStreamWriteError(
            quic::StreamId id,
            std::pair<quic::QuicErrorCode, folly::Optional<folly::StringPiece>>
            error) noexcept override {
        LOG(ERROR) << "MVFSTConnector write error with stream=" << id
                   << " error=" << toString(error);
    }

    void start() {
        auto evb = _networkThread.getEventBase();
        folly::SocketAddress addr(host_.c_str(), port_);

        evb->runInEventBaseThreadAndWait([&] {
            auto sock = std::make_unique<folly::AsyncUDPSocket>(evb);
            auto fizzClientContext = std::make_shared<fizz::client::FizzClientContext>();
            fizzClientContext->setSupportedCiphers({fizz::CipherSuite::TLS_CHACHA20_POLY1305_SHA256});
            auto fizzClientQuicHandshakeContext =
                    FizzClientQuicHandshakeContext::Builder()
                            .setCertificateVerifier(std::make_shared<ClientsideCertificateVerifier>())
                            .setFizzClientContext(fizzClientContext)
                            .build();
            quicClient_ = std::make_shared<quic::QuicClientTransport>(
                    evb, std::move(sock), std::move(fizzClientQuicHandshakeContext));
            quicClient_->addNewPeerAddress(addr);
            quicClient_->setCongestionControllerFactory(
                    std::make_shared<DefaultCongestionControllerFactory>());

            TransportSettings settings;
            settings.idleTimeout = std::chrono::milliseconds(1000);
            settings.connectUDP = true;
            settings.defaultCongestionController = quic::CongestionControlType::BBR;
            quicClient_->setTransportSettings(settings);

//                quicClient_->setTransportStatsCallback(
//                        std::make_shared<LogQuicStats>("client"));

            LOG(INFO) << "MvFstConnector connecting to " << addr.describe();
            quicClient_->start(this);
            _connected = true;
        });
        startDone_.wait();
        auto stream = quicClient_->createUnidirectionalStream();
        //auto stream = quicClient_->createBidirectionalStream();
        if (stream.hasError()) {
            LOG(ERROR) << "Error creating a new stream: " << stream.error();
            _connected = false;
            return;
        }
        _streamId = stream.value();
        quicClient_->setReadCallback(_streamId, this);
    }

    ~MvFstConnector() override = default;

    bool sendMessage(const char* buffer, size_t bufferSize, bool eof) {
        auto evb = _networkThread.getEventBase();
        _eof = eof;
        evb->runInEventBaseThreadAndWait([=] {
            pendingOutput_.append(folly::IOBuf::copyBuffer(buffer, bufferSize));
            //sendMessage(_streamId, pendingOutput_, bufferSize);
            notifySend();
        });

        return true;
    }

    void notifySend() {
        _networkThread.getEventBase()->runInEventBaseThread([&]() {
            if (!quicClient_) {
                VLOG(5) << "notifyDataForStream(" << _streamId << "): socket is closed.";
                return;
            }
            auto res = quicClient_->notifyPendingWriteOnStream(_streamId, this);
            if (res.hasError()) {
                LOG(FATAL) << quic::toString(res.error());
            }
        });
    }

    bool closeConnection() {
//        quicClient_->closeTransport();
//        quicClient_->detachEventBase();
//        quicClient_->closeGracefully();
        return true;
    }

    bool isConnected() const {
        return _connected;
    }

    bool isAllAcked() {
        return quicClient_->getState()->outstandings.packets.empty();
    }

private:
    void sendMessage(quic::StreamId id, BufQueue& data, uint64_t maxToSend) {
        uint64_t canBeSent = std::min(maxToSend, data.chainLength());
        auto message = ;
        auto res = quicClient_->writeChain(id, message->clone(), true);
        if (res.hasError()) {
            LOG(ERROR) << "MVFSTConnector writeChain error=" << uint32_t(res.error());
        } else {
            pendingOutput_.erase(id);
        }
    }

    bool _eof = false;
    std::string host_;
    uint16_t port_;
    std::shared_ptr<quic::QuicClientTransport> quicClient_;
    BufQueue pendingOutput_;
    folly::fibers::Baton startDone_;
    folly::fibers::Baton _sendDone;
    folly::ScopedEventBaseThread _networkThread;
    unsigned long _streamId;

    bool _connected = false;
};

bool Protocol::MvfstProtocolQUIC::openProtocol(std::string address, uint port) {
    static bool isGoogleLoggingOpened = false;
    if (!isGoogleLoggingOpened)
    {
        FLAGS_logtostderr = true;
        FLAGS_minloglevel = 0;
        FLAGS_stderrthreshold = 0;
        google::InitGoogleLogging("XXX-client");
        isGoogleLoggingOpened = true;
    }

    LOG(INFO) << "Starting MVFST";
    _mvfst = new MvFstConnector(address, port);
    _mvfst->start();
    return _mvfst->isConnected();
}

bool Protocol::MvfstProtocolQUIC::openProtocolServer(uint port) {
    // TODO implement
    return false;
}

bool Protocol::MvfstProtocolQUIC::send(const char *buffer, size_t bufferSize, bool eof) {
    if (!_mvfst->isConnected()) {
        return false;
    }
    return _mvfst->sendMessage(buffer, bufferSize, eof);
}

bool Protocol::MvfstProtocolQUIC::closeProtocol() {
    _mvfst->closeConnection();
    delete _mvfst;
    _mvfst = nullptr;
    return true;
}

Protocol::MvfstProtocolQUIC::~MvfstProtocolQUIC() {
    delete _mvfst;
}

bool Protocol::MvfstProtocolQUIC::isAllSent() {
    auto waitingTime = std::chrono::system_clock::now();
    while (!_mvfst->isAllAcked()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    std::cout << "Waited for acks: " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - waitingTime).count() << " ms" << std::endl;
    return true;
}

