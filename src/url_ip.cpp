// Copyright 2016-2019 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//

#include "url_ip.h"

namespace whatwg {

// IPv4

void ipv4_serialize(uint32_t ipv4, std::string& output) {
    for (unsigned shift = 24; shift != 0; shift -= 8) {
        unsigned_to_str<uint32_t>((ipv4 >> shift) & 0xFF, output, 10);
        output.push_back('.');
    }
    unsigned_to_str<uint32_t>(ipv4 & 0xFF, output, 10);
}

// IPv6

static std::size_t longest_zero_sequence(
    const uint16_t* first, const uint16_t* last,
    const uint16_t*& compress, const uint16_t*& compress_end)
{
    std::size_t last_count = 0;
    for (auto it = first; it != last; ++it) {
        if (*it == 0) {
            auto ite = it + 1;
            while (ite != last && *ite == 0) ++ite;
            const std::size_t count = ite - it;
            if (last_count < count) {
                last_count = count;
                compress = it;
                compress_end = ite;
            }
            if (ite == last) break;
            it = ite; // ++it in the loop skips non-zero number
        }
    }
    return last_count;
}

void ipv6_serialize(const uint16_t(&address)[8], std::string& output) {
    const uint16_t *first = std::begin(address);
    const uint16_t *last = std::end(address);

    const uint16_t *compress;
    const uint16_t *compress_end;
    if (longest_zero_sequence(first, last, compress, compress_end) <= 1)
        compress = compress_end = last;

    // "it" pointer corresponds to pieceIndex in the URL standard
    for (auto it = first; true;) {
        if (it == compress) {
            output.append("::", it == first ? 2 : 1);
            if ((it = compress_end) == last) break;
        }
        unsigned_to_str<uint32_t>(*it, output, 16);
        if (++it == last) break;
        output.push_back(':');
    }
}


} // namespace whatwg
