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
    unsigned stport;
    std::string stportBandwidth = "40M";
    unsigned bufferSize = 1024;
    unsigned udpTarget = 30;
    std::string filePath;

    try {
        po::options_description desc("Allowed options");
        desc.add_options()
                ("help", "produce help message")
                ("protocol", po::value<std::string>(), "Set protocol")
                ("address", po::value<std::string>(), "Set destination address")
                ("port", po::value<unsigned>(), "Set destination port")
                ("stport", po::value<unsigned>(), "Set speed test port")
                ("udpTarget", po::value<unsigned>(), "Set UDP transfer target speed (Megabytes per second)")
                ("stportBW", po::value<std::string>(), "Set speed test bandwidth (UDP ONLY). M for megabytes")

                ("buffer-size", po::value<unsigned>(), "Set file reading buffer size")
                ("file", po::value<std::string>(), "Set path to file for transfer")
                ;

        po::variables_map vm;
        po::store(po::command_line_parser(argc, argv).options(desc).allow_unregistered().run(), vm);
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

        if (vm.count("udpTarget")) {
            udpTarget = vm["udpTarget"].as<unsigned>();
        }

        if (vm.count("stport")) {
            stport = vm["stport"].as<unsigned>();
        } else {
            std::cout << "stport was not set, defaulting to " << port + 1 << "(port + 1)" << std::endl;
            stport = port + 1;
        }
        if (vm.count("stportBW")) {
            stportBandwidth = vm["stportBW"].as<std::string>();
        }

        if (vm.count("buffer-size")) {
            bufferSize = vm["buffer-size"].as<unsigned>();
        } else {
            std::cout << "buffer-size was not set, defaulting to 1024" << std::endl;
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

    Protocol::Options options;
    options.UDPtarget = udpTarget;

    Client client(protocolType, dest, port, options);

    client.runSpeedTest(stport, stportBandwidth);
    client.send(filePath, bufferSize, 5);
    client.runSpeedTest(stport, stportBandwidth);

    client.printStatistics();

    return 0;
}
