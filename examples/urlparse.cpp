// Copyright 2016-2019 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//

#include "url.h"
#include "url_search_params.h"
#include <fstream>
#include <iostream>
// conversion
#include <codecvt>
#include <locale>
// json
#include "json_writer/json_writer.h"
// https://github.com/kazuho/picojson
# include "picojson/picojson.h"


template <class ...Args>
std::wstring to_wstr(Args&& ...args) {
    try {
        static std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> convW;
        return convW.from_bytes(std::forward<Args>(args)...);
    }
    catch (std::exception&) {
        return L"<invalid UTF-8>";
    }
}

std::wstring to_wstr(whatwg::url::str_view_type sv) {
    // std::wstring_convert::from_bytes doesn't support std::string_view
    // https://en.cppreference.com/w/cpp/locale/wstring_convert/from_bytes
    return to_wstr(sv.data(), sv.data() + sv.length());
}

void cout_str(const wchar_t* str) {
    std::wcout << str;
}

void cout_str(const char* str) {
    std::wcout << to_wstr(str);
}

void cout_str(const char16_t* str) {
    std::wcout << (const wchar_t*)str;
}

void cout_str(const char32_t* str) {
    for (; *str; str++)
        std::wcout.put(static_cast<wchar_t>(*str));
}

template <class T>
void cout_name_str(const char* name, T&& str) {
    if (!str.empty()) {
        std::wcout << name << ": " << to_wstr(std::forward<T>(str)) << "\n";
    }
}

void cout_host_type(const whatwg::url& url){
    const char* szHostType;
    if (url.is_null(whatwg::url::HOST)) {
        szHostType = "null";
    } else {
        switch (url.host_type()) {
        case whatwg::HostType::Empty: szHostType = "Empty"; break;
        case whatwg::HostType::Opaque: szHostType = "Opaque"; break;
        case whatwg::HostType::Domain: szHostType = "Domain"; break;
        case whatwg::HostType::IPv4: szHostType = "IPv4"; break;
        case whatwg::HostType::IPv6: szHostType = "IPv6"; break;
        }
    }
    std::wcout << "host_type: " << szHostType << "\n";
}

void cout_url(const whatwg::url& url) {
#if 1
    cout_name_str("HREF", url.href());
    cout_name_str("origin", url.origin());

    cout_name_str("protocol", url.protocol());
    cout_name_str("username", url.username());
    cout_name_str("password", url.password());
    cout_host_type(url);
    cout_name_str("host", url.host());
    cout_name_str("hostname", url.hostname());
    cout_name_str("port", url.port());
    cout_name_str("path", url.path());
    cout_name_str("pathname", url.pathname());
    cout_name_str("search", url.search());
    cout_name_str("hash", url.hash());
#else
    static const std::initializer_list<std::pair<whatwg::url::PartType, const char*>> parts{
        { whatwg::url::SCHEME, "SCHEME" },
        { whatwg::url::USERNAME, "USERNAME" },
        { whatwg::url::PASSWORD, "PASSWORD" },
        { whatwg::url::HOST, "HOST" },
        { whatwg::url::PORT, "PORT" },
        { whatwg::url::PATH, "PATH" },
        { whatwg::url::QUERY, "QUERY" },
        { whatwg::url::FRAGMENT, "FRAGMENT" }
    };

    cout_name_str("HREF", url.href());
    for (const auto& part : parts) {
        if (part.first == whatwg::url::HOST)
            cout_host_type(url);
        cout_name_str(part.second, url.get_part_view(part.first));
    }
#endif
}

void cout_url_eol(const whatwg::url& url) {
    cout_url(url);
    std::wcout << std::endl;
}

template <typename CharT>
void url_testas(const CharT* str_url, whatwg::url* base = nullptr)
{
    // source data
    cout_str(str_url);  std::wcout << "\n";
    if (base) {
        std::wcout << "BASE: " << to_wstr(base->href()) << "\n";
    }

    // url parse result
    whatwg::url url;
    if (whatwg::success(url.parse(str_url, base))) {
        // serialized
        cout_url(url);
    } else {
        std::wcout << " ^--FAILURE\n";
    }
    std::wcout << std::endl;
}

template <typename CharT>
void url_testas(const CharT* str_url, const CharT* str_base)
{
    if (str_base) {
        whatwg::url url_base;
        if (whatwg::success(url_base.parse(str_base, nullptr))) {
            url_testas(str_url, &url_base);
        } else {
            cout_str(str_base);  std::wcout << "\n";
            std::wcout << " ^-BASE-PARSE-FAILURE\n";
        }
    } else {
        url_testas(str_url);
    }
}

