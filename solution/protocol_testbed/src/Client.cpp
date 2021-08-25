#include "Client.h"
#include <fstream>
#include <iostream>
#include <cstring>
#include <openssl/sha.h>
#include <vector>
//#include <experimental/filesystem>
#include <appProto/utils.h>
#include <iomanip>

#include <math.h>

#include<boost/filesystem.hpp>

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


void Client::send(const std::string& path, size_t bufferSize, uint retryCount, Utils::TimeRecorder *timeRecorder) {
    
    //auto* hash = new char[SHA256_DIGEST_LENGTH] {0};

    auto startTime = std::chrono::system_clock::now();

    //std::cout << "Calculating checksum ..." << std::endl;
    /* 
    timeRecorder->writeInfoWithTime("Calcunating checksum");
    if (!Utils::calculateSha256(path, (unsigned char*) hash)) {
        std::cout << "Unable to calculate hash for file: " << path << std::endl;
        return;
    }
    timeRecorder->writeInfoWithTime("Checksum calculated");
    std::stringstream hashHexStrStream;
    hashHexStrStream << std::hex;

    for( int i(0) ; i < SHA256_DIGEST_LENGTH; ++i )
        hashHexStrStream << std::setw(2) << std::setfill('0') << (int)hash[i];

    std::cout << "Checksum calculated: " << hashHexStrStream.str() << std::endl;
    */

    //| Protocol ID (2 bytes) | Protocol version (2 bytes) | checksum for the file (64 bytes) | file size (64 bytes) | file name (1024 bytes ending 0 bytes) | file itself... |

    //_elapsedHashSeconds = std::chrono::system_clock::now() - startTime;
    std::string recordFilename = "vehicle_velocity.csv";
    boost::filesystem::ifstream fileStream(recordFilename);
    auto numOfRecords = Utils::getLineNum(recordFilename);
    
    //get length of file
    //fileStream.seekg(0, std::ios::end);
    //_fileSize = fileStream.tellg();
    //fileStream.seekg(0, std::ios::beg);

    //std::unique_ptr<char[]> header(new char[ISE_HEADER_SIZE] {0});

    // Composing header
    // Protocol ID is 0
    //Utils::writeToByteArray(header.get(), ISE_PROTOCOL_OFFSET, {0x00, 0x00});
    // Version is 0
    //Utils::writeToByteArray(header.get(), ISE_PROTOCOL_VERSION_OFFSET, {0x00, 0x00});
    // SHA-256 file checksum
    //Utils::writeToByteArray(header.get(), ISE_CHECKSUM_OFFSET, std::vector<char>(hash, hash + SHA256_DIGEST_LENGTH));
    // File size
    //Utils::writeToByteArray(header.get(), ISE_FILE_SIZE_OFFSET, Utils::convertToCharArray(_fileSize, ISE_FILE_SIZE));

    // File Name
    //std::string fileName = std::filesystem::path(path).filename();
    //std::string fileName = path;
    //const size_t last_slash_idx = fileName.find_last_of("\\/");
    //if (std::string::npos != last_slash_idx)
    //{
    //    fileName.erase(0, last_slash_idx + 1);
    //}
    //Utils::writeToByteArray(header.get(), ISE_FILE_NAME_OFFSET, std::vector<char>(fileName.begin(), fileName.end()));

    //delete[] hash;

    //uint buffersNeeded = _fileSize / bufferSize; //numOfBuffer
    //uint lastPacketSize = _fileSize % bufferSize;

    Protocol::ProtocolPtr protocol = Protocol::ProtocolFactory::getInstance(_type);
    startTime = std::chrono::system_clock::now();
    timeRecorder->writeInfoWithTime("Connecting");
    if (!protocol->openProtocol(_address, _port, _options)) {
        protocol->closeProtocol();
        return;
    }
    timeRecorder->writeInfoWithTime("Connected");
    std::cout << "Connection established. Time taken: "
    << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - startTime).count() << " milliseconds" << std::endl;
    //std::cout << "Sending header" << std::endl;
    // Sending header
    /*
    timeRecorder->writeInfoWithTime("Sending header");
    if (!sendWithRetries(header.get(), ISE_HEADER_SIZE, retryCount, protocol,
                         buffersNeeded == 0 && lastPacketSize == 0)) {
        std::cout << "Client: Connection failed" << std::endl;
        protocol->closeProtocol();
        _elapsedSeconds = std::chrono::system_clock::now() - startTime;
        return;
    }
    timeRecorder->writeInfoWithTime("Header sent");
    */

    std::cout << "buffersNeeded: " << numOfRecords << std::endl;
    double onePercentage = 1.0/(numOfRecords) * 100; 
    int numOfRecordsForOnePercent = (int) (1.0 / onePercentage);
    if (numOfRecordsForOnePercent <= 0) {
        numOfRecordsForOnePercent = 1;
    }

    boost::filesystem::ifstream file("vehicle_velocity.csv");
    std::string line;
    auto intermediateTimeStamp = std::chrono::system_clock::now();
    // Sending full buffer amounts
    for (uint i = 0; i < numOfRecords; ++i) {
        getline(file, line);
        const char *buffer = line.c_str();
        bufferSize = sizeof(buffer);
        //fileStream.read(buffer.get(), bufferSize);
        if ((i + 1) % numOfRecordsForOnePercent == 0) {
            if (_type == Protocol::MVFST_QUIC)
                protocol->isAllSent(); //If there's items that haven't been sent to the server yet, wait here.
            std::cout << "Sending file: " << (double)((i + 1) * onePercentage) << "%" << std::endl;
        
            timeRecorder->writeInfoWithTime(std::to_string((double)((i + 1) * onePercentage)) + "% " +  "file sent");
            uint elapsedSeconds = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - intermediateTimeStamp).count();
            uint recordsSent = numOfRecordsForOnePercent * bufferSize;
            std::cout << "    Sent " << recordsSent << " bytes over "
                <<  elapsedSeconds/1000.0 << " seconds." << std::endl;
            std::cout << "    Speed is " << ((double) recordsSent) / elapsedSeconds / 1000 * 8 << " mb/s." << std::endl;
            intermediateTimeStamp = std::chrono::system_clock::now();
        }
        if (!sendWithRetries(buffer, bufferSize, retryCount, protocol,
                             i == numOfRecords - 1)) {
            std::cout << "Client: Connection failed" << std::endl;
            protocol->closeProtocol();
            _elapsedSeconds = std::chrono::system_clock::now() - startTime;
            return;
        }
    }

    // Sending last buffer
    /*
    if (lastPacketSize > 0) {
        fileStream.read(buffer.get(), lastPacketSize);
        sendWithRetries(buffer.get(), lastPacketSize, retryCount, protocol, true);
    }
    */

    timeRecorder->writeInfoWithTime("File sent");

    protocol->isAllSent();

    protocol->closeProtocol();
    _elapsedSeconds = std::chrono::system_clock::now() - startTime;
    std::cout << "File sent" << std::endl;
    fileStream.close();
}

bool Client::sendWithRetries(const char *buffer, size_t bufferSize, uint retryCount, Protocol::ProtocolPtr& protocol, bool eof) {
    for (uint retry = 0; retry < retryCount; ++retry) {
        if (protocol->send(buffer, bufferSize, eof)) {
            return true;
        } else {
            if (retry < retryCount - 1)
            {
                std::cout << "Connection dropped, retrying: (" << retry + 1 << ")"  << std::endl;
                protocol->closeProtocol();
                protocol->openProtocol(_address, _port, _options);
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
