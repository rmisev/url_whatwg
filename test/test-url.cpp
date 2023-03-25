// Copyright 2016-2023 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//

#include "url.h"
#include "doctest-main.h"
#include "test-utils.h"


const char* urls_to_str(const char* s1) {
    return s1;
}
std::string urls_to_str(const char* s1, const char* s2) {
    return std::string(s1) + " AGAINST " + s2;
}
std::string urls_to_str(const char* s1, const whatwg::url& u2) {
    return urls_to_str(s1, u2.to_string().c_str());
}

template <class ...Args>
void check_url_contructor(whatwg::url_result expected_res, Args&&... args)
{
    const auto stringify_args = [&] { return urls_to_str(args...); };
    try {
        whatwg::url url(args...);
        CHECK_MESSAGE(whatwg::url_result::Ok == expected_res, "URL: " << stringify_args());
    }
    catch (whatwg::url_error& ex) {
        CHECK_MESSAGE(ex.result() == expected_res, "URL: " << stringify_args());
    }
    catch (...) {
        FAIL_CHECK("Unknown exception with URL: " << stringify_args());
    }
}

TEST_CASE("url constructor") {
    // valid URL
    check_url_contructor(whatwg::url_result::Ok, "http://example.org/p");
    // invalid URLs
    check_url_contructor(whatwg::url_result::EmptyHost, "http:///");
    check_url_contructor(whatwg::url_result::EmptyHost, "http://%C2%AD/p"); // U+00AD - IDNA ignored code point
    check_url_contructor(whatwg::url_result::IdnaError, "http://xn--a/p");
    check_url_contructor(whatwg::url_result::InvalidPort, "http://h:a/p");
    check_url_contructor(whatwg::url_result::InvalidIpv4Address, "http://1.2.3.256/p");
    check_url_contructor(whatwg::url_result::InvalidIpv6Address, "http://[1::2::3]/p");
    check_url_contructor(whatwg::url_result::InvalidDomainCharacter, "http://h[]/p");
    check_url_contructor(whatwg::url_result::RelativeUrlWithoutBase, "relative");
    check_url_contructor(whatwg::url_result::RelativeUrlWithCannotBeABase, "relative", "about:blank");
    // empty (invalid) base
    const whatwg::url base;
    check_url_contructor(whatwg::url_result::InvalidBase, "http://h/", base);
}

// Copy/move construction/assignment

const char test_url[] = "http://h:123/p?a=b&c=d#frag";
const char test_rel_url[] = "//h:123/p?a=b&c=d#frag";
const char test_base_url[] = "http://example.org/p";
const pairs_list_t<std::string> test_url_params = { {"a","b"}, {"c","d"} };

void check_test_url(whatwg::url& url) {
    CHECK(url.href() == test_url);
    CHECK(url.origin() == "http://h:123");
    CHECK(url.protocol() == "http:");
    CHECK(url.host() == "h:123");
    CHECK(url.hostname() == "h");
    CHECK(url.port() == "123");
    CHECK(url.path() == "/p?a=b&c=d");
    CHECK(url.pathname() == "/p");
    CHECK(url.search() == "?a=b&c=d");
    CHECK(url.hash() == "#frag");
    CHECK(list_eq(url.search_params(), test_url_params));
}

TEST_CASE("url copy constructor") {
    whatwg::url url1(test_url);
    whatwg::url url2(url1);

    // CHECK(url2 == url1);
    check_test_url(url2);
}

TEST_CASE("url copy assignment") {
    whatwg::url url1(test_url);
    whatwg::url url2;

    url2 = url1;

    // CHECK(url2 == url1);
    check_test_url(url2);
}

TEST_CASE("url move constructor") {
    whatwg::url url0(test_url);
    whatwg::url url{ std::move(url0) };
    check_test_url(url);
}

TEST_CASE("url move assignment") {
    whatwg::url url;
    url = whatwg::url(test_url);
    check_test_url(url);
}

// url parsing constructor with base URL

TEST_CASE("url parsing constructor with base URL") {
    const whatwg::url base(test_base_url);
    {
        // pointer to base URL
        whatwg::url url(test_rel_url, &base);
        check_test_url(url);
    } {
        whatwg::url url(test_rel_url, base);
        check_test_url(url);
    }
}

TEST_CASE("url parsing constructor with base URL string") {
    whatwg::url url(test_rel_url, test_base_url);
    check_test_url(url);
}

// Parse URL

