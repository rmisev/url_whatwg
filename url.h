// Copyright 2016 Rimas Misevičius
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// URL Standard
// https://url.spec.whatwg.org/
//
// Infra Standard - fundamental concepts upon which standards are built
// https://infra.spec.whatwg.org/
//

#ifndef WHATWG_URL_H
#define WHATWG_URL_H

#include "buffer.h"
#include "url_canon.h"
#include "url_idna.h"
#include "url_ip.h"
#include "url_util.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <string>
#include <type_traits>
#include <utility>


template<typename CharT, typename Traits = std::char_traits<CharT>>
class str_view {
public:
    str_view(const CharT* ptr, std::size_t len) : ptr_(ptr), len_(len) {}

    const CharT* data() const { return ptr_; }
    std::size_t length() const { return len_; }
    std::size_t size() const { return len_; }

    CharT operator[](std::size_t ind) const {
        return ptr_[ind];
    }

    int compare(str_view x) const {
        const int cmp = Traits::compare(ptr_, x.ptr_, std::min(len_, x.len_));
        return cmp != 0 ? cmp : (len_ == x.len_ ? 0 : len_ < x.len_ ? -1 : 1);
    }

    bool equal(str_view x) const {
        return len_ == x.len_ && Traits::compare(ptr_, x.ptr_, len_) == 0;
    }

    void append_to(std::basic_string<CharT>& str) const {
        str.append(ptr_, len_);
    }

    void remove_prefix(size_t n) {
        ptr_ += n;
        len_ -= n;
    }
    void remove_suffix(size_t n) {
        len_ -= n;
    }

private:
    const CharT* ptr_;
    std::size_t len_;
};

namespace whatwg {

namespace detail {


struct scheme_info {
    str_view<char> scheme;
    int default_port;           // -1 if none
    unsigned is_special: 1;     // "ftp", "file", "gopher", "http", "https", "ws", "wss"
    unsigned is_local : 1;      // "about", "blob", "data", "filesystem"
    unsigned is_http : 1;       // "http", "https"
    unsigned is_network : 1;    // "ftp" or an HTTP(S) scheme
    unsigned is_fetch : 1;      // "about", "blob", "data", "file", "filesystem", or a network scheme
    // other
    unsigned is_file : 1;       // "file"
    unsigned is_ws : 1;         // "ws", "wss"

    // for search operations
    operator const str_view<char>&() const { return scheme; }
};

// todo: str_equal(..) nenaudojama
template <typename CharT>
bool str_equal(const CharT* first, const CharT* last, const char* strz) {
    for (const CharT* it = first; it < last; it++) {
        if (*strz == 0 || *it != *strz) return false;
        strz++;
    }
    return *strz == 0;
}

// Lowercase the ASCII character
template <typename CharT>
inline CharT AsciiToLower(CharT c) {
    return (c <= 'Z' && c >= 'A') ? (c | 0x20) : c;
}

// strz must be in lower case
template <typename CharT>
inline bool AsciiEqualNoCase(const CharT* first, const CharT* last, const char* strz) {
    for (const CharT* it = first; it < last; it++) {
        if (*strz == 0 || AsciiToLower(*it) != *strz) return false;
        strz++;
    }
    return *strz == 0;
}

extern const scheme_info* get_scheme_info(const str_view<char> src);
inline const scheme_info* get_scheme_info(const char* name, std::size_t len) {
    return get_scheme_info(str_view<char>(name, len));
}


} // namespace detail

// URL class

class url {
public:
    enum PartType {
        SCHEME = 0,
        SCHEME_SEP,
        USERNAME,
        PASSWORD,
        HOST_START,
        HOST,
        PORT,
        PATH,
        QUERY,
        FRAGMENT,
        PART_COUNT
    };

    /**
    * \brief Default constructor.
    * Constructs empty URL
    */
    url()
        : scheme_inf_(nullptr)
        , flags_(INITIAL_FLAGS)
        , path_segment_count_(0)
        //TODO??: part_end_.fill(0)
    {}
    // copy constructor
    url(const url& src)
        : norm_url_(src.norm_url_)
        , part_end_(src.part_end_)
        , scheme_inf_(src.scheme_inf_)
        , flags_(src.flags_)
        , path_segment_count_(src.path_segment_count_)
    {}
    // move constructor
    url(url&& src)
        : norm_url_(std::move(src.norm_url_))
        , part_end_(src.part_end_)
        , scheme_inf_(src.scheme_inf_)
        , flags_(src.flags_)
        , path_segment_count_(src.path_segment_count_)
    {}

    void clear();

    template <typename CharT>
    bool parse(const CharT* first, const CharT* last, const url* base);
    // wchar_t
    bool parse(const wchar_t* first, const wchar_t* last, const url* base) {
        typedef std::conditional<sizeof(wchar_t) == sizeof(char16_t), char16_t, char32_t>::type CharT;
        return parse(reinterpret_cast<const CharT*>(first), reinterpret_cast<const CharT*>(last), base);
    }

    template <class BufferT>
    bool parse(const BufferT& str, const url* base) {
        return parse(str.data(), str.data() + str.size(), base);
    }

    template <typename CharT>
    bool parse(const CharT* strz, const url* base) {
        const CharT* last = strz;
        while (*last) last++;
        return parse(strz, last, base);
    }

    // get serialized URL
    const std::string& href() const {
        return norm_url_;
    }

    std::string protocol() const {
        return std::string(norm_url_.data(), part_end_[SCHEME] ? part_end_[SCHEME] + 1 : 0);
    }

    std::string username() const {
        return get_part(USERNAME);
    }

    std::string password() const {
        return get_part(PASSWORD);
    }

    std::string host() const {
        if (is_null(HOST))
            return std::string("");
        if (is_null(PORT))
            return get_part(HOST);
        // "host:port"
        const size_t offset = part_end_[HOST_START];
        return std::string(norm_url_.data() + offset, part_end_[PORT] - offset);
    }

    std::string hostname() const {
        return get_part(HOST);
    }

    std::string port() const {
        return get_part(PORT);
    }

    std::string pathname() const {
        // https://url.spec.whatwg.org/#dom-url-pathname
        // already serialized as needed
        return get_part(PATH);
    }

    std::string search() const {
        if (is_empty(QUERY))
            return std::string("");
        // return with '?'
        return std::string(norm_url_.data() + part_end_[QUERY - 1], norm_url_.data() + part_end_[QUERY]);
    }

    std::string hash() const {
        if (is_empty(FRAGMENT))
            return std::string("");
        // return with '#'
        return std::string(norm_url_.data() + part_end_[FRAGMENT - 1], norm_url_.data() + part_end_[FRAGMENT]);
    }

    // get info
    
    std::string get_part(PartType t) const;

    str_view<char> get_part_view(PartType t) const;

    bool is_empty(const PartType t) const;

    bool is_null(const PartType t) const {
        return !(flags_ & (1u << t));
    }

    bool is_special_scheme() const {
        return scheme_inf_ && scheme_inf_->is_special;
    }

    bool is_file_scheme() const {
        return scheme_inf_ && scheme_inf_->is_file;
    }

    // URL includes credentials?
    bool has_credentials() const {
        return !is_empty(USERNAME) || !is_empty(PASSWORD);
    }

protected:
    enum UrlFlag : unsigned {
        // not null flags
        SCHEME_FLAG = (1u << SCHEME),
        USERNAME_FLAG = (1u << USERNAME),
        PASSWORD_FLAG = (1u << PASSWORD),
        HOST_FLAG = (1u << HOST),
        PORT_FLAG = (1u << PORT),
        PATH_FLAG = (1u << PATH),
        QUERY_FLAG = (1u << QUERY),
        FRAGMENT_FLAG = (1u << FRAGMENT),
        // other flags
        CANNOT_BE_BASE_FLAG = (1u << (PART_COUNT + 0)),
        // initial flags (empty (but not null) parts)
        INITIAL_FLAGS = SCHEME_FLAG | USERNAME_FLAG | PASSWORD_FLAG | PATH_FLAG,
    };

