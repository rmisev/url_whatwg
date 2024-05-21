// Copyright 2017-2024 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//
#ifndef UPA_IDNA_H
#define UPA_IDNA_H

// #include "idna/idna.h"
// Copyright 2017-2024 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//
#ifndef UPA_IDNA_IDNA_H
#define UPA_IDNA_IDNA_H

// #include "bitmask_operators.hpp"
#ifndef JSS_BITMASK_HPP
#define JSS_BITMASK_HPP

// (C) Copyright 2015 Just Software Solutions Ltd
//
// Distributed under the Boost Software License, Version 1.0.
//
// Boost Software License - Version 1.0 - August 17th, 2003
//
// Permission is hereby granted, free of charge, to any person or
// organization obtaining a copy of the software and accompanying
// documentation covered by this license (the "Software") to use,
// reproduce, display, distribute, execute, and transmit the
// Software, and to prepare derivative works of the Software, and
// to permit third-parties to whom the Software is furnished to
// do so, all subject to the following:
//
// The copyright notices in the Software and this entire
// statement, including the above license grant, this restriction
// and the following disclaimer, must be included in all copies
// of the Software, in whole or in part, and all derivative works
// of the Software, unless such copies or derivative works are
// solely in the form of machine-executable object code generated
// by a source language processor.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
// KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
// PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT SHALL THE
// COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE
// LIABLE FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN
// CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include<type_traits>

template<typename E>
struct enable_bitmask_operators{
    static const bool enable=false;
};

template<typename E>
typename std::enable_if<enable_bitmask_operators<E>::enable,E>::type
operator|(E lhs,E rhs){
    typedef typename std::underlying_type<E>::type underlying;
    return static_cast<E>(
        static_cast<underlying>(lhs) | static_cast<underlying>(rhs));
}

template<typename E>
typename std::enable_if<enable_bitmask_operators<E>::enable,E>::type
operator&(E lhs,E rhs){
    typedef typename std::underlying_type<E>::type underlying;
    return static_cast<E>(
        static_cast<underlying>(lhs) & static_cast<underlying>(rhs));
}

template<typename E>
typename std::enable_if<enable_bitmask_operators<E>::enable,E>::type
operator^(E lhs,E rhs){
    typedef typename std::underlying_type<E>::type underlying;
    return static_cast<E>(
        static_cast<underlying>(lhs) ^ static_cast<underlying>(rhs));
}

template<typename E>
typename std::enable_if<enable_bitmask_operators<E>::enable,E>::type
operator~(E lhs){
    typedef typename std::underlying_type<E>::type underlying;
    return static_cast<E>(
        ~static_cast<underlying>(lhs));
}

template<typename E>
typename std::enable_if<enable_bitmask_operators<E>::enable,E&>::type
operator|=(E& lhs,E rhs){
    typedef typename std::underlying_type<E>::type underlying;
    lhs=static_cast<E>(
        static_cast<underlying>(lhs) | static_cast<underlying>(rhs));
    return lhs;
}

template<typename E>
typename std::enable_if<enable_bitmask_operators<E>::enable,E&>::type
operator&=(E& lhs,E rhs){
    typedef typename std::underlying_type<E>::type underlying;
    lhs=static_cast<E>(
        static_cast<underlying>(lhs) & static_cast<underlying>(rhs));
    return lhs;
}

template<typename E>
typename std::enable_if<enable_bitmask_operators<E>::enable,E&>::type
operator^=(E& lhs,E rhs){
    typedef typename std::underlying_type<E>::type underlying;
    lhs=static_cast<E>(
        static_cast<underlying>(lhs) ^ static_cast<underlying>(rhs));
    return lhs;
}

#endif

// #include "idna_table.h"
// Copyright 2017-2024 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//
#ifndef UPA_IDNA_IDNA_TABLE_H
#define UPA_IDNA_IDNA_TABLE_H

#include <cstddef>
#include <cstdint>