template <typename CharT>
void url_parse_to_json(json_writer& json, const CharT* str_url, whatwg::url* base = nullptr)
{
    json.object_start();

    json.name("input"); json.value(str_url);
    if (base) {
        json.name("base"); json.value(base->href());
    }

    // url parse result
    whatwg::url url;
    if (whatwg::success(url.parse(str_url, base))) {
        json.name("href");     json.value(url.href());
        json.name("origin");   json.value(url.origin()); // ne visada reikia
        json.name("protocol"); json.value(url.protocol());
        json.name("username"); json.value(url.username());
        json.name("password"); json.value(url.password());
        json.name("host");     json.value(url.host());
        json.name("hostname"); json.value(url.hostname());
        json.name("port");     json.value(url.port());
        json.name("pathname"); json.value(url.pathname());
        json.name("search");   json.value(url.search());
        json.name("hash");     json.value(url.hash());
    } else {
        json.name("failure");  json.value_bool(true);
    }

    json.object_end();
}

class SamplesOutput {
public:
    virtual bool open() { return true; };
    virtual void close() {};
    virtual void comment(whatwg::url::str_view_type sv) {
        auto strw = to_wstr(sv);
        std::wcout << strw << std::endl;
        for (auto c : strw) std::wcout << '~';
        std::wcout << std::endl;
    }
    virtual void output(const char* str_url, whatwg::url* base) {
        url_testas(str_url, base);
    }
};

class SamplesOutputJson : public SamplesOutput {
public:
    SamplesOutputJson(std::string&& fname)
        : fname_(std::forward<std::string>(fname))
        , json_(fout_, 2)
    {}

    bool open() override {
        fout_.open(fname_, std::ios_base::out | std::ios_base::binary);
        if (!fout_.is_open()) {
            std::cerr << "Can't create results file: " << fname_ << std::endl;
            return false;
        }
        json_.array_start();
        return true;
    };

    void close() override {
        json_.array_end();
    }

    void comment(whatwg::url::str_view_type sv) override {
        json_.value(sv.data(), sv.data() + sv.length());
    }
    void output(const char* str_url, whatwg::url* base) override {
        url_parse_to_json(json_, str_url, base);
    }
private:
    std::string fname_;
    std::ofstream fout_;
    json_writer json_;
};

/*
 * URL samples reader
 *
 * File format:

COMMENT:<comment>
BASE:<base URL>
URL:
<url1>
"<url2 as JSON string>"

SET:<setter name>
url:<URL to parse>
val:<new value>

**/

bool read_setter(std::ifstream& file, const char* name, const char* name_end);

void read_samples(const char* file_name, SamplesOutput& out)
{
    std::cout << "========== " << file_name << " ==========\n";
    std::ifstream file(file_name, std::ios_base::in);
    if (!file.is_open()) {
        std::cerr << "Can't open samples file: " << file_name << std::endl;
        return;
    }

    if (!out.open())
        return;

    enum class State {
        header, url
    } state = State::header;
    whatwg::url url_base;

    std::string line;
    while (std::getline(file, line)) {
        switch (state) {
        case State::header: {
            bool ok = true;
            auto icolon = line.find(':');
            if (icolon != line.npos) {
                whatwg::url::str_view_type val{line.data() + icolon + 1, line.length() - (icolon + 1) };
                if (line.compare(0, icolon, "BASE") == 0) {
                    const auto res = url_base.parse(val, nullptr);
                    ok = whatwg::success(res);
                } else if (line.compare(0, icolon, "COMMENT") == 0) {
                    out.comment(val);
                } else if (line.compare(0, icolon, "URL") == 0) {
                    state = State::url;
                } else if (line.compare(0, icolon, "SET") == 0) {
                    if (!read_setter(file, val.data(), val.data() + val.length()))
                        return;
                } else {
                    ok = false;
                }
            } else {
                ok = false;
            }
            if (!ok) {
                std::cerr << "Error in header" << std::endl;
                return;
            }
            break;
        }
        case State::url: {
            if (line.length() > 0) {
                if (line[0] == '"') {
                    // parse JSON string
                    picojson::value v;
                    std::string err = picojson::parse(v, line);
                    if (!err.empty()) {
                        std::cerr << "Skip invalid line: " << line << std::endl;
                        continue;
                    }
                    line = v.get<std::string>();
                }
                out.output(line.c_str(), (url_base.href().empty() ? nullptr : &url_base));
            } else {
                state = State::header;
                url_base.clear();
            }
            break;
        }
        }
    }

    out.close();
}

