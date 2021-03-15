cmake_minimum_required(VERSION 3.17)
project(protocol_testbed)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_VERBOSE_MAKEFILE TRUE)

find_package(OpenSSL)

add_executable(protocol_testbed_client
        ClientMain.cpp
        src/Client.cpp
        src/Client.h
        src/protocol/ProtocolType.h
        src/protocol/ProtocolInterface.h src/protocol/ProtocolFactory.cpp src/protocol/ProtocolFactory.h src/protocol/DummyProtocol.cpp src/protocol/DummyProtocol.h src/protocol/OpenSSLProtocolTLS.cpp src/protocol/OpenSSLProtocolTLS.h)

target_include_directories(protocol_testbed_client
        PUBLIC src
        ${OPENSSL_INCLUDE_DIR}
        )

message(STATUS "OpenSSL Libraries: ${OPENSSL_SSL_LIBRARY} ${OPENSSL_CRYPTO_LIBRARY}")
target_link_libraries(protocol_testbed_client ${OPENSSL_SSL_LIBRARY} ${OPENSSL_CRYPTO_LIBRARY})