TEST_CASE("url::parse must clear old URL data") {
    whatwg::url url;

    CHECK(whatwg::success(url.parse("about:blank", nullptr)));
    CHECK_FALSE(url.empty());

    CHECK(whatwg::success(url.parse("http://host-1/", nullptr)));
    CHECK(url.host("host-2"));

    CHECK(url.host() == "host-2");
    CHECK(url.href() == "http://host-2/");
}

// Swap

TEST_CASE("swap(url&, url&)") {
    const std::string str_url_1 = "http://host-1:123/path-1?a=1&b=2#frag-1";
    const std::string str_url_2 = "http://host-2:321/path-2?c=3&d=4#frag-2";

    whatwg::url url_1(str_url_1);
    whatwg::url url_2(str_url_2);

    using std::swap;

    // Swap URLs with uninitialised search parameters
    swap(url_1, url_2);
    CHECK(url_1.href() == str_url_2);
    CHECK(url_2.href() == str_url_1);

    // Swap with one search parameter initialised
    auto& prm_1 = url_1.search_params();
    CHECK(prm_1.to_string() == "c=3&d=4");
    swap(url_1, url_2);
    CHECK(url_1.href() == str_url_1);
    CHECK(url_2.href() == str_url_2);

    // Initialise both search parameters
    CHECK(url_1.get_part_view(whatwg::url::QUERY) == url_1.search_params().to_string());
    CHECK(url_2.get_part_view(whatwg::url::QUERY) == url_2.search_params().to_string());

    // Swap with both search parameters initialised
    swap(url_1, url_2);
    CHECK(url_1.href() == str_url_2);
    CHECK(url_2.href() == str_url_1);
    CHECK(url_1.get_part_view(whatwg::url::QUERY) == url_1.search_params().to_string());
    CHECK(url_2.get_part_view(whatwg::url::QUERY) == url_2.search_params().to_string());

    // Are url and url_search_params still linked correctly?
    url_1.search_params().append("e", "10");
    url_2.search_params().append("f", "20");
    CHECK(url_1.get_part_view(whatwg::url::QUERY) == url_1.search_params().to_string());
    CHECK(url_2.get_part_view(whatwg::url::QUERY) == url_2.search_params().to_string());
}

// Valid or invalid URL

TEST_CASE("url::is_valid()") {
    // empty url invalid
    whatwg::url url;
    CHECK(url.empty());
    CHECK_FALSE(url.is_valid());

    // parse valid URL
    CHECK(url.parse("wss://host:88/path", nullptr) == whatwg::url_result::Ok);
    CHECK(url.href() == "wss://host:88/path");
    CHECK(url.is_valid());

    // href setter must not change original url on failure
    CHECK_FALSE(url.href("http://h:65616/p"));
    CHECK(url.href() == "wss://host:88/path");
    CHECK(url.is_valid());

    // url::parse must reset VALID_FLAG on failure
    CHECK(url.parse("http://h:8a/p", nullptr) == whatwg::url_result::InvalidPort);
    CHECK_FALSE(url.is_valid());

    // invalid URL must ignore setters (except href)
    std::string href{ url.href() };

    url.protocol("https");
    CHECK(url.href() == href);
    CHECK_FALSE(url.is_valid());

    url.username("user");
    CHECK(url.href() == href);
    CHECK_FALSE(url.is_valid());

    url.password("psw");
    CHECK(url.href() == href);
    CHECK_FALSE(url.is_valid());

    url.host("host:1");
    CHECK(url.href() == href);
    CHECK_FALSE(url.is_valid());

    url.hostname("host");
    CHECK(url.href() == href);
    CHECK_FALSE(url.is_valid());

    url.port("12");
    CHECK(url.href() == href);
    CHECK_FALSE(url.is_valid());

    url.pathname("path");
    CHECK(url.href() == href);
    CHECK_FALSE(url.is_valid());

    url.search("a=b");
    CHECK(url.href() == href);
    CHECK_FALSE(url.is_valid());

    url.search_params().append("c", "d");
    CHECK(url.href() == href);
    CHECK_FALSE(url.is_valid());

    url.hash("hash");
    CHECK(url.href() == href);
    CHECK_FALSE(url.is_valid());

    // href setter invoked with valid URL string as input
    // makes URL valid
    url.href("http://example.com/");
    CHECK(url.href() == "http://example.com/");
    CHECK_FALSE(url.empty());
    CHECK(url.is_valid());
}

