#ifndef PROTOCOL_TESTBED_UTILS_H
#define PROTOCOL_TESTBED_UTILS_H

#include <openssl/sha.h>

namespace Utils {
    bool compareBytes(const std::vector<unsigned char>& vector, size_t from, size_t rangeLength, const std::vector<unsigned char>& target) {

        for (size_t i = 0; i < rangeLength; ++i) {
            if (vector.at(from + i) != target[i]) {
                return false;
            }
        }
        return true;
    }

    bool calculateSha256(const std::string& path, unsigned char* hash)
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
}

#endif //PROTOCOL_TESTBED_UTILS_H
