﻿#ifndef WHATWG_URL_IP_H
#define WHATWG_URL_IP_H

#include "url_canon.h"
#include "url_result.h"
#include <cassert>
#include <cstdint> // uint32_t
#include <limits>
#include <string>

namespace whatwg {

// IPv4 parser

// TODO-WARN: validationErrorFlag
template <typename CharT>
static inline url_result ipv4_parse_number(const CharT* first, const CharT* last, uint32_t& number) {
    // Figure out the base
    detail::SharedCharTypes base;
    uint32_t radix;
    if (first < last && first[0] == '0') {
        const size_t len = last - first;
        if (len == 1) {
            number = 0;
            return url_result::Ok;
        } else {
            // len >= 2
            if (first[1] == 'X' || first[1] == 'x') {
                base = detail::CHAR_HEX;
                radix = 16;
                first += 2;
            } else {
                base = detail::CHAR_OCT;
                radix = 8;
                first += 1;
            }
        }
        // Skip leading zeros (*)
        while (first < last && first[0] == '0')
            first++;
    } else {
        base = detail::CHAR_DEC;
        radix = 10;
    }
    // if all characters '0' (*) OR
    // if input is the empty string, then return zero
    if (first == last) {
        number = 0;
        return url_result::Ok;
    }

    // check if valid digits (we known all chars are ASCII)
    for (auto it = first; it != last; it++) {
        const unsigned char uc = static_cast<unsigned char>(*it);
        if (!IsCharOfType(uc, base))
            return url_result::False;
    }

    // convert to integer
    // max 32-bit value is
    // HEX: FFFFFFFF    (8 digits)
    // DEC: 4294967295  (10 digits)
    // OCT: 37777777777 (11 digits)
    if (last - first > 11)
        return url_result::InvalidIpv4Address; // int overflow

    // Use the 64-bit to get a big number (no hex, decimal, or octal
    // number can overflow a 64-bit number in <= 16 characters).
    uint64_t num = 0;
    if (radix <= 10) {
        for (auto it = first; it != last; it++)
            num = num * radix + (*it - '0');
    } else {
        for (auto it = first; it != last; it++)
            num = num * radix + detail::HexCharToValue(static_cast<unsigned char>(*it));
    }

    // Check for 32-bit overflow.
    if (num > std::numeric_limits<uint32_t>::max())
        return url_result::InvalidIpv4Address; // int overflow

    number = static_cast<uint32_t>(num);
    return url_result::Ok;
}

template <typename CharT>
inline url_result ipv4_parse(const CharT* first, const CharT* last, uint32_t& ipv4) {
    typedef std::make_unsigned<CharT>::type UCharT;

    // Comes from: 6.1. If part is the empty string, return input
    if (first == last)
        return url_result::False;

    // <1>.<2>.<3>.<4>.<->
    const CharT* part[6];
    int dot_count = 0;

    // split on "."
    part[0] = first;
    for (auto it = first; it != last; it++) {
        UCharT uc = static_cast<UCharT>(*it);
        if (uc == '.') {
            if (dot_count == 4)
                return url_result::False; // 4. If parts has more than four items, return input
            if (part[dot_count] == it)
                return url_result::False; // 6.1. If part is the empty string, return input
            part[++dot_count] = it + 1; // skip '.'
        } else if (uc >= 0x80 || !detail::IsIPv4Char(static_cast<unsigned char>(uc))) {
            // not IPv4 character
            return url_result::False;
        }
    }

    // 3. If the last item in parts is the empty string, then:
    //    1. set validationErrorFlag.
    //    2. If parts has more than one item, then remove the last item from parts.
    int part_count;
    if (dot_count > 0 && part[dot_count] == last) {
        part_count = dot_count;
    } else {
        part_count = dot_count + 1;
        part[part_count] = last + 1; // bus -1
    }
    // 4. If parts has more than four items, return input
    if (part_count > 4)
        return url_result::False;

    // IPv4 numbers
    uint32_t number[4];
    for (int ind = 0; ind < part_count; ind++) {
        url_result res = ipv4_parse_number(part[ind], part[ind + 1] - 1, number[ind]);
        if (res != url_result::Ok) return res;
    }
    // TODO-WARN:
    // 7. If validationErrorFlag is set, validation error
    // 8. If any item in numbers is greater than 255, validation error

    // 9. If any but the last item in numbers is greater than 255, return failure
    for (int ind = 0; ind < part_count - 1; ind++) {
        if (number[ind] > 255) return url_result::InvalidIpv4Address;
    }
    // 10. If the last item in numbers is greater than or equal to 256**(5 - the number of items in numbers),
    // validation error, return failure
    ipv4 = number[part_count - 1];
    if (ipv4 > (std::numeric_limits<uint32_t>::max() >> (8 * (part_count - 1))))
        return url_result::InvalidIpv4Address;

    // 14.1. Increment ipv4 by n * 256**(3 - counter). 
    for (int counter = 0; counter < part_count - 1; counter++) {
        ipv4 += number[counter] << (8 * (3 - counter));
    }

    return url_result::Ok;
}

// IPv4 serializer

void ipv4_serialize(uint32_t ipv4, std::string& output);


// IPv6 parser

template <typename CharT, typename IntT>
inline void get_hex_number(const CharT*& pointer, const CharT* last, IntT& value) {
    value = 0;
    while (pointer != last && detail::Is8BitChar(*pointer)) {
        unsigned char uc = static_cast<unsigned char>(*pointer);
        if (!detail::IsHexChar(uc)) break;
        // HEX
        value = value * 0x10 + detail::HexCharToValue(uc);
        pointer++;
    }
}

template <typename CharT>
inline bool IsAsciiDigit(CharT ch) {
    return ch <= '9' && ch >= '0';
}

template <typename CharT>
inline bool ipv6_parse(const CharT* first, const CharT* last, uint16_t(&pieces)[8]) {
    std::fill(std::begin(pieces), std::end(pieces), 0);
    int piece_pointer = 0;      // zero
    int compress_pointer = 0;   // null
    bool is_ipv4 = false;

    const size_t len = last - first;
    // minimalus yra "::"
    if (len < 2) return false;

    const CharT* pointer = first;
    // 5. If c is ":", run these substeps:
    if (pointer[0] == ':') {
        if (pointer[1] != ':') {
            // TODO-ERR: validation error
            return false;
        }
        pointer += 2;
        compress_pointer = ++piece_pointer;
    }

    // Main
    while (pointer < last) {
        if (piece_pointer == 8) {
            // TODO-ERR: validation error
            return false;
        }
        if (pointer[0] == ':') {
            if (compress_pointer) {
                // TODO-ERR: validation error
                return false;
            }
            pointer++;
            compress_pointer = ++piece_pointer;
            continue;
        }

        // HEX
        uint16_t value;
        auto pointer0 = pointer;
        get_hex_number(pointer, pointer + std::min(last - pointer, 4), value);
        if (pointer != last) {
            const CharT ch = *pointer;
            if (ch == '.') {
                if (pointer == pointer0) {
                    // TODO-ERR: validation error
                    return false;
                }
                pointer = pointer0;
                is_ipv4 = true;
                break;
            } if (ch == ':') {
                if (++pointer == last) {
                    // TODO-ERR: validation error
                    return false;
                }
            } else {
                // TODO-ERR: validation error
                return false;
            }
        }
        pieces[piece_pointer++] = value;
    }

    if (is_ipv4) {
        if (piece_pointer > 6) {
            // TODO-ERR: validation error
            return false;
        }
        int numbers_seen = 0;
        while (pointer < last) {
            if (numbers_seen > 0) {
                if (*pointer == '.' && numbers_seen < 4) {
                    ++pointer;
                } else {
                    // TODO-ERR: validation error
                    return false;
                }
            }
            if (pointer == last || !IsAsciiDigit(*pointer)) {
                // TODO-ERR: validation error
                return false;
            }
            // While c is an ASCII digit, run these subsubsteps
            unsigned ipv4Piece = *(pointer++) - '0';
            while (pointer != last && IsAsciiDigit(*pointer)) {
                if (ipv4Piece == 0) // leading zero
                    return false; // TODO-ERR: validation error
                ipv4Piece = ipv4Piece * 10 + (*pointer - '0');
                if (ipv4Piece > 255)
                    return false; // TODO-ERR: validation error
                pointer++;
            }
            pieces[piece_pointer] = pieces[piece_pointer] * 0x100 + ipv4Piece;
            numbers_seen++;
            if (!(numbers_seen & 1)) // 2 or 4
                piece_pointer++;
        }
        // If c is the EOF code point and numbersSeen is not 4
        if (numbers_seen != 4)  {
            //TODO-ERR: validation error
            return false;
        }
    }

    // Finale
    if (compress_pointer) {
        if (int diff = 8 - piece_pointer) {
            for (int ind = piece_pointer - 1; ind >= compress_pointer; ind--) {
                pieces[ind + diff] = pieces[ind];
                pieces[ind] = 0;
            }
        }
    } else if (piece_pointer != 8) {
        // TODO-ERR: validation error
        return false;
    }
    return true;
}

// IPv6 serializer

void ipv6_serialize(const uint16_t(&pieces)[8], std::string& output);


} // namespace whatwg

#endif // WHATWG_URL_IP_H
