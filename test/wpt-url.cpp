// Copyright 2016-2023 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//

//
// url library tests
//

#include "url.h"
#include "url_cleanup.h"
#include "ddt/DataDrivenTest.hpp"
#include "test-utils.h"

#include "picojson_fffd.h"

#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <unicode/uversion.h>


// Test runner

bool run_parser_tests(DataDrivenTest& ddt, std::ifstream& file);
bool run_host_parser_tests(DataDrivenTest& ddt, std::ifstream& file);
bool run_idna_v2_tests(DataDrivenTest& ddt, std::ifstream& file);
bool run_setter_tests(DataDrivenTest& ddt, std::ifstream& file);
bool run_percent_encoding_tests(DataDrivenTest& ddt, std::ifstream& file);

using RunTests = bool(*)(DataDrivenTest& ddt, std::ifstream& file);
int test_from_file(RunTests run_tests, const char* file_name);

int main(int argc, char** argv)
{
    int err = 0;

    // URL web-platform-tests
    err |= test_from_file(run_parser_tests, "wpt/urltestdata.json");
    err |= test_from_file(run_host_parser_tests, "wpt/toascii.json");
    err |= test_from_file(run_setter_tests, "wpt/setters_tests.json");
    err |= test_from_file(run_percent_encoding_tests, "wpt/percent-encoding.json");
#if (U_ICU_VERSION_MAJOR_NUM) >= 66
    // Only ICU 66.1 or latter passes all IdnaTestV2.json tests
    err |= test_from_file(run_idna_v2_tests, "wpt/IdnaTestV2.json");
#endif

    // additional tests
    err |= test_from_file(run_parser_tests, "data/my-urltestdata.json");
    err |= test_from_file(run_setter_tests, "data/my-setters_tests.json");

    // Free memory
    whatwg::url_cleanup();

    return err;
}

int test_from_file(RunTests run_tests, const char* file_name)
{
    DataDrivenTest ddt;
    ddt.config_show_passed(false);
    ddt.config_debug_break(true);

    std::cout << "========== " << file_name << " ==========\n";
    std::ifstream file(file_name, std::ios_base::in | std::ios_base::binary);
    if (!file.is_open()) {
        std::cerr << "Can't open tests file: " << file_name << std::endl;
        return 4;
    }

    if (!run_tests(ddt, file))
        return 2; // JSON error

    return ddt.result();
}

// URL parser test

class ParserObj : public std::map<std::string, std::string> {
public:
    bool failure = false;
};

//
// https://github.com/web-platform-tests/wpt/blob/master/url/url-constructor.any.js
// https://github.com/web-platform-tests/wpt/blob/master/url/url-origin.any.js
//
void test_parser(DataDrivenTest& ddt, ParserObj& obj)
{
    // https://github.com/web-platform-tests/wpt/blob/master/url/README.md
    // `base`: an absolute URL as a string whose parsing without a base of its own must succeed.
    // `input`: an URL as a string to be parsed with `base` as its base URL.
    const std::string& base = obj["base"];
    const std::string& input = obj["input"];

    const std::string str_case("Parsing <" + input + "> " + (base.empty() ? "without base" : "against <" + base + ">"));

    ddt.test_case(str_case, [&](DataDrivenTest::TestCase& tc) {
        whatwg::url url;

        bool parse_success;
        if (!base.empty()) {
            whatwg::url url_base;
            parse_success =
                url_base.parse(base, nullptr) == whatwg::url_result::Ok &&
                url.parse(input, &url_base) == whatwg::url_result::Ok;
        } else {
            parse_success = url.parse(input, nullptr) == whatwg::url_result::Ok;
        }

        // check "failure"
        tc.assert_equal(obj.failure, !parse_success, "parse failure");

        // attributes
        if (parse_success && !obj.failure) {
            tc.assert_equal(obj["href"], url.href(), "href");
            auto it_origin = obj.find("origin");
            if (it_origin != obj.end())
                tc.assert_equal(it_origin->second, url.origin(), "origin");
            tc.assert_equal(obj["protocol"], url.protocol(), "protocol");
            tc.assert_equal(obj["username"], url.username(), "username");
            tc.assert_equal(obj["password"], url.password(), "password");
            tc.assert_equal(obj["host"], url.host(), "host");
            tc.assert_equal(obj["hostname"], url.hostname(), "hostname");
            tc.assert_equal(obj["port"], url.port(), "port");
            tc.assert_equal(obj["pathname"], url.pathname(), "pathname");
            tc.assert_equal(obj["search"], url.search(), "search");
            auto it_search_params = obj.find("searchParams");
            if (it_search_params != obj.end())
                tc.assert_equal(it_search_params->second, url.search_params().to_string(), "searchParams");
            tc.assert_equal(obj["hash"], url.hash(), "hash");
        }

        // https://github.com/web-platform-tests/wpt/pull/10955
        // https://github.com/web-platform-tests/wpt/blob/master/url/failure.html
        // If a URL fails to parse with any valid base, it must also fail to parse with no base,
        // i.e. when used as a base URL itself.
        if (obj.failure && !base.empty()) {
            parse_success = url.parse(input, nullptr) == whatwg::url_result::Ok;
            // check "failure"
            tc.assert_equal(obj.failure, !parse_success, "parse failure WITH NO BASE");
        }
    });
}