TEST_CASE("Parse URL with invalid base") {
    SUBCASE("Empty base") {
        const whatwg::url base;
        CHECK_FALSE(base.is_valid());

        whatwg::url url;
        CHECK(url.parse("https://h/", &base) == whatwg::url_result::InvalidBase);
        CHECK_FALSE(url.is_valid());

        CHECK(url.parse("http://host/", nullptr) == whatwg::url_result::Ok);
        CHECK(url.is_valid());
        CHECK(url.parse("https://h/", &base) == whatwg::url_result::InvalidBase);
        CHECK_FALSE(url.is_valid());
    }
    SUBCASE("Invalid base") {
        whatwg::url base;
        CHECK_FALSE(whatwg::success(base.parse("http://h:65616/p", nullptr)));
        CHECK_FALSE(base.is_valid());

        whatwg::url url;
        CHECK(url.parse("https://h/", &base) == whatwg::url_result::InvalidBase);
        CHECK_FALSE(url.is_valid());

        CHECK(url.parse("/path", &base) == whatwg::url_result::InvalidBase);
        CHECK_FALSE(url.is_valid());

        CHECK(url.parse("http://host/", nullptr) == whatwg::url_result::Ok);
        CHECK(url.is_valid());
        CHECK(url.parse("https://h/", &base) == whatwg::url_result::InvalidBase);
        CHECK_FALSE(url.is_valid());
    }
}

// Empty URL

TEST_CASE("Empty URL") {
    whatwg::url url;
    CHECK(url.empty());

    url.protocol("http");
    CHECK(url.empty());

    url.protocol("about");
    CHECK(url.empty());

    url.username("user");
    CHECK(url.empty());

    url.password("psw");
    CHECK(url.empty());

    url.host("hp:1");
    CHECK(url.empty());

    url.hostname("h");
    CHECK(url.empty());

    url.port("12");
    CHECK(url.empty());

    url.pathname("path");
    CHECK(url.empty());

    url.search("a=b");
    CHECK(url.empty());

    url.search_params().append("c", "d");
    CHECK(url.empty());

    url.hash("hash");
    CHECK(url.empty());

    // href setter invoked with valid URL string as input
    // makes URL not empty
    url.href("http://example.com/");
    CHECK_FALSE(url.empty());
    CHECK(url.href() == "http://example.com/");
}

// URL parts

TEST_CASE("url::is_empty and url::is_null") {
    whatwg::url url;

    CHECK(url.is_empty(whatwg::url::SCHEME));
    CHECK(url.is_null(whatwg::url::HOST));

    CHECK(whatwg::success(url.parse("http://example.org/", nullptr)));
    CHECK_FALSE(url.is_empty(whatwg::url::SCHEME));
    CHECK_FALSE(url.is_null(whatwg::url::HOST));
}

// Origin tests

TEST_SUITE("Check origin") {
    TEST_CASE("http:") {
        whatwg::url url("http://host:123/path");
        CHECK(url.origin() == "http://host:123");
    }
    TEST_CASE("blob:") {
        whatwg::url url("blob:http://host:123/path");
        CHECK(url.origin() == "http://host:123");
    }
    TEST_CASE("blob: x 3") {
        whatwg::url url("blob:blob:blob:http://host:123/path");
        CHECK(url.origin() == "http://host:123");
    }
    TEST_CASE("file:") {
        whatwg::url url("file://host/path");
        CHECK(url.origin() == "null");
    }
    TEST_CASE("non-spec:") {
        whatwg::url url("non-spec://host:123/path");
        CHECK(url.origin() == "null");
    }
}

// URL serializing

static void check_serialize(std::string str_url, std::string str_hash) {
    whatwg::url u(str_url + str_hash);
    CHECK(u.serialize(false) == str_url + str_hash);
    CHECK(u.serialize(true) == str_url);
}

TEST_CASE("URL serializing") {
    check_serialize("http://h/", "");
    check_serialize("http://h/", "#");
    check_serialize("http://h/", "#f");
    check_serialize("http://h/?q", "");
    check_serialize("http://h/?q", "#");
    check_serialize("http://h/?q", "#f");
}

// URL equivalence

static bool are_equal(std::string atr_a, std::string atr_b, bool exclude_fragments) {
    whatwg::url a(atr_a);
    whatwg::url b(atr_b);
    return whatwg::equals(a, b, exclude_fragments);
}

