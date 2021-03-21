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
                       public quic::QuicSocket::WriteCallback {
public:
    MvFstConnector(std::string  host, uint16_t port)
            : host_(std::move(host)), port_(port) {}

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
    }

    void onConnectionError(
            std::pair<quic::QuicErrorCode, std::string> error) noexcept override {
        LOG(ERROR) << "MvFstConnector error: " << toString(error.first);
        startDone_.post();
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
        sendMessage(id, pendingOutput_[id]);
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
        folly::ScopedEventBaseThread networkThread("EchoClientThread");
        auto evb = networkThread.getEventBase();
        folly::SocketAddress addr(host_.c_str(), port_);

        evb->runInEventBaseThreadAndWait([&] {
            auto sock = std::make_unique<folly::AsyncUDPSocket>(evb);
            auto fizzClientContext =
                    FizzClientQuicHandshakeContext::Builder()
                            //.setCertificateVerifier(test::createTestCertificateVerifier())
                            .build();
            quicClient_ = std::make_shared<quic::QuicClientTransport>(
                    evb, std::move(sock), std::move(fizzClientContext));
            quicClient_->setHostname("echo.com");
            quicClient_->addNewPeerAddress(addr);

            TransportSettings settings;
            quicClient_->setTransportSettings(settings);

//                quicClient_->setTransportStatsCallback(
//                        std::make_shared<LogQuicStats>("client"));

            LOG(INFO) << "MvFstConnector connecting to " << addr.describe();
            quicClient_->start(this);
        });
        _connected = true;
        startDone_.wait();
        LOG(INFO) << "MvFstConnector stopping client";
    }

    ~MvFstConnector() override = default;

    bool sendMessage(const char* buffer, size_t bufferSize) {
        folly::ScopedEventBaseThread networkThread("EchoClientThread");
        auto evb = networkThread.getEventBase();
        evb->runInEventBaseThreadAndWait([=] {
                // create new stream for each message
                auto streamId = quicClient_->createBidirectionalStream().value();
                quicClient_->setReadCallback(streamId, this);
                pendingOutput_[streamId].append(folly::IOBuf::copyBuffer(buffer, bufferSize));
                sendMessage(streamId, pendingOutput_[streamId]);
        });
        return true;
    }

    bool isConnected() const {
        return _connected;
    }

private:
    void sendMessage(quic::StreamId id, BufQueue& data) {
        auto message = data.move();
        auto res = quicClient_->writeChain(id, message->clone(), true);
        if (res.hasError()) {
            LOG(ERROR) << "MvFstConnector writeChain error=" << uint32_t(res.error());
        } else {
            auto str = message->moveToFbString().toStdString();
            LOG(INFO) << "MvFstConnector wrote \"" << str << "\""
                      << ", len=" << str.size() << " on stream=" << id;
            // sent whole message
            pendingOutput_.erase(id);
        }
    }

    std::string host_;
    uint16_t port_;
    std::shared_ptr<quic::QuicClientTransport> quicClient_;
    std::map<quic::StreamId, BufQueue> pendingOutput_;
    std::map<quic::StreamId, uint64_t> recvOffsets_;
    folly::fibers::Baton startDone_;
    bool _connected = false;
};

bool Protocol::MvfstProtocolQUIC::openProtocol(std::string address, uint port) {
    _mvfst = new MvFstConnector(address, port);
    _mvfst->start();
    return _mvfst->isConnected();
}

bool Protocol::MvfstProtocolQUIC::openProtocolServer(uint port) {
    // TODO implement
    return false;
}

bool Protocol::MvfstProtocolQUIC::send(const char *buffer, size_t bufferSize) {
    if (!_mvfst->isConnected()) {
        return false;
    }
    _mvfst->sendMessage(buffer, bufferSize);
    return _mvfst->isConnected();
}

bool Protocol::MvfstProtocolQUIC::closeProtocol() {
    delete _mvfst;
    _mvfst = nullptr;
    return true;
}

Protocol::MvfstProtocolQUIC::~MvfstProtocolQUIC() {
    delete _mvfst;
}