//
// https://github.com/web-platform-tests/wpt/blob/master/url/toascii.window.js
//
void test_host_parser(DataDrivenTest& ddt, ParserObj& obj)
{
    // Test file format (toascii.json):
    // https://github.com/web-platform-tests/wpt/tree/master/url#toasciijson
    // https://github.com/web-platform-tests/wpt/pull/5976
    static const auto make_url = [](const std::string& host)->std::string {
        std::string str_url("http://");
        str_url += host;
        str_url += "/x";
        return str_url;
    };

    const std::string& input = obj["input"];
    std::string str_case("Parse URL with host: \"" + input + "\"");

    ddt.test_case(str_case, [&](DataDrivenTest::TestCase& tc) {
        const std::string input_url(make_url(input));

        whatwg::url url;
        const bool parse_success = url.parse(input_url, nullptr) == whatwg::url_result::Ok;

        // check "failure"
        tc.assert_equal(obj.failure, !parse_success, "parse failure");

        // attributes
        if (parse_success && !obj.failure) {
            const std::string& output = obj["output"];
            const std::string output_url(make_url(output));

            tc.assert_equal(output_url, url.href(), "href");
            tc.assert_equal(output, url.host(), "host");
            tc.assert_equal(output, url.hostname(), "hostname");
            tc.assert_equal("/x", url.pathname(), "pathname");
        }
    });

    str_case = "Set URL.host to: \"" + input + "\"";
    ddt.test_case(str_case, [&](DataDrivenTest::TestCase& tc) {
        whatwg::url url(make_url("x"), nullptr);

        url.host(input);
        if (!obj.failure) {
            const std::string& output = obj["output"];
            tc.assert_equal(output, url.host(), "host");
        } else {
            tc.assert_equal("x", url.host(), "host");
        }
    });

    str_case = "Set URL.hostname to: \"" + input + "\"";
    ddt.test_case(str_case, [&](DataDrivenTest::TestCase& tc) {
        whatwg::url url(make_url("x"), nullptr);

        url.hostname(input);
        if (!obj.failure) {
            const std::string& output = obj["output"];
            tc.assert_equal(output, url.hostname(), "hostname");
        } else {
            tc.assert_equal("x", url.hostname(), "hostname");
        }
    });
}

