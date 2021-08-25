#include "Client.h"
#include <fstream>
#include <iostream>
#include <cstring>
#include <openssl/sha.h>
#include <vector>
//#include <experimental/filesystem>
#include <appProto/utils.h>
#include <iomanip>
#include <thread>

#include <math.h>

#include<boost/filesystem.hpp>
#include<nlohmann/json.hpp>

#include "appProto/FileSendHeader.h"
#include "protocol/ProtocolFactory.h"
#include "util/CSV.h"

Client::Client(Protocol::ProtocolType type, std::string address, uint port, Protocol::Options options)
    : _type(type)
    , _address(std::move(address))
    , _port(port)
    , _options(options) {

}

Client::~Client() { }

void Client::runSpeedTest(uint port, const std::string& bandwidth) const {
    std::cout << "Running speed test (iperf3):" << std::endl;
    std::string udpFlag = " -u";
    std::string baseCommand = "iperf3 -c " + _address + " -p " + std::to_string(port);
    std::string tcpCommand = baseCommand;
    std::string udpCommand = baseCommand + " -u -l 1000 -b " + bandwidth;
    system(tcpCommand.c_str());
    system(udpCommand.c_str());
    std::cout << "Measuring latency " << std::endl;
    system(("ping -c 10 " + _address).c_str());
}


