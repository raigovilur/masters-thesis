#ifndef PROTOCOL_TESTBED_SERVER_H
#define PROTOCOL_TESTBED_SERVER_H

#include <cstddef>
#include <utility>
#include <vector>
#include <map>
#include "serverProtocol/ServerInterface.h"
#include "appProto/ProtocolType.h"
#include <fstream>
#include <cassert>

class Server : public ServerProto::ServerReceiveCallback {
private:
    class ClientData {
    public:
        explicit ClientData(std::vector<unsigned char> newBytes) : header(std::move(newBytes)) { }

        std::vector<unsigned char> header;
        bool headerProcessed = false;
        std::unique_ptr<std::ofstream> outputStream = nullptr;
        std::string fileName;
        uint64_t fileSize = 0;
        uint64_t receivedFileSize = 0;

        bool fileReceived() const {
            assert(receivedFileSize <= fileSize); // We should never receive more than the file size
            return receivedFileSize >= fileSize ;
        }
    };

public:
    bool start(Protocol::ProtocolType type, const std::string& listenAddress, ushort port);

    bool consume(std::string client, unsigned char *bytes, size_t packetLength) override;

private:
    static bool processHeader(ClientData& data) ;
    static bool processDataStream(ClientData &client, unsigned char *bytes, size_t processStart, size_t packetLength);
    std::map<std::string, ClientData> _clientHeaders;

};


#endif //PROTOCOL_TESTBED_SERVER_H
