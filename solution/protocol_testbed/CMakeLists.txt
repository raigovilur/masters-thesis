cmake_minimum_required(VERSION 3.17)
project(protocol_testbed)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_VERBOSE_MAKEFILE TRUE)

if(NOT DEFINED MVFST_DIR)
    set(MVFST_DIR "/usr/local/lib")
endif()
message("${MVFST_DIR}")


find_package(OpenSSL)
find_package(fmt)
find_package(folly)
find_package(fizz)
find_package(mvfst)
find_package(gflags)

add_executable(protocol_testbed_client
        ClientMain.cpp
        src/Client.cpp
        src/Client.h
        src/appProto/ProtocolType.h
        src/protocol/ProtocolInterface.h
        src/protocol/ProtocolFactory.cpp
        src/protocol/ProtocolFactory.h
        src/protocol/DummyProtocol.cpp
        src/protocol/DummyProtocol.h
        src/protocol/OpenSSLProtocolTLS.cpp
        src/protocol/OpenSSLProtocolTLS.h
        src/protocol/OpenSSLProtocolDTLS.cpp
        src/protocol/OpenSSLProtocolDTLS.h
        src/protocol/MvfstProtocolQUIC.cpp
        src/protocol/MvfstProtocolQUIC.h
        src/appProto/FileSendHeader.h
        )

message(STATUS "${mvfst_INCLUDE_DIRS}")
message(STATUS "${FOLLY_INCLUDE_DIR}")
target_include_directories(protocol_testbed_client
        PUBLIC src
        ${OPENSSL_INCLUDE_DIR}
        ${FOLLY_INCLUDE_DIR}
        ${mvfst_INCLUDE_DIRS}
        )

target_link_libraries(protocol_testbed_client ${OPENSSL_SSL_LIBRARY} ${OPENSSL_CRYPTO_LIBRARY} pthread mvfst::mvfst_transport mvfst::mvfst_client mvfst::mvfst_fizz_client)

add_executable(protocol_testbed_server
        ServerMain.cpp
        src/serverProtocol/ServerMvfstProtocolQUIC.cpp
        src/serverProtocol/ServerMvfstProtocolQUIC.h
        src/serverProtocol/ServerInterface.h
        src/serverProtocol/ServerProtocolFactory.h
        src/serverProtocol/ServerProtocolFactory.cpp
        src/Server.cpp
        src/Server.h
        )

target_include_directories(protocol_testbed_server
        PUBLIC src
        ${OPENSSL_INCLUDE_DIR}
        ${mvfst_INCLUDE_DIRS}
        )
target_link_libraries(protocol_testbed_server
        ${OPENSSL_SSL_LIBRARY}
        ${OPENSSL_CRYPTO_LIBRARY}
        pthread
        mvfst::mvfst_transport
        mvfst::mvfst_state_machine
        mvfst::mvfst_server
        mvfst::mvfst_codec_pktbuilder
        mvfst::mvfst_codec_types
        mvfst::mvfst_fizz_handshake
        fizz::fizz
        Folly::folly)