TEST_CASE("URL equivalence") {
    CHECK(are_equal("http://h/#f", "http://h/#f", false));
    CHECK(are_equal("http://h/#f", "http://h/#f", true));

    CHECK_FALSE(are_equal("http://h/", "http://h/#", false));
    CHECK(are_equal("http://h/", "http://h/#", true));

    CHECK_FALSE(are_equal("http://h/", "http://h/#f", false));
    CHECK(are_equal("http://h/", "http://h/#f", true));

    CHECK_FALSE(are_equal("http://h/#", "http://h/#f", false));
    CHECK(are_equal("http://h/#", "http://h/#f", true));

    CHECK_FALSE(are_equal("http://h/#f1", "http://h/#f2", false));
    CHECK(are_equal("http://h/#f1", "http://h/#f2", true));

    CHECK_FALSE(are_equal("http://h1/", "http://h2/", false));
    CHECK_FALSE(are_equal("http://h1/", "http://h2/", true));
}

// UTF-8 in hostname

TEST_CASE("Valid UTF-8 in hostname") {
    static const char szUrl[] = { 'h', 't', 't', 'p', ':', '/', '/', char(0xC4), char(0x84), '/', '\0' }; // valid

    whatwg::url url;
    REQUIRE(whatwg::success(url.parse(szUrl, nullptr)));
    CHECK(url.hostname() == "xn--2da");
}
TEST_CASE("Valid percent encoded utf-8 in hostname") {
    static const char szUrl[] = { 'h', 't', 't', 'p', ':', '/', '/', '%', 'C', '4', '%', '8', '4', '/', '\0' }; // valid

    whatwg::url url;
    REQUIRE(whatwg::success(url.parse(szUrl, nullptr)));
    CHECK(url.hostname() == "xn--2da");
}
TEST_CASE("Invalid utf-8 in hostname") {
    static const char szUrl1[] = { 'h', 't', 't', 'p', ':', '/', '/', '%', 'C', '4', char(0x84), '/', '\0' }; // invalid
    static const char szUrl2[] = { 'h', 't', 't', 'p', ':', '/', '/', char(0xC4), '%', '8', '4', '/', '\0' }; // invalid

    check_url_contructor(whatwg::url_result::IdnaError, szUrl1);
    check_url_contructor(whatwg::url_result::IdnaError, szUrl2);
}

// UTF-16 in hostname

TEST_CASE("Valid UTF-16 in hostname") {
    static const char16_t szUrl[] = { 'h', 't', 't', 'p', ':', '/', '/', char16_t(0xD800), char16_t(0xDC00), '/', '\0' };

    whatwg::url url;
    REQUIRE(whatwg::success(url.parse(szUrl, nullptr)));
    CHECK(url.hostname() == "xn--2n7c");
}
TEST_CASE("Invalid UTF-16 in hostname") {
    static const char16_t szUrl1[] = { 'h', 't', 't', 'p', ':', '/', '/', char16_t(0xD800), '/', '\0' };
    static const char16_t szUrl2[] = { 'h', 't', 't', 'p', ':', '/', '/', char16_t(0xDC00), '/', '\0' };

    CHECK_THROWS_AS(whatwg::url{ szUrl1 }, whatwg::url_error);
    CHECK_THROWS_AS(whatwg::url{ szUrl2 }, whatwg::url_error);
}

// UTF-32 in hostname

TEST_CASE("Valid UTF-32 in hostname") {
    static const char32_t szUrl[] = { 'h', 't', 't', 'p', ':', '/', '/', char32_t(0x10000u), '/', '\0' };

    whatwg::url url;
    REQUIRE(whatwg::success(url.parse(szUrl, nullptr)));
    CHECK(url.hostname() == "xn--2n7c");
}
TEST_CASE("Invalid UTF-32 in hostname") {
    static const char32_t szUrl1[] = { 'h', 't', 't', 'p', ':', '/', '/', char32_t(0xD800), '/', '\0' };
    static const char32_t szUrl2[] = { 'h', 't', 't', 'p', ':', '/', '/', char32_t(0xDFFF), '/', '\0' };
    static const char32_t szUrl3[] = { 'h', 't', 't', 'p', ':', '/', '/', char32_t(0x110000u), '/', '\0' };

    CHECK_THROWS_AS(whatwg::url{ szUrl1 }, whatwg::url_error);
    CHECK_THROWS_AS(whatwg::url{ szUrl2 }, whatwg::url_error);
    CHECK_THROWS_AS(whatwg::url{ szUrl3 }, whatwg::url_error);
}

// URL utilities

