#include <glog/logging.h>
#include <quic/server/QuicServerTransport.h>


#include <quic/api/QuicSocket.h>

#include <folly/io/async/EventBase.h>
#include <quic/common/BufUtil.h>

#include <fizz/crypto/Utils.h>
#include <folly/init/Init.h>

#include "src/Server.h"
#include "src/appProto/ProtocolType.h"

int main(int argc, char *argv[]) {
    FLAGS_logtostderr = true;
    folly::Init init(&argc, &argv);
    fizz::CryptoUtils::init();

    Server server;
    server.start(Protocol::OpenSSL_DTLS1_2, "0.0.0.0", 12345);

}