namespace upa { // NOLINT(modernize-concat-nested-namespaces)
namespace idna { // NOLINT(modernize-concat-nested-namespaces)
namespace util {

// ASCII
const std::uint8_t AC_VALID = 0x01;
const std::uint8_t AC_MAPPED = 0x02;
const std::uint8_t AC_DISALLOWED_STD3 = 0x04;

// Unicode
const std::uint32_t CP_DISALLOWED = 0;
const std::uint32_t CP_VALID = 0x0001 << 16;
const std::uint32_t CP_MAPPED = 0x0002 << 16;
const std::uint32_t CP_DEVIATION = CP_VALID | CP_MAPPED; // 0x0003 << 16
const std::uint32_t CP_DISALLOWED_STD3 = 0x0004 << 16;
const std::uint32_t CP_NO_STD3_VALID = CP_VALID | CP_DISALLOWED_STD3;
const std::uint32_t CP_NO_STD3_MAPPED = CP_MAPPED | CP_DISALLOWED_STD3;
const std::uint32_t MAP_TO_ONE = 0x0008 << 16;
// General_Category=Mark
const std::uint32_t CAT_MARK = 0x0010 << 16;
// ContextJ
const std::uint32_t CAT_Virama   = 0x0020 << 16;
//todo: as values >1
const std::uint32_t CAT_Joiner_D = 0x0040 << 16;
const std::uint32_t CAT_Joiner_L = 0x0080 << 16;
const std::uint32_t CAT_Joiner_R = 0x0100 << 16;
const std::uint32_t CAT_Joiner_T = 0x0200 << 16;
// Bidi
//todo: as values >1
const std::uint32_t CAT_Bidi_L    = 0x0400 << 16;
const std::uint32_t CAT_Bidi_R_AL = 0x0800 << 16;
const std::uint32_t CAT_Bidi_AN   = 0x1000 << 16;
const std::uint32_t CAT_Bidi_EN   = 0x2000 << 16;
const std::uint32_t CAT_Bidi_ES_CS_ET_ON_BN = 0x4000 << 16;
const std::uint32_t CAT_Bidi_NSM  = 0x8000 << 16;

// BEGIN-GENERATED
const std::size_t blockShift = 4;
const std::uint32_t blockMask = 0xF;
const std::uint32_t defaultStart = 0x323B0;
const std::uint32_t defaultValue = 0;
const std::uint32_t specRange1 = 0xE0100;
const std::uint32_t specRange2 = 0xE01EF;
const std::uint32_t specValue = 0x20000;

extern const std::uint32_t blockData[];
extern const std::uint16_t blockIndex[];
extern const char32_t allCharsTo[];

extern const std::uint32_t comp_disallowed[5];
extern const std::uint32_t comp_disallowed_std3[21];

extern const std::uint8_t asciiData[128];
// END-GENERATED


inline std::uint32_t getStatusMask(bool useSTD3ASCIIRules) {
    return useSTD3ASCIIRules ? (0x0007 << 16) : (0x0003 << 16);
}

inline std::uint32_t getValidMask(bool useSTD3ASCIIRules, bool transitional) {
    const std::uint32_t status_mask = getStatusMask(useSTD3ASCIIRules);
    // (CP_DEVIATION = CP_VALID | CP_MAPPED) & ~CP_MAPPED ==> CP_VALID
    return transitional ? status_mask : (status_mask & ~CP_MAPPED);
}

inline std::uint32_t getCharInfo(uint32_t cp) {
    if (cp >= defaultStart) {
        if (cp >= specRange1 && cp <= specRange2) {
            return specValue;
        }
        return defaultValue;
    }
    return blockData[(blockIndex[cp >> blockShift] << blockShift) | (cp & blockMask)];
}

template <class StrT>
inline std::size_t apply_mapping(uint32_t val, StrT& output) {
    if (val & MAP_TO_ONE) {
        output.push_back(val & 0xFFFF);
        return 1;
    }
    if (val & 0xFFFF) {
        std::size_t len = (val & 0xFFFF) >> 13;
        std::size_t ind = val & 0x1FFF;
        if (len == 7) {
            len += ind >> 8;
            ind &= 0xFF;
        }
        const auto* ptr = static_cast<const char32_t*>(allCharsTo) + ind;
        output.append(ptr, len);
        return len;
    }
    return 0;
}

} // namespace util
} // namespace idna
} // namespace upa

#endif // UPA_IDNA_IDNA_TABLE_H

// #include "iterate_utf.h"
// Copyright 2017-2024 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//
#ifndef UPA_IDNA_ITERATE_UTF_H
#define UPA_IDNA_ITERATE_UTF_H

#include <cstdint>

