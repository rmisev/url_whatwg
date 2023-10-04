// Copyright 2016-2023 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//

#include "upa/url_search_params.h"


namespace upa {

// The percent encoding/mapping table of URL search parameter bytes. If
// corresponding entry is '%', then byte must be percent encoded, otherwise -
// replaced with table value (for example, replace ' ' with '+').
// The table is based on:
// https://url.spec.whatwg.org/#concept-urlencoded-serializer

const char url_search_params::kEncByte[0x100] = {
//   0    1    2    3    4    5    6    7    8    9    A    B    C    D    E    F
    '%', '%', '%', '%', '%', '%', '%', '%', '%', '%', '%', '%', '%', '%', '%', '%', // 0
    '%', '%', '%', '%', '%', '%', '%', '%', '%', '%', '%', '%', '%', '%', '%', '%', // 1
    '+', '%', '%', '%', '%', '%', '%', '%', '%', '%', '*', '%', '%', '-', '.', '%', // 2
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '%', '%', '%', '%', '%', '%', // 3
    '%', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', // 4
    'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '%', '%', '%', '%', '_', // 5
    '%', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', // 6
    'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '%', '%', '%', '%', '%', // 7
    '%', '%', '%', '%', '%', '%', '%', '%', '%', '%', '%', '%', '%', '%', '%', '%', // 8
    '%', '%', '%', '%', '%', '%', '%', '%', '%', '%', '%', '%', '%', '%', '%', '%', // 9
    '%', '%', '%', '%', '%', '%', '%', '%', '%', '%', '%', '%', '%', '%', '%', '%', // A
    '%', '%', '%', '%', '%', '%', '%', '%', '%', '%', '%', '%', '%', '%', '%', '%', // B
    '%', '%', '%', '%', '%', '%', '%', '%', '%', '%', '%', '%', '%', '%', '%', '%', // C
    '%', '%', '%', '%', '%', '%', '%', '%', '%', '%', '%', '%', '%', '%', '%', '%', // D
    '%', '%', '%', '%', '%', '%', '%', '%', '%', '%', '%', '%', '%', '%', '%', '%', // E
    '%', '%', '%', '%', '%', '%', '%', '%', '%', '%', '%', '%', '%', '%', '%', '%'  // F
};

} // namespace upa