inline static void AsciiTrimWhiteSpace(const char*& first, const char*& last) {
    auto ascii_ws = [](char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; };
    // trim space
    while (first < last && ascii_ws(*first)) first++;
    while (first < last && ascii_ws(*(last - 1))) last--;
}

bool read_setter(std::ifstream& file, const char* name, const char* name_end) {
    AsciiTrimWhiteSpace(name, name_end);
    std::string strName(name, name_end);

    whatwg::url url;

    std::string line;
    bool ok = true;
    while (std::getline(file, line)) {
        if (line.length() == 0) break;
        auto icolon = line.find(':');
        if (icolon != line.npos) {
            whatwg::url::str_view_type val{ line.data() + icolon + 1, line.length() - (icolon + 1) };
            if (line.compare(0, icolon, "url") == 0) {
                std::cout << "URL=" << val << std::endl;
                ok = whatwg::success(url.parse(val, nullptr));
            } else if (line.compare(0, icolon, "val") == 0) {
                // set value
                if (strName == "protocol") {
                    url.protocol(val);
                } else if (strName == "username") {
                    url.username(val);
                } else if (strName == "password") {
                    url.password(val);
                } else if (strName == "host") {
                    url.host(val);
                } else if (strName == "hostname") {
                    url.hostname(val);
                } else if (strName == "port") {
                    url.port(val);
                } else if (strName == "pathname") {
                    url.pathname(val);
                } else if (strName == "search") {
                    url.search(val);
                } else if (strName == "hash") {
                    url.hash(val);
                } else {
                    std::cerr << "Unknown setter: " << strName << std::endl;
                    return false;
                }
                std::cout << strName << "=" << val << std::endl;
                cout_url_eol(url);
            }
        }
        if (!ok) {
            std::cerr << "Error in line:\n" << line << std::endl;
            break;
        }
    }
    return ok;
}

bool AsciiEqualsIgnoreCase(const char* test, const char* lcase) {
    auto ascii_lc = [](char c) { return (c >= 'A' && c <= 'Z') ? c + ('a' - 'A') : c; };
    while (*test && ascii_lc(*test) == *lcase) test++, lcase++;
    return *test == 0 && *lcase == 0;
}

const char* end_of_file_name(const char* fname) {
    const char* last = fname + std::strlen(fname);
    for (const char* p = last; p > fname; ) {
        p--;
        switch (p[0]) {
        case '.':
            return p;
        case '/':
        case '\\':
            return last;
        }
    }
    return last;
}

void read_samples(const char* file_name) {
    const char* ext = end_of_file_name(file_name);
    if (!AsciiEqualsIgnoreCase(ext, ".json")) {
        std::string fn_out(file_name, ext);
        fn_out.append(".json");
        SamplesOutputJson out(std::move(fn_out));
        read_samples(file_name, out);
    } else {
        std::cerr << "Samples file can not be .json: " << file_name << std::endl;
    }
}

// interactive url parsing

void test_interactive(const char* szBaseUrl)
{
    std::cout << "Enter URL; enter empty line to exit\n";

    std::string str;
    while (std::getline(std::cin, str)) {
        if (str.empty()) break;
        url_testas(str.c_str(), szBaseUrl);
    }
}

// Main

int main(int argc, char *argv[])
{
    // set user-preferred locale
    setlocale(LC_ALL, "");

    if (argc > 1) {
        const char* flag = argv[1];
        if (flag[0] == '-') {
            switch (flag[1]) {
            case 'g':
                if (argc > 2) {
                    read_samples(argv[2]);
                    return 0;
                }
            case 't':
                if (argc > 2) {
                    SamplesOutput out;
                    read_samples(argv[2], out);
                    return 0;
                }
            }
        } else if (argc == 2) {
            test_interactive(argv[1]);
            return 0;
        }
    } else {
        test_interactive(nullptr);
        return 0;
    }
    std::cerr <<
        "urlparse [<base URL>]\n"
        "urlparse -g <samples file>\n"
        "urlparse -t <samples file>\n"
        "\n"
        " Without options - read URL samples form console and output to console\n"
        " -g  Read samples and output to the same name file with .json extension\n"
        " -t  Read samples and output to console\n";
    return 0;
}
