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
        uint64_t numOfRecords = 0;
        uint64_t receivedNumOfRecords = 0;
        uint64_t receivedBytes = 0;
        uint64_t numOfRecordsForOnePercent = 0;
        ushort percentageReceived = 0;
        uint64_t lastPercentageReceivedBytes = 0;
        std::chrono::time_point<std::chrono::high_resolution_clock> startTime;
        std::chrono::time_point<std::chrono::high_resolution_clock> lastPercentStartTime;
        //Utils::CSVWriter throughputRecordCSV = Utils::CSVWriter(",");

        bool fileReceived() const {
            assert(receivedNumOfRecords <= numOfRecords); // We should never receive more than the file size
            return receivedNumOfRecords >= numOfRecords;
        }

        void printPercentageStatsIfFullPercent() {
            ushort currentPercentage = receivedNumOfRecords / numOfRecordsForOnePercent;
            // Print when enough time has passed too (30) seconds!
            auto currentTime = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - lastPercentStartTime);
            if (currentPercentage > percentageReceived || duration.count() > 60000) {
                uint64_t sentBytes = receivedBytes - lastPercentageReceivedBytes;
                std::cout << "Data transfer: " << currentPercentage << "% (Total: "<< receivedNumOfRecords << " received out of " << numOfRecords << " bytes. " <<"). Sent " << sentBytes << " over " << duration.count()/1000.0 << " milliseconds" << std::endl;
                std::cout << "    Speed is " << ((double) sentBytes) / duration.count() * 8 << " mb/s." << std::endl;
                percentageReceived = currentPercentage;
                lastPercentageReceivedBytes = receivedNumOfRecords;
                lastPercentStartTime = currentTime;
            }
        }
    };

public:
    bool start(Protocol::ProtocolType type, const std::string& listenAddress, ushort port, uint cipher);
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