//
// https://github.com/web-platform-tests/wpt/blob/master/url/IdnaTestV2.window.js
//
void test_idna_v2(DataDrivenTest& ddt, ParserObj& obj)
{
    // Test file format (toascii.json):
    // https://github.com/web-platform-tests/wpt/tree/master/url#toasciijson
    // https://github.com/web-platform-tests/wpt/pull/5976
    static const auto make_url = [](const std::string& host) -> std::string {
        std::string str_url("http://");
        str_url += host;
        str_url += "/x";
        return str_url;
    };

    static const auto encodeHostEndingCodePoints = [](const std::string& input) -> std::string {
        if (input.find_first_of(":/?#\\") != input.npos)
            return encodeURIComponent(input);
        return input;
    };

    const std::string& input = obj["input"];
    const std::string& comment = obj["comment"];
    std::string str_case("ToASCII(\"" + input + "\")");
    if (!comment.empty())
        str_case += " " + comment;

    ddt.test_case(str_case, [&](DataDrivenTest::TestCase& tc) {
        const std::string input_url(make_url(encodeHostEndingCodePoints(input)));

        whatwg::url url;
        const bool parse_success = url.parse(input_url, nullptr) == whatwg::url_result::Ok;

        // check "failure"
        tc.assert_equal(obj.failure, !parse_success, "parse failure");

        // attributes
        if (parse_success && !obj.failure) {
            const std::string& output = obj["output"];
            const std::string output_url(make_url(output));

            tc.assert_equal(output, url.host(), "host");
            tc.assert_equal(output, url.hostname(), "hostname");
            tc.assert_equal("/x", url.pathname(), "pathname");
            tc.assert_equal(output_url, url.href(), "href");
        }
    });
}

// URL setter test

struct SetterObj {
    explicit SetterObj(std::string setter)
        : m_setter(std::move(setter))
    {}

    std::string m_setter;
    std::string m_href;
    std::string m_new_value;
    std::map<std::string, std::string> m_expected;
};

//
// https://github.com/web-platform-tests/wpt/blob/master/url/url-setters.any.js
//
void test_setter(DataDrivenTest& ddt, SetterObj& obj)
{
    const std::string str_case("URL(\"" + obj.m_href + "\")." + obj.m_setter + "(\"" + obj.m_new_value + "\");");

    ddt.test_case(str_case, [&](DataDrivenTest::TestCase& tc) {
        // url parsing must succeed
        whatwg::url url(obj.m_href, nullptr);

        // Attributes

        // set value
        if (obj.m_setter == "href") {
            url.href(obj.m_new_value);
        } else if (obj.m_setter == "protocol") {
            url.protocol(obj.m_new_value);
        } else if (obj.m_setter == "username") {
            url.username(obj.m_new_value);
        } else if (obj.m_setter == "password") {
            url.password(obj.m_new_value);
        } else if (obj.m_setter == "host") {
            url.host(obj.m_new_value);
        } else if (obj.m_setter == "hostname") {
            url.hostname(obj.m_new_value);
        } else if (obj.m_setter == "port") {
            url.port(obj.m_new_value);
        } else if (obj.m_setter == "pathname") {
            url.pathname(obj.m_new_value);
        } else if (obj.m_setter == "search") {
            url.search(obj.m_new_value);
        } else if (obj.m_setter == "hash") {
            url.hash(obj.m_new_value);
        }

        // test result
        std::string str_val;
        for (const auto& p : obj.m_expected) {
            if (p.first == "href") str_val = url.href();
            else if (p.first == "origin") str_val = url.origin();
            else if (p.first == "protocol") str_val = url.protocol();
            else if (p.first == "username") str_val = url.username();
            else if (p.first == "password") str_val = url.password();
            else if (p.first == "host") str_val = url.host();
            else if (p.first == "hostname") str_val = url.hostname();
            else if (p.first == "port") str_val = url.port();
            else if (p.first == "pathname") str_val = url.pathname();
            else if (p.first == "search") str_val = url.search();
            else if (p.first == "hash") str_val = url.hash();

            tc.assert_equal(p.second, str_val, p.first);
        }
    });
}

// URL percent encoding test

struct EncodingObj {
    std::string m_input;
    std::map<std::string, std::string> m_output;
};

//
// https://github.com/web-platform-tests/wpt/blob/master/url/percent-encoding.window.js
//
void test_percent_encoding(DataDrivenTest& ddt, EncodingObj& obj)
{
    // URL library supports only UTF-8 encoding
    const std::string& input = obj.m_input;
    const std::string& output = obj.m_output.at("utf-8");

    const std::string str_case = input;

    ddt.test_case(str_case, [&](DataDrivenTest::TestCase& tc) {
        // test percent_encode function
        tc.assert_equal(output, whatwg::percent_encode(input, whatwg::special_query_no_encode_set), "percent_encode function");

        // test URL parser
        std::string str_url("https://example.org/?");
        str_url += input;
        str_url += "#";
        str_url += input;
        const whatwg::url url(str_url);
        tc.assert_equal("#" + output, url.hash(), "url.hash()"); // utf-8
        tc.assert_equal("?" + output, url.search(), "url.search()"); // any encoding
    });
}