    bool is_null(const UrlFlag flag) const {
        return !(flags_ & flag);
    }

    // scheme
    template <class StringT>
    void set_scheme_str(const StringT str) {
        norm_url_.resize(0); // clear all
        part_end_[SCHEME] = str.length();
        norm_url_.append(str.data(), str.length());
        norm_url_ += ':';
    }
    void set_scheme(const url& src) {
        set_scheme_str(src.get_part_view(SCHEME));
        scheme_inf_ = src.scheme_inf_;
    }
    void set_scheme(const str_view<char> str) {
        set_scheme_str(str);
        scheme_inf_ = detail::get_scheme_info(str);
    }
    // TODO: remove b and e --> len
    void set_scheme(std::size_t b, std::size_t e) {
        assert(b == 0);
        part_end_[SCHEME] = e;
        scheme_inf_ = detail::get_scheme_info(get_part_view(SCHEME));
    }
    void clear_scheme() {
        norm_url_.resize(0); // clear all
        part_end_[SCHEME] = 0;
        //??: part_end_[SCHEME_SEP] = 0;
        scheme_inf_ = nullptr;
    }

    // path util
    str_view<char> get_path_first_string(size_t len) const;
    // path shortening
    bool get_path_rem_last(std::size_t& path_end, unsigned& path_segment_count) const;
    bool get_shorten_path(std::size_t& path_end, unsigned& path_segment_count) const;
    
    // flags
    void set_flag(const UrlFlag flag) {
        flags_ |= flag;
    }
    bool cannot_be_base() const {
        return !!(flags_ & CANNOT_BE_BASE_FLAG);
    }
    void set_cannot_be_base() {
        set_flag(CANNOT_BE_BASE_FLAG);
    }

private:
    std::string norm_url_;
    std::array<std::size_t, PART_COUNT> part_end_;
    const detail::scheme_info* scheme_inf_;
    unsigned flags_;
    unsigned path_segment_count_;

    friend class url_serializer;
    friend class url_setter;
    friend class url_parser;
};


class url_serializer {
public:
    url_serializer(url& dest_url)
        : url_(dest_url)
        , last_pt_(url::SCHEME)
    {}

    ~url_serializer();

    void new_url() { url_.clear(); }
    void reserve(size_t new_cap) { url_.norm_url_.reserve(new_cap); }

    // set data
    void set_scheme(const url& src) { url_.set_scheme(src); }
    void set_scheme(const str_view<char> str) { url_.set_scheme(str); }
    void set_scheme(std::size_t b, std::size_t e) { url_.set_scheme(b, e); }

    //TODO atskirti inline
    std::string& start_scheme() {
        url_.norm_url_.resize(0);
        return url_.norm_url_;
    }
    void save_scheme() {
        set_scheme(0, url_.norm_url_.length());
        url_.norm_url_.push_back(':');
    }
    void clear_scheme() {
        assert(last_pt_ == url::SCHEME);
        url_.clear_scheme();
    }

    void fill_parts_offset(url::PartType t1, url::PartType t2, size_t offset);
    std::string& start_part(url::PartType new_pt);
    void save_part();

    void clear_host();
    void empty_host();

    // path
    // TODO: append_to_path() --> append_empty_to_path()
    void append_to_path();
    std::string& start_path_segment();
    void save_path_segment();
    // if '/' not required:
    std::string& start_path_string();
    void save_path_string();

    void shorten_path();

    typedef bool (url::*PathOpFn)(std::size_t& path_end, unsigned& segment_count) const;
    void append_parts(const url& src, url::PartType t1, url::PartType t2, PathOpFn pathOpFn = nullptr);

    // flags
    void set_flag(const url::UrlFlag flag) { url_.set_flag(flag); }
    // IMPORTANT: cannot-be-a-base-URL flag must be set before or just after
    // SCHEME set; because other part's serialization depends on this flag
    void set_cannot_be_base() {
        assert(last_pt_ == url::SCHEME);
        url_.set_cannot_be_base();
    }

    // get info
    str_view<char> get_part_view(url::PartType t) const { return url_.get_part_view(t); }
    bool is_empty(const url::PartType t) const { return url_.is_empty(t); }
    bool is_empty_path() const { return url_.path_segment_count_ == 0; }
    bool is_null(const url::PartType t) const  { return url_.is_null(t); }
    bool is_special_scheme() const { return url_.is_special_scheme(); }
    bool is_file_scheme() const { return url_.is_file_scheme(); }
    bool has_credentials() const { return url_.has_credentials(); }
    const detail::scheme_info* scheme_inf() const { return url_.scheme_inf_; }

#if 0
    std::string& start_username();
    std::string& start_password();
    std::string& start_host();
    std::string& start_port();
    std::string& start_path();
    std::string& start_query();
    std::string& start_fragment();

    void save_username();
    void save_password();
    void save_host();
    void save_port();
    void save_path();
    void save_query();
    void save_fragment();
#endif

protected:
    url& url_;
    url::PartType last_pt_;
};

// TODO
class url_setter : public url_serializer {
public:
    url_setter(url& dest_url)
        : url_serializer(dest_url)
    {}

    //???
    void reserve(size_t new_cap) { strp_.reserve(new_cap); }

    std::string& start_scheme() {
        return strp_;
    }
    void save_scheme() {
        replace_part(url::SCHEME, strp_.data(), strp_.length());
        set_scheme(0, strp_.length());
    }
    void clear_scheme() {
        //?? assert(last_pt_ == url::SCHEME);
        strp_.clear();
    }

protected:
    void replace_part(url::PartType new_pt, const char* str, size_t len) {
        //TODO: start_part, jei new_pt > last_pt_
        int diff = static_cast<int>(len)-static_cast<int>(url_.part_[new_pt].len);
        url_.norm_url_.replace(url_.part_[new_pt].offset, url_.part_[new_pt].len, str, len);
        for (auto it = std::begin(url_.part_) + new_pt + 1; it < std::end(url_.part_); it++) {
            if (!it->offset) break;
            it->offset += diff;
        }
    }
protected:
    std::string strp_;
};


class url_parser {
public:
    enum State {
        not_set_state = 0,
        scheme_start_state,
        scheme_state,
        no_scheme_state,
        special_relative_or_authority_state,
        path_or_authority_state,
        relative_state,
        relative_slash_state,
        special_authority_slashes_state,
        special_authority_ignore_slashes_state,
        authority_state,
        host_state,
        hostname_state,
        port_state,
        file_state,
        file_slash_state,
        file_host_state,
        path_start_state,
        path_state,
        cannot_be_base_URL_path_state,
        query_state,
        fragment_state
    };

    template <typename CharT>
    static bool url_parse(url_serializer& urls, const CharT* first, const CharT* last, const url* base, State state_override = not_set_state);

    template <typename CharT>
    static bool parse_host(url_serializer& urls, const CharT* first, const CharT* last);

    template <typename CharT>
    static bool parse_opaque_host(url_serializer& urls, const CharT* first, const CharT* last);

    template <typename CharT>
    static ParseResult parse_ipv4(url_serializer& urls, const CharT* first, const CharT* last);

    template <typename CharT>
    static bool parse_ipv6(url_serializer& urls, const CharT* first, const CharT* last);

    template <typename CharT>
    static void parse_path(url_serializer& urls, const CharT* first, const CharT* last);

private:
    template <typename CharT>
    static bool do_path_segment(const CharT* pointer, const CharT* last, std::string& output);

