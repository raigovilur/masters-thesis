#include <glog/logging.h>
#include <quic/server/QuicServerTransport.h>


#include <quic/api/QuicSocket.h>

#include <folly/io/async/EventBase.h>
#include <quic/common/BufUtil.h>

#include <fizz/crypto/Utils.h>
#include <folly/init/Init.h>
#include <boost/program_options.hpp>
#include <iostream>

#include "src/Server.h"
#include "src/appProto/ProtocolType.h"

namespace po = boost::program_options;
int main(int argc, char *argv[]) {

    Protocol::ProtocolType protocolType;
    std::string listenAddress;
    unsigned port;
    std::string filePath;
    std::string pkeyPath;
    std::string certPath;

    try {
        po::options_description desc("Allowed options");
        desc.add_options()
                ("help", "produce help message")
                ("protocol", po::value<std::string>(), "Set protocol")
                ("address", po::value<std::string>(), "Set listen address")
                ("port", po::value<unsigned>(), "Set listen port")
                ("path", po::value<std::string>(), "Set destination base path")
                ("pkey", po::value<std::string>(), "Private key path")
                ("cert", po::value<std::string>(), "Certificate path")
                ;

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        if (vm.count("help")) {
            std::cout << desc << std::endl;
            return 0;
        }

        if (vm.count("pkey")) {
            pkeyPath = vm["pkey"].as<std::string>();
        } else {
            std::cerr << "Private key was not set." << std::endl;
            return -1;
        }

        if (vm.count("cert")) {
            certPath = vm["cert"].as<std::string>();
        } else {
            std::cerr << "Certificate was not set." << std::endl;
            return -1;
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
            listenAddress = vm["address"].as<std::string>();
        } else {
            std::cout << "Listen address was not set. Defaulting to 0.0.0.0" << std::endl;
            listenAddress = "0.0.0.0";
        }

        if (vm.count("port")) {
            port = vm["port"].as<unsigned>();
        } else {
            std::cout << "Port was not set, defaulting to 80" << std::endl;
            port = 80;
        }

        if (vm.count("path")) {
            filePath = vm["path"].as<std::string>();
        } else {
            std::cout << "File path was not set. Defaulting to local directory" << std::endl;
            filePath = "";
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


    FLAGS_logtostderr = true;
    // Not passing program arguments to folly:
    int arguments = 1; //Only exe name
    folly::Init init(&arguments, &argv);
    fizz::CryptoUtils::init();

    Server server;
    server.readCertificate(pkeyPath, certPath);
    server.start(protocolType, listenAddress, port);

}