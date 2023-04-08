// Copyright 2016-2023 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//

#ifndef WHATWG_CONFIG_H
#define WHATWG_CONFIG_H

// Define WHATWG_CPP_20 if compiler supports C++20 or later standard
#if defined(_MSVC_LANG) ? (_MSVC_LANG >= 202002) : (__cplusplus >= 202002)
# define WHATWG_CPP_20
#endif

// Define WHATWG_CPP_17 if compiler supports C++17 or later standard
// https://devblogs.microsoft.com/cppblog/msvc-now-correctly-reports-__cplusplus/
#if defined(_MSVC_LANG) ? (_MSVC_LANG >= 201703) : (__cplusplus >= 201703)
# define WHATWG_CPP_17
# define WHATWG_FALLTHROUGH [[fallthrough]];
# define WHATWG_CONSTEXPR_17 constexpr
# define WHATWG_NOEXCEPT_17 noexcept
#else
# define WHATWG_FALLTHROUGH
# define WHATWG_CONSTEXPR_17
# define WHATWG_NOEXCEPT_17
#endif

// Define WHATWG_CPP_14 if compiler supports C++14 or later
#if defined(_MSVC_LANG) ? (_MSVC_LANG >= 201402) : (__cplusplus >= 201402)
# define WHATWG_CPP_14
#endif

#endif // WHATWG_CONFIG_H
