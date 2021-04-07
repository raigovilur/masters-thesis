#include <iostream>
#include <fstream>

#include "Client.h"
#include "appProto/ProtocolType.h"
#include <boost/program_options.hpp>

namespace po = boost::program_options;

int main(int argc, char *argv[]) {

    Protocol::ProtocolType protocolType;
    std::string dest;
    unsigned port;
    unsigned bufferSize;
    std::string filePath;

    try {
        po::options_description desc("Allowed options");
        desc.add_options()
                ("help", "produce help message")
                ("protocol", po::value<std::string>(), "Set protocol")
                ("address", po::value<std::string>(), "Set destination address")
                ("port", po::value<unsigned>(), "Set destination port")
                ("buffer-size", po::value<unsigned>(), "Set file reading buffer size")
                ("file", po::value<std::string>(), "Set path to file for transfer")
                ;

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        if (vm.count("help")) {
            std::cout << desc << std::endl;
            return 0;
        }

        if (vm.count("protocol")) {
            std::string protoArgValue = vm["protocol"].as<std::string>();
            if(protoArgValue == "DTLS") {
                protocolType = Protocol::OpenSSL_DTLS1_2;
            } else if (protoArgValue == "TLS") {
                protocolType = Protocol::OpenSSL_TLS;
            } else if (protoArgValue == "QUIC") {
                protocolType = Protocol::MVFST_QUIC;
            } else {
                std::cerr << "Invalid protocol specified: " << protoArgValue << std::endl;
                return -1;
            }
        } else {
            std::cerr << "Protocol was not set." << std::endl;
            return -1;
        }

        if (vm.count("address")) {
            dest = vm["address"].as<std::string>();
        } else {
            std::cerr << "Address was not set." << std::endl;
            return -1;
        }

        if (vm.count("port")) {
            port = vm["port"].as<unsigned>();
        } else {
            std::cout << "Port was not set, defaulting to 80" << std::endl;
            port = 80;
        }

        if (vm.count("bufferSize")) {
            port = vm["bufferSize"].as<unsigned>();
        } else {
            std::cout << "BufferSize was not set, defaulting to 1024" << std::endl;
            bufferSize = 1024;
        }

        if (vm.count("file")) {
            filePath = vm["file"].as<std::string>();
        } else {
            std::cerr << "File path was not set." << std::endl;
            return -1;
        }
    }
    catch(std::exception& e) {
        std::cerr << "error: " << e.what() << std::endl;
        return 1;
    }
    catch(...) {
        std::cerr << "Exception of unknown type!" << std::endl;
        return 1;
    }

    Client client(protocolType, dest, port);

    client.send(filePath, bufferSize, 5);
    //client.send("/home/raigo/repos/masters-thesis/solution/protocol_testbed/src/protocol/OpenSSLProtocolDTLS.cpp", 1024, 5);

    client.printStatistics();

    return 0;
}