// Copyright 2016-2023 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//
// This file contains portions of modified code from:
// https://cs.chromium.org/chromium/src/url/url_canon_etc.cc
// Copyright 2013 The Chromium Authors. All rights reserved.
//

// URL Standard
// https://url.spec.whatwg.org/
//
// Infra Standard - fundamental concepts upon which standards are built
// https://infra.spec.whatwg.org/
//

#ifndef WHATWG_URL_H
#define WHATWG_URL_H

#include "config.h"
#include "buffer.h"
#include "int_cast.h"
#include "str_arg.h"
#include "url_host.h"
#include "url_idna.h"
#include "url_percent_encode.h"
#include "url_result.h"
#include "url_search_params.h"
#include "url_utf.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <string>
#include <utility>
#include <vector>
#if defined(WHATWG__CPP_17) && defined(__has_include)
# if __has_include(<filesystem>)
#  include <filesystem>
#  define WHATWG_URL_USE_FILESYSTEM
# endif
#endif

// not yet
#define WHATWG_URL_USE_ENCODING 0

namespace whatwg {

// forward declarations

namespace detail {
    class url_serializer;
    class url_setter;
    class url_parser;
}

/// @brief URL class
///
/// Follows specification in
/// https://url.spec.whatwg.org/#url-class
///
class url {
public:
    // types
    using str_view_type = url_str_view_t;

    /// Enumeration to identify URL's parts (URL record members) defined here:
    /// https://url.spec.whatwg.org/#url-representation
    enum PartType {
        SCHEME = 0,
        SCHEME_SEP,
        USERNAME,
        PASSWORD,
        HOST_START,
        HOST,
        PORT,
        PATH_PREFIX,    // "/."
        PATH,
        QUERY,
        FRAGMENT,
        PART_COUNT
    };

    /// @brief Default constructor.
    ///
    /// Constructs empty URL.
    url() = default;

    /// @brief Copy constructor.
    ///
    /// @param[in] other  URL to copy from
    url(const url& other) = default;

    /// @brief Move constructor.
    ///
    /// Constructs the URL with the contents of @a other using move semantics.
    ///
    /// @param[in,out] other  URL to move to this object
    url(url&& other) noexcept;

    /// @brief Copy assignment.
    ///
    /// @param[in] other  URL to copy from
    url& operator=(const url& other) = default;

    /// @brief Move assignment.
    ///
    /// Replaces the contents with those of @a other using move semantics.
    ///
    /// @param[in,out] other  URL to move to this object
    url& operator=(url&& other) WHATWG__NOEXCEPT_17;

    /// @brief Parsing constructor.
    ///
    /// Throws url_error exception on parse error.
    ///
    /// @param[in] str_url URL string to parse
    /// @param[in] pbase   pointer to base URL, may be `nullptr`
    template <class T, enable_if_str_arg_t<T> = 0>
    explicit url(T&& str_url, const url* pbase = nullptr);

    /// @brief Parsing constructor.
    ///
    /// Throws url_error exception on parse error.
    ///
    /// @param[in] str_url URL string to parse
    /// @param[in] base    base URL
    template <class T, enable_if_str_arg_t<T> = 0>
    explicit url(T&& str_url, const url& base);

    /// @brief Parsing constructor.
    ///
    /// Throws url_error exception on parse error.
    ///
    /// @param[in] str_url  URL string to parse
    /// @param[in] str_base base URL string
    template <class T, class TB, enable_if_str_arg_t<T> = 0, enable_if_str_arg_t<TB> = 0>
    explicit url(T&& str_url, TB&& str_base);

    /// @brief clear URL
    /// Make URL empty.
    void clear();

    // Parser

    /// @brief Parses given URL string against base URL.
    ///
    /// @param[in] str_url URL string to parse
    /// @param[in] base    pointer to base URL, may be nullptr
    /// @return error code (@a url_result::Ok on success)
    template <class T, enable_if_str_arg_t<T> = 0>
    url_result parse(T&& str_url, const url* base) {
        const auto inp = make_str_arg(std::forward<T>(str_url));
        return do_parse(inp.begin(), inp.end(), base);
    }

    // Setters

    /// @brief The href setter
    ///
    /// Parses given URL string, and in the case of success assigns parsed URL value.
    /// On parse failure leaves URL value unchanged.
    ///
    /// @param[in] str URL string to parse
    /// @return `true` - on success; `false` - on failure
    template <class StrT, enable_if_str_arg_t<StrT> = 0>
    bool href(StrT&& str);

    /// @brief The protocol setter
    ///
    /// Parses given string and on succes sets the URL's protocol.
    /// More info: https://url.spec.whatwg.org/#dom-url-protocol
    ///
    /// @param[in] str string to parse
    /// @return `true` - on success; `false` - on failure (URL protocol unchanged)
    template <class StrT, enable_if_str_arg_t<StrT> = 0>
    bool protocol(StrT&& str);

    /// @brief The username setter
    ///
    /// Parses given string and on succes sets the URL's username.
    /// More info: https://url.spec.whatwg.org/#dom-url-username
    ///
    /// @param[in] str string to parse
    /// @return `true` - on success; `false` - if username can not be set
    template <class StrT, enable_if_str_arg_t<StrT> = 0>
    bool username(StrT&& str);

    /// @brief The password setter
    ///
    /// Parses given string and on succes sets the URL's password.
    /// More info: https://url.spec.whatwg.org/#dom-url-password
    ///
    /// @param[in] str string to parse
    /// @return `true` - on success; `false` - if password can not be set
    template <class StrT, enable_if_str_arg_t<StrT> = 0>
    bool password(StrT&& str);

    /// @brief The host setter
    ///
    /// Parses given string and on succes sets the URL's host and port.
    /// More info: https://url.spec.whatwg.org/#dom-url-host
    ///
    /// @param[in] str string to parse
    /// @return `true` - on success; `false` - on failure (URL's host and port unchanged)
    template <class StrT, enable_if_str_arg_t<StrT> = 0>
    bool host(StrT&& str);

    /// @brief The hostname setter
    ///
    /// Parses given string and on succes sets the URL's host.
    /// More info: https://url.spec.whatwg.org/#dom-url-hostname
    ///
    /// @param[in] str string to parse
    /// @return `true` - on success; `false` - on failure (URL's host unchanged)
    template <class StrT, enable_if_str_arg_t<StrT> = 0>
    bool hostname(StrT&& str);

    /// @brief The port setter
    ///
    /// Parses given string and on succes sets the URL's port.
    /// More info: https://url.spec.whatwg.org/#dom-url-port
    ///
    /// @param[in] str string to parse
    /// @return `true` - on success; `false` - on failure (URL's port unchanged)
    template <class StrT, enable_if_str_arg_t<StrT> = 0>
    bool port(StrT&& str);

    /// @brief The pathname setter
    ///
    /// Parses given string and on succes sets the URL's path.
    /// More info: https://url.spec.whatwg.org/#dom-url-pathname
    ///
    /// @param[in] str string to parse
    /// @return `true` - on success; `false` - on failure (URL's path unchanged)
    template <class StrT, enable_if_str_arg_t<StrT> = 0>
    bool pathname(StrT&& str);

    /// @brief The search setter
    ///
    /// Parses given string and on succes sets the URL's query.
    /// More info: https://url.spec.whatwg.org/#dom-url-search
    ///
    /// @param[in] str string to parse
    /// @return `true` - on success; `false` - on failure (URL's query unchanged)
    template <class StrT, enable_if_str_arg_t<StrT> = 0>
    bool search(StrT&& str);

    /// @brief The hash setter
    ///
    /// Parses given string and on succes sets the URL's fragment.
    /// More info: https://url.spec.whatwg.org/#dom-url-hash
    ///
    /// @param[in] str string to parse
    /// @return `true` - on success; `false` - on failure (URL's fragment unchanged)
    template <class StrT, enable_if_str_arg_t<StrT> = 0>
    bool hash(StrT&& str);

    // Getters

    /// @brief The href getter
    ///
    /// More info: https://url.spec.whatwg.org/#dom-url-href
    ///
    /// @return serialized URL
    str_view_type href() const;

    /// @brief The origin getter
    ///
    /// More info: https://url.spec.whatwg.org/#dom-url-origin
    ///
    /// @return ASCII serialized URL's origin
    std::string origin() const;

    /// @brief The protocol getter
    ///
    /// More info: https://url.spec.whatwg.org/#dom-url-protocol
    ///
    /// @return URL's scheme, followed by U+003A (:)
    str_view_type protocol() const;

    /// @brief The username getter
    ///
    /// More info: https://url.spec.whatwg.org/#dom-url-username
    ///
    /// @return URL’s username
    str_view_type username() const;

    /// @brief The password getter
    ///
    /// More info: https://url.spec.whatwg.org/#dom-url-password
    ///
    /// @return URL’s password
    str_view_type password() const;

    /// @brief The host getter
    ///
    /// More info: https://url.spec.whatwg.org/#dom-url-host
    ///
    /// @return URL’s host, serialized, followed by U+003A (:) and URL’s port, serialized
    str_view_type host() const;

    /// @brief The hostname getter
    ///
    /// More info: https://url.spec.whatwg.org/#dom-url-hostname
    ///
    /// @return URL’s host, serialized
    str_view_type hostname() const;

    /// @brief The host_type getter
    ///
    /// @return URL’s host type as HostType enumeration value
    HostType host_type() const noexcept;

    /// @brief The port getter
    ///
    /// More info: https://url.spec.whatwg.org/#dom-url-port
    ///
    /// @return URL’s port, serialized, if URL’s port is not null, otherwise empty string
    str_view_type port() const;

    /// @return URL’s port, converted to `int` value, if URL’s port is not null,
    ///   otherwise `-1`
    int port_int() const;

    /// @return URL’s port, converted to `int` value, if URL’s port is not null,
    ///   otherwise default port, if URL's scheme has default port,
    ///   otherwise `-1`
    int real_port_int() const;

    /// @brief The path getter
    ///
    /// @return URL's path, serialized, followed by U+003F (?) and URL’s query
    str_view_type path() const;

    /// @brief The pathname getter
    ///
    /// More info: https://url.spec.whatwg.org/#dom-url-pathname
    ///
    /// @return URL’s path, serialized
    str_view_type pathname() const;

    /// @brief The search getter
    ///
    /// More info: https://url.spec.whatwg.org/#dom-url-search
    ///
    /// @return empty string or U+003F (?), followed by URL’s query
    str_view_type search() const;

    /// @brief The hash getter
    ///
    /// More info: https://url.spec.whatwg.org/#dom-url-hash
    ///
    /// @return empty string or U+0023 (#), followed by URL’s fragment
    str_view_type hash() const;