// Read tests in JSON format

namespace {
    enum class TestType {
        UrlParser,
        HostParser,
        IdnaTestV2
    };

    // parses urltestdata.json, toascii.json
    class root_context : public picojson::deny_parse_context {
        DataDrivenTest& m_ddt;
        TestType m_ttype;
    public:
        root_context(DataDrivenTest& ddt, TestType ttype)
            : m_ddt(ddt)
            , m_ttype(ttype)
        {}

        // array only as root
        bool parse_array_start() { return true; }
        bool parse_array_stop(std::size_t) { return true; }

        template <typename Iter> bool parse_array_item(picojson::input<Iter>& in, std::size_t) {
            picojson::value item;

            // parse the array item
            picojson::default_parse_context ctx(&item);
            if (!picojson::_parse(ctx, in))
                return false;

            // analyze array item
            if (item.is<picojson::object>()) {
                ParserObj obj;

                for (const auto& p : item.get<picojson::object>()) {
                    switch (m_ttype) {
                    case TestType::UrlParser:
                        // boolean fields
                        if (p.first == "failure") {
                            obj.failure = p.second.evaluate_as_boolean();
                            continue;
                        }
                        if (p.first == "relativeTo") {
                            // skip field intended for browsers only; see:
                            // https://github.com/web-platform-tests/wpt/pull/39203
                            // https://github.com/web-platform-tests/wpt/blob/master/url/failure.html
                            // https://github.com/web-platform-tests/wpt/blob/master/url/resources/a-element.js
                            continue;
                        }
                        break;
                    case TestType::HostParser:
                        if (p.first == "output" && p.second.is<picojson::null>()) {
                            obj.failure = true;
                            continue;
                        }
                        break;
                    case TestType::IdnaTestV2:
                        if (p.first == "input" && p.second.is<std::string>() && p.second.get<std::string>().empty()) {
                            return true; // cannot test empty string input through new URL()
                        }
                        if (p.first == "output" && p.second.is<picojson::null>()) {
                            obj.failure = true;
                            continue;
                        }
                        break;
                    }
                    // do not store null values
                    if (p.second.is<picojson::null>())
                        continue;
                    // string fields
                    if (!p.second.is<std::string>())
                        return false; // error: need string
                    obj[p.first] = p.second.get<std::string>();
                }
                // run item test
                switch (m_ttype) {
                case TestType::UrlParser:
                    test_parser(m_ddt, obj);
                    break;
                case TestType::HostParser:
                    test_host_parser(m_ddt, obj);
                    break;
                case TestType::IdnaTestV2:
                    test_idna_v2(m_ddt, obj);
                    break;
                }
            } else if (item.is<std::string>()) {
                // comment
                // std::cout << item.get<std::string>() << std::endl;
            } else {
                std::cout << "[ERR: invalid file]" << std::endl;
                return false;
            }
            return true;
        }

        // deny object as root
        bool parse_object_start() { return false; }
        bool parse_object_stop() { return false; }
    };

    // parses setters_tests.json
    class root_context_setter : public picojson::deny_parse_context {
        DataDrivenTest& m_ddt;
        std::string m_setter_name;
    public:
        explicit root_context_setter(DataDrivenTest& ddt) : m_ddt(ddt) {}

        // array only as root
        bool parse_array_start() { return true; }
        bool parse_array_stop(std::size_t) { return true; }

