// Copyright 2016-2019 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//

#ifndef URL_RESULT_H
#define URL_RESULT_H

#include <stdexcept>

namespace whatwg {

// URL error codes

enum class url_result {
    // success
    Ok = 0,
    False,                         // setter ignored the value; non-ipv4 address (internal)
    // failure
    InvalidSchemeCharacter,        // invalid scheme character
    EmptyHost,                     // host cannot be empty
    IdnaError,                     // invalid international domain name
    InvalidPort,                   // invalid port number
    InvalidIpv4Address,            // invalid IPv4 address
    InvalidIpv6Address,            // invalid IPv6 address
    InvalidDomainCharacter,        // invalid domain character
    RelativeUrlWithoutBase,        // relative URL without a base
    RelativeUrlWithCannotBeABase,  // relative URL with a cannot-be-a-base base
    SetHostOnCannotBeABaseUrl,     // a cannot-be-a-base URL doesn't have a host to set
    Overflow,                      // URLs is too long
};

inline bool success(url_result res) {
    return res == url_result::Ok;
}

// URL exception

class url_error : public std::runtime_error {
public:

    explicit url_error(url_result res, const char* what_arg)
        : std::runtime_error(what_arg)
        , res_(res)
    {}

    url_result result() const {
        return res_;
    }
private:
    url_result res_;
};

}

#endif // URL_RESULT_H