namespace upa { // NOLINT(modernize-concat-nested-namespaces)
namespace idna { // NOLINT(modernize-concat-nested-namespaces)
namespace util {

// Get code point from UTF-8

constexpr char32_t kReplacementCharacter = 0xFFFD;

// https://encoding.spec.whatwg.org/#utf-8-decoder
inline uint32_t getCodePoint(const char*& it, const char* last) {
    const auto uchar = [](char c) { return static_cast<unsigned char>(c); };
    // assume it != last
    uint32_t c1 = uchar(*it++);
    if (c1 >= 0x80) {
        if (c1 < 0xC2 || c1 > 0xF4)
            return kReplacementCharacter;
        if (c1 <= 0xDF) {
            // 2-bytes
            if (it == last || uchar(*it) < 0x80 || uchar(*it) > 0xBF)
                return kReplacementCharacter;
            c1 = ((c1 & 0x1F) << 6) | (uchar(*it++) & 0x3F);
        } else if (c1 <= 0xEF) {
            // 3-bytes
            const unsigned char clb = c1 == 0xE0 ? 0xA0 : 0x80;
            const unsigned char cub = c1 == 0xED ? 0x9F : 0xBF;
            if (it == last || uchar(*it) < clb || uchar(*it) > cub)
                return kReplacementCharacter;
            c1 = ((c1 & 0x0F) << 6) | (uchar(*it++) & 0x3F);
            if (it == last || uchar(*it) < 0x80 || uchar(*it) > 0xBF)
                return kReplacementCharacter;
            c1 = (c1 << 6) | (uchar(*it++) & 0x3F);
        } else {
            // 4-bytes
            const unsigned char clb = c1 == 0xF0 ? 0x90 : 0x80;
            const unsigned char cub = c1 == 0xF4 ? 0x8F : 0xBF;
            if (it == last || uchar(*it) < clb || uchar(*it) > cub)
                return kReplacementCharacter;
            c1 = ((c1 & 0x07) << 6) | (uchar(*it++) & 0x3F);
            if (it == last || uchar(*it) < 0x80 || uchar(*it) > 0xBF)
                return kReplacementCharacter;
            c1 = (c1 << 6) | (uchar(*it++) & 0x3F);
            if (it == last || uchar(*it) < 0x80 || uchar(*it) > 0xBF)
                return kReplacementCharacter;
            c1 = (c1 << 6) | (uchar(*it++) & 0x3F);
        }
    }
    return c1;
}

// Get code point from UTF-16

template <class T>
inline bool is_surrogate_lead(T ch) {
    return (ch & 0xFFFFFC00) == 0xD800;
}

template <class T>
inline bool is_surrogate_trail(T ch) {
    return (ch & 0xFFFFFC00) == 0xDC00;
}

// Get a supplementary code point value(U + 10000..U + 10ffff)
// from its lead and trail surrogates.
inline uint32_t get_suplementary(uint32_t lead, uint32_t trail) {
    const uint32_t surrogate_offset = (static_cast<uint32_t>(0xD800) << 10) + 0xDC00 - 0x10000;
    return (lead << 10) + trail - surrogate_offset;
}

// assumes it != last

inline uint32_t getCodePoint(const char16_t*& it, const char16_t* last) {
    // assume it != last
    const uint32_t c1 = *it++;
    if (is_surrogate_lead(c1) && it != last) {
        const uint32_t c2 = *it;
        if (is_surrogate_trail(c2)) {
            ++it;
            return get_suplementary(c1, c2);
        }
    }
    return c1;
}

// Get code point from UTF-32

inline uint32_t getCodePoint(const char32_t*& it, const char32_t*) {
    // assume it != last
    return *it++;
}

} // namespace util
} // namespace idna
} // namespace upa

#endif // UPA_IDNA_ITERATE_UTF_H

// #include "nfc.h"
// Copyright 2024 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//
#ifndef UPA_IDNA_NFC_H
#define UPA_IDNA_NFC_H

#include <string>

namespace upa { // NOLINT(modernize-concat-nested-namespaces)
namespace idna {


void compose(std::u32string& str);
void canonical_decompose(std::u32string& str);

void normalize_nfc(std::u32string& str);
bool is_normalized_nfc(const char32_t* first, const char32_t* last);


} // namespace idna
} // namespace upa

#endif // UPA_IDNA_NFC_H

#include <algorithm>
#include <string>
#include <type_traits> // std::make_unsigned

namespace upa { // NOLINT(modernize-concat-nested-namespaces)
namespace idna {

enum class Option {
    Default           = 0,
    UseSTD3ASCIIRules = 0x0001,
    Transitional      = 0x0002,
    VerifyDnsLength   = 0x0004,
    CheckHyphens      = 0x0008,
    CheckBidi         = 0x0010,
    CheckJoiners      = 0x0020,
    // ASCII optimization
    InputASCII        = 0x1000,
};

} // namespace idna
} // namespace upa