    template <typename CharT>
    static bool do_simple_path(const CharT* pointer, const CharT* last, std::string& output);
};

namespace detail {

// canonical version of each possible input letter in the scheme
extern const char kSchemeCanonical[0x80];

// part start
extern const uint8_t kPartStart[url::PART_COUNT];

}

// Removable URL chars

template <typename CharT, typename UCharT = std::make_unsigned<CharT>::type>
auto to_unsigned(CharT ch) -> UCharT {
    return static_cast<std::make_unsigned<CharT>::type>(ch);
}

// chars to trim (C0 control or space: U+0000 to U+001F or U+0020)
template <typename CharT>
inline bool is_trim_char(CharT ch) {
    return to_unsigned(ch) <= ' ';
}

// chars what should be removed from the URL (ASCII tab or newline: U+0009, U+000A, U+000D)
template <typename CharT>
inline bool is_removable_char(CharT ch) {
    return ch == '\r' || ch == '\n' || ch == '\t';
}

template <class InputIt>
inline void do_trim(InputIt& first, InputIt& last) {
    // remove leading C0 controls and space
    while (first < last && is_trim_char(*first))
        first++;
    // remove trailing C0 controls and space
    auto rit = std::reverse_iterator<InputIt>(last);
    auto rend = std::reverse_iterator<InputIt>(first);
    while (rit < rend && is_trim_char(*rit))
        rit++;
    last = rit.base();
}

// DoRemoveURLWhitespace
// https://cs.chromium.org/chromium/src/url/url_canon_etc.cc
template <typename CharT>
inline void do_remove_whitespace(const CharT*& first, const CharT*& last, simple_buffer<CharT>& buff) {
    // Fast verification that there’s nothing that needs removal. This is the 99%
    // case, so we want it to be fast and don't care about impacting the speed
    // when we do find whitespace.
    for (auto it = first; it < last; it++) {
        if (!is_removable_char(*it))
            continue;
        // copy non whitespace chars into the new buffer and return it
        buff.reserve(last - first);
        buff.append(first, it);
        for (; it < last; it++) {
            if (!is_removable_char(*it))
                buff.push_back(*it);
        }
        first = buff.data();
        last = buff.data() + buff.size();
        break;
    }
}

// reverse find

template<class InputIt, class T>
InputIt find_last(InputIt first, InputIt last, const T& value) {
    for (auto it = last; it > first;) {
        --it;
        if (*it == value) return it;
    }
    return last;
}

// special chars

template <typename CharT>
inline bool is_slash(CharT ch) {
    return ch == '/' || ch == '\\';
}

template <typename CharT>
inline bool is_ascii_digit(CharT ch) {
    return ch <= '9' && ch >= '0';
}

template <typename CharT>
inline bool is_ascii_alpha(CharT ch) {
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
}

template <typename CharT>
inline bool is_Windows_drive(CharT c1, CharT c2) {
    return is_ascii_alpha(c1) && (c2 == ':' || c2 == '|');
}

template <typename CharT>
inline bool is_normalized_Windows_drive(CharT c1, CharT c2) {
    return is_ascii_alpha(c1) && c2 == ':';
}

// Scheme chars

template <typename CharT>
inline bool is_first_scheme_char(CharT ch) {
    return is_ascii_alpha(ch);
}

template <typename CharT>
inline bool is_authority_end_char(CharT c) {
    return c == '/' || c == '?' || c == '#';
}

template <typename CharT>
inline bool is_special_authority_end_char(CharT c) {
    return c == '/' || c == '?' || c == '#' || c == '\\';
}

// url class

inline std::string url::get_part(PartType t) const {
    if (t == SCHEME)
        return std::string(norm_url_.data(), part_end_[SCHEME]);
    // begin & end offsets
    const size_t b = part_end_[t - 1] + detail::kPartStart[t];
    const size_t e = part_end_[t];
    return std::string(norm_url_.data() + b, e > b ? e - b : 0);
}

inline str_view<char> url::get_part_view(PartType t) const {
    if (t == SCHEME)
        return str_view<char>(norm_url_.data(), part_end_[SCHEME]);
    // begin & end offsets
    const size_t b = part_end_[t - 1] + detail::kPartStart[t];
    const size_t e = part_end_[t];
    return str_view<char>(norm_url_.data() + b, e > b ? e - b : 0);
}

inline bool url::is_empty(const PartType t) const {
    if (t == SCHEME)
        return part_end_[SCHEME] == 0;
    // begin & end offsets
    const size_t b = part_end_[t - 1] + detail::kPartStart[t];
    const size_t e = part_end_[t];
    return b >= e;
}

inline void url::clear() {
    norm_url_.resize(0);
    part_end_.fill(0);
    scheme_inf_ = nullptr;
    flags_ = INITIAL_FLAGS;
    path_segment_count_ = 0;
}

template <typename CharT>
inline bool url::parse(const CharT* first, const CharT* last, const url* base) {
    url_serializer urls(*this); // new URL

    // reset URL
    urls.new_url();

    // remove any leading and trailing C0 control or space:
    do_trim(first, last);
    //TODO-WARN: validation error if trimmed

    return url_parser::url_parse(urls, first, last, base);
}

// https://url.spec.whatwg.org/#concept-basic-url-parser
template <typename CharT>
inline bool url_parser::url_parse(url_serializer& urls, const CharT* first, const CharT* last, const url* base, State state_override)
{
    typedef std::make_unsigned<CharT>::type UCharT;

    // remove all ASCII tab or newline from URL
    simple_buffer<CharT> buff_no_ws;
    do_remove_whitespace(first, last, buff_no_ws);
    //TODO-WARN: validation error if removed

    // reserve size (TODO: bet jei bus naudojama base?)
    auto length = std::distance(first, last);
    urls.reserve(length + 32); // žr.: GURL::InitCanonical(..)

    const char* encoding = "UTF-8";
    // TODO: If encoding override is given, set encoding to the result of getting an output encoding from encoding override. 

    auto pointer = first;
    State state = state_override ? state_override : scheme_start_state;

    // has scheme?
    if (state == scheme_start_state) {
        if (pointer != last && is_first_scheme_char(*pointer)) {
            state = scheme_state; // this appends first char to buffer
        } else if (!state_override) {
            state = no_scheme_state;
        } else {
            //TODO-ERR: validation error
            return false;
        }
    }

    if (state == scheme_state) {
        bool is_scheme = false;
        state = no_scheme_state;
        auto it_colon = std::find(pointer, last, ':');
        if (it_colon < last) {
            is_scheme = true;
            // start of scheme
            std::string& str_scheme = urls.start_scheme();
            // first char
            str_scheme.push_back(detail::kSchemeCanonical[*pointer]);
            // other chars
            for (auto it = pointer + 1; it < it_colon; it++) {
                UCharT ch = static_cast<UCharT>(*it);
                char c = (ch < 0x80) ? detail::kSchemeCanonical[ch] : 0;
                if (c) {
                    str_scheme.push_back(c);
                } else {
                    is_scheme = false;
                    break;
                }
            }
            if (is_scheme) {
                pointer = it_colon + 1; // skip ':'
                urls.save_scheme();
                // kas toliau
                //if (state_override) {
                //TODO:
                // 1. If state override is given, run these subsubsteps:
                //    1. If url’s scheme is a special scheme and buffer is not, then return.
                //    2. If url’s scheme is not a special scheme and buffer is, then return.
                // 2. Set url’s scheme to buffer.
                // 4. If state override is given, then return.
                //}
                if (urls.is_file_scheme()) {
                    // TODO-WARN: if remaining does not start with "//", validation error.
                    state = file_state;
                } else {
                    if (urls.is_special_scheme()) {
                        if (base && urls.get_part_view(url::SCHEME).equal(base->get_part_view(url::SCHEME))) {
                            // NOTE: This means that base’s cannot-be-a-base-URL flag is unset
                            state = special_relative_or_authority_state;
                        } else {
                            state = special_authority_slashes_state;
                        }
                    } else if (pointer < last && *pointer == '/') {
                        state = path_or_authority_state;
                        pointer++;
                    } else {
                        urls.set_cannot_be_base();
                        // append an empty string to url’s path
                        // urls.append_to_path();
                        urls.start_path_string(); // ne start_path_segment()
                        urls.save_path_string();
                        state = cannot_be_base_URL_path_state;
                    }
                }
            } else {
                // remove invalid scheme
                urls.clear_scheme();
            }
        }
        if (state_override && !is_scheme) {
            //TODO-ERR: validation error
            return false;
        }
    }

    if (state == no_scheme_state) {
        if (base) {
            if (base->cannot_be_base()) {
                if (pointer < last && *pointer == '#') {
                    // SVARBU: set_cannot_be_base() turi būti prieš append_parts(..),
                    // kad pastarasis teisingai vykdytų serializavimą
                    urls.set_cannot_be_base();
                    urls.set_scheme(*base);
                    urls.append_parts(*base, url::PATH, url::QUERY);
                    //TODO: url’s fragment to the empty string
                    state = fragment_state;
                    pointer++;
                } else {
                    // TODO-ERR: 1. validation error
                    return false;
                }
            } else {
                state = base->is_file_scheme() ? file_state : relative_state;
            }
        } else {
            //TODO-ERR: 1. validation error
            return false;
        }
    }
    
    if (state == special_relative_or_authority_state) {
        if (pointer + 1 < last && pointer[0] == '/' && pointer[1] == '/') {
            state = special_authority_ignore_slashes_state;
            pointer += 2; // skip "//"
        } else {
            //TODO-WARN: validation error
            state = relative_state;
        }
    }

    if (state == path_or_authority_state) {
        if (pointer < last && pointer[0] == '/') {
            state = authority_state;
            pointer++; // skip "/"
        } else {
            state = path_state;
        }
    }

    if (state == relative_state) {
        // std::assert(base != nullptr);
        urls.set_scheme(*base);
        if (pointer == last) {
            // EOF code point
            // Set url’s username to base’s username, url’s password to base’s password, url’s host to base’s host,
            // url’s port to base’s port, url’s path to base’s path, and url’s query to base’s query
            urls.append_parts(*base, url::USERNAME, url::QUERY);
            return true; // EOF
        } else {
            const CharT ch = *(pointer++);
            switch (ch) {
            case '/':
                state = relative_slash_state;
                break;
            case '?':
                // Set url’s username to base’s username, url’s password to base’s password, url’s host to base’s host,
                // url’s port to base’s port, url’s path to base’s path, url’s query to the empty string, and state to query state.
                urls.append_parts(*base, url::USERNAME, url::PATH);
                state = query_state;    // sets query to the empty string
                break;
            case '#':
                // Set url’s username to base’s username, url’s password to base’s password, url’s host to base’s host,
                // url’s port to base’s port, url’s path to base’s path, url’s query to base’s query, url’s fragment to the empty string
                urls.append_parts(*base, url::USERNAME, url::QUERY);
                state = fragment_state; // sets fragment to the empty string
                break;
            case '\\':
                if (urls.is_special_scheme()) {
                    //TODO-WARN: validation error
                    state = relative_slash_state;
                    break;
                }
            default:
                // Set url’s username to base’s username, url’s password to base’s password, url’s host to base’s host,
                // url’s port to base’s port, url’s path to base’s path, and then remove url’s path’s last entry, if any
                urls.append_parts(*base, url::USERNAME, url::PATH, &url::get_path_rem_last);
                state = path_state;
                pointer--;
            }
        }
    }

    if (state == relative_slash_state) {
        // EOF ==> 0 ==> default:
        switch (pointer != last ? *pointer : 0) {
        case '/':
            if (urls.is_special_scheme())
                state = special_authority_ignore_slashes_state;
            else
                state = authority_state;
            pointer++;
            break;
        case '\\':
            if (urls.is_special_scheme()) {
                // TODO-WARN: validation error
                state = special_authority_ignore_slashes_state;
                pointer++;
                break;
            }
        default:
            // set url’s username to base’s username, url’s password to base’s password, url’s host to base’s host,
            // url’s port to base’s port
            urls.append_parts(*base, url::USERNAME, url::PORT);
            state = path_state;
        }
    }

    if (state == special_authority_slashes_state) {
        if (pointer + 1 < last && pointer[0] == '/' && pointer[1] == '/') {
            state = special_authority_ignore_slashes_state;
            pointer += 2; // skip "//"
        } else {
            //TODO-WARN: validation error
            state = special_authority_ignore_slashes_state;
        }
    }

    if (state == special_authority_ignore_slashes_state) {
        auto it = pointer;
        while (it < last && is_slash(*it)) it++;
        // if (it != pointer) // TODO-WARN: validation error
        pointer = it;
        state = authority_state;
    }

    // TODO?: credentials serialization do after host parsing,
    //       because if host null ==> then no credentials serialization
    if (state == authority_state) {
        // TODO: saugoti end_of_authority ir naudoti kituose state
        auto end_of_authority = urls.is_special_scheme() ?
            std::find_if(pointer, last, [](CharT c) { return c == '/' || c == '?' || c == '#' || c == '\\'; }) :
            std::find_if(pointer, last, [](CharT c) { return c == '/' || c == '?' || c == '#'; });
        
        auto it_eta = find_last(pointer, end_of_authority, '@');
        if (it_eta != end_of_authority) {
            if (std::distance(it_eta, end_of_authority) == 1) {
                // disallow empty host, example: "http://u:p@/"
                return false; // TODO-ERR: 2.1. validation error, failure
            }
            //TODO-WARN: validation error
            auto it_colon = std::find(pointer, it_eta, ':');
            // url includes credentials?
            const bool not_empty_password = std::distance(it_colon, it_eta) > 1;
            if (not_empty_password || std::distance(pointer, it_colon) > 0 /*not empty username*/) {
                // username
                std::string& str_username = urls.start_part(url::USERNAME);
                detail::AppendStringOfType(pointer, it_colon, detail::CHAR_USERINFO, str_username); // UTF-8 percent encode, @ -> %40
                urls.save_part();
                // password
                if (not_empty_password) {
                    std::string& str_password = urls.start_part(url::PASSWORD);
                    detail::AppendStringOfType(it_colon + 1, it_eta, detail::CHAR_USERINFO, str_password); // UTF-8 percent encode, @ -> %40
                    urls.save_part();
                }
            }
            // after '@'
            pointer = it_eta + 1;
        }
        state = host_state;
    }

    if (state == host_state || state == hostname_state) {
        if (state_override && urls.is_file_scheme()) {
            state = file_host_state;
        } else {
            auto end_of_authority = urls.is_special_scheme() ?
                std::find_if(pointer, last, [](CharT c) { return c == '/' || c == '?' || c == '#' || c == '\\'; }) :
                std::find_if(pointer, last, [](CharT c) { return c == '/' || c == '?' || c == '#'; });

            bool in_square_brackets = false; // [] flag
            bool is_port = false;
            auto it_host_end = pointer;
            for (; it_host_end < end_of_authority; it_host_end++) {
                const CharT ch = *it_host_end;
                if (ch == ':') {
                    if (!in_square_brackets) {
                        is_port = true;
                        break;
                    }
                } else if (ch == '[') {
                    in_square_brackets = true;
                } else if (ch == ']') {
                    in_square_brackets = false;
                }
            }

            // if buffer is the empty string
            if (pointer == it_host_end) {
                // make sure that if port is present or scheme is special, host is non-empty
                if (is_port || urls.is_special_scheme()) {
                    // TODO-ERR: validation error, host failure
                    return false;
                } else if (state_override && (urls.has_credentials() || !urls.is_null(url::PORT))) {
                    // TODO-WARN: validation error
                    return true;
                }
            }

            // parse and set host:
            if (!parse_host(urls, pointer, it_host_end))
                return false;

            if (is_port) {
                pointer = it_host_end + 1; // skip ':'
                state = port_state;
                if (state_override == hostname_state)
                    return true;
            } else {
                pointer = it_host_end;
                state = path_start_state;
                if (state_override)
                    return true;
            }
        }
    }

    if (state == port_state) {
        auto end_of_authority = urls.is_special_scheme() ?
            std::find_if(pointer, last, [](CharT c) { return c == '/' || c == '?' || c == '#' || c == '\\'; }) :
            std::find_if(pointer, last, [](CharT c) { return c == '/' || c == '?' || c == '#'; });

        auto end_of_digits = std::find_if_not(pointer, end_of_authority, is_ascii_digit<CharT>);

        if (end_of_digits == end_of_authority || state_override) {
            if (pointer < end_of_digits) {
                // is port
                int port = 0;
                for (auto it = pointer; it < end_of_digits; it++) {
                    port = port * 10 + (*it - '0');
                    if (port > 0xFFFF) return false; // TODO-ERR: (2-1-2) validation error, failure
                }
                // set port if not default
                if (urls.scheme_inf() == nullptr || urls.scheme_inf()->default_port != port) {
                    urls.start_part(url::PORT) += std::to_string(port);
                    urls.save_part();
                    urls.set_flag(url::PORT_FLAG);
                } //TODO: (2-1-3) Set url’s port to null
            }
            if (state_override)
                return true; // (2-2)
            state = path_start_state;
            pointer = end_of_authority;
        } else {
            // TODO-ERR: (3) validation error, failure
            return false;
        }
    }

    if (state == file_state) {
        if (!urls.is_file_scheme())
            urls.set_scheme({ "file", 4 });
        // EOF ==> 0 ==> default:
        switch (pointer != last ? *pointer : 0) {
        case '\\':
            // TODO-WARN: validation error
        case '/':
            state = file_slash_state;
            pointer++;
            break;

        default:
            if (base && base->is_file_scheme()) {
                if (pointer == last) {
                    // EOF code point
                    // Set url's host to base's host, url's path to base's path, and url's query to base's query
                    urls.append_parts(*base, url::HOST, url::QUERY);
                    return true; // EOF
                } else {
                    const CharT ch = *(pointer++);
                    switch (ch) {
                    case '?':
                        // Set url's host to base's host, url's path to base's path, url's query to the empty string
                        urls.append_parts(*base, url::HOST, url::PATH);
                        state = query_state; // sets query to the empty string
                        break;
                    case '#':
                        // Set url's host to base's host, url's path to base's path, url's query to base's query, url's fragment to the empty string
                        urls.append_parts(*base, url::HOST, url::QUERY);
                        state = fragment_state; // sets fragment to the empty string
                        break;
                    default:
                        if (pointer + 1 == last // remaining consists of one code point
                            || (pointer + 1 < last && !is_Windows_drive(ch, pointer[0]))
                            || (pointer + 2 < last && !is_special_authority_end_char(pointer[1]))
                            ) {
                            // set url’s host to base’s host, url’s path to base’s path, and then shorten url’s path
                            urls.append_parts(*base, url::HOST, url::PATH, &url::get_shorten_path);
                            // Note: This is a (platform-independent) Windows drive letter quirk.
                        }
                        //else // TODO-WARN: validation error
                        state = path_state;
                        pointer--;
                    }
                }
            } else {
                state = path_state;
            }
        }
    }

    if (state == file_slash_state) {
        // EOF ==> 0 ==> default:
        switch (pointer != last ? *pointer : 0) {
        case '\\':
            // TODO-WARN: validation error
        case '/':
            state = file_host_state;
            pointer++;
            break;

        default:
            if (base && base->is_file_scheme()) {
                str_view<char> base_path = base->get_path_first_string(2);
                // if base’s path[0] is a normalized Windows drive letter
                if (base_path.length() == 2 &&
                    is_normalized_Windows_drive(base_path[0], base_path[1])) {
                    // append base’s path[0] to url’s path
                    std::string& str_path = urls.start_path_segment();
                    str_path.append(base_path.data(), 2); // "C:"
                    urls.save_path_segment();
                    // Note: This is a (platform - independent) Windows drive letter quirk.
                    // Both url’s and base’s host are null under these conditions and therefore not copied.
                }
            }
            state = path_state;
        }
    }

    if (state == file_host_state) {
        auto end_of_authority = std::find_if(pointer, last, is_special_authority_end_char<CharT>);

        if (pointer == end_of_authority) {
            // buffer is the empty string
            // set empty host
            urls.start_part(url::HOST);
            urls.save_part();
            urls.set_flag(url::HOST_FLAG);
            // if state override is given, then return
            if (state_override)
                return true;
            state = path_start_state;
        } else if (!state_override && pointer + 2 == end_of_authority && is_Windows_drive(pointer[0], pointer[1])) {
            // buffer is a Windows drive letter
            // TODO-WARN: validation error
            state = path_state;
            // Note: This is a (platform - independent) Windows drive letter quirk.
            // buffer is not reset here and instead used in the path state.
            // TODO: buffer is not reset here and instead used in the path state
        } else {
            // parse and set host:
            if (!parse_host(urls, pointer, end_of_authority))
                return false; // TODO-ERR: failure
            // if host is "localhost", then set host to the empty string
            if (urls.get_part_view(url::HOST).equal({ "localhost", 9 })) {
                // set empty host
                urls.empty_host();
            }
            // if state override is given, then return
            if (state_override)
                return true;
            pointer = end_of_authority;
            state = path_start_state;
        }
    }

    if (state == path_start_state) {
        if (urls.is_special_scheme()) {
            if (pointer != last) {
                const CharT ch = *pointer;
                if (ch == '/') pointer++;
                else if (ch == '\\') {
                    // TODO-WARN: validation error
                    pointer++;
                }
            }
            state = path_state;
        } else if (pointer != last) {
            if (!state_override) {
                switch (pointer[0]) {
                case '?':
                    // TODO: set url’s query to the empty string
                    state = query_state;
                    pointer++;
                    break;
                case '#':
                    // TODO: set url’s fragment to the empty string
                    state = fragment_state;
                    pointer++;
                    break;
                case '/':
                    pointer++;
                default:
                    state = path_state;
                    break;
                }
            } else {
                if (pointer[0] == '/') pointer++;
                state = path_state;
            }
        } else {
            // EOF
            return true;
        }
    }

    if (state == path_state) {
        // TODO: saugoti end_of_path ir naudoti kituose state
        auto end_of_path = state_override ? last :
            std::find_if(pointer, last, [](CharT c) { return c == '?' || c == '#'; });

        parse_path(urls, pointer, end_of_path);
        pointer = end_of_path;
        
        if (pointer == last) {
            return true;
        } else {
            const CharT ch = *pointer++;
            if (ch == '?') {
                // TODO: set url’s query to the empty string
                state = query_state;
            } else {
                // ch == '#'
                // TODO: set url’s fragment to the empty string
                state = fragment_state;
            }
        }
    }

    if (state == cannot_be_base_URL_path_state) {
        auto end_of_path = state_override ? last :
            std::find_if(pointer, last, [](CharT c) { return c == '?' || c == '#'; });

        // UTF-8 percent encode using the C0 control percent-encode set,
        // and append the result to url’s path[0]
        std::string& str_path = urls.start_path_string(); // ne start_path_segment()
        do_simple_path(pointer, end_of_path, str_path);
        urls.save_path_string();
        pointer = end_of_path;

        if (pointer == last) {
            return true;
        } else {
            const CharT ch = *pointer++;
            if (ch == '?') {
                // TODO: set url’s query to the empty string
                state = query_state;
            } else {
                // ch == '#'
                // TODO: set url’s fragment to the empty string
                state = fragment_state;
            }
        }
    }

    if (state == query_state) {
        auto end_of_query = state_override ? last : std::find(pointer, last, '#');

        // TODO-WARN:
        //for (auto it = pointer; it < end_of_query; it++) {
        //  UCharT c = static_cast<UCharT>(*it);
        //  // 1. If c is not a URL code point and not "%", validation error.
        //  // 2. If c is "%" and remaining does not start with two ASCII hex digits, validation error.
        //}

        // scheme_inf_ == nullptr, if unknown scheme
        if (!urls.scheme_inf() || !urls.scheme_inf()->is_special || urls.scheme_inf()->is_ws)
            encoding = "UTF-8";

        // Set buffer to the result of encoding buffer using encoding
        // percent encode: (c < 0x21 || c > 0x7E || c == 0x22 || c == 0x23 || c == 0x3C || c == 0x3E)
        // TODO: dabar palaiko tik encoding = "UTF-8"; kitų palaikymą galima padaryti pagal:
        // https://cs.chromium.org/chromium/src/url/url_canon_query.cc?rcl=1479817139&l=93
        std::string& str_query = urls.start_part(url::QUERY);
        detail::AppendStringOfType(pointer, end_of_query, detail::CHAR_QUERY, str_query); // šis dar koduoja 0x27 '
        urls.save_part();
        urls.set_flag(url::QUERY_FLAG);

        pointer = end_of_query;
        if (pointer == last)
            return true; // EOF
        // *pointer == '#'
        //TODO: set url’s fragment to the empty string
        state = fragment_state;
        pointer++; // skip '#'
    }

    if (state == fragment_state) {
        // pagal: https://cs.chromium.org/chromium/src/url/url_canon_etc.cc : DoCanonicalizeRef(..)
        std::string& str_frag = urls.start_part(url::FRAGMENT);
        while (pointer < last) {
            // UTF-8 percent encode c using the C0 control percent-encode set & ignore '\0'
            UCharT uch = static_cast<UCharT>(*pointer);
            if (uch >= 0x80) {
                // invalid utf-8/16/32 sequences will be replaced with kUnicodeReplacementCharacter
                detail::AppendUTF8EscapedChar(pointer, last, str_frag);
            } else {
                if (uch >= 0x20) {
                    // Normal ASCII characters are just appended
                    str_frag.push_back(static_cast<unsigned char>(uch));
                } else if (uch) {
                    // C0 control characters (except '\0') are escaped
                    detail::AppendEscapedChar(uch, str_frag);
                } else { // uch == 0
                    // TODO-WARN: validation error
                }
                pointer++;
            }
            // TODO-WARN:
            // If c is not a URL code point and not "%", validation error.
            // If c is "%" and remaining does not start with two ASCII hex digits, validation error.
        }
        urls.save_part();
        urls.set_flag(url::FRAGMENT_FLAG);
    }

    return true;
}

// internal functions

template <typename CharT>
static inline bool is_valid_host_chars(const CharT* first, const CharT* last) {
    return std::none_of(first, last, detail::IsInvalidHostChar<CharT>);
}

template <typename CharT>
static inline bool is_valid_opaque_host_chars(const CharT* first, const CharT* last) {
    return std::none_of(first, last, [](CharT c) {
        return detail::IsInvalidHostChar(c) && c != '%';
    });
}

template <typename CharT>
inline bool url_parser::parse_host(url_serializer& urls, const CharT* first, const CharT* last) {
    typedef std::make_unsigned<CharT>::type UCharT;

    // ši sąlyga turi būti patikrinta prieš kreipiantis
    // (todo angl?: this condition must by verified before calling)
    if (first == last) {
        // https://github.com/whatwg/url/issues/79
        // https://github.com/whatwg/url/pull/189
        // set empty host
        urls.start_part(url::HOST);
        urls.save_part();
        urls.set_flag(url::HOST_FLAG);
        return true;
    }
    assert(first < last);

    if (*first == '[') {
        if (*(last - 1) == ']') {
            return parse_ipv6(urls, first + 1, last - 1);
        } else {
            // TODO-ERR: validation error
            return false;
        }
    }

    if (!urls.is_special_scheme())
        return parse_opaque_host(urls, first, last);

    // check if host has non ascii characters or percent sign
    bool has_no_ascii = false;
    bool has_escaped = false;
    for (auto it = first; it < last;) {
        UCharT uch = static_cast<UCharT>(*it++);
        if (uch >= 0x80) {
            has_no_ascii = true;
        } else {
            unsigned char uc8;
            if (uch == '%' && detail::DecodeEscaped(it, last, uc8)) {
                has_escaped = true;
                if ((has_no_ascii = has_no_ascii || (uc8 >= 0x80))) break;
            }
        }
    }

    //TODO: klaidų nustatymas pagal standartą

    std::basic_string<char16_t> buff_uc;
    if (has_no_ascii) {
        if (has_escaped) {
            simple_buffer<unsigned char> buff_utf8;

            uint32_t code_point;
            for (auto it = first; it < last;) {
                url_util::read_utf_char(it, last, code_point);
                if (code_point == '%') {
                    // unescape until escaped
                    unsigned char uc8;
                    if (detail::DecodeEscaped(it, last, uc8)) {
                        buff_utf8.push_back(uc8);
                        while (it < last && *it == '%') {
                            it++; // skip '%'
                            if (!detail::DecodeEscaped(it, last, uc8))
                                uc8 = '%';
                            buff_utf8.push_back(uc8);
                        }
                        detail::ConvertUTF8ToUTF16(buff_utf8.data(), buff_utf8.data() + buff_utf8.size(), buff_uc);
                        buff_utf8.clear();
                    } else {
                        detail::AppendUTF16Value(code_point, buff_uc);
                    }
                } else {
                    detail::AppendUTF16Value(code_point, buff_uc);
                }
            }
        } else {
            detail::ConvertToUTF16(first, last, buff_uc);
        }
    } else {
        // (first,last) has only ASCII characters
        // Net ir ASCII turi praeiti IDNToASCII patikrinimą;
        // tačiau žr.: https://github.com/jsdom/whatwg-url/issues/50
        //  ^-- kad korektiškai veiktų reikia Unicode 9 palaikymo
        if (has_escaped) {
            for (auto it = first; it < last;) {
                unsigned char uc8 = static_cast<unsigned char>(*it++);
                if (uc8 == '%')
                    detail::DecodeEscaped(it, last, uc8);
                buff_uc.push_back(uc8);
            }
        } else {
            buff_uc.append(first, last);
        }
    }

    // domain to ASCII
    simple_buffer<char16_t> buff_ascii;

    if (!IDNToASCII(buff_uc.data(), buff_uc.length(), buff_ascii))
        return false;
    if (!is_valid_host_chars(buff_ascii.data(), buff_ascii.data() + buff_ascii.size())) {
        //TODO-ERR: validation error
        return false;
    }
    // IPv4
    const ParseResult res = parse_ipv4(urls, buff_ascii.begin(), buff_ascii.end());
    if (res == RES_FALSE) {
        std::string& str_host = urls.start_part(url::HOST);
        str_host.append(buff_ascii.begin(), buff_ascii.end());
        urls.save_part();
        urls.set_flag(url::HOST_FLAG);
    }
    return res != RES_ERROR;
}

template <typename CharT>
inline bool url_parser::parse_opaque_host(url_serializer& urls, const CharT* first, const CharT* last) {
    if (!is_valid_opaque_host_chars(first, last))
        return false; //TODO-ERR: failure

    std::string& str_host = urls.start_part(url::HOST);
    //TODO: UTF-8 percent encode it using the C0 control percent-encode set
    //detail::AppendStringOfType(first, last, detail::CHAR_C0_CTRL, str_host);
    // do_simple_path(..) tą daro, tačiau galimi warning(), o jų čia nereikia,
    // todėl reikalinga kita kodavimo f-ja:
    do_simple_path(first, last, str_host);
    urls.save_part();
    urls.set_flag(url::HOST_FLAG);
    return true;
}

template <typename CharT>
inline ParseResult url_parser::parse_ipv4(url_serializer& urls, const CharT* first, const CharT* last) {
    uint32_t ipv4;

    const ParseResult res = ipv4_parse(first, last, ipv4);
    if (res == RES_OK) {
        std::string& str_ipv4 = urls.start_part(url::HOST);
        ipv4_serialize(ipv4, str_ipv4);
        urls.save_part();
        urls.set_flag(url::HOST_FLAG);
    }
    return res;
}

template <typename CharT>
inline bool url_parser::parse_ipv6(url_serializer& urls, const CharT* first, const CharT* last) {
    uint16_t ipv6addr[8];

    if (ipv6_parse(first, last, ipv6addr)) {
        std::string& str_ipv6 = urls.start_part(url::HOST);
        str_ipv6.push_back('[');
        ipv6_serialize(ipv6addr, str_ipv6);
        str_ipv6.push_back(']');
        urls.save_part();
        urls.set_flag(url::HOST_FLAG);
        return true;
    }
    return false;
}

template <typename CharT>
inline void url_parser::parse_path(url_serializer& urls, const CharT* first, const CharT* last) {
    // path state; includes:
    // 1. [ (/,\) - 1, 2, 3, 4 - [ 1 (if first segment), 2 ] ]
    // 2. [ 1 ... 4 ]
    auto double_dot = [](const CharT* const pointer, const int len) -> bool {
        if (len == 2) {
            return (pointer[0] == '.' && pointer[1] == '.');
        } else if (len == 4) {
            return detail::AsciiEqualNoCase(pointer, pointer + 4, ".%2e") ||
                   detail::AsciiEqualNoCase(pointer, pointer + 4, "%2e.");
        } else if (len == 6) {
            return detail::AsciiEqualNoCase(pointer, pointer + 6, "%2e%2e");
        } else
            return false;
    };
    auto single_dot = [](const CharT* const pointer, const int len) -> bool {
        if (len == 1) {
            return pointer[0] == '.';
        } else if (len == 3) {
            return detail::AsciiEqualNoCase(pointer, pointer + 3, "%2e");
        } else
            return false;
    };

    // parse path's segments
    auto pointer = first;
    while (true) {
        auto end_of_segment = urls.is_special_scheme()
            ? std::find_if(pointer, last, is_slash<CharT>)
            : std::find(pointer, last, '/');

        int len = end_of_segment - pointer;
        bool is_last = end_of_segment == last;
        // TODO-WARN: 1. If url is special and c is "\", validation error.

        if (double_dot(pointer, len)) {
            urls.shorten_path();
            if (is_last) urls.append_to_path();
        } else if (single_dot(pointer, len)) {
            if (is_last) urls.append_to_path();
        } else {
            if (len == 2 &&
                urls.is_file_scheme() &&
                urls.is_empty_path() &&
                is_Windows_drive(pointer[0], pointer[1]))
            {
                if (!urls.is_null(url::HOST)) {
                    // 1. If url’s host is non-null, validation error
                    // TODO-WARN: validation error
                    // 2. Set url’s host to null
                    urls.clear_host();
                }
                // and replace the second code point in buffer with ":"
                std::string& str_path = urls.start_path_segment();
                str_path += static_cast<char>(pointer[0]);
                str_path += ':';
                urls.save_path_segment();
                //Note: This is a (platform-independent) Windows drive letter quirk.
            } else {
                std::string& str_path = urls.start_path_segment();
                do_path_segment(pointer, end_of_segment, str_path);
                urls.save_path_segment();
                // end of segment
                pointer = end_of_segment;
            }
        }
        // next segment
        if (is_last) break;
        pointer = end_of_segment + 1; // skip '/' or '\'
    }
}

template <typename CharT>
inline bool url_parser::do_path_segment(const CharT* pointer, const CharT* last, std::string& output) {
    typedef std::make_unsigned<CharT>::type UCharT;

    // TODO-WARN: 2. [ 1 ... 2 ] validation error.
    bool success = true;
    while (pointer < last) {
        // UTF-8 percent encode c using the default encode set
        UCharT uch = static_cast<UCharT>(*pointer);
        if (uch >= 0x80) {
            // invalid utf-8/16/32 sequences will be replaced with 0xfffd
            success &= detail::AppendUTF8EscapedChar(pointer, last, output);
        } else {
            // Just append the 7-bit character, possibly escaping it.
            unsigned char uc = static_cast<unsigned char>(uch);
            if (!IsCharOfType(uc, detail::CHAR_PATH))
                detail::AppendEscapedChar(uc, output);
            else
                output.push_back(uc);
            pointer++;
        }
    }
    return success;
}

template <typename CharT>
inline bool url_parser::do_simple_path(const CharT* pointer, const CharT* last, std::string& output) {
    typedef std::make_unsigned<CharT>::type UCharT;

    // 3. of "cannot-be-a-base-URL path state"
    // TODO-WARN: 3. [ 1 ... 2 ] validation error.
    //  1. If c is not EOF code point, not a URL code point, and not "%", validation error.
    //  2. If c is "%" and remaining does not start with two ASCII hex digits, validation error.

    bool success = true;
    while (pointer < last) {
        // UTF-8 percent encode c using the C0 control percent-encode set
        UCharT uch = static_cast<UCharT>(*pointer);
        if (uch >= 0x80) {
            // invalid utf-8/16/32 sequences will be replaced with 0xfffd
            success &= detail::AppendUTF8EscapedChar(pointer, last, output);
        } else {
            // Just append the 7-bit character, escaping C0 control chars:
            if (uch <= 0x1f)
                detail::AppendEscapedChar(uch, output);
            else
                output.push_back(static_cast<unsigned char>(uch));
            pointer++;
        }
    }
    return success;
}

// path util

inline str_view<char> url::get_path_first_string(size_t len) const {
    str_view<char> pathv = get_part_view(PATH);
    if (pathv.length() == 0 || cannot_be_base())
        return pathv;
    // skip '/'
    pathv.remove_prefix(1);
    if (pathv.length() == len || (pathv.length() > len && pathv[len] == '/')) {
        return str_view<char>(pathv.data(), len);
    }
    return str_view<char>(pathv.data(), 0);
}

// path shortening

inline bool url::get_path_rem_last(std::size_t& path_end, unsigned& path_segment_count) const {
    if (path_segment_count_ > 0) {
        // Remove path’s last item
        const char* first = norm_url_.data() + part_end_[url::PATH-1];
        const char* last = norm_url_.data() + part_end_[url::PATH];
        const char* it = find_last(first, last, '/');
        if (it == last) it = first; // jei nebuvo '/' išmesim visą kelią
        // shorten
        path_end = it - norm_url_.data();
        path_segment_count = path_segment_count_ - 1;
        return true;
    }
    return false;
}

inline bool url::get_shorten_path(std::size_t& path_end, unsigned& path_segment_count) const {
    if (path_segment_count_ == 0)
        return false;
    if (is_file_scheme() && path_segment_count_ == 1) {
        str_view<char> path1 = get_path_first_string(2);
        if (path1.length() == 2 && is_normalized_Windows_drive(path1[0], path1[1]))
            return false;
    }
    // Remove path's last item
    return get_path_rem_last(path_end, path_segment_count);
}

// class url_serializer

inline void url_serializer::shorten_path() {
    assert(last_pt_ <= url::PATH);
    if (url_.get_shorten_path(url_.part_end_[url::PATH], url_.path_segment_count_))
        url_.norm_url_.resize(url_.part_end_[url::PATH]);
}

inline url_serializer::~url_serializer() {
    switch (last_pt_) {
    case url::SCHEME:
        // if url’s host is null and url’s scheme is "file"
        if (url_.is_file_scheme())
            url_.norm_url_.append("//");
        break;
    }
}

inline void url_serializer::fill_parts_offset(url::PartType t1, url::PartType t2, size_t offset) {
    for (int ind = t1; ind < t2; ind++)
        url_.part_end_[ind] = offset;
}

inline std::string& url_serializer::start_part(url::PartType new_pt) {
    // užpido ir tuščių dalių (iki new_pt) offset-ą
    url::PartType fill_start_pt = static_cast<url::PartType>(static_cast<int>(last_pt_)+1);
    switch (last_pt_) {
    case url::SCHEME:
        // if host is non-null or scheme is "file"
        if (new_pt <= url::HOST || url_.is_file_scheme())
            url_.norm_url_.append("//");
        break;
    case url::USERNAME:
        if (new_pt == url::PASSWORD) {
            url_.norm_url_ += ':';
            break;
        } else {
            url_.part_end_[url::PASSWORD] = url_.norm_url_.length();
            fill_start_pt = url::HOST_START; // (url::PASSWORD + 1)
        }
    case url::PASSWORD:
        if (new_pt == url::HOST) {
            url_.norm_url_ += '@';
            break;
        }
        // NOT REACHABLE (TODO: throw?)
        break;
    case url::HOST:
        if (new_pt == url::PORT) {
            url_.norm_url_ += ':';
            break;
        }
    case url::PORT:
        break;
    case url::PATH:
        if (new_pt == url::PATH) // continue on path
            return url_.norm_url_;
        break;
    }

    fill_parts_offset(fill_start_pt, new_pt, url_.norm_url_.length());

    switch (new_pt) {
    case url::QUERY:
        url_.norm_url_ += '?';
        break;
    case url::FRAGMENT:
        url_.norm_url_ += '#';
        break;
    }

    assert(last_pt_ < new_pt);
    // value to url_.part_end_[new_pt] will be assigned in save_part()
    last_pt_ = new_pt;
    return url_.norm_url_;
}

inline void url_serializer::save_part() {
    url_.part_end_[last_pt_] = url_.norm_url_.length();
}

// append_empty_to_path() kviečiamas trijose vietose:
// 1) scheme_state -> [2.9.] -> cannot_be_base_URL_path_state
// 2) path_state -> [1.2. ".." ]
// 3) path_state -> [1.3. "." ]
inline void url_serializer::append_to_path() { // append_empty_to_path();
    start_path_segment();
    save_path_segment();
}

inline std::string& url_serializer::start_path_segment() {
    // lipdom prie esamo kelio: / seg1 / seg2 / ... / segN
    std::string& str_path = start_part(url::PATH);
    str_path += '/';
    return str_path;
}

inline void url_serializer::save_path_segment() {
    save_part();
    url_.path_segment_count_++;
}

inline std::string& url_serializer::start_path_string() {
    //return start_part(url::PATH);
    if (last_pt_ != url::PATH)
        return start_part(url::PATH);
    return url_.norm_url_;
}

inline void url_serializer::save_path_string() {
    assert(url_.path_segment_count_ <= 1);
    save_part();
    url_.path_segment_count_ = 1;
}


inline void url_serializer::clear_host() {
    assert(!is_empty(url::SCHEME));
    assert(last_pt_ == url::HOST || (last_pt_ == url::PATH && is_empty_path()));
    // if last_pt_ == url::PATH
    url_.part_end_[url::PATH] = 0;
    // paliekam tik "scheme:"
    url_.norm_url_.resize(url_.part_end_[url::SCHEME] + 1);
    fill_parts_offset(url::SCHEME_SEP, url::PATH, 0);
    url_.flags_ &= ~url::HOST_FLAG; // set to null
    last_pt_ = url::SCHEME;
}

inline void url_serializer::empty_host() {
    assert(last_pt_ == url::HOST);
    const std::size_t host_end = url_.part_end_[url::HOST_START];
    url_.part_end_[url::HOST] = host_end;
    url_.norm_url_.resize(host_end);
}

// append parts from other url

inline void url_serializer::append_parts(const url& src, url::PartType t1, url::PartType t2, PathOpFn pathOpFn) {
    // See URL serializing
    // https://url.spec.whatwg.org/#concept-url-serializer
    url::PartType ifirst;
    if (t1 <= url::HOST) {
        // authority, host
        if (!src.is_null(url::HOST)) {
            if (t1 == url::USERNAME && src.has_credentials())
                ifirst = url::USERNAME;
            else
                ifirst = url::HOST;
        } else {
            ifirst = url::PATH;
        }
    } else {
        // t1 == PATH
        ifirst = t1;
    }

    // copy parts & str
    if (ifirst <= t2) {
        int ilast = t2;
        for (; ilast >= ifirst; ilast--) {
            if (src.part_end_[ilast])
                break;
        }
        if (ifirst <= ilast) {
            // prepare buffer to append data
            // IMPORTANT: do before any url_ members modifications!
            std::string& norm_url = start_part(ifirst);
            
            // last part and url_.path_segment_count_
            std::size_t lastp_end = src.part_end_[ilast];
            if (pathOpFn && ilast == url::PATH) {
                unsigned segment_count = src.path_segment_count_;
                // https://isocpp.org/wiki/faq/pointers-to-members
                // todo: use std::invoke (c++17)
                (src.*pathOpFn)(lastp_end, segment_count);
                url_.path_segment_count_ = segment_count;
            } else if (ifirst <= url::PATH && url::PATH <= ilast) {
                url_.path_segment_count_ = src.path_segment_count_;
            }
            // src
            const std::size_t offset = src.part_end_[ifirst - 1] + detail::kPartStart[ifirst];
            const char* first = src.norm_url_.data() + offset;
            const char* last = src.norm_url_.data() + lastp_end;
            // dest                     
            int delta = static_cast<int>(norm_url.length()) - offset;
            // copy normalized url string from src
            norm_url.append(first, last);
            // adjust url_.part_end_
            for (int ind = ifirst; ind < ilast; ind++) {
                // if (src.part_end_[ind]) // nebereikia tikrinti, nes užpildyta
                url_.part_end_[ind] = src.part_end_[ind] + delta;
            }
            // ilast part from lastp
            url_.part_end_[ilast] = lastp_end + delta;
            last_pt_ = static_cast<url::PartType>(ilast);
        }
    }

    // copy not null flags
    unsigned mask = 0;
    for (int ind = t1; ind <= t2; ind++) {
        mask |= (1u << ind);
    }
    url_.flags_ = (url_.flags_ & ~mask) | (src.flags_ & mask);
}


} // namespace whatwg

#endif // WHATWG_URL_H
