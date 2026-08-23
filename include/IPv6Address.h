#pragma once

#include <IPAddress.h>
#include <cstring>

class IPv6Address {
private:
    uint32_t _address[4]{};

public:
    IPv6Address() = default;

    IPv6Address(const uint32_t* address) {
        if (address) {
            memcpy(_address, address, sizeof(_address));
        }
    }

    IPv6Address(const uint8_t* address) {
        if (address) {
            memcpy(_address, address, 16);
        }
    }

    IPv6Address(const IPAddress& addr) {
        for (int i = 0; i < 16; ++i) {
            reinterpret_cast<uint8_t*>(_address)[i] = addr[i];
        }
    }

    operator const uint32_t*() const {
        return _address;
    }

    operator IPAddress() const {
        return IPAddress(IPv6, reinterpret_cast<const uint8_t*>(_address));
    }

    uint8_t operator[](int index) const {
        return reinterpret_cast<const uint8_t*>(_address)[index];
    }
};