// enable bit mask operators on upa::idna::Option
template<>
struct enable_bitmask_operators<upa::idna::Option> {
    static const bool enable = true;
};

namespace upa { // NOLINT(modernize-concat-nested-namespaces)
namespace idna {
namespace detail {

// Bit flags
inline bool has(Option option, const Option value) {
    return (option & value) == value;
}

inline Option domain_options(bool be_strict, bool is_input_ascii) {
    // https://url.spec.whatwg.org/#concept-domain-to-ascii
    // https://url.spec.whatwg.org/#concept-domain-to-unicode
    // Note. The to_unicode ignores Option::VerifyDnsLength
    auto options = be_strict
        ? Option::CheckBidi | Option::CheckJoiners | Option::UseSTD3ASCIIRules | Option::VerifyDnsLength
        : Option::CheckBidi | Option::CheckJoiners;
    if (is_input_ascii)
        options |= Option::InputASCII;
    return options;
}

template <typename CharT>
constexpr char ascii_to_lower_char(CharT c) noexcept {
    return static_cast<char>((c <= 'Z' && c >= 'A') ? (c | 0x20) : c);
}

// IDNA map and normalize NFC
template <typename CharT>
inline bool map(std::u32string& mapped, const CharT* input, const CharT* input_end, Option options, bool is_to_ascii) {
    using UCharT = typename std::make_unsigned<CharT>::type;

    // P1 - Map
    if (has(options, Option::InputASCII)) {
        // The input is in ASCII and can contain `xn--` labels
        mapped.reserve(input_end - input);
        if (has(options, Option::UseSTD3ASCIIRules)) {
            for (const auto* it = input; it != input_end; ++it) {
                const auto cp = static_cast<UCharT>(*it);
                switch (util::asciiData[cp]) {
                case util::AC_VALID:
                    mapped.push_back(cp);
                    break;
                case util::AC_MAPPED:
                    mapped.push_back(cp | 0x20);
                    break;
                default:
                    // util::AC_DISALLOWED_STD3
                    if (is_to_ascii)
                        return false;
                    mapped.push_back(cp);
                }
            }
        } else {
            for (const auto* it = input; it != input_end; ++it)
                mapped.push_back(ascii_to_lower_char(*it));
        }
    } else {
        const uint32_t status_mask = util::getStatusMask(has(options, Option::UseSTD3ASCIIRules));
        for (auto it = input; it != input_end; ) {
            const uint32_t cp = util::getCodePoint(it, input_end);
            const uint32_t value = util::getCharInfo(cp);

            switch (value & status_mask) {
            case util::CP_VALID:
                mapped.push_back(cp);
                break;
            case util::CP_MAPPED:
                if (has(options, Option::Transitional) && cp == 0x1E9E) {
                    // replace U+1E9E capital sharp s by “ss”
                    mapped.append(U"ss", 2);
                } else {
                    util::apply_mapping(value, mapped);
                }
                break;
            case util::CP_DEVIATION:
                if (has(options, Option::Transitional)) {
                    util::apply_mapping(value, mapped);
                } else {
                    mapped.push_back(cp);
                }
                break;
            default:
                // CP_DISALLOWED
                // CP_NO_STD3_MAPPED, CP_NO_STD3_VALID if Option::UseSTD3ASCIIRules
                // Starting with Unicode 15.1.0 - don't record an error
                if (is_to_ascii && // to_ascii optimization
                    ((value & util::CP_DISALLOWED_STD3) == 0
                    ? !std::binary_search(std::begin(util::comp_disallowed), std::end(util::comp_disallowed), cp)
                    : !std::binary_search(std::begin(util::comp_disallowed_std3), std::end(util::comp_disallowed_std3), cp)))
                    return false;
                mapped.push_back(cp);
                break;
            }
        }

        // P2 - Normalize
        normalize_nfc(mapped);
    }

    return true;
}

bool to_ascii_mapped(std::string& domain, const std::u32string& mapped, Option options);
bool to_unicode_mapped(std::u32string& domain, const std::u32string& mapped, Option options);

} // namespace detail


/// @brief Implements the Unicode IDNA ToASCII
///
/// See: https://www.unicode.org/reports/tr46/#ToASCII
/// 
/// @param[out] domain buffer to store result string
/// @param[in]  input source domain string
/// @param[in]  input_end the end of source domain string
/// @param[in]  options
/// @return `true` on success, or `false` on failure
template <typename CharT>
inline bool to_ascii(std::string& domain, const CharT* input, const CharT* input_end, Option options) {
    // P1 - Map and further processing
    std::u32string mapped;
    domain.clear();
    return
        detail::map(mapped, input, input_end, options, true) &&
        detail::to_ascii_mapped(domain, mapped, options);
}

/// @brief Implements the Unicode IDNA ToUnicode
///
/// See: https://www.unicode.org/reports/tr46/#ToUnicode
/// 
/// @param[out] domain buffer to store result string
/// @param[in]  input source domain string
/// @param[in]  input_end the end of source domain string
/// @param[in]  options
/// @return `true` on success, or `false` on errors
template <typename CharT>
inline bool to_unicode(std::u32string& domain, const CharT* input, const CharT* input_end, Option options) {
    // P1 - Map and further processing
    std::u32string mapped;
    detail::map(mapped, input, input_end, options, false);
    return detail::to_unicode_mapped(domain, mapped, options);
}

/// @brief Implements the domain to ASCII algorithm
///
/// See: https://url.spec.whatwg.org/#concept-domain-to-ascii
///
/// @param[out] domain buffer to store result string
/// @param[in]  input source domain string
/// @param[in]  input_end the end of source domain string
/// @param[in]  be_strict
/// @param[in]  is_input_ascii
/// @return `true` on success, or `false` on failure
template <typename CharT>
inline bool domain_to_ascii(std::string& domain, const CharT* input, const CharT* input_end,
    bool be_strict = false, bool is_input_ascii = false)
{
    const bool res = to_ascii(domain, input, input_end, detail::domain_options(be_strict, is_input_ascii));

    // 3. If result is the empty string, domain-to-ASCII validation error, return failure.
    //
    // Note. Result of to_ascii can be the empty string if input consists entirely of
    // IDNA ignored code points.
    return res && !domain.empty();
}

/// @brief Implements the domain to Unicode algorithm
///
/// See: https://url.spec.whatwg.org/#concept-domain-to-unicode
///
/// @param[out] domain buffer to store result string
/// @param[in]  input source domain string
/// @param[in]  input_end the end of source domain string
/// @param[in]  be_strict
/// @param[in]  is_input_ascii
/// @return `true` on success, or `false` on errors
template <typename CharT>
inline bool domain_to_unicode(std::u32string& domain, const CharT* input, const CharT* input_end,
    bool be_strict = false, bool is_input_ascii = false)
{
    return to_unicode(domain, input, input_end, detail::domain_options(be_strict, is_input_ascii));
}

/// @brief Encodes Unicode version
///
/// The version is encoded as follows: <version 1st number> * 0x1000000 +
/// <version 2nd number> * 0x10000 + <version 3rd number> * 0x100 + <version 4th number>
///
/// For example for Unicode version 15.1.0 it returns 0x0F010000
///
/// @param[in] n1 version 1st number
/// @param[in] n2 version 2nd number
/// @param[in] n3 version 3rd number
/// @param[in] n4 version 4th number
/// @return encoded Unicode version
constexpr unsigned make_unicode_version(unsigned n1, unsigned n2 = 0,
    unsigned n3 = 0, unsigned n4 = 0) noexcept {
    return n1 << 24 | n2 << 16 | n3 << 8 | n4;
}

/// @brief Gets Unicode version that IDNA library conforms to
///
/// @return encoded Unicode version
/// @see make_unicode_version
inline unsigned unicode_version() {
    return make_unicode_version(15, 1);
}


} // namespace idna
} // namespace upa

#endif // UPA_IDNA_IDNA_H

// #include "idna/punycode.h"
// Copyright 2017-2024 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//
#ifndef UPA_IDNA_PUNYCODE_H
#define UPA_IDNA_PUNYCODE_H

#include <string>

namespace upa { // NOLINT(modernize-concat-nested-namespaces)
namespace idna { // NOLINT(modernize-concat-nested-namespaces)
namespace punycode {

enum class status {
    success = 0,
    bad_input = 1,  // Input is invalid.
    big_output = 2, // Output would exceed the space provided.
    overflow = 3    // Wider integers needed to process input.
};

status encode(std::string& output, const char32_t* first, const char32_t* last);
status decode(std::u32string& output, const char32_t* first, const char32_t* last);

} // namespace punycode
} // namespace idna
} // namespace upa

#endif // UPA_IDNA_PUNYCODE_H


#endif // UPA_IDNA_H
