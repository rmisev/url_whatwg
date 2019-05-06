#ifndef WHATWG_URLCANON_H
#define WHATWG_URLCANON_H

#include "buffer.h"
#include "url_utf.h"
#include <string>

namespace whatwg {
namespace detail {

// https://cs.chromium.org/chromium/src/url/url_canon_internal.h

// Character type handling -----------------------------------------------------

// Bits that identify different character types. These types identify different
// bits that are set for each 8-bit character in the kSharedCharTypeTable.
enum SharedCharTypes {
  // Characters that do not require escaping in queries. Characters that do
  // not have this flag will be escaped; see url_canon_query.cc
  CHAR_QUERY = 1,

  // Valid in the username/password field.
  CHAR_USERINFO = 2,

  // Valid in a IPv4 address (digits plus dot and 'x' for hex).
  CHAR_IPV4 = 4,

  // Valid in an ASCII-representation of a hex digit (as in %-escaped).
  CHAR_HEX = 8,

  // Characters that do not require escaping in fragment (not fragment percent-encode set)
  CHAR_FRAG = 32,

  // Characters that do not require escaping in path (not path percent-encode set)
  CHAR_PATH = 64,

  // Forbidden host code points:
  // U+0000, U+0009, U+000A, U+000D, U+0020, "#", "%", "/", ":", "?", "@", "[", "\", "]"
  // see: https://url.spec.whatwg.org/#forbidden-host-code-point
  CHAR_HOST_INV = 128,
};

// This table contains the flags in SharedCharTypes for each 8-bit character.
// Some canonicalization functions have their own specialized lookup table.
// For those with simple requirements, we have collected the flags in one
// place so there are fewer lookup tables to load into the CPU cache.
//
// Using an unsigned char type has a small but measurable performance benefit
// over using a 32-bit number.
extern const unsigned char kSharedCharTypeTable[0x100];

// char size
inline bool Is8BitChar(char) {
    return true;  // this case is specialized to avoid a warning
}
inline bool Is8BitChar(unsigned char) {
    return true;  // this case is specialized to avoid a warning
}
template <typename CharT>
inline bool Is8BitChar(CharT c) {
    return c <= 255;
}

// More readable wrappers around the character type lookup table.
inline bool IsCharOfType(unsigned char c, SharedCharTypes type) {
  return !!(kSharedCharTypeTable[c] & type);
}

inline bool IsIPv4Char(unsigned char c) {
  return IsCharOfType(c, CHAR_IPV4);
}
inline bool IsHexChar(unsigned char c) {
    return IsCharOfType(c, CHAR_HEX);
}

// IsInvalidHostChar supports >8bit charcters
template <typename CharT>
inline bool IsInvalidHostChar(CharT c) {
    return Is8BitChar(c) && IsCharOfType(static_cast<unsigned char>(c), CHAR_HOST_INV);
}

// Appends the given string to the output, escaping characters that do not
// match the given |type| in SharedCharTypes.
void AppendStringOfType(const char* first, const char* last,
    SharedCharTypes type, std::string& output);
void AppendStringOfType(const char16_t* first, const char16_t* last,
    SharedCharTypes type, std::string& output);
void AppendStringOfType(const char32_t* first, const char32_t* last,
    SharedCharTypes type, std::string& output);

// Maps the hex numerical values 0x0 to 0xf to the corresponding ASCII digit
// that will be used to represent it.
extern const char kHexCharLookup[0x10];

// This lookup table allows fast conversion between ASCII hex letters and their
// corresponding numerical value. The 8-bit range is divided up into 8
// regions of 0x20 characters each. Each of the three character types (numbers,
// uppercase, lowercase) falls into different regions of this range. The table
// contains the amount to subtract from characters in that range to get at
// the corresponding numerical value.
//
// See HexDigitToValue for the lookup.
extern const char kCharToHexLookup[8];

// Assumes the input is a valid hex digit! Call IsHexChar before using this.
inline unsigned char HexCharToValue(unsigned char c) {
  return c - kCharToHexLookup[c / 0x20];
}

// Write a single character, escaped, to the output. This always escapes: it
// does no checking that thee character requires escaping.
// Escaping makes sense only 8 bit chars, so code works in all cases of
// input parameters (8/16bit).
template<typename UINCHAR, typename OUTCHAR>
inline void AppendEscapedChar(UINCHAR ch, std::basic_string<OUTCHAR>& output) {
  output.push_back('%');
  output.push_back(kHexCharLookup[(ch >> 4) & 0xf]);
  output.push_back(kHexCharLookup[ch & 0xf]);
}

// The character we'll substitute for undecodable or invalid characters.
//extern const base::char16 kUnicodeReplacementCharacter;

// UTF-8 functions ------------------------------------------------------------

// Helper used by AppendUTF8Value below. We use an unsigned parameter so there
// are no funny sign problems with the input, but then have to convert it to
// a regular char for appending.
inline void AppendCharToOutput(unsigned char ch, std::string& output) {
    output.push_back(static_cast<char>(ch));
}

// Writes the given character to the output as UTF-8. This does NO checking
// of the validity of the Unicode characters; the caller should ensure that
// the value it is appending is valid to append.
inline void AppendUTF8Value(unsigned char_value, std::string& output) {
    url_utf::append_utf8<std::string, AppendCharToOutput>(char_value, output);
}

// Writes the given character to the output as UTF-8, escaping ALL
// characters (even when they are ASCII). This does NO checking of the
// validity of the Unicode characters; the caller should ensure that the value
// it is appending is valid to append.
inline void AppendUTF8EscapedValue(unsigned char_value, std::string& output) {
    url_utf::append_utf8<std::string, AppendEscapedChar>(char_value, output);
}

// Escaping functions ---------------------------------------------------------

// Writes the given character to the output as UTF-8, escaped. Call this
// function only when the input is wide. Returns true on success. Failure
// means there was some problem with the encoding, we'll still try to
// update the |*begin| pointer and add a placeholder character to the
// output so processing can continue.

template <typename CharT>
inline bool AppendUTF8EscapedChar(const CharT*& first, const CharT* last, std::string& output) {
    // url_util::read_utf_char(..) will handle invalid characters for us and give
    // us the kUnicodeReplacementCharacter, so we don't have to do special
    // checking after failure, just pass through the failure to the caller.
    uint32_t code_point;
    bool success = url_utf::read_utf_char(first, last, code_point);
    AppendUTF8EscapedValue(code_point, output);
    return success;
}

// Given a character after '%' at |*first| in the string, this will decode
// the escaped value and put it into |*unescaped_value| on success (returns
// true). On failure, this will return false, and will not write into
// |*unescaped_value|.
//
// |*first| will be updated to point after the last character of the escape
// sequence. On failure, |*first| will be unchanged.

template <typename CharT>
inline bool DecodeEscaped(const CharT*& first, const CharT* last, unsigned char& unescaped_value) {
    if (last - first < 2 ||
        !Is8BitChar(first[0]) || !Is8BitChar(first[1])) {
        // Invalid escape sequence because there's not enough room, or the
        // digits are not ASCII.
        return false;
    }

    unsigned char uc1 = static_cast<unsigned char>(first[0]);
    unsigned char uc2 = static_cast<unsigned char>(first[1]);
    if (!IsHexChar(uc1) || !IsHexChar(uc2)) {
        // Invalid hex digits, fail.
        return false;
    }

    // Valid escape sequence.
    unescaped_value = (HexCharToValue(uc1) << 4) + HexCharToValue(uc2);
    first += 2;
    return true;
}


} // namespace detail
} // namespace whatwg

#endif // WHATWG_URLCANON_H
