#include "MvfstProtocolQUIC.h"


#include <iostream>
#include <string>
#include <thread>

#include <glog/logging.h>

#include <folly/fibers/Baton.h>
#include <folly/io/async/ScopedEventBaseThread.h>

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
    class CertificateVerifier : public fizz::CertificateVerifier {
    public:
        ~CertificateVerifier() override = default;

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
            : host_(std::move(host)), port_(port), _networkThread("EchoClientThread"){}

    void readAvailable(quic::StreamId streamId) noexcept override {
        auto readData = quicClient_->read(streamId, 0);
        if (readData.hasError()) {
            LOG(ERROR) << "MvFstConnector failed read from stream=" << streamId
                       << ", error=" << (uint32_t)readData.error();
        }
        auto copy = readData->first->clone();
        if (recvOffsets_.find(streamId) == recvOffsets_.end()) {
            recvOffsets_[streamId] = copy->length();
        } else {
            recvOffsets_[streamId] += copy->length();
        }
        LOG(INFO) << "Client received data=" << copy->moveToFbString().toStdString()
                  << " on stream=" << streamId;
    }

    void readError(
            quic::StreamId streamId,
            std::pair<quic::QuicErrorCode, folly::Optional<folly::StringPiece>>
            error) noexcept override {
        LOG(ERROR) << "MvFstConnector failed read from stream=" << streamId
                   << ", error=" << toString(error);
        // A read error only terminates the ingress portion of the stream state.
        // Your application should probably terminate the egress portion via
        // resetStream
        _connected = false;
        _sendDone.post();
    }

    void onNewBidirectionalStream(quic::StreamId id) noexcept override {
        LOG(INFO) << "MvFstConnector: new bidirectional stream=" << id;
        quicClient_->setReadCallback(id, this);
    }

    void onNewUnidirectionalStream(quic::StreamId id) noexcept override {
        LOG(INFO) << "MvFstConnector: new unidirectional stream=" << id;
        quicClient_->setReadCallback(id, this);
    }

    void onStopSending(
            quic::StreamId id,
            quic::ApplicationErrorCode /*error*/) noexcept override {
        VLOG(10) << "MvFstConnector got StopSending stream id=" << id;
        _connected = false;
    }

    void onConnectionEnd() noexcept override {
        LOG(INFO) << "MvFstConnector connection end";
        _connected = false;
        _sendDone.post();
        _shutdownDone.post();
    }

    void onConnectionError(
            std::pair<quic::QuicErrorCode, std::string> error) noexcept override {
        LOG(ERROR) << "MvFstConnector error: " << toString(error.first);
        startDone_.post();
        _sendDone.post();
        _connected = false;
    }

    void onTransportReady() noexcept override {
        startDone_.post();
        _connected = true;
    }

    void onStreamWriteReady(quic::StreamId id, uint64_t maxToSend) noexcept
    override {
        LOG(INFO) << "MvFstConnector socket is write ready with maxToSend="
                  << maxToSend;
        //sendMessage(id, pendingOutput_[id]);

    }

    void onStreamWriteError(
            quic::StreamId id,
            std::pair<quic::QuicErrorCode, folly::Optional<folly::StringPiece>>
            error) noexcept override {
        LOG(ERROR) << "MvFstConnector write error with stream=" << id
                   << " error=" << toString(error);
        _connected = false;
    }

    void start() {
        auto evb = _networkThread.getEventBase();
        folly::SocketAddress addr(host_.c_str(), port_);

        evb->runInEventBaseThreadAndWait([&] {
            auto sock = std::make_unique<folly::AsyncUDPSocket>(evb);
            auto fizzClientContext =
                    FizzClientQuicHandshakeContext::Builder()
                            .setCertificateVerifier(std::make_shared<CertificateVerifier>())
                            .build();
            quicClient_ = std::make_shared<quic::QuicClientTransport>(
                    evb, std::move(sock), std::move(fizzClientContext));
            //quicClient_->setHostname("print_out.com");
            quicClient_->addNewPeerAddress(addr);

            TransportSettings settings;
            quicClient_->setTransportSettings(settings);

//                quicClient_->setTransportStatsCallback(
//                        std::make_shared<LogQuicStats>("client"));

            LOG(INFO) << "MvFstConnector connecting to " << addr.describe();
            quicClient_->start(this);
            _connected = true;
        });
        startDone_.wait();
        _streamId = quicClient_->createBidirectionalStream().value();
        quicClient_->setReadCallback(_streamId, this);
    }

    ~MvFstConnector() override = default;

    bool sendMessage(const char* buffer, size_t bufferSize, bool eof) {
        return sendMessage(_streamId, *folly::IOBuf::copyBuffer(buffer, bufferSize), eof);
    }

    bool closeConnection() {
//        quicClient_->closeTransport();
//        quicClient_->detachEventBase();
//        quicClient_->closeGracefully();
    }

    bool isConnected() const {
        return _connected;
    }

private:
    bool sendMessage(quic::StreamId id, folly::IOBuf& data, bool eof) {
        auto evb = _networkThread.getEventBase();
        bool isSent = false;
        if (data.empty())
        {
            return false;
        }

        evb->runInEventBaseThreadAndWait([&] {
            auto res = quicClient_->writeChain(id, data.clone(), eof);
            if (res.hasError()) {
                LOG(ERROR) << "MvFstConnector writeChain error=" << res.error();
                isSent = false;
            } else {
//                // sent whole message
                isSent = true;
            }
            _sendDone.post();
        });
        _sendDone.wait();
        return isSent;
    }

    std::string host_;
    uint16_t port_;
    std::shared_ptr<quic::QuicClientTransport> quicClient_;
    std::map<quic::StreamId, uint64_t> recvOffsets_;
    folly::fibers::Baton startDone_;
    folly::fibers::Baton _shutdownDone;
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
