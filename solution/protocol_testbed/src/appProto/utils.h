#ifndef PROTOCOL_TESTBED_UTILS_H
#define PROTOCOL_TESTBED_UTILS_H

namespace Utils {
    bool compareBytes(const std::vector<unsigned char>& vector, size_t from, size_t rangeLength, const std::vector<unsigned char>& target) {

        for (size_t i = 0; i < rangeLength; ++i) {
            if (vector.at(from + i) != target[i]) {
                return false;
            }
        }
        return true;
    }
}

#endif //PROTOCOL_TESTBED_UTILS_H
