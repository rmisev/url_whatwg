#include "url.h"
#include "url_search_params.h"
#include "doctest-main.h"
#include "test-utils.h"
#include <map>

// The old macro __cpp_coroutines is still used in Visual Studio 2019 16.6
#if defined(__cpp_impl_coroutine) || defined(__cpp_coroutines)
#if __has_include(<experimental/generator>)
# include <experimental/generator>
# define TEST_COROUTINE_GENERATOR
using std::experimental::generator;
#endif
#endif


#define TEST_ITERABLES_DATA {    \
        {{ 'a' }, { 'a', 'a' }}, \
        {{ 'b' }, { 'b', 'b' }}  \
    }

TEST_CASE_TEMPLATE_DEFINE("Various string pairs iterables", CharT, test_iterables) {
    using string_t = std::basic_string<CharT>;

    const pairs_list_t<std::string> output = TEST_ITERABLES_DATA;

    SUBCASE("array") {
        const std::pair<string_t, string_t> arr_pairs[] = TEST_ITERABLES_DATA;
        whatwg::url_search_params params(arr_pairs);
        CHECK(list_eq(params, output));
    }
    SUBCASE("std::initializer_list") {
        const pairs_list_t<string_t> lst_pairs = TEST_ITERABLES_DATA;
        whatwg::url_search_params params(lst_pairs);
        CHECK(list_eq(params, output));
    }
    SUBCASE("std::map") {
        const std::map<string_t, string_t> map_pairs(TEST_ITERABLES_DATA);
        whatwg::url_search_params params(map_pairs);
        CHECK(list_eq(params, output));
    }
    SUBCASE("std::vector") {
        const std::vector<std::pair<string_t, string_t>> vec_pairs(TEST_ITERABLES_DATA);
        whatwg::url_search_params params(vec_pairs);
        CHECK(list_eq(params, output));
    }

#ifdef TEST_COROUTINE_GENERATOR
    SUBCASE("coroutine generator") {
        auto pairs_gen = []() -> generator<std::pair<string_t, string_t>> {
            const pairs_list_t<string_t> lst_pairs = TEST_ITERABLES_DATA;
            for (const auto& p : lst_pairs)
                co_yield p;
        };

        whatwg::url_search_params params(pairs_gen());
        CHECK(list_eq(params, output));
    }
#endif
}

TEST_CASE_TEMPLATE_INVOKE(test_iterables, char, wchar_t, char16_t, char32_t);
#ifdef __cpp_char8_t
TEST_CASE_TEMPLATE_INVOKE(test_iterables, char8_t);
#endif


// sort test

TEST_CASE("url_search_params::sort()") {
    struct {
        const char* comment;
        pairs_list_t<std::u32string> input;
        pairs_list_t<std::string> output;
    } lst[] = {
        {
            "Sort U+104 before U+41104",
            {{U"a\U00041104", U"2"}, {U"a\u0104", U"1"}},
            {{mk_string(u8"a\u0104"), "1"}, {mk_string(u8"a\U00041104"), "2"}},
        }, {
            "Sort U+105 before U+41104",
            {{U"a\U00041104", U"2"}, {U"a\u0105", U"1"}},
            {{mk_string(u8"a\u0105"), "1"}, {mk_string(u8"a\U00041104"), "2"}},
        }, {
            "Sort U+41104 before U+F104",
            {{U"a\uF104", U"2"}, {U"a\U00041104", U"1"}},
            {{mk_string(u8"a\U00041104"), "1"}, {mk_string(u8"a\uF104"), "2"}},
        }
    };
    for (const auto& val : lst) {
        INFO(val.comment);
        whatwg::url_search_params params(val.input);
        params.sort();
        CHECK(list_eq(params, val.output));
    }
}
