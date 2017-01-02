﻿#include "url_ip.h"

namespace whatwg {

template <typename UIntT>
inline void unsigned_to_str(UIntT num, std::string& output, UIntT base) {
    static const char digit[] = "0123456789abcdef";

    // divider
    UIntT divider = 1;
    const UIntT num0 = num / base;
    while (divider <= num0) divider *= base;

    // convert
    do {
        output.push_back(digit[num / divider % base]);
        divider /= base;
    } while (divider);
}

void ipv4_serialize(uint32_t ipv4, std::string& output) {
    for (unsigned shift = 24; shift != 0; shift -= 8) {
        unsigned_to_str<uint32_t>((ipv4 >> shift) & 0xFF, output, 10);
        output.push_back('.');
    }
    unsigned_to_str<uint32_t>(ipv4 & 0xFF, output, 10);
}


} // namespace whatwg
