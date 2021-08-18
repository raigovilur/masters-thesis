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
#include <chrono>
#include <iostream>
#include <thread>

//#include "util/CSVWriter.h"
//#include "util/Timer.h"

class Server : public ServerProto::ServerReceiveCallback {
private:
    class ClientData {
    public:
        explicit ClientData(std::vector<unsigned char> newBytes) : header(std::move(newBytes)) { }

        std::vector<unsigned char> header;
        bool headerProcessed = false;
        std::unique_ptr<std::ofstream> outputStream = nullptr;
        std::string fileName;
        std::vector<unsigned char> checksum;
        uint64_t fileSize = 0;
        uint64_t receivedFileSize = 0;
        uint64_t onePercentBytes = 0;
        ushort percentageReceived = 0;
        uint64_t lastPercentageFileSize = 0;
        std::chrono::time_point<std::chrono::system_clock> startTime;
        std::chrono::time_point<std::chrono::system_clock> lastPercentStartTime;
        //Utils::CSVWriter throughputRecordCSV = Utils::CSVWriter(",");

        bool fileReceived() const {
            assert(receivedFileSize <= fileSize); // We should never receive more than the file size
            return receivedFileSize >= fileSize;
        }

        void printPercentageStatsIfFullPercent() {
            ushort currentPercentage = receivedFileSize / onePercentBytes;
            // Print when enough time has passed too (30) seconds!
            auto currentTime = std::chrono::system_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastPercentStartTime);
            if (currentPercentage > percentageReceived || duration.count() > 60000) {
                uint64_t sentBytes = receivedFileSize - lastPercentageFileSize;
                std::cout << "File transfer: " << currentPercentage << "% (Total: "<< receivedFileSize << " received out of " << fileSize << " bytes. " <<"). Sent " << sentBytes << " over " << duration.count() << " milliseconds" << std::endl;
                std::cout << "    Speed is " << ((double) sentBytes) / duration.count() / 1000 * 8 << " mb/s." << std::endl;
                /*
                throughputRecordCSV.newRow() << currentPercentage << ((double) sentBytes) / duration.count() / 1000 * 8;
                if (currentPercentage >= 99) {
                    std::string throughputRecordFileName = std::string("./out") + std::string("/") + Utils::getCurrentDateTime() + std::string("/") + std::string("performance.csv");
                    throughputRecordCSV.writeToFile(throughputRecordFileName, false);
                }
                */
                percentageReceived = currentPercentage;
                lastPercentageFileSize = receivedFileSize;
                lastPercentStartTime = currentTime;
            }
        }
    };

public:
    bool start(Protocol::ProtocolType type, const std::string& listenAddress, ushort port);
    bool readCertificate(const std::string& privKeyPath, const std::string& certPath);

    bool consume(std::string client, const unsigned char *bytes, size_t packetLength) override;
    bool consumeInternal(const std::string& client, const unsigned char *bytes, size_t packetLength);

private:
    static bool processHeader(ClientData& data) ;
    static bool processDataStream(ClientData &client, const unsigned char *bytes, size_t processStart, size_t packetLength);
    std::map<std::string, ClientData> _clientHeaders;
    std::string _cert;
    std::string _privKey;

    std::unique_ptr<std::thread> _t1;

};


#endif //PROTOCOL_TESTBED_SERVER_H
