// Copyright 2016-2019 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//
// This file contains portions of modified code from:
// https://cs.chromium.org/chromium/src/url/url_canon_internal.cc
// Copyright 2013 The Chromium Authors. All rights reserved.
//

#include "url_percent_encode.h"

namespace whatwg {

#ifndef WHATWG__CPP_17

// These data were generated by tools/dumpCharBitSets.cpp program.
const code_point_set fragment_no_encode_set = {
    0x00, 0x00, 0x00, 0x00, 0xfa, 0xff, 0xff, 0xaf, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0x7f
};
const code_point_set query_no_encode_set = {
    0x00, 0x00, 0x00, 0x00, 0xf2, 0xff, 0xff, 0xaf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f
};
const code_point_set special_query_no_encode_set = {
    0x00, 0x00, 0x00, 0x00, 0x72, 0xff, 0xff, 0xaf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f
};
const code_point_set path_no_encode_set = {
    0x00, 0x00, 0x00, 0x00, 0xf2, 0xff, 0xff, 0x2f, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0x57
};
const code_point_set raw_path_no_encode_set = {
    0x00, 0x00, 0x00, 0x00, 0xd2, 0xff, 0xff, 0x2f, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0x57
};
const code_point_set userinfo_no_encode_set = {
    0x00, 0x00, 0x00, 0x00, 0xf2, 0x7f, 0xff, 0x03, 0xfe, 0xff, 0xff, 0x87, 0xfe, 0xff, 0xff, 0x47
};
const code_point_set component_no_encode_set = {
    0x00, 0x00, 0x00, 0x00, 0x82, 0x67, 0xff, 0x03, 0xfe, 0xff, 0xff, 0x87, 0xfe, 0xff, 0xff, 0x47
};
const code_point_set host_forbidden_set = {
    0x01, 0x26, 0x00, 0x00, 0x29, 0x80, 0x00, 0xd4, 0x01, 0x00, 0x00, 0x78
};
const code_point_set hex_digit_set = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x03, 0x7e, 0x00, 0x00, 0x00, 0x7e
};
const code_point_set ipv4_char_set = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0xff, 0x03, 0x7e, 0x00, 0x00, 0x01, 0x7e, 0x00, 0x00, 0x01
};

#endif

namespace detail {

// Maps hex numerical values to ASCII digits
const char kHexCharLookup[0x10] = {
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
};

// Used to convert ASCII hex digit to numerical value
const char kCharToHexLookup[8] = {
    0,         // 0x00 - 0x1f
    '0',       // 0x20 - 0x3f: digits 0 - 9 are 0x30 - 0x39
    'A' - 10,  // 0x40 - 0x5f: letters A - F are 0x41 - 0x46
    'a' - 10,  // 0x60 - 0x7f: letters a - f are 0x61 - 0x66
    0,         // 0x80 - 0x9F
    0,         // 0xA0 - 0xBF
    0,         // 0xC0 - 0xDF
    0,         // 0xE0 - 0xFF
};


} // namespace detail
} // namespace whatwg