void Client::send(const std::string& path, size_t bufferSize, uint retryCount, Utils::TimeRecorder *timeRecorder, uint cipher) {

    auto* hash = new char[SHA256_DIGEST_LENGTH] {0};

    auto startTime = std::chrono::high_resolution_clock::now();

    std::string recordFilename = path;
    boost::filesystem::ifstream fileStream(recordFilename);
    auto numOfRecords = Utils::getLineNum(recordFilename) - 1;

    std::unique_ptr<char[]> header(new char[ISE_HEADER_SIZE] {0});

    // Composing header
    // Protocol ID is 0
    Utils::writeToByteArray(header.get(), ISE_PROTOCOL_OFFSET, {0x00, 0x00});
    // Version is 0
    Utils::writeToByteArray(header.get(), ISE_PROTOCOL_VERSION_OFFSET, {0x00, 0x00});
    // SHA-256 dummy checksum
    Utils::writeToByteArray(header.get(), ISE_CHECKSUM_OFFSET, std::vector<char>(hash, hash + SHA256_DIGEST_LENGTH));
    // File size
    Utils::writeToByteArray(header.get(), ISE_FILE_SIZE_OFFSET, Utils::convertToCharArray(numOfRecords, ISE_FILE_SIZE));
    // File Name
    std::string fileName = path;
    const size_t last_slash_idx = fileName.find_last_of("\\/");
    if (std::string::npos != last_slash_idx)
    {
        fileName.erase(0, last_slash_idx + 1);
    }
    Utils::writeToByteArray(header.get(), ISE_FILE_NAME_OFFSET, std::vector<char>(fileName.begin(), fileName.end()));

    Protocol::ProtocolPtr protocol = Protocol::ProtocolFactory::getInstance(_type);
    startTime = std::chrono::high_resolution_clock::now();
    timeRecorder->writeInfoWithTime("Connecting");
    if (!protocol->openProtocol(_address, _port, cipher, _options)) {
        protocol->closeProtocol();
        return;
    }
    timeRecorder->writeInfoWithTime("Connected");
    std::cout << "Connection established. Time taken: "
    << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - startTime).count() << " milliseconds" << std::endl;

     if (!sendWithRetries(header.get(), ISE_HEADER_SIZE, retryCount, protocol,
                         false, cipher)) {
        std::cout << "Client: Connection failed" << std::endl;
        protocol->closeProtocol();
        _elapsedSeconds = std::chrono::high_resolution_clock::now() - startTime;
        return;
    }


    std::cout << "Number of Records: " << numOfRecords << std::endl;
    double onePercentage = 1.0/(numOfRecords) * 100; 
    int numOfRecordsForOnePercent = (int) (1.0 / onePercentage);
    if (numOfRecordsForOnePercent <= 0) {
        numOfRecordsForOnePercent = 1;
    }

    boost::filesystem::ifstream file(recordFilename);
    std::string line;
    auto intermediateTimeStamp = std::chrono::high_resolution_clock::now();
    // Sending full buffer amounts
    std::chrono::microseconds t1 = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch());
    std::vector<std::string> columns;
    std::chrono::milliseconds timespan(10);
    for (uint i = 0; i <= numOfRecords; ++i) {
        std::this_thread::sleep_for(timespan);
        getline(file, line);
        if (i == 0) {
            columns = Utils::parseCSVLine(line);
            continue;
        }
        auto data = Utils::parseCSVLine(line);

        if (data.size() != columns.size()) {
            std::cout << "Data Format Error" << std::endl;
            continue;
        }

        nlohmann::json j;
        for(uint i = 0; i < data.size(); ++i) {
            j[columns[i]] = data[i];
        }
        auto epoch = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch());
        long now = epoch.count();
        j["timestamp"] = now;
        std::string s = j.dump();
        const char *buffer = s.c_str();
        bufferSize = s.length();

        if ((i + 1) % numOfRecordsForOnePercent == 0) {
            if (_type == Protocol::MVFST_QUIC)
                protocol->isAllSent(); //If there's items that haven't been sent to the server yet, wait here.
            std::cout << "Sent " << i - 1 << " Records (" << (double)((i + 1) * onePercentage) << "%)" << std::endl;
        
            timeRecorder->writeInfoWithTime(std::to_string((double)((i + 1) * onePercentage)) + "% " +  "file sent");
            auto elapsedSeconds = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - intermediateTimeStamp).count();
            uint recordsSent = numOfRecordsForOnePercent * bufferSize;
            /*
            std::cout << "    Sent " << recordsSent << " bytes over "
                <<  elapsedSeconds/1000.0 << " milliseconds." << std::endl;
            */
            //std::cout << "    Speed is " << ((double) recordsSent) / elapsedSeconds * 8 << " mb/s." << std::endl;
            intermediateTimeStamp = std::chrono::high_resolution_clock::now();
        }
        if (!sendWithRetries(buffer, bufferSize, retryCount, protocol,
                             i == numOfRecords, cipher)) {
            std::cout << "Client: Connection failed" << std::endl;
            protocol->closeProtocol();
            _elapsedSeconds = std::chrono::high_resolution_clock::now() - startTime;
            return;
        }
    }
    std::chrono::microseconds t2 = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch());

    timeRecorder->writeInfoWithTime("Records sent");

    protocol->isAllSent();
    protocol->closeProtocol();
    _elapsedSeconds = std::chrono::high_resolution_clock::now() - startTime;
    std::cout << "Records sent" << std::endl;
    fileStream.close();
}

bool Client::sendWithRetries(const char *buffer, size_t bufferSize, uint retryCount, Protocol::ProtocolPtr& protocol, bool eof, uint cipher) {
    for (uint retry = 0; retry < retryCount; ++retry) {
        if (protocol->send(buffer, bufferSize, eof)) {
            return true;
        } else {
            if (retry < retryCount - 1)
            {
                std::cout << "Connection dropped, retrying: (" << retry + 1 << ")"  << std::endl;
                protocol->closeProtocol();
                protocol->openProtocol(_address, _port, cipher, _options);
                ++_connectionDrops;
            }
        }
    }
    return false;
}

void Client::printStatistics() const {
    std::cout << "Statistics:" << std::endl;
    std::cout << "Sent " << _fileSize << " bytes" << std::endl;
    std::cout << "    over " << _elapsedSeconds.count() << " seconds " << std::endl;
    std::cout << "    Average file transfer speed was " << (_fileSize / _elapsedSeconds.count()) / 1000.0 / 1000 * 8 << " mbit/s" << std::endl;
    std::cout << "Connection dropped " << _connectionDrops << " times " << std::endl;
    std::cout << "Hashing took " << _elapsedHashSeconds.count() << " seconds " << std::endl;
}