        template <typename Iter> bool parse_array_item(picojson::input<Iter>& in, std::size_t) {
            picojson::value item;

            // parse the array item
            picojson::default_parse_context ctx(&item);
            if (!picojson::_parse(ctx, in))
                return false;

            // analyze array item
            if (item.is<picojson::object>()) {
                SetterObj obj(m_setter_name);

                try {
                    const picojson::object& o = item.get<picojson::object>();
                    obj.m_href = o.at("href").get<std::string>();
                    obj.m_new_value = o.at("new_value").get<std::string>();
                    const picojson::object& oexp = o.at("expected").get<picojson::object>();
                    for (const auto& pexp : oexp) {
                        if (!pexp.second.is<std::string>())
                            return false; // error: string is expected
                        obj.m_expected[pexp.first] = pexp.second.get<std::string>();
                    }
                }
                catch (const std::out_of_range& ex) {
                    std::cout << "[ERR:invalid file]: " << ex.what() << std::endl;
                    return false;
                }

                test_setter(m_ddt, obj);
            } else {
                std::cout << "[ERR: invalid file]" << std::endl;
                return false;
            }
            return true;
        }

        // object only as root
        bool parse_object_start() { return true; }
        bool parse_object_stop() { return true; }

        template <typename Iter> bool parse_object_item(picojson::input<Iter>& in, const std::string& name) {
            if (name == "comment") {
                // skip
                picojson::null_parse_context nullctx;
                return picojson::_parse(nullctx, in);
            }
            m_setter_name = name;
            // parse array
            return picojson::_parse(*this, in);
        }
    };

    // parses percent-encoding.json
    class root_context_percent_encoding : public picojson::deny_parse_context {
    protected:
        DataDrivenTest& m_ddt;
    public:
        explicit root_context_percent_encoding(DataDrivenTest& ddt)
            : m_ddt(ddt)
        {}

        // array only as root
        bool parse_array_start() { return true; }
        bool parse_array_stop(std::size_t) { return true; }

        template <typename Iter> bool parse_array_item(picojson::input<Iter>& in, std::size_t) {
            picojson::value item;

            // parse the array item
            picojson::default_parse_context ctx(&item);
            if (!picojson::_parse(ctx, in))
                return false;

            // analyze array item
            if (item.is<picojson::object>()) {
                EncodingObj obj;

                try {
                    const picojson::object& o = item.get<picojson::object>();
                    obj.m_input = o.at("input").get<std::string>();
                    const picojson::object& o_output = o.at("output").get<picojson::object>();
                    for (const auto& p : o_output) {
                        if (!p.second.is<std::string>())
                            return false; // error: string is expected
                        obj.m_output[p.first] = p.second.get<std::string>();
                    }
                }
                catch (const std::out_of_range& ex) {
                    std::cout << "[ERR:invalid file]: " << ex.what() << std::endl;
                    return false;
                }

                test_percent_encoding(m_ddt, obj);
            } else if (item.is<std::string>()) {
                // comment
                // std::cout << item.get<std::string>() << std::endl;
            } else {
                std::cout << "[ERR: invalid file]" << std::endl;
                return false;
            }
            return true;
        }

        // deny object as root
        bool parse_object_start() { return false; }
        bool parse_object_stop() { return false; }
    };
} // namespace

template <typename Context>
bool run_some_tests(Context &ctx, std::ifstream& file) {
    std::string err;

    // for unformatted reading use std::istreambuf_iterator
    // http://stackoverflow.com/a/17776228/3908097
    picojson::_parse(ctx, std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>(), &err);

    if (!err.empty()) {
        std::cerr << err << std::endl;
        return false;
    }
    return true;
}

bool run_parser_tests(DataDrivenTest& ddt, std::ifstream& file) {
    root_context ctx(ddt, TestType::UrlParser);
    return run_some_tests(ctx, file);
}

bool run_host_parser_tests(DataDrivenTest& ddt, std::ifstream& file) {
    root_context ctx(ddt, TestType::HostParser);
    return run_some_tests(ctx, file);
}

bool run_idna_v2_tests(DataDrivenTest& ddt, std::ifstream& file) {
    root_context ctx(ddt, TestType::IdnaTestV2);
    return run_some_tests(ctx, file);
}

bool run_setter_tests(DataDrivenTest& ddt, std::ifstream& file) {
    root_context_setter ctx(ddt);
    return run_some_tests(ctx, file);
}

bool run_percent_encoding_tests(DataDrivenTest& ddt, std::ifstream& file) {
    root_context_percent_encoding ctx(ddt);
    return run_some_tests(ctx, file);
}
