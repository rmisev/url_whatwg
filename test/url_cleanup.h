// Copyright 2016-2021 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//

#ifndef WHATWG_URL_CLEANUP_H
#define WHATWG_URL_CLEANUP_H

// For ICU cleanup
#include "url_idna.h"
#include "unicode/uclean.h"

namespace whatwg {

inline void url_cleanup()
{
    // ICU cleanup
    whatwg::IDNClose();
    u_cleanup();
}

}

#endif // WHATWG_URL_CLEANUP_H
