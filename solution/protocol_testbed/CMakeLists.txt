cmake_minimum_required(VERSION 3.17)
project(protocol_testbed)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_VERBOSE_MAKEFILE TRUE)

if(NOT DEFINED MVFST_DIR)
    set(MVFST_DIR "/usr/local/lib")
endif()
message("${MVFST_DIR}")


find_package(OpenSSL)
find_package(fmt HINTS ${MVFST_DIR}/deps/lib/cmake/fmt)
find_package(folly HINTS ${MVFST_DIR}/deps/lib/cmake/folly)
find_package(fizz HINTS ${MVFST_DIR}/deps/lib/cmake/fizz)
find_package(mvfst HINTS ${MVFST_DIR}/lib/cmake/mvfst)

add_executable(protocol_testbed_client
        ClientMain.cpp
        src/Client.cpp
        src/Client.h
        src/protocol/ProtocolType.h
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
        )

target_include_directories(protocol_testbed_client
        PUBLIC src
        ${OPENSSL_INCLUDE_DIR}
        )

target_link_libraries(protocol_testbed_client ${OPENSSL_SSL_LIBRARY} ${OPENSSL_CRYPTO_LIBRARY} pthread)

add_executable(protocol_testbed_server
        ServerMain.cpp
        )

target_include_directories(protocol_testbed_server
        PUBLIC src
        ${OPENSSL_INCLUDE_DIR}
        )
target_link_libraries(protocol_testbed_server ${OPENSSL_SSL_LIBRARY} ${OPENSSL_CRYPTO_LIBRARY} pthread)