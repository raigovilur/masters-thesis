#ifndef PROTOCOL_TESTBED_UTILS_H
#define PROTOCOL_TESTBED_UTILS_H

#include <openssl/sha.h>
#include <fstream>
#include <string>

#include "ProtocolType.h"

namespace Utils {
    inline bool compareBytes(const std::vector<unsigned char>& vector, size_t from, size_t rangeLength, const std::vector<unsigned char>& target) {

        for (size_t i = 0; i < rangeLength; ++i) {
            if (vector.at(from + i) != target[i]) {
                return false;
            }
        }
        return true;
    }

    inline bool calculateSha256(const std::string& path, unsigned char* hash)
    {
        std::ifstream fp(path, std::ios::binary);

        if (not fp.good()) {
            std::cout << "Cannot open file to checksum: \"" << path << "\": " << std::strerror(errno) << std::endl;
            return false;
        }

        constexpr const std::size_t bufferSize {1 << 12 };
        char buffer[bufferSize];

        SHA256_CTX ctx;
        SHA256_Init(&ctx);

        while (fp.good()) {
            fp.read(buffer, bufferSize);
            SHA256_Update(&ctx, buffer, fp.gcount());
        }

        SHA256_Final(hash, &ctx);
        fp.close();

        return true;
    }

    inline void writeToByteArray(char* array, size_t offset, const std::vector<char>& whatToWrite) {
        for (size_t i = 0; i < whatToWrite.size(); ++i) {
            array[offset + i] = whatToWrite[i];
        }
    }

    inline std::vector<char> convertToCharArray(size_t value, ushort charArraySize) {
        std::vector<char> output;
        output.resize(charArraySize);

        for (int i = charArraySize - 1; i >= 0; --i) {
            output[i] = value & 0xFF;
            value = value >> 8;
        }

        return output;
    }

    template<typename T>
    inline T convertToUnsignedTemplated(const std::vector<unsigned char>& data, size_t dataStart, int byteLength) {
        T output = 0;
        for (int i = 0; i < byteLength; ++i) {
            output = output << 8;
            output = output | data[dataStart + i];
        }
        return output;
    }

    template<typename T>
    inline T convertToUnsignedTemplated(const unsigned char* data, size_t dataStart, int byteLength) {
        T output = 0;
        for (int i = 0; i < byteLength; ++i) {
            output = output << 8;
            output = output | data[dataStart + i];
        }
        return output;
    }

    inline uint64_t convertToUint64_t(const std::vector<unsigned char>& data, size_t dataStart, int byteLength) {
        uint64_t output = 0;
        for (int i = 0; i < byteLength; ++i) {
            output = output << 8;
            output = output | data[dataStart + i];
        }
        return output;
    }

        
    inline std::string getProtocolName(Protocol::ProtocolType type) {

        switch (type) {
            case Protocol::OpenSSL_DTLS1_2:
                return "DTLSv1.2";
            case Protocol::OpenSSL_TLS:
                return "TLSv1.3";
            case Protocol::MVFST_QUIC:
                return "QUIC";
            case Protocol::TCP:
                return "TCP";
            case Protocol::UDP:
                return "UDP";
            default:
                std::cerr << "Invalid protocol specified: " << std::endl;
                return "";
        }
    }

}

#endif //PROTOCOL_TESTBED_UTILS_H