    /// @brief The searchParams getter
    ///
    /// More info: https://url.spec.whatwg.org/#dom-url-searchparams
    ///
    /// @return reference to this’s query object (url_search_params class)
    url_search_params& search_params();

    /// @brief URL serializer
    ///
    /// Returns serialized URL in a string_view as defined here:
    /// https://url.spec.whatwg.org/#concept-url-serializer
    ///
    /// @param[in] exclude_fragment exclude fragment when serializing
    /// @return serialized URL as string_view
    str_view_type serialize(bool exclude_fragment = false) const;

    // Get url info

    /// @brief Checks whether the URL is empty
    ///
    /// @return `true` if URL is empty, `false` otherwise
    bool empty() const noexcept;

    /// Function to get ASCII string of any URL's part (URL record member) defined here:
    /// https://url.spec.whatwg.org/#url-representation
    /// 
    /// @param[in] t URL's part
    /// @return URL's part string; it is empty if part is empty or null
    str_view_type get_part_view(PartType t) const;

    /// @param[in] t URL's part
    /// @return `true` if URL's part @a t is empty or null, `false` otherwise
    bool is_empty(const PartType t) const;

    /// @param[in] t URL's part
    /// @return `true` if URL's part @a t is null, `false` otherwise
    bool is_null(const PartType t) const noexcept;

    /// @return `true` if URL's scheme is special ("ftp", "file", "http", "https", "ws", or "wss"),
    ///   `false` otherwise; see: https://url.spec.whatwg.org/#special-scheme
    bool is_special_scheme() const noexcept;

    /// @return `true` if URL's scheme is "file", `false` otherwise
    bool is_file_scheme() const noexcept;

    /// @return `true` if URL includes credentials (username, password), `false` otherwise
    bool has_credentials() const;

    /// @return serialized URL as `std::string`
    std::string to_string() const;

private:
    struct scheme_info {
        str_view_type scheme;
        int default_port;           // -1 if none
        unsigned is_special : 1;    // "ftp", "file", "http", "https", "ws", "wss"
        unsigned is_file : 1;       // "file"
        unsigned is_ws : 1;         // "ws", "wss"
    };

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
        OPAQUE_PATH_FLAG = (1u << (PART_COUNT + 0)),
        // host type
        HOST_TYPE_SHIFT = (PART_COUNT + 1),
        HOST_TYPE_MASK = (7u << HOST_TYPE_SHIFT),

        // initial flags (empty (but not null) parts)
        // https://url.spec.whatwg.org/#url-representation
        INITIAL_FLAGS = SCHEME_FLAG | USERNAME_FLAG | PASSWORD_FLAG | PATH_FLAG,
    };

    static const unsigned kPartFlagMask[url::PART_COUNT];

    // parsing constructor
    template <class T, enable_if_str_arg_t<T> = 0>
    explicit url(T&& str_url, const url* base, const char* what_arg);

    // parser
    template <typename CharT>
    url_result do_parse(const CharT* first, const CharT* last, const url* base);

    // get scheme info
    static const scheme_info kSchemes[];
    static const scheme_info* get_scheme_info(str_view_type src);

    // set scheme
    template <class StringT>
    void set_scheme_str(const StringT str);
    void set_scheme(const url& src);
    void set_scheme(const str_view_type str);
    void set_scheme(std::size_t scheme_length);

    // path util
    str_view_type get_path_first_string(std::size_t len) const;
    // path shortening
    bool get_path_rem_last(std::size_t& path_end, std::size_t& path_segment_count) const;
    bool get_shorten_path(std::size_t& path_end, std::size_t& path_segment_count) const;

    // flags
    void set_flag(const UrlFlag flag) noexcept;

    bool has_an_opaque_path() const noexcept;
    void set_has_an_opaque_path() noexcept;

    void set_host_type(const HostType ht) noexcept;

    // info
    bool canHaveUsernamePasswordPort() const;

    // search params
    void clear_search_params() noexcept;
    void parse_search_params();

private:
    std::string norm_url_;
    std::array<std::size_t, PART_COUNT> part_end_ = {};
    const scheme_info* scheme_inf_ = nullptr;
    unsigned flags_ = INITIAL_FLAGS;
    std::size_t path_segment_count_ = 0;
    detail::url_search_params_ptr search_params_ptr_;

    friend detail::url_serializer;
    friend detail::url_setter;
    friend detail::url_parser;
    friend class url_search_params;
};

/// @brief URL equivalence
///
/// Determines if @a lhs equals to @a rhs, optionally with an @a exclude_fragments flag.
/// More info: https://url.spec.whatwg.org/#concept-url-equals
///
/// @param[in] lhs,rhs URLs to compare
/// @param[in] exclude_fragments exclude fragments when comparing
inline bool equals(const url& lhs, const url& rhs, bool exclude_fragments = false) {
    return lhs.serialize(exclude_fragments) == rhs.serialize(exclude_fragments);
}


namespace detail {

class url_serializer : public host_output {
public:
    // types
    using str_view_type = url::str_view_type;

    url_serializer(url& dest_url)
        : url_(dest_url)
        , last_pt_(url::SCHEME)
    {}

    ~url_serializer();

    void new_url() {
        if (!url_.empty())
            url_.clear();
    }
    virtual void reserve(std::size_t new_cap) { url_.norm_url_.reserve(new_cap); }

    // set data
    void set_scheme(const url& src) { url_.set_scheme(src); }
    void set_scheme(const str_view_type str) { url_.set_scheme(str); }
    void set_scheme(std::size_t scheme_length) { url_.set_scheme(scheme_length); }

    // set scheme
    virtual std::string& start_scheme();
    virtual void save_scheme();

    // set url's part
    void fill_parts_offset(url::PartType t1, url::PartType t2, std::size_t offset);
    virtual std::string& start_part(url::PartType new_pt);
    virtual void save_part();

    virtual void clear_part(const url::PartType /*pt*/) {}

    // set empty host
    void set_empty_host();

    // empties not empty host
    virtual void empty_host();

    // host_output overrides
    std::string& hostStart() override;
    void hostDone(HostType ht) override;

    // Path operations

    // append the empty string to url’s path (list)
    void append_empty_path_segment();
    // append string to url's path (list)
    virtual std::string& start_path_segment();
    virtual void save_path_segment();
    virtual void commit_path();
    // if '/' not required:
    std::string& start_path_string();
    void save_path_string();

    virtual void shorten_path();
    //UNUSED// retunrs how many slashes are removed
    //virtual std::size_t remove_leading_path_slashes();

    typedef bool (url::*PathOpFn)(std::size_t& path_end, std::size_t& segment_count) const;
    void append_parts(const url& src, url::PartType t1, url::PartType t2, PathOpFn pathOpFn = nullptr);

    // flags
    void set_flag(const url::UrlFlag flag) { url_.set_flag(flag); }
    void set_host_type(const HostType ht) { url_.set_host_type(ht); }
    // IMPORTANT: has-an-opaque-path flag must be set before or just after
    // SCHEME set; because other part's serialization depends on this flag
    void set_has_an_opaque_path() {
        assert(last_pt_ == url::SCHEME);
        url_.set_has_an_opaque_path();
    }

    // get info
    str_view_type get_part_view(url::PartType t) const { return url_.get_part_view(t); }
    bool is_empty(const url::PartType t) const { return url_.is_empty(t); }
    virtual bool is_empty_path() const {
        assert(!url_.has_an_opaque_path());
        // path_segment_count_ has meaning only if path is a list (path isn't opaque)
        return url_.path_segment_count_ == 0;
    }
    bool is_null(const url::PartType t) const  { return url_.is_null(t); }
    bool is_special_scheme() const { return url_.is_special_scheme(); }
    bool is_file_scheme() const { return url_.is_file_scheme(); }
    bool has_credentials() const { return url_.has_credentials(); }
    const url::scheme_info* scheme_inf() const { return url_.scheme_inf_; }
    int port_int() const { return url_.port_int(); }

protected:
    void adjust_path_prefix();

    std::size_t get_part_pos(const url::PartType pt) const;
    std::size_t get_part_len(const url::PartType pt) const;
    void replace_part(const url::PartType new_pt, const char* str, const std::size_t len);
    void replace_part(const url::PartType last_pt, const char* str, const std::size_t len,
        const url::PartType first_pt, const std::size_t len0);

protected:
    url& url_;
    // last serialized URL's part
    url::PartType last_pt_;
};


class url_setter : public url_serializer {
public:
    url_setter(url& dest_url)
        : url_serializer(dest_url)
        , use_strp_(true)
        , curr_pt_(url::SCHEME)
    {}

    ~url_setter();

    //???
    void reserve(std::size_t new_cap) override;

    // set scheme
    std::string& start_scheme() override;
    void save_scheme() override;

    // set/clear/empty url's part
    std::string& start_part(url::PartType new_pt) override;
    void save_part() override;

    void clear_part(const url::PartType pt) override;
    void empty_part(const url::PartType pt); // override

    void empty_host() override;

    // path
    std::string& start_path_segment() override;
    void save_path_segment() override;
    void commit_path() override;

    void shorten_path() override;
    //UNUSED// retunrs how many slashes are removed
    //std::size_t remove_leading_path_slashes() override;
    bool is_empty_path() const override;

    void potentially_strip_trailing_spaces_from_an_opaque_path();

protected:
    url::PartType find_last_part(url::PartType pt) const;

#if 0
    void insert_part(url::PartType new_pt, const char* str, std::size_t len);
#endif

protected:
    bool use_strp_;
    // buffer for URL's part
    std::string strp_;
    // path segment end positions in the strp_
    std::vector<std::size_t> path_seg_end_;
    // current URL's part
    url::PartType curr_pt_;
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
        opaque_path_state,
        query_state,
        fragment_state
    };

    template <typename CharT>
    static url_result url_parse(url_serializer& urls, const CharT* first, const CharT* last, const url* base, State state_override = not_set_state);

    template <typename CharT>
    static url_result parse_host(url_serializer& urls, const CharT* first, const CharT* last);

    template <typename CharT>
    static void parse_path(url_serializer& urls, const CharT* first, const CharT* last);

private:
    template <typename CharT>
    static bool do_path_segment(const CharT* pointer, const CharT* last, std::string& output);

    template <typename CharT>
    static bool do_simple_path(const CharT* pointer, const CharT* last, std::string& output);
};


// url_error what() values
extern const char kURLParseError[];
extern const char kBaseURLParseError[];

// canonical version of each possible input letter in the scheme
extern const char kSchemeCanonical[0x80];

// part start
extern const uint8_t kPartStart[url::PART_COUNT];

inline int port_from_str(const char* first, const char* last) {
    int port = 0;
    for (auto it = first; it != last; ++it) {
        port = port * 10 + (*it - '0');
    }
    return port;
}

// Removable URL chars

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

