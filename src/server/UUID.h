//
// Created by msullivan on 11/11/24.
//

#include <random>
#include <sstream>
#include <iomanip>

class UUID {
public:
    UUID()
    {
        generate();
    }

    std::string toString() const
    {
        std::ostringstream oss;
        oss << std::hex << std::setfill('0');
        for (size_t i = 0; i < 16; ++i) {
            oss << std::setw(2) << static_cast<int>(data[i]);
            if (i == 3 || i == 5 || i == 7 || i == 9) {
                oss << '-';
            }
        }
        return oss.str();
    }

private:
    unsigned char data[16];

    void generate() {
        std::random_device rd;
        std::mt19937 mt(rd());
        std::uniform_int_distribution<int> dist(0, 255);
        for (size_t i = 0; i < 16; ++i) {
            data[i] = static_cast<unsigned char>(dist(mt));
        }
        // Set the version and variant bits according to RFC 4122
        data[6] = (data[6] & 0x0f) | 0x40; // Version 4
        data[8] = (data[8] & 0x3f) | 0x80; // Variant 10
    }
};