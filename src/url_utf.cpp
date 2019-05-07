// Copyright 2016-2019 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//

#include "url_utf.h"

namespace whatwg {

bool url_utf::convert_utf8_to_utf16(const char* first, const char* last, simple_buffer<char16_t>& output) {
    bool success = true;
    for (auto it = first; it < last;) {
        uint32_t code_point;
        success &= read_utf_char(it, last, code_point);
        append_utf16(code_point, output);
    }
    return success;
}

//
// (c) 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
//

// Following two arrays have values from corresponding macros in ICU 61.1 library's
// include\unicode\utf8.h file.

// Internal bit vector for 3-byte UTF-8 validity check, for use in U8_IS_VALID_LEAD3_AND_T1.
// Each bit indicates whether one lead byte + first trail byte pair starts a valid sequence.
// Lead byte E0..EF bits 3..0 are used as byte index,
// first trail byte bits 7..5 are used as bit index into that byte.
const uint8_t url_utf::k_U8_LEAD3_T1_BITS[16] = { 0x20, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x10, 0x30, 0x30 };

// Internal bit vector for 4-byte UTF-8 validity check, for use in U8_IS_VALID_LEAD4_AND_T1.
// Each bit indicates whether one lead byte + first trail byte pair starts a valid sequence.
// First trail byte bits 7..4 are used as byte index,
// lead byte F0..F4 bits 2..0 are used as bit index into that byte.
const uint8_t url_utf::k_U8_LEAD4_T1_BITS[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1E, 0x0F, 0x0F, 0x0F, 0x00, 0x00, 0x00, 0x00 };

}
