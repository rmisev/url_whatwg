// Copyright 2019-2023 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//

#include "url_percent_encode.h"
#include <iomanip>
#include <iostream>

#ifndef WHATWG_CPP_17
#error "This file requires C++17 compiler"
#endif

using namespace whatwg;

void dumpSet(std::ostream& out, const char* name, const code_point_set& cpset) {
    out << std::setfill('0') << std::hex;

    out << "const code_point_set " << name << " = {\n    ";
    // find last non-zero value in array
    std::size_t size = cpset.arr_size();
    while (size > 0 && cpset.arr_val(size - 1) == 0)
        --size;
    // output array values
    for (std::size_t i = 0; i < size; ++i) {
        if (i > 0) out << ", ";
        out << "0x" << std::setw(2) << unsigned(cpset.arr_val(i));
    }
    out << "\n};\n";
}

#define DUMP_SET(name) dumpSet(out, #name, name)

void dumpAll(std::ostream& out) {
    out << "// These data were generated by tools/dumpCharBitSets.cpp program.\n";

    DUMP_SET(fragment_no_encode_set);
    DUMP_SET(query_no_encode_set);
    DUMP_SET(special_query_no_encode_set);
    DUMP_SET(path_no_encode_set);
    DUMP_SET(raw_path_no_encode_set);
    DUMP_SET(posix_path_no_encode_set);
    DUMP_SET(userinfo_no_encode_set);
    DUMP_SET(component_no_encode_set);

    DUMP_SET(host_forbidden_set);
    DUMP_SET(domain_forbidden_set);
    DUMP_SET(hex_digit_set);
    DUMP_SET(ipv4_char_set);
}

int main() {
    dumpAll(std::cout);

    return 0;
}