TEST_CASE("url_from_file_path") {
    SUBCASE("POSIX path") {
        CHECK(whatwg::url_from_file_path("/").href() == "file:///");
        CHECK(whatwg::url_from_file_path("/path").href() == "file:///path");
        CHECK(whatwg::url_from_file_path("/path %#?").href() == "file:///path%20%25%23%3F");
        CHECK(whatwg::url_from_file_path("/c:\\end").href() == "file:///c%3A%5Cend");
        CHECK(whatwg::url_from_file_path("/c|\\end").href() == "file:///c%7C%5Cend");
        CHECK(whatwg::url_from_file_path("/c:/last").href() == "file:///c%3A/last");
        CHECK(whatwg::url_from_file_path("/c|/last").href() == "file:///c%7C/last");
        // empty path
        CHECK_THROWS_AS(whatwg::url_from_file_path(""), whatwg::url_error);
        // non absolute path
        CHECK_THROWS_AS(whatwg::url_from_file_path("path"), whatwg::url_error);
    }
    SUBCASE("Windows path") {
        // https://docs.microsoft.com/en-us/windows/win32/fileio/naming-a-file
        CHECK(whatwg::url_from_file_path("C:\\").href() == "file:///C:/");
        CHECK(whatwg::url_from_file_path("C:\\path").href() == "file:///C:/path");
        CHECK(whatwg::url_from_file_path("C|\\path").href() == "file:///C:/path");
        CHECK(whatwg::url_from_file_path("C:/path").href() == "file:///C:/path");
        CHECK(whatwg::url_from_file_path("C:\\path %#").href() == "file:///C:/path%20%25%23");
        CHECK(whatwg::url_from_file_path("\\\\h\\path").href() == "file://h/path");
        // https://docs.microsoft.com/en-us/dotnet/standard/io/file-path-formats
        // https://docs.microsoft.com/en-us/windows/win32/fileio/maximum-file-path-limitation
        CHECK(whatwg::url_from_file_path("\\\\?\\D:\\very_long_path").href() == "file:///D:/very_long_path");
        CHECK(whatwg::url_from_file_path("\\\\?\\UNC\\h\\very_long_path").href() == "file://h/very_long_path");
        CHECK(whatwg::url_from_file_path("\\\\.\\D:\\very_long_path").href() == "file:///D:/very_long_path");
        CHECK(whatwg::url_from_file_path("\\\\.\\UNC\\h\\very_long_path").href() == "file://h/very_long_path");
        // non absolute path
        CHECK_THROWS_AS(whatwg::url_from_file_path("\\"), whatwg::url_error);
        CHECK_THROWS_AS(whatwg::url_from_file_path("C:path"), whatwg::url_error);
        // invalid UNC
        CHECK_THROWS_AS(whatwg::url_from_file_path("\\\\"), whatwg::url_error);
        CHECK_THROWS_AS(whatwg::url_from_file_path("\\\\h"), whatwg::url_error);
        CHECK_THROWS_AS(whatwg::url_from_file_path("\\\\h\\"), whatwg::url_error);
        CHECK_THROWS_AS(whatwg::url_from_file_path("\\\\h\\\\"), whatwg::url_error);
        CHECK_THROWS_AS(whatwg::url_from_file_path(std::string{ '\\', '\\', 'h', '\\', 'a', '\0', 'b' }), whatwg::url_error);
        CHECK_THROWS_AS(whatwg::url_from_file_path("\\\\h\\a/b"), whatwg::url_error);
        CHECK_THROWS_AS(whatwg::url_from_file_path("\\\\a/b\\path"), whatwg::url_error);
        CHECK_THROWS_AS(whatwg::url_from_file_path("\\\\C:\\path"), whatwg::url_error);
        CHECK_THROWS_AS(whatwg::url_from_file_path("\\\\C|\\path"), whatwg::url_error);
        // invalid hostname
        CHECK_THROWS_AS(whatwg::url_from_file_path("\\\\a b\\path"), whatwg::url_error);
        // unsupported pathes
        CHECK_THROWS_AS(whatwg::url_from_file_path("\\\\?\\Volume{b75e2c83-0000-0000-0000-602f00000000}\\Test\\Foo.txt"), whatwg::url_error);
        CHECK_THROWS_AS(whatwg::url_from_file_path("\\\\.\\Volume{b75e2c83-0000-0000-0000-602f00000000}\\Test\\Foo.txt"), whatwg::url_error);
    }
}