template <typename CharT>
inline void do_trim(const CharT*& first, const CharT*& last) {
    // remove leading C0 controls and space
    while (first < last && is_trim_char(*first))
        ++first;
    // remove trailing C0 controls and space
    while (first < last && is_trim_char(*(last-1)))
        --last;
}

// DoRemoveURLWhitespace
// https://cs.chromium.org/chromium/src/url/url_canon_etc.cc
template <typename CharT>
inline void do_remove_whitespace(const CharT*& first, const CharT*& last, simple_buffer<CharT>& buff) {
    // Fast verification that there's nothing that needs removal. This is the 99%
    // case, so we want it to be fast and don't care about impacting the speed
    // when we do find whitespace.
    for (auto it = first; it < last; ++it) {
        if (!is_removable_char(*it))
            continue;
        // copy non whitespace chars into the new buffer and return it
        buff.reserve(last - first);
        buff.append(first, it);
        for (; it < last; ++it) {
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
inline InputIt find_last(InputIt first, InputIt last, const T& value) {
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

// Windows drive letter

// https://url.spec.whatwg.org/#windows-drive-letter

template <typename CharT>
inline bool is_Windows_drive(CharT c1, CharT c2) {
    return is_ascii_alpha(c1) && (c2 == ':' || c2 == '|');
}

template <typename CharT>
inline bool is_normalized_Windows_drive(CharT c1, CharT c2) {
    return is_ascii_alpha(c1) && c2 == ':';
}

// https://url.spec.whatwg.org/#start-with-a-windows-drive-letter
template <typename CharT>
inline bool starts_with_Windows_drive(const CharT* pointer, const CharT* last) {
    const auto length = last - pointer;
#if 1
    return
        (length == 2 || (length > 2 && detail::is_special_authority_end_char(pointer[2]))) &&
        detail::is_Windows_drive(pointer[0], pointer[1]);
#else
    return
        length >= 2 &&
        detail::is_Windows_drive(pointer[0], pointer[1]) &&
        (length == 2 || detail::is_special_authority_end_char(pointer[2]));
#endif
}

// check string starts with absolute Windows drive path (for example: "C:\\path" or "C:/path")
template <typename CharT>
inline bool starts_with_Windows_drive_absolute_path(const CharT* pointer, const CharT* last) {
    return
        last - pointer > 2 &&
        detail::is_Windows_drive(pointer[0], pointer[1]) &&
        (pointer[2] == '\\' || pointer[2] == '/');
}

// check string starts with sv (ASCII)
template <typename CharT>
inline bool starts_with(const CharT* first, const CharT* last, url::str_view_type sv) {
    if (last - first >= static_cast<std::ptrdiff_t>(sv.length())) {
        for (auto c : sv) {
            if (*first != c)
                return false;
            ++first;
        }
        return true;
    }
    return false;
}

} // namespace detail


// url class

inline url::url(url&& other) noexcept
    : norm_url_(std::move(other.norm_url_))
    , part_end_(std::move(other.part_end_))
    , scheme_inf_(other.scheme_inf_)
    , flags_(other.flags_)
    , path_segment_count_(other.path_segment_count_)
    , search_params_ptr_(std::move(other.search_params_ptr_))
{
    search_params_ptr_.set_url_ptr(this);
}

inline url& url::operator=(url&& other) WHATWG__NOEXCEPT_17 {
    // move data
    norm_url_ = std::move(other.norm_url_);
    part_end_ = std::move(other.part_end_);
    scheme_inf_ = other.scheme_inf_;
    flags_ = other.flags_;
    path_segment_count_ = other.path_segment_count_;
    search_params_ptr_ = std::move(other.search_params_ptr_);

    // setup search params
    search_params_ptr_.set_url_ptr(this);

    return *this;
}

// url getters

inline url::str_view_type url::href() const {
    return norm_url_;
}

inline std::string url::to_string() const {
    return norm_url_;
}

// Origin
// https://url.spec.whatwg.org/#concept-url-origin

// ASCII serialization of an origin
// https://html.spec.whatwg.org/multipage/browsers.html#ascii-serialisation-of-an-origin
inline std::string url::origin() const {
    if (is_special_scheme()) {
        if (is_file_scheme())
            return "null"; // opaque origin
        // "scheme://"
        std::string str_origin(norm_url_, 0, part_end_[SCHEME_SEP]);
        // "host:port"
        str_origin.append(norm_url_.data() + part_end_[HOST_START], norm_url_.data() + part_end_[PORT]);
        return str_origin;
    } else if (get_part_view(SCHEME) == url::str_view_type{ "blob", 4 }) {
        // Warning: this library does not support blob URL store, so it allways assumes
        // URL's blob URL entry is null and retrieves origin from the URL's path.
        url u;
        if (u.parse(get_part_view(PATH), nullptr) == url_result::Ok)
            return u.origin();
    }
    return "null"; // opaque origin
}

inline url::str_view_type url::protocol() const {
    // "scheme:"
    return str_view_type(norm_url_.data(), part_end_[SCHEME] ? part_end_[SCHEME] + 1 : 0);
}

inline url::str_view_type url::username() const {
    return get_part_view(USERNAME);
}

inline url::str_view_type url::password() const {
    return get_part_view(PASSWORD);
}

inline url::str_view_type url::host() const {
    if (is_null(HOST))
        return str_view_type();
    // "hostname:port"
    const std::size_t b = part_end_[HOST_START];
    const std::size_t e = is_null(PORT) ? part_end_[HOST] : part_end_[PORT];
    return str_view_type(norm_url_.data() + b, e - b);
}

inline url::str_view_type url::hostname() const {
    return get_part_view(HOST);
}

inline HostType url::host_type() const noexcept {
    return static_cast<HostType>((flags_ & HOST_TYPE_MASK) >> HOST_TYPE_SHIFT);
}

inline url::str_view_type url::port() const {
    return get_part_view(PORT);
}

inline int url::port_int() const {
    const auto vport = get_part_view(PORT);
    return vport.length() ? detail::port_from_str(vport.data(), vport.data() + vport.length()) : -1;
}

inline int url::real_port_int() const {
    const auto vport = get_part_view(PORT);
    if (vport.length()) {
        return detail::port_from_str(vport.data(), vport.data() + vport.length());
    } else {
        return scheme_inf_ ? scheme_inf_->default_port : -1;
    }
}

// pathname + search
inline url::str_view_type url::path() const {
    // "pathname?query"
    const std::size_t b = part_end_[PATH - 1];
    const std::size_t e = part_end_[QUERY] ? part_end_[QUERY] : part_end_[PATH];
    return str_view_type(norm_url_.data() + b, e ? e - b : 0);
}

inline url::str_view_type url::pathname() const {
    // https://url.spec.whatwg.org/#dom-url-pathname
    // already serialized as needed
    return get_part_view(PATH);
}

inline url::str_view_type url::search() const {
    if (is_empty(QUERY))
        return str_view_type();
    // return with '?'
    const std::size_t b = part_end_[QUERY - 1];
    const std::size_t e = part_end_[QUERY];
    return str_view_type(norm_url_.data() + b, e - b);
}

inline url::str_view_type url::hash() const {
    if (is_empty(FRAGMENT))
        return str_view_type();
    // return with '#'
    const std::size_t b = part_end_[FRAGMENT - 1];
    const std::size_t e = part_end_[FRAGMENT];
    return str_view_type(norm_url_.data() + b, e - b);
}

inline url_search_params& url::search_params() {
    if (!search_params_ptr_)
        search_params_ptr_.init(this);
    return *search_params_ptr_;
}

inline void url::clear_search_params() noexcept {
    if (search_params_ptr_)
        search_params_ptr_.clear_params();
}

inline void url::parse_search_params() {
    if (search_params_ptr_)
        search_params_ptr_.parse_params(get_part_view(QUERY));
}

inline url::str_view_type url::serialize(bool exclude_fragment) const {
    if (exclude_fragment && part_end_[FRAGMENT])
        return str_view_type(norm_url_.data(), part_end_[QUERY]);
    else
        return norm_url_;
}

// Get url info

inline bool url::empty() const noexcept {
    return norm_url_.empty();
}

inline url::str_view_type url::get_part_view(PartType t) const {
    if (t == SCHEME)
        return str_view_type(norm_url_.data(), part_end_[SCHEME]);
    // begin & end offsets
    const std::size_t b = part_end_[t - 1] + detail::kPartStart[t];
    const std::size_t e = part_end_[t];
    return str_view_type(norm_url_.data() + b, e > b ? e - b : 0);
}

inline bool url::is_empty(const PartType t) const {
    if (t == SCHEME)
        return part_end_[SCHEME] == 0;
    // begin & end offsets
    const std::size_t b = part_end_[t - 1] + detail::kPartStart[t];
    const std::size_t e = part_end_[t];
    return b >= e;
}

inline bool url::is_null(const PartType t) const noexcept {
    return !(flags_ & (1u << t));
}

inline bool url::is_special_scheme() const noexcept {
    return scheme_inf_ && scheme_inf_->is_special;
}

inline bool url::is_file_scheme() const noexcept {
    return scheme_inf_ && scheme_inf_->is_file;
}

inline bool url::has_credentials() const {
    return !is_empty(USERNAME) || !is_empty(PASSWORD);
}

// set scheme

template <class StringT>
inline void url::set_scheme_str(const StringT str) {
    norm_url_.clear(); // clear all
    part_end_[SCHEME] = str.length();
    norm_url_.append(str.data(), str.length());
    norm_url_ += ':';
}

inline void url::set_scheme(const url& src) {
    set_scheme_str(src.get_part_view(SCHEME));
    scheme_inf_ = src.scheme_inf_;
}

inline void url::set_scheme(const str_view_type str) {
    set_scheme_str(str);
    scheme_inf_ = get_scheme_info(str);
}

inline void url::set_scheme(std::size_t scheme_length) {
    part_end_[SCHEME] = scheme_length;
    scheme_inf_ = get_scheme_info(get_part_view(SCHEME));
}

// flags

inline void url::set_flag(const UrlFlag flag) noexcept {
    flags_ |= flag;
}

inline bool url::has_an_opaque_path() const noexcept {
    return !!(flags_ & OPAQUE_PATH_FLAG);
}

inline void url::set_has_an_opaque_path() noexcept {
    set_flag(OPAQUE_PATH_FLAG);
}

inline void url::set_host_type(const HostType ht) noexcept {
    flags_ = (flags_ & ~HOST_TYPE_MASK) | HOST_FLAG | (static_cast<unsigned int>(ht) << HOST_TYPE_SHIFT);
}

inline bool url::canHaveUsernamePasswordPort() const {
    return !(is_empty(url::HOST) || is_file_scheme());
}

// url parsing

template <class T, enable_if_str_arg_t<T>>
inline url::url(T&& str_url, const url* pbase)
    : url(std::forward<T>(str_url), pbase, detail::kURLParseError)
{}

template <class T, enable_if_str_arg_t<T>>
inline url::url(T&& str_url, const url& base)
    : url(std::forward<T>(str_url), &base, detail::kURLParseError)
{}

template <class T, class TB, enable_if_str_arg_t<T>, enable_if_str_arg_t<TB>>
inline url::url(T&& str_url, TB&& str_base)
    : url(std::forward<T>(str_url), url(std::forward<TB>(str_base), nullptr, detail::kBaseURLParseError))
{}

template <class T, enable_if_str_arg_t<T>>
inline url::url(T&& str_url, const url* base, const char* what_arg) {
    const auto inp = make_str_arg(std::forward<T>(str_url));
    const auto res = do_parse(inp.begin(), inp.end(), base);
    if (res != url_result::Ok)
        throw url_error(res, what_arg);
}

inline void url::clear() {
    norm_url_.clear();
    part_end_.fill(0);
    scheme_inf_ = nullptr;
    flags_ = INITIAL_FLAGS;
    path_segment_count_ = 0;
    clear_search_params();
}

template <typename CharT>
inline url_result url::do_parse(const CharT* first, const CharT* last, const url* base) {
    url_result res;
    {
        detail::url_serializer urls(*this);

        // reset URL
        urls.new_url();

        // remove any leading and trailing C0 control or space:
        detail::do_trim(first, last);
        //TODO-WARN: validation error if trimmed

        res = detail::url_parser::url_parse(urls, first, last, base);
    }
    parse_search_params();
    return res;
}

template <class StrT, enable_if_str_arg_t<StrT>>
inline bool url::href(StrT&& str) {
    url u; // parsedURL

    const auto inp = make_str_arg(std::forward<StrT>(str));
    if (u.do_parse(inp.begin(), inp.end(), nullptr) == url_result::Ok) {
        *this = std::move(u);
        return true;
    }
    return false;
}

template <class StrT, enable_if_str_arg_t<StrT>>
inline bool url::protocol(StrT&& str) {
    if (!empty()) {
        detail::url_setter urls(*this);

        const auto inp = make_str_arg(std::forward<StrT>(str));
        return detail::url_parser::url_parse(urls, inp.begin(), inp.end(), nullptr, detail::url_parser::scheme_start_state) == url_result::Ok;
    }
    return false;
}

template <class StrT, enable_if_str_arg_t<StrT>>
inline bool url::username(StrT&& str) {
    if (canHaveUsernamePasswordPort()) {
        detail::url_setter urls(*this);

        const auto inp = make_str_arg(std::forward<StrT>(str));

        std::string& str_username = urls.start_part(url::USERNAME);
        // UTF-8 percent encode it using the userinfo encode set
        detail::AppendStringOfType(inp.begin(), inp.end(), userinfo_no_encode_set, str_username);
        urls.save_part();
        return true;
    }
    return false;
}

template <class StrT, enable_if_str_arg_t<StrT>>
inline bool url::password(StrT&& str) {
    if (canHaveUsernamePasswordPort()) {
        detail::url_setter urls(*this);

        const auto inp = make_str_arg(std::forward<StrT>(str));

        std::string& str_password = urls.start_part(url::PASSWORD);
        // UTF-8 percent encode it using the userinfo encode set
        detail::AppendStringOfType(inp.begin(), inp.end(), userinfo_no_encode_set, str_password);
        urls.save_part();
        return true;
    }
    return false;
}

template <class StrT, enable_if_str_arg_t<StrT>>
inline bool url::host(StrT&& str) {
    if (!has_an_opaque_path() && !empty()) {
        detail::url_setter urls(*this);

        const auto inp = make_str_arg(std::forward<StrT>(str));
        return detail::url_parser::url_parse(urls, inp.begin(), inp.end(), nullptr, detail::url_parser::host_state) == url_result::Ok;
    }
    return false;
}

template <class StrT, enable_if_str_arg_t<StrT>>
inline bool url::hostname(StrT&& str) {
    if (!has_an_opaque_path() && !empty()) {
        detail::url_setter urls(*this);

        const auto inp = make_str_arg(std::forward<StrT>(str));
        return detail::url_parser::url_parse(urls, inp.begin(), inp.end(), nullptr, detail::url_parser::hostname_state) == url_result::Ok;
    }
    return false;
}

template <class StrT, enable_if_str_arg_t<StrT>>
inline bool url::port(StrT&& str) {
    if (canHaveUsernamePasswordPort()) {
        detail::url_setter urls(*this);

        const auto inp = make_str_arg(std::forward<StrT>(str));
        const auto* first = inp.begin();
        const auto* last = inp.end();

        if (first == last) {
            urls.clear_part(url::PORT);
            return true;
        } else {
            return detail::url_parser::url_parse(urls, first, last, nullptr, detail::url_parser::port_state) == url_result::Ok;
        }
    }
    return false;
}

template <class StrT, enable_if_str_arg_t<StrT>>
inline bool url::pathname(StrT&& str) {
    if (!has_an_opaque_path() && !empty()) {
        detail::url_setter urls(*this);

        const auto inp = make_str_arg(std::forward<StrT>(str));
        return detail::url_parser::url_parse(urls, inp.begin(), inp.end(), nullptr, detail::url_parser::path_start_state) == url_result::Ok;
    }
    return false;
}

template <class StrT, enable_if_str_arg_t<StrT>>
inline bool url::search(StrT&& str) {
    bool res = false;
    if (!empty()) {
        {
            detail::url_setter urls(*this);

            const auto inp = make_str_arg(std::forward<StrT>(str));
            const auto* first = inp.begin();
            const auto* last = inp.end();

            if (first == last) {
                urls.clear_part(url::QUERY);
                // empty context object's query object's list
                clear_search_params();
                urls.potentially_strip_trailing_spaces_from_an_opaque_path();
                return true;
            }
            if (*first == '?') ++first;
            res = detail::url_parser::url_parse(urls, first, last, nullptr, detail::url_parser::query_state) == url_result::Ok;
        }
        // set context object's query object's list to the result of parsing input
        parse_search_params();
    }
    return res;
}

template <class StrT, enable_if_str_arg_t<StrT>>
inline bool url::hash(StrT&& str) {
    if (!empty()) {
        detail::url_setter urls(*this);

        const auto inp = make_str_arg(std::forward<StrT>(str));
        const auto* first = inp.begin();
        const auto* last = inp.end();

        if (first == last) {
            urls.clear_part(url::FRAGMENT);
            urls.potentially_strip_trailing_spaces_from_an_opaque_path();
            return true;
        }
        if (*first == '#') ++first;
        return detail::url_parser::url_parse(urls, first, last, nullptr, detail::url_parser::fragment_state) == url_result::Ok;
    }
    return false;
}


namespace detail {

// https://url.spec.whatwg.org/#concept-basic-url-parser
template <typename CharT>
inline url_result url_parser::url_parse(url_serializer& urls, const CharT* first, const CharT* last, const url* base, State state_override)
{
    using UCharT = typename std::make_unsigned<CharT>::type;

    // remove all ASCII tab or newline from URL
    simple_buffer<CharT> buff_no_ws;
    detail::do_remove_whitespace(first, last, buff_no_ws);
    //TODO-WARN: validation error if removed

    // reserve size (TODO: But what if `base` is used?)
    const auto length = std::distance(first, last);
    urls.reserve(length + 32);

#if WHATWG_URL_USE_ENCODING
    const char* encoding = "UTF-8";
    // TODO: If encoding override is given, set encoding to the result of getting an output encoding from encoding override.
#endif

    auto pointer = first;
    State state = state_override ? state_override : scheme_start_state;

    // has scheme?
    if (state == scheme_start_state) {
        if (pointer != last && detail::is_first_scheme_char(*pointer)) {
            state = scheme_state; // this appends first char to buffer
        } else if (!state_override) {
            state = no_scheme_state;
        } else {
            //TODO-ERR: validation error
            return url_result::InvalidSchemeCharacter;
        }
    }

    if (state == scheme_state) {
        // Deviation from URL stdandart's [ 2. ... if c is ":", run ... ] to
        // [ 2. ... if c is ":", or EOF and state override is given, run ... ]
        // This lets protocol setter to pass input without adding ':' to the end.
        // Similiar deviation exists in nodejs, see:
        // https://github.com/nodejs/node/pull/11917#pullrequestreview-28061847

        // first scheme char has been checked in the scheme_start_state, so skip it
        const auto end_of_scheme = std::find_if_not(pointer + 1, last,
            [](CharT c) -> bool {
                const UCharT uch = static_cast<UCharT>(c);
                return uch < 0x80 && detail::kSchemeCanonical[uch];
            });
        const bool is_scheme = end_of_scheme != last
            ? *end_of_scheme == ':'
            : state_override != not_set_state;

        if (is_scheme) {
            // start of scheme
            std::string& str_scheme = urls.start_scheme();
            // append scheme chars
            for (auto it = pointer; it != end_of_scheme; ++it) {
                const UCharT uch = static_cast<UCharT>(*it);
                str_scheme.push_back(detail::kSchemeCanonical[uch]);
            }

            if (state_override) {
                const url::scheme_info* scheme_inf = url::get_scheme_info(str_scheme);
                const bool is_special_old = urls.is_special_scheme();
                const bool is_special_new = scheme_inf && scheme_inf->is_special;
                if (is_special_old != is_special_new)
                    return url_result::False;
                // new URL("http://u:p@host:88/).protocol("file:");
                if (scheme_inf && scheme_inf->is_file && (urls.has_credentials() || !urls.is_null(url::PORT)))
                    return url_result::False;
                // new URL("file:///path).protocol("http:");
                if (urls.is_file_scheme() && urls.is_empty(url::HOST))
                    return url_result::False;
                // OR ursl.is_empty(url::HOST) && scheme_inf->no_empty_host

                // set url's scheme
                urls.save_scheme();

                // https://github.com/whatwg/url/pull/328
                // optimization: compare ports if scheme has the default port
                if (scheme_inf && scheme_inf->default_port >= 0 &&
                    urls.port_int() == scheme_inf->default_port) {
                    // set url's port to null
                    urls.clear_part(url::PORT);
                }

                // if state override is given, then return
                return url_result::Ok;
            }
            urls.save_scheme();

            pointer = end_of_scheme + 1; // skip ':'
            if (urls.is_file_scheme()) {
                // TODO-WARN: if remaining does not start with "//", validation error.
                state = file_state;
            } else {
                if (urls.is_special_scheme()) {
                    if (base && urls.get_part_view(url::SCHEME) == base->get_part_view(url::SCHEME)) {
                        assert(base->is_special_scheme()); // and therefore does not have an opaque path
                        state = special_relative_or_authority_state;
                    } else {
                        state = special_authority_slashes_state;
                    }
                } else if (pointer < last && *pointer == '/') {
                    state = path_or_authority_state;
                    ++pointer;
                } else {
                    // set url’s path to the empty string (so path becomes opaque,
                    // see: https://url.spec.whatwg.org/#url-opaque-path)
                    urls.set_has_an_opaque_path();
                    // To complete the set url's path to the empty string, following functions must be called:
                    //  urls.start_path_string();
                    //  urls.save_path_string();
                    // but the same functions will be called in the opaque_path_state, so skip them here.
                    state = opaque_path_state;
                }
            }
        } else if (!state_override) {
            state = no_scheme_state;
        } else {
            //TODO-ERR: validation error
            return url_result::InvalidSchemeCharacter;
        }
    }

    if (state == no_scheme_state) {
        if (base) {
            if (base->has_an_opaque_path()) {
                if (pointer < last && *pointer == '#') {
                    urls.set_scheme(*base);
                    urls.append_parts(*base, url::PATH, url::QUERY);
                    //TODO: url's fragment to the empty string
                    state = fragment_state;
                    ++pointer;
                } else {
                    // TODO-ERR: 1. validation error
                    return url_result::RelativeUrlWithCannotBeABase;
                }
            } else {
                state = base->is_file_scheme() ? file_state : relative_state;
            }
        } else {
            //TODO-ERR: 1. validation error
            return url_result::RelativeUrlWithoutBase;
        }
    }

    if (state == special_relative_or_authority_state) {
        if (last - pointer > 1 && pointer[0] == '/' && pointer[1] == '/') {
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
            ++pointer; // skip "/"
        } else {
            state = path_state;
        }
    }

    if (state == relative_state) {
        // std::assert(base != nullptr);
        urls.set_scheme(*base);
        if (pointer == last) {
            // EOF code point
            // Set url's username to base's username, url's password to base's password, url's host to base's host,
            // url's port to base's port, url's path to base's path, and url's query to base's query
            urls.append_parts(*base, url::USERNAME, url::QUERY);
            return url_result::Ok; // EOF
        } else {
            const CharT ch = *pointer++;
            switch (ch) {
            case '/':
                state = relative_slash_state;
                break;
            case '?':
                // Set url's username to base's username, url's password to base's password, url's host to base's host,
                // url's port to base's port, url's path to base's path, url's query to the empty string, and state to query state.
                urls.append_parts(*base, url::USERNAME, url::PATH);
                state = query_state;    // sets query to the empty string
                break;
            case '#':
                // Set url's username to base's username, url's password to base's password, url's host to base's host,
                // url's port to base's port, url's path to base's path, url's query to base's query, url's fragment to the empty string
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
                // Set url's username to base's username, url's password to base's password, url's host to base's host,
                // url's port to base's port, url's path to base's path, and then remove url's path's last entry, if any
                urls.append_parts(*base, url::USERNAME, url::PATH, &url::get_path_rem_last);
                state = path_state;
                --pointer;
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
            ++pointer;
            break;
        case '\\':
            if (urls.is_special_scheme()) {
                // TODO-WARN: validation error
                state = special_authority_ignore_slashes_state;
                ++pointer;
                break;
            }
        default:
            // set url's username to base's username, url's password to base's password, url's host to base's host,
            // url's port to base's port
            urls.append_parts(*base, url::USERNAME, url::PORT);
            state = path_state;
        }
    }

    if (state == special_authority_slashes_state) {
        if (last - pointer > 1 && pointer[0] == '/' && pointer[1] == '/') {
            state = special_authority_ignore_slashes_state;
            pointer += 2; // skip "//"
        } else {
            //TODO-WARN: validation error
            state = special_authority_ignore_slashes_state;
        }
    }

    if (state == special_authority_ignore_slashes_state) {
        auto it = pointer;
        while (it < last && detail::is_slash(*it)) ++it;
        // if (it != pointer) // TODO-WARN: validation error
        pointer = it;
        state = authority_state;
    }

    // TODO?: credentials serialization do after host parsing,
    //       because if host null ==> then no credentials serialization
    if (state == authority_state) {
        // TODO: saugoti end_of_authority ir naudoti kituose state
        const auto end_of_authority = urls.is_special_scheme() ?
            std::find_if(pointer, last, detail::is_special_authority_end_char<CharT>) :
            std::find_if(pointer, last, detail::is_authority_end_char<CharT>);

        const auto it_eta = detail::find_last(pointer, end_of_authority, CharT('@'));
        if (it_eta != end_of_authority) {
            if (std::distance(it_eta, end_of_authority) == 1) {
                // disallow empty host, example: "http://u:p@/"
                return url_result::EmptyHost; // TODO-ERR: 2.1. validation error, failure
            }
            //TODO-WARN: validation error
            const auto it_colon = std::find(pointer, it_eta, ':');
            // url includes credentials?
            const bool not_empty_password = std::distance(it_colon, it_eta) > 1;
            if (not_empty_password || std::distance(pointer, it_colon) > 0 /*not empty username*/) {
                // username
                std::string& str_username = urls.start_part(url::USERNAME);
                detail::AppendStringOfType(pointer, it_colon, userinfo_no_encode_set, str_username); // UTF-8 percent encode, @ -> %40
                urls.save_part();
                // password
                if (not_empty_password) {
                    std::string& str_password = urls.start_part(url::PASSWORD);
                    detail::AppendStringOfType(it_colon + 1, it_eta, userinfo_no_encode_set, str_password); // UTF-8 percent encode, @ -> %40
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
            const auto end_of_authority = urls.is_special_scheme() ?
                std::find_if(pointer, last, detail::is_special_authority_end_char<CharT>) :
                std::find_if(pointer, last, detail::is_authority_end_char<CharT>);

            bool in_square_brackets = false; // [] flag
            bool is_port = false;
            auto it_host_end = pointer;
            for (; it_host_end < end_of_authority; ++it_host_end) {
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
                    // TODO-ERR: validation error
                    return url_result::EmptyHost;
                } else if (state_override && (urls.has_credentials() || !urls.is_null(url::PORT))) {
                    return url_result::False; // can not make host empty
                }
            }

            // 2.2. If state override is given and state override is hostname state, then return
            if (is_port && state_override == hostname_state)
                return url_result::False; // host with port not accepted

            // parse and set host:
            const url_result res = parse_host(urls, pointer, it_host_end);
            if (res != url_result::Ok)
                return res; // TODO-ERR: failure

            if (is_port) {
                pointer = it_host_end + 1; // skip ':'
                state = port_state;
            } else {
                pointer = it_host_end;
                state = path_start_state;
                if (state_override)
                    return url_result::Ok;
            }
        }
    }

    if (state == port_state) {
        const auto end_of_digits = std::find_if_not(pointer, last, detail::is_ascii_digit<CharT>);

        const bool is_end_of_authority =
            end_of_digits == last || // EOF
            detail::is_authority_end_char(end_of_digits[0]) ||
            (end_of_digits[0] == '\\' && urls.is_special_scheme());

        if (is_end_of_authority || state_override) {
            if (pointer < end_of_digits) {
                // is port
                int port = 0;
                for (auto it = pointer; it < end_of_digits; ++it) {
                    port = port * 10 + (*it - '0');
                    if (port > 0xFFFF)
                        return url_result::InvalidPort; // TODO-ERR: (2-1-2) validation error, failure
                }
                // set port if not default
                if (urls.scheme_inf() == nullptr || urls.scheme_inf()->default_port != port) {
                    unsigned_to_str(port, urls.start_part(url::PORT), 10);
                    urls.save_part();
                    urls.set_flag(url::PORT_FLAG);
                } else {
                    // (2-1-3) Set url's port to null
                    urls.clear_part(url::PORT);
                }
            }
            if (state_override)
                return url_result::Ok; // (2-2)
            state = path_start_state;
            pointer = end_of_digits;
        } else {
            // TODO-ERR: (3) validation error, failure (contains non-digit)
            return url_result::InvalidPort;
        }
    }

    if (state == file_state) {
        if (!urls.is_file_scheme())
            urls.set_scheme(url::str_view_type{ "file", 4 });
        // ensure file URL's host is not null
        urls.set_empty_host();
        // EOF ==> 0 ==> default:
        switch (pointer != last ? *pointer : 0) {
        case '\\':
            // TODO-WARN: validation error
        case '/':
            state = file_slash_state;
            ++pointer;
            break;

        default:
            if (base && base->is_file_scheme()) {
                if (pointer == last) {
                    // EOF code point
                    // Set url's host to base's host, url's path to base's path, and url's query to base's query
                    urls.append_parts(*base, url::HOST, url::QUERY);
                    return url_result::Ok; // EOF
                } else {
                    switch (*pointer) {
                    case '?':
                        // Set url's host to base's host, url's path to base's path, url's query to the empty string
                        urls.append_parts(*base, url::HOST, url::PATH);
                        state = query_state; // sets query to the empty string
                        ++pointer;
                        break;
                    case '#':
                        // Set url's host to base's host, url's path to base's path, url's query to base's query, url's fragment to the empty string
                        urls.append_parts(*base, url::HOST, url::QUERY);
                        state = fragment_state; // sets fragment to the empty string
                        ++pointer;
                        break;
                    default:
                        if (!detail::starts_with_Windows_drive(pointer, last)) {
                            // set url's host to base's host, url's path to base's path, and then shorten url's path
                            urls.append_parts(*base, url::HOST, url::PATH, &url::get_shorten_path);
                            // Note: This is a (platform-independent) Windows drive letter quirk.
                        } else {
                            // TODO-WARN: validation error
                            // set url's host to base's host
                            urls.append_parts(*base, url::HOST, url::HOST);
                        }
                        state = path_state;
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
            ++pointer;
            break;

        default:
            if (base && base->is_file_scheme()) {
                // It is important to first set host, then path, otherwise serializer
                // will fail.

                // set url's host to base's host
                urls.append_parts(*base, url::HOST, url::HOST);
                // path
                if (!detail::starts_with_Windows_drive(pointer, last)) {
                    url::str_view_type base_path = base->get_path_first_string(2);
                    // if base's path[0] is a normalized Windows drive letter
                    if (base_path.length() == 2 &&
                        detail::is_normalized_Windows_drive(base_path[0], base_path[1])) {
                        // append base's path[0] to url's path
                        std::string& str_path = urls.start_path_segment();
                        str_path.append(base_path.data(), 2); // "C:"
                        urls.save_path_segment();
                        // Note: This is a (platform - independent) Windows drive letter quirk.
                    }
                }
            }
            state = path_state;
        }
    }

    if (state == file_host_state) {
        const auto end_of_authority = std::find_if(pointer, last, detail::is_special_authority_end_char<CharT>);

        if (pointer == end_of_authority) {
            // buffer is the empty string
            // set empty host
            urls.set_empty_host();
            // if state override is given, then return
            if (state_override)
                return url_result::Ok;
            state = path_start_state;
        } else if (!state_override && end_of_authority - pointer == 2 &&
            detail::is_Windows_drive(pointer[0], pointer[1])) {
            // buffer is a Windows drive letter
            // TODO-WARN: validation error
            state = path_state;
            // Note: This is a (platform - independent) Windows drive letter quirk.
            // buffer is not reset here and instead used in the path state.
            // TODO: buffer is not reset here and instead used in the path state
        } else {
            // parse and set host:
            const url_result res = parse_host(urls, pointer, end_of_authority);
            if (res != url_result::Ok)
                return res; // TODO-ERR: failure
            // if host is "localhost", then set host to the empty string
            if (urls.get_part_view(url::HOST) == url::str_view_type{ "localhost", 9 }) {
                // set empty host
                urls.empty_host();
            }
            // if state override is given, then return
            if (state_override)
                return url_result::Ok;
            pointer = end_of_authority;
            state = path_start_state;
        }
    }

    if (state == path_start_state) {
        if (urls.is_special_scheme()) {
            if (pointer != last) {
                const CharT ch = *pointer;
                if (ch == '/') ++pointer;
                else if (ch == '\\') {
                    // TODO-WARN: validation error
                    ++pointer;
                }
            }
            state = path_state;
        } else if (pointer != last) {
            if (!state_override) {
                switch (pointer[0]) {
                case '?':
                    // TODO: set url's query to the empty string
                    state = query_state;
                    ++pointer;
                    break;
                case '#':
                    // TODO: set url's fragment to the empty string
                    state = fragment_state;
                    ++pointer;
                    break;
                case '/':
                    ++pointer;
                default:
                    state = path_state;
                    break;
                }
            } else {
                if (pointer[0] == '/') ++pointer;
                state = path_state;
            }
        } else {
            // EOF
            if (state_override && urls.is_null(url::HOST))
                urls.append_empty_path_segment();
            // otherwise path is empty
            urls.commit_path();
            return url_result::Ok;
        }
    }

    if (state == path_state) {
        const auto end_of_path = state_override ? last :
            std::find_if(pointer, last, [](CharT c) { return c == '?' || c == '#'; });

        parse_path(urls, pointer, end_of_path);
        pointer = end_of_path;

        // the end of path parse
        urls.commit_path();

        if (pointer == last) {
            return url_result::Ok; // EOF
        } else {
            const CharT ch = *pointer++;
            if (ch == '?') {
                // TODO: set url's query to the empty string
                state = query_state;
            } else {
                // ch == '#'
                // TODO: set url's fragment to the empty string
                state = fragment_state;
            }
        }
    }

    if (state == opaque_path_state) {
        const auto end_of_path =
            std::find_if(pointer, last, [](CharT c) { return c == '?' || c == '#'; });

        // UTF-8 percent encode using the C0 control percent-encode set,
        // and append the result to url's path string
        std::string& str_path = urls.start_path_string();
        do_simple_path(pointer, end_of_path, str_path);
        urls.save_path_string();
        pointer = end_of_path;

        if (pointer == last) {
            return url_result::Ok; // EOF
        } else {
            const CharT ch = *pointer++;
            if (ch == '?') {
                // TODO: set url's query to the empty string
                state = query_state;
            } else {
                // ch == '#'
                // TODO: set url's fragment to the empty string
                state = fragment_state;
            }
        }
    }

    if (state == query_state) {
        const auto end_of_query = state_override ? last : std::find(pointer, last, '#');

        // TODO-WARN:
        //for (auto it = pointer; it < end_of_query; ++it) {
        //  UCharT c = static_cast<UCharT>(*it);
        //  // 1. If c is not a URL code point and not "%", validation error.
        //  // 2. If c is "%" and remaining does not start with two ASCII hex digits, validation error.
        //}

#if WHATWG_URL_USE_ENCODING
        // scheme_inf_ == nullptr, if unknown scheme
        if (!urls.scheme_inf() || !urls.scheme_inf()->is_special || urls.scheme_inf()->is_ws)
            encoding = "UTF-8";
#endif

        // Let query_cpset be the special-query percent-encode set if url is special;
        // otherwise the query percent-encode set.
        const auto& query_cpset = urls.is_special_scheme()
            ? special_query_no_encode_set
            : query_no_encode_set;

        // Percent-encode after encoding, with encoding, buffer, and query_cpset, and append
        // the result to url’s query.
        // TODO: now supports UTF-8 encoding only, maybe later add other encodings
        std::string& str_query = urls.start_part(url::QUERY);
        // detail::AppendStringOfType(pointer, end_of_query, query_cpset, str_query);
        while (pointer != end_of_query) {
            // UTF-8 percent encode c using the fragment percent-encode set
            // and ignore '\0'
            const UCharT uch = static_cast<UCharT>(*pointer);
            if (uch >= 0x80) {
                // invalid utf-8/16/32 sequences will be replaced with kUnicodeReplacementCharacter
                detail::AppendUTF8EscapedChar(pointer, end_of_query, str_query);
            } else {
                // Just append the 7-bit character, possibly escaping it.
                const unsigned char uc = static_cast<unsigned char>(uch);
                if (!detail::isCharInSet(uc, query_cpset))
                    detail::AppendEscapedChar(uch, str_query);
                else
                    str_query.push_back(uc);
                ++pointer;
            }
            // TODO-WARN:
            // If c is not a URL code point and not "%", validation error.
            // If c is "%" and remaining does not start with two ASCII hex digits, validation error.
            // Let bytes be the result of encoding c using encoding ...
        }
        urls.save_part();
        urls.set_flag(url::QUERY_FLAG);

        pointer = end_of_query;
        if (pointer == last)
            return url_result::Ok; // EOF
        // *pointer == '#'
        //TODO: set url's fragment to the empty string
        state = fragment_state;
        ++pointer; // skip '#'
    }

    if (state == fragment_state) {
        // https://url.spec.whatwg.org/#fragment-state
        std::string& str_frag = urls.start_part(url::FRAGMENT);
        while (pointer < last) {
            // UTF-8 percent encode c using the fragment percent-encode set
            const UCharT uch = static_cast<UCharT>(*pointer);
            if (uch >= 0x80) {
                // invalid utf-8/16/32 sequences will be replaced with kUnicodeReplacementCharacter
                detail::AppendUTF8EscapedChar(pointer, last, str_frag);
            } else {
                // Just append the 7-bit character, possibly escaping it.
                const unsigned char uc = static_cast<unsigned char>(uch);
                if (detail::isCharInSet(uc, fragment_no_encode_set)) {
                    str_frag.push_back(uc);
                } else {
                    // other characters are escaped
                    detail::AppendEscapedChar(uch, str_frag);
                }
                ++pointer;
            }
            // TODO-WARN:
            // If c is not a URL code point and not "%", validation error.
            // If c is "%" and remaining does not start with two ASCII hex digits, validation error.
        }
        urls.save_part();
        urls.set_flag(url::FRAGMENT_FLAG);
    }

    return url_result::Ok;
}

// internal functions

template <typename CharT>
inline url_result url_parser::parse_host(url_serializer& urls, const CharT* first, const CharT* last) {
    return host_parser::parse_host(first, last, !urls.is_special_scheme(), urls);
}

template <typename CharT>
inline void url_parser::parse_path(url_serializer& urls, const CharT* first, const CharT* last) {
    // path state; includes:
    // 1. [ (/,\) - 1, 2, 3, 4 - [ 1 (if first segment), 2 ] ]
    // 2. [ 1 ... 4 ]
    static const auto escaped_dot = [](const CharT* const pointer) -> bool {
        return pointer[0] == '%' && pointer[1] == '2' &&
            (pointer[2] == 'e' || pointer[2] == 'E');
    };
    const auto double_dot = [](const CharT* const pointer, const std::size_t len) -> bool {
        if (len == 2) {
            return (pointer[0] == '.' && pointer[1] == '.');
        } else if (len == 4) {
            // ".%2e" or "%2e."
            return (pointer[0] == '.' && escaped_dot(pointer + 1)) ||
                (escaped_dot(pointer) && pointer[3] == '.');
        } else if (len == 6) {
            // "%2e%2e"
            return escaped_dot(pointer) && escaped_dot(pointer + 3);
        } else
            return false;
    };
    const auto single_dot = [](const CharT* const pointer, const std::size_t len) -> bool {
        if (len == 1) {
            return pointer[0] == '.';
        } else if (len == 3) {
            return escaped_dot(pointer); // "%2e"
        } else
            return false;
    };

    // parse path's segments
    auto pointer = first;
    while (true) {
        const auto end_of_segment = urls.is_special_scheme()
            ? std::find_if(pointer, last, detail::is_slash<CharT>)
            : std::find(pointer, last, '/');

        // end_of_segment >= pointer
        const std::size_t len = end_of_segment - pointer;
        const bool is_last = end_of_segment == last;
        // TODO-WARN: 1. If url is special and c is "\", validation error.

        if (double_dot(pointer, len)) {
            urls.shorten_path();
            if (is_last) urls.append_empty_path_segment();
        } else if (single_dot(pointer, len)) {
            if (is_last) urls.append_empty_path_segment();
        } else {
            if (len == 2 &&
                urls.is_file_scheme() &&
                urls.is_empty_path() &&
                detail::is_Windows_drive(pointer[0], pointer[1]))
            {
                // replace the second code point in buffer with ":"
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
    using UCharT = typename std::make_unsigned<CharT>::type;

    // TODO-WARN: 2. [ 1 ... 2 ] validation error.
    bool success = true;
    while (pointer < last) {
        // UTF-8 percent encode c using the default encode set
        const UCharT uch = static_cast<UCharT>(*pointer);
        if (uch >= 0x80) {
            // invalid utf-8/16/32 sequences will be replaced with 0xfffd
            success &= detail::AppendUTF8EscapedChar(pointer, last, output);
        } else {
            // Just append the 7-bit character, possibly escaping it.
            const unsigned char uc = static_cast<unsigned char>(uch);
            if (!detail::isCharInSet(uc, path_no_encode_set))
                detail::AppendEscapedChar(uc, output);
            else
                output.push_back(uc);
            ++pointer;
        }
    }
    return success;
}

template <typename CharT>
inline bool url_parser::do_simple_path(const CharT* pointer, const CharT* last, std::string& output) {
    using UCharT = typename std::make_unsigned<CharT>::type;

    // 3. of "opaque path state"
    // TODO-WARN: 3. [ 1 ... 2 ] validation error.
    //  1. If c is not EOF code point, not a URL code point, and not "%", validation error.
    //  2. If c is "%" and remaining does not start with two ASCII hex digits, validation error.

    bool success = true;
    while (pointer < last) {
        // UTF-8 percent encode c using the C0 control percent-encode set (U+0000 ... U+001F and >U+007E)
        const UCharT uch = static_cast<UCharT>(*pointer);
        if (uch >= 0x7f) {
            // invalid utf-8/16/32 sequences will be replaced with 0xfffd
            success &= detail::AppendUTF8EscapedChar(pointer, last, output);
        } else {
            // Just append the 7-bit character, escaping C0 control chars:
            if (uch <= 0x1f)
                detail::AppendEscapedChar(uch, output);
            else
                output.push_back(static_cast<unsigned char>(uch));
            ++pointer;
        }
    }
    return success;
}

} // namespace detail


// path util

inline url::str_view_type url::get_path_first_string(std::size_t len) const {
    str_view_type pathv = get_part_view(PATH);
    if (pathv.length() == 0 || has_an_opaque_path())
        return pathv;
    // skip '/'
    pathv.remove_prefix(1);
    if (pathv.length() == len || (pathv.length() > len && pathv[len] == '/')) {
        return str_view_type(pathv.data(), len);
    }
    return str_view_type(pathv.data(), 0);
}

// path shortening

inline bool url::get_path_rem_last(std::size_t& path_end, std::size_t& path_segment_count) const {
    if (path_segment_count_ > 0) {
        // Remove path's last item
        const char* const first = norm_url_.data() + part_end_[url::PATH-1];
        const char* const last = norm_url_.data() + part_end_[url::PATH];
        const char* it = detail::find_last(first, last, '/');
        if (it == last) it = first; // remove full path if '/' not found
        // shorten
        path_end = it - norm_url_.data();
        path_segment_count = path_segment_count_ - 1;
        return true;
    }
    return false;
}

// https://url.spec.whatwg.org/#shorten-a-urls-path

inline bool url::get_shorten_path(std::size_t& path_end, std::size_t& path_segment_count) const {
    assert(!has_an_opaque_path());
    if (path_segment_count_ == 0)
        return false;
    if (is_file_scheme() && path_segment_count_ == 1) {
        const str_view_type path1 = get_path_first_string(2);
        if (path1.length() == 2 &&
            detail::is_normalized_Windows_drive(path1[0], path1[1]))
            return false;
    }
    // Remove path's last item
    return get_path_rem_last(path_end, path_segment_count);
}


namespace detail {

// url_serializer class

inline void url_serializer::shorten_path() {
    assert(last_pt_ <= url::PATH);
    if (url_.get_shorten_path(url_.part_end_[url::PATH], url_.path_segment_count_))
        url_.norm_url_.resize(url_.part_end_[url::PATH]);
}

#if 0 // UNUSED
static inline std::size_t count_leading_path_slashes(const char* first, const char* last) {
    return std::distance(first,
        std::find_if_not(first, last, [](char c){ return c == '/'; }));
}

inline std::size_t url_serializer::remove_leading_path_slashes() {
    assert(last_pt_ == url::PATH);
    std::size_t count = count_leading_path_slashes(
        url_.norm_url_.data() + url_.part_end_[url::PATH-1],
        url_.norm_url_.data() + url_.part_end_[url::PATH]);
    if (count > 1) {
        count -= 1;
        url_.norm_url_.erase(url_.part_end_[url::PATH-1], count);
        url_.part_end_[url::PATH] -= count;
        url_.path_segment_count_ -= count;
        return count;
    }
    return 0;
}
#endif

inline url_serializer::~url_serializer() {
}

// set scheme

inline std::string& url_serializer::start_scheme() {
    url_.norm_url_.clear(); // clear all
    return url_.norm_url_;
}

inline void url_serializer::save_scheme() {
    set_scheme(url_.norm_url_.length());
    url_.norm_url_.push_back(':');
}

// set url's part

inline void url_serializer::fill_parts_offset(url::PartType t1, url::PartType t2, std::size_t offset) {
    for (int ind = t1; ind < t2; ++ind)
        url_.part_end_[ind] = offset;
}

inline std::string& url_serializer::start_part(url::PartType new_pt) {
    // offsets of empty parts (until new_pt) are also filled
    url::PartType fill_start_pt = static_cast<url::PartType>(static_cast<int>(last_pt_)+1);
    switch (last_pt_) {
    case url::SCHEME:
        // if host is non-null
        if (new_pt <= url::HOST)
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
    default: break;
    }

    fill_parts_offset(fill_start_pt, new_pt, url_.norm_url_.length());

    switch (new_pt) {
    case url::QUERY:
        url_.norm_url_ += '?';
        break;
    case url::FRAGMENT:
        url_.norm_url_ += '#';
        break;
    default: break;
    }

    assert(last_pt_ < new_pt || (last_pt_ == new_pt && is_empty(last_pt_)));
    // value to url_.part_end_[new_pt] will be assigned in the save_part()
    last_pt_ = new_pt;
    return url_.norm_url_;
}

inline void url_serializer::save_part() {
    url_.part_end_[last_pt_] = url_.norm_url_.length();
}

// The append_empty_path_segment() appends the empty string to url’s path (list);
// it is called from these places:
// 1) path_start_state -> [5.]
// 2) path_state -> [1.2.2. ".." ]
// 3) path_state -> [1.3. "." ]
inline void url_serializer::append_empty_path_segment() {
    start_path_segment();
    save_path_segment();
}

inline std::string& url_serializer::start_path_segment() {
    // appends new segment to path: / seg1 / seg2 / ... / segN
    std::string& str_path = start_part(url::PATH);
    str_path += '/';
    return str_path;
}

inline void url_serializer::save_path_segment() {
    save_part();
    url_.path_segment_count_++;
}

inline void url_serializer::commit_path() {
    // "/." path prefix
    adjust_path_prefix();
}

inline void url_serializer::adjust_path_prefix() {
    // "/." path prefix
    // https://url.spec.whatwg.org/#url-serializing (4.1.)
    str_view_type new_prefix;
    if (is_null(url::HOST) && url_.path_segment_count_ > 1) {
        const auto pathname = get_part_view(url::PATH);
        if (pathname.length() > 1 && pathname[0] == '/' && pathname[1] == '/')
            new_prefix = { "/.", 2 };
    }
    if (is_empty(url::PATH_PREFIX) != new_prefix.empty())
        replace_part(url::PATH_PREFIX, new_prefix.data(), new_prefix.length());
}

inline std::string& url_serializer::start_path_string() {
    return start_part(url::PATH);
}

inline void url_serializer::save_path_string() {
    assert(url_.path_segment_count_ == 0);
    save_part();
}


inline void url_serializer::set_empty_host() {
    start_part(url::HOST);
    save_part();
    set_host_type(HostType::Empty);
}

inline void url_serializer::empty_host() {
    assert(last_pt_ >= url::HOST);
    const std::size_t host_end = url_.part_end_[url::HOST_START];
    if (last_pt_ == url::HOST) {
        url_.part_end_[url::HOST] = host_end;
        url_.norm_url_.resize(host_end);
    } else if (last_pt_ > url::HOST) {
        const std::size_t diff = url_.part_end_[url::HOST] - host_end;
        if (diff) {
            for (int pt = url::HOST; pt <= last_pt_; ++pt)
                url_.part_end_[pt] -= diff;
            url_.norm_url_.erase(host_end, diff);
        }
    }
    url_.set_host_type(HostType::Empty);
}

// host_output overrides

inline std::string& url_serializer::hostStart() {
    return start_part(url::HOST);
}

inline void url_serializer::hostDone(HostType ht) {
    save_part();
    set_host_type(ht);

    // non-null host
    if (!is_empty(url::PATH_PREFIX)) {
        // remove '/.' path prefix
        replace_part(url::PATH_PREFIX, nullptr, 0);
    }
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
            ifirst = url::PATH_PREFIX;
        }
    } else {
        // t1 == PATH
        ifirst = t1;
    }

    // copy flags; they can be used when copying / serializing url parts below
    unsigned mask = 0;
    for (int ind = t1; ind <= t2; ++ind) {
        mask |= url::kPartFlagMask[ind];
    }
    url_.flags_ = (url_.flags_ & ~mask) | (src.flags_ & mask);

    // copy parts & str
    if (ifirst <= t2) {
        int ilast = t2;
        for (; ilast >= ifirst; --ilast) {
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
                std::size_t segment_count = src.path_segment_count_;
                // https://isocpp.org/wiki/faq/pointers-to-members
                // todo: use std::invoke (c++17)
                (src.*pathOpFn)(lastp_end, segment_count);
                url_.path_segment_count_ = segment_count;
            } else if (ifirst <= url::PATH && url::PATH <= ilast) {
                url_.path_segment_count_ = src.path_segment_count_;
            }
            // src
            const std::size_t offset = src.part_end_[ifirst - 1] + detail::kPartStart[ifirst];
            const char* const first = src.norm_url_.data() + offset;
            const char* const last = src.norm_url_.data() + lastp_end;
            // dest
            const std::ptrdiff_t delta = checked_diff<std::ptrdiff_t>(norm_url.length(), offset);
            // copy normalized url string from src
            norm_url.append(first, last);
            // adjust url_.part_end_
            for (int ind = ifirst; ind < ilast; ++ind) {
                // if (src.part_end_[ind]) // it is known, that src.part_end_[ind] has value, so check isn't needed
                url_.part_end_[ind] = src.part_end_[ind] + delta;
            }
            // ilast part from lastp
            url_.part_end_[ilast] = lastp_end + delta;
            last_pt_ = static_cast<url::PartType>(ilast);
        }
    }
}

// replace part in url

inline std::size_t url_serializer::get_part_pos(const url::PartType pt) const {
    return pt > url::SCHEME ? url_.part_end_[pt - 1] : 0;
}

inline std::size_t url_serializer::get_part_len(const url::PartType pt) const {
    return url_.part_end_[pt] - url_.part_end_[pt - 1];
}

inline void url_serializer::replace_part(const url::PartType new_pt, const char* str, const std::size_t len) {
    replace_part(new_pt, str, len, new_pt, 0);
}

inline void url_serializer::replace_part(const url::PartType last_pt, const char* str, const std::size_t len,
    const url::PartType first_pt, const std::size_t len0)
{
    const std::size_t b = get_part_pos(first_pt);
    const std::size_t l = url_.part_end_[last_pt] - b;
    url_.norm_url_.replace(b, l, str, len);
    std::fill(std::begin(url_.part_end_) + first_pt, std::begin(url_.part_end_) + last_pt, b + len0);
    // adjust positions
    const std::ptrdiff_t diff = checked_diff<std::ptrdiff_t>(len, l);
    if (diff) {
        for (auto it = std::begin(url_.part_end_) + last_pt; it != std::end(url_.part_end_); ++it) {
            if (*it == 0) break;
            // perform arithmetics using signed type ptrdiff_t, because diff can be negative
            *it = static_cast<std::ptrdiff_t>(*it) + diff;
        }
    }
}


// url_setter class

inline url_setter::~url_setter() {
}

//???
inline void url_setter::reserve(std::size_t new_cap) {
    strp_.reserve(new_cap);
}

// set scheme

inline std::string& url_setter::start_scheme() {
    return strp_;
}

inline void url_setter::save_scheme() {
    replace_part(url::SCHEME, strp_.data(), strp_.length());
    set_scheme(strp_.length());
}

// set/clear/empty url's part

inline std::string& url_setter::start_part(url::PartType new_pt) {
    assert(new_pt > url::SCHEME);
    curr_pt_ = new_pt;
    if (url_.part_end_[new_pt]) {
        use_strp_ = true;
        switch (new_pt) {
        case url::HOST:
            if (get_part_len(url::SCHEME_SEP) < 3)
                strp_ = "://";
            else
                strp_.clear();
            break;
        case url::PASSWORD:
        case url::PORT:
            strp_ = ':';
            break;
        case url::QUERY:
            strp_ = '?';
            break;
        case url::FRAGMENT:
            strp_ = '#';
            break;
        default:
            strp_.clear();
            break;
        }
        return strp_;
    } else {
        use_strp_ = false;
        last_pt_ = find_last_part(new_pt);
        return url_serializer::start_part(new_pt);
    }
}

inline void url_setter::save_part() {
    if (use_strp_) {
        if (curr_pt_ == url::HOST) {
            if (get_part_len(url::SCHEME_SEP) < 3)
                // SCHEME_SEP, USERNAME, PASSWORD, HOST_START; HOST
                replace_part(url::HOST, strp_.data(), strp_.length(), url::SCHEME_SEP, 3);
            else
                replace_part(url::HOST, strp_.data(), strp_.length());
        } else {
            const bool empty_val = strp_.length() <= detail::kPartStart[curr_pt_];
            switch (curr_pt_) {
            case url::USERNAME:
            case url::PASSWORD:
                if (!empty_val && !has_credentials()) {
                    strp_ += '@';
                    // USERNAME, PASSWORD; HOST_START
                    replace_part(url::HOST_START, strp_.data(), strp_.length(), curr_pt_, strp_.length() - 1);
                    break;
                } else if (empty_val && is_empty(curr_pt_ == url::USERNAME ? url::PASSWORD : url::USERNAME)) {
                    // both username and password will be empty, so also drop '@'
                    replace_part(url::HOST_START, "", 0, curr_pt_, 0);
                    break;
                }
            default:
                if ((curr_pt_ == url::PASSWORD || curr_pt_ == url::PORT) && empty_val)
                    strp_.clear(); // drop ':'
                replace_part(curr_pt_, strp_.data(), strp_.length());
                break;
            }
        }
        // cleanup
        strp_.clear();
    } else {
        url_serializer::save_part();
    }
}

inline void url_setter::clear_part(const url::PartType pt) {
    if (url_.part_end_[pt]) {
        replace_part(pt, "", 0);
        url_.flags_ &= ~(1u << pt); // set to null
    }
}

inline void url_setter::empty_part(const url::PartType pt) {
    if (url_.part_end_[pt]) {
        replace_part(pt, "", 0);
    }
}

inline void url_setter::empty_host() {
    empty_part(url::HOST);
    url_.set_host_type(HostType::Empty);
}

inline std::string& url_setter::start_path_segment() {
    //curr_pt_ = url::PATH; // not used
    strp_ += '/';
    return strp_;
}

inline void url_setter::save_path_segment() {
    path_seg_end_.push_back(strp_.length());
}

inline void url_setter::commit_path() {
    // fill part_end_ until url::PATH if not filled
    for (int ind = url::PATH; ind > 0; --ind) {
        if (url_.part_end_[ind]) break;
        url_.part_end_[ind] = url_.norm_url_.length();
    }
    // replace path part
    replace_part(url::PATH, strp_.data(), strp_.length());
    url_.path_segment_count_ = path_seg_end_.size();

    // "/." path prefix
    adjust_path_prefix();
}

// https://url.spec.whatwg.org/#shorten-a-urls-path

inline void url_setter::shorten_path() {
    if (path_seg_end_.size() == 1) {
        if (is_file_scheme() && strp_.length() == 3 &&
            detail::is_normalized_Windows_drive(strp_[1], strp_[2]))
            return;
        path_seg_end_.pop_back();
        strp_.clear();
    } else if (path_seg_end_.size() >= 2) {
        path_seg_end_.pop_back();
        strp_.resize(path_seg_end_.back());
    }
}

#if 0 // UNUSED
inline std::size_t url_setter::remove_leading_path_slashes() {
    std::size_t count = count_leading_path_slashes(strp_.data(), strp_.data() + strp_.length());
    if (count > 1) {
        count -= 1;
        strp_.erase(0, count);
        path_seg_end_.erase(path_seg_end_.begin(), std::next(path_seg_end_.begin(), count));
        return count;
    }
    return 0;
}
#endif

inline bool url_setter::is_empty_path() const {
    assert(!url_.has_an_opaque_path());
    // path_seg_end_ has meaning only if path is a list (path isn't opaque)
    return path_seg_end_.empty();
}

// https://url.spec.whatwg.org/#potentially-strip-trailing-spaces-from-an-opaque-path

inline void url_setter::potentially_strip_trailing_spaces_from_an_opaque_path() {
    if (url_.has_an_opaque_path() &&
        is_null(url::FRAGMENT) &&
        is_null(url::QUERY)) {
        // Remove all trailing U+0020 SPACE code points from URL’s path.
        // Note. If the entire path consists of spaces or is empty, then last non-space
        // character in the url_.norm_url_ is scheme separator (:).
        const auto newlen = url_.norm_url_.find_last_not_of(' ') + 1;
        url_.norm_url_.resize(newlen);
        for (int ind = url::PATH; ind < url_.part_end_.size() && url_.part_end_[ind]; ++ind)
            url_.part_end_[ind] = newlen;
    }
}

inline url::PartType url_setter::find_last_part(url::PartType pt) const {
    for (int ind = pt; ind > 0; --ind)
        if (url_.part_end_[ind])
            return static_cast<url::PartType>(ind);
    return url::SCHEME;
}

#if 0
inline void url_setter::insert_part(url::PartType new_pt, const char* str, std::size_t len) {
    assert(new_pt > url::SCHEME);
    if (url_.part_end_[new_pt]) {
        replace_part(new_pt, str, len);
    } else {
        last_pt_ = find_last_part(new_pt);
        url_serializer::start_part(new_pt).append(str, len);
        url_serializer::save_part();
    }
}
#endif

// Check UNC path
//
// Input - path string with the first two backslashes skipped
//
template <typename CharT>
inline bool is_unc_path(const CharT* first, const CharT* last)
{
    // https://docs.microsoft.com/en-us/openspecs/windows_protocols/ms-dfsc/149a3039-98ce-491a-9268-2f5ddef08192
    std::size_t path_components_count = 0;
    const auto* start = first;
    while (start != last) {
        const CharT* pcend = std::find(start, last, '\\');
        // path components MUST be at least one character in length
        if (start == pcend)
            return false;
        // path components MUST NOT contain a backslash (\) or a null; also disallow '/'
        // in path component because URL parser will treat it as a directory separator
        if (std::find_if(start, pcend, [](CharT c) { return c == '\0' || c == '/'; }) != pcend)
            return false;

        ++path_components_count;

        // disallow windows drive in the host-name because URL parser will make it a path
        if (path_components_count == 1 &&
            pcend - start == 2 &&
            detail::is_Windows_drive(start[0], start[1]))
            return false;

        if (pcend == last) break;
        start = pcend + 1; // skip '\'
    }
    // A valid UNC path MUST contain two or more path components
    return path_components_count >= 2;
}

} // namespace detail


// URL utilities

/// @brief Make URL from OS file path
///
/// Throws url_error exception on error.
///
/// @param[in] str absolute file path string
/// @return file URL
template <class StrT, enable_if_str_arg_t<StrT> = 0>
inline url url_from_file_path(StrT&& str) {
    const auto inp = make_str_arg(std::forward<StrT>(str));
    const auto* first = inp.begin();
    const auto* last = inp.end();

    if (first != last) {
        const auto* pointer = first;
        const code_point_set* no_encode_set = nullptr;

        std::string str_url("file://");

        if (*pointer == '/') {
            // Absolute Unix path
            no_encode_set = &unix_path_no_encode_set;
        } else {
            // Windows path?
            bool is_unc = false;

            // https://docs.microsoft.com/en-us/dotnet/standard/io/file-path-formats
            // https://docs.microsoft.com/en-us/windows/win32/fileio/maximum-file-path-limitation
            if (detail::starts_with(pointer, last, { "\\\\", 2 })) {
                pointer += 2; // skip '\\'

                // UNC path or DOS device path
                if (detail::starts_with(pointer, last, { "?\\", 2 }) ||
                    detail::starts_with(pointer, last, { ".\\", 2 })) {
                    pointer += 2; // skip "?\" or ".\"
                    // DOS device path
                    if (detail::starts_with(pointer, last, { "UNC\\", 4 })) {
                        pointer += 4; // skip "UNC\"
                        is_unc = true;
                    }
                } else {
                    // UNC path
                    is_unc = true;
                }
            }
            if (is_unc
                ? detail::is_unc_path(pointer, last)
                : detail::starts_with_Windows_drive_absolute_path(pointer, last)) {
                no_encode_set = &raw_path_no_encode_set;
                if (!is_unc) str_url.push_back('/'); // start path
            } else {
                throw url_error(url_result::UnsupportedPath, "Unsupported path");
            }
        }

        // make URL
        detail::AppendStringOfType(pointer, last, *no_encode_set, str_url);
        return url(str_url);
    } else {
        throw url_error(url_result::EmptyPath, "Empty path");
    }
}

#ifdef WHATWG_URL_USE_FILESYSTEM

/// @brief Make URL from OS file path
///
/// Throws url_error exception on error.
///
/// @param[in] path absolute file path
/// @return file URL
inline url url_from_file_path(const std::filesystem::path& path) {
    // Use the native format, to let the string version of this function detect OS specifics
#ifdef _WIN32
    return url_from_file_path(path.native()); // UTF-16
#else
    return url_from_file_path(path.u8string()); // UTF-8
#endif
}

#endif


} // namespace whatwg

#endif // WHATWG_URL_H
