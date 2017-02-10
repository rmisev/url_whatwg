#include "url.h"
#include <iostream>
// conversion
#include <codecvt>
#include <locale>


template <class ...Args>
std::wstring to_wstr(Args&& ...args) {
    static std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> convW;
    return convW.from_bytes(std::forward<Args>(args)...);
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

void cout_url(const whatwg::url& url) {
    static char* part_name[whatwg::url::PART_COUNT] = {
        "SCHEME",
        "USERNAME",
        "PASSWORD",
        "HOST",
        "PORT",
        "PATH",
        "QUERY",
        "FRAGMENT"
    };

    std::wcout << "HREF: " << to_wstr(url.href()) << "\n";

    // print parts
    for (int part = whatwg::url::SCHEME; part < whatwg::url::PART_COUNT; part++) {
        std::string strPart;
        switch (part) {
        case whatwg::url::PATH:
            strPart = url.pathname();
            break;
        default:
            strPart = url.get_part(static_cast<whatwg::url::PartType>(part));
            break;
        }
        if (!strPart.empty()) {
            std::wcout << part_name[part] << ": " << to_wstr(strPart) << "\n";
        }
    }
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
    if (url.parse(str_url, base)) {
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
    whatwg::url url_base;
    if (url_base.parse(str_base, nullptr)) {
        url_testas(str_url, &url_base);
    } else {
        cout_str(str_base);  std::wcout << "\n";
        std::wcout << " ^-BASE-PARSE-FAILURE\n";
    }
}


// Main

void test_parser();
void run_unit_tests();

int main()
{
    // set user-preferred locale
    setlocale(LC_ALL, "");

    test_parser();
    run_unit_tests();

    return 0;
}

void test_parser()
{
    url_testas("file://d:/laikina/%2e./tek%stas.txt", nullptr);
    url_testas("filesystem:http://www.example.com/temporary/", nullptr);

    url_testas("ssh://example.net");

    url_testas("blob:550e8400-e29b-41d4-a716-446655440000#aboutABBA");
    url_testas("invalid^scheme://example.com", nullptr);

    // iš; https://github.com/whatwg/url/issues/162
    url_testas("http://example.com/%61%62%63a%2e%64%65%7e%7f%80%81");

    url_testas("mailto:vardenis@example.com", nullptr);

    const char* szUrl = "http://user:pass@klausimėlis.lt/?key=ąče#frag";
    url_testas(szUrl);
    // char16_t
    std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> conv16;
    url_testas(conv16.from_bytes(szUrl).c_str());
    // char32_t
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> conv32;
    url_testas(conv32.from_bytes(szUrl).c_str());
    url_testas(conv32.from_bytes("http://example.net").c_str());
    // wchar_t
    url_testas(to_wstr(szUrl).c_str());
    // --

    url_testas("http://user:pass@klausim%c4%97lis.lt/?key=ąče#frag");
    url_testas("http://user:pass@klausim%25lis.lt/?key=ąče#frag");

    url_testas("https://username:pass@word@example.com:123/path/data?abc=ąbč&key=value&key2=value2#fragid1-ą");

    url_testas("   wss\r:\n/\t/abc.lt/ \t ", nullptr);

    url_testas("file://example.com/bandymas/#123", nullptr);

    url_testas("http://example.com:8080/bandymas/#123", nullptr);
    url_testas("http://example.com:80/bandymas/?#", nullptr);

    // No need for null passwords
    // https://github.com/whatwg/url/issues/181
    url_testas("http://:@domain.lt/");
    // https://github.com/whatwg/url/pull/186
    url_testas("https://test:@test.lt/");

    // base url
    whatwg::url url_base[2];
    url_base[0].parse("http://example.org/foo/bar", nullptr);
    url_base[1].parse("http://example.org/test", nullptr);

    // https://webkit.org/blog/7086/url-parsing-in-webkit/
    // http://w3c-test.org/url/url-constructor.html
    url_testas("http://f:0/c", &url_base[0]);
    url_testas("file:a", &url_base[1]);
    url_testas("file:..", &url_base[1]);
    url_testas("https://@@@example");
    url_testas("example", &url_base[1]);

    // IPv4 testai
    url_testas("http://127.1/kelias/", nullptr);
    url_testas("http://127.0.0.1/kelias/", nullptr);
    url_testas("http://12%37.0.0.1/kelias/", nullptr);
    url_testas("http://0x7f.0.0.1/kelias/", nullptr);

    // IPv6 testai
    url_testas("http://[1:2:3:4::6:7:8]/kelias/");  // rust-url bug (fixed)
    url_testas("http://[1:2:3:4:5:6:7:8]/kelias/");
    url_testas("http://[1:2::7:8]/kelias/");
    url_testas("http://[1:2:3::]/kelias/");
    url_testas("http://[::6:7:8]/kelias/");
    url_testas("http://[0::0]");
    url_testas("http://[::]");
    url_testas("http://[0:f:0:0:f:f:0:0]");
    url_testas("http://[::1.2.3.4]");
    // bounds checking
    url_testas("http://[::1.2.3.4.5]");
    url_testas("http://[1:2:3:4:5:6:1.2.3.4.5]");
    // https://github.com/whatwg/url/issues/195
    // URL standard bugs (see: "IPv6 parser" "10.7. If c is not the EOF code point, increase pointer by one.")
    // - praleis 'X' (ar jo vietoje bet kokį ne skaitmenį) be klaidų
    url_testas("http://[::1.2.3.4X]");
    // must be failure:
    url_testas("http://[::1.2.3.]");
    url_testas("http://[::1.2.]");
    url_testas("http://[::1.]");
    
    // jsdom/whatwg-url parser BUG (fixed: https://github.com/jsdom/whatwg-url/pull/66)
    // https://quuz.org/url/ IPv6 serializer bug (no compressing trailing zeros):
    url_testas("http://[2::0]");
    url_testas("http://[2::]");

    // IDNA testai
    // http://www.unicode.org/reports/tr46/#Implementation_Notes
    url_testas("http://%E5%8D%81%zz.com/");
    url_testas("http://%C3%BF-abc.com/");

    // https://github.com/jsdom/whatwg-url/issues/50
    url_testas("https://r3---sn-p5qlsnz6.googlevideo.com/");

    // test https://url.spec.whatwg.org/#path-state
    // 1.4.1. scheme is "file", url’s path is empty, and buffer is a Windows drive letter
    url_testas("file://example.net/C:/");

    // https://url.spec.whatwg.org/#concept-url-serializer
    url_testas("file:///example.net/C:/");
    url_testas("file:/example.net/C:/");
    url_testas("file:example.net/C:/");

    // file and ? or #
    // jsdom/whatwg-url parser BUG
    url_testas("file:#frag");
    url_testas("file:?q=v");
    // papildomai
    url_testas("file:##frag");
    url_testas("file:??q=v");
    url_testas("file:#/pa/pa");
    url_testas("file:##/pa/pa");
    // only "file" scheme
    url_testas("file:");
    // kiti "file" atvejai
    url_testas("file:/");
    url_testas("file://");
    url_testas("file:///");

    // failure: empty host
    url_testas("http:#abc");

    // iš: https://github.com/whatwg/url/issues/97
    url_testas("file://y/.hostname = \"x:123\"");
    // https://github.com/whatwg/url/issues/210
    url_testas("file:///C%3A/a/b/c");
    url_testas("file:///C%7C/a/b/c");
    // mano išvesti
    url_testas("file:///c%3a/../b");
    url_testas("file:///c:/../b");
    // žr.: url_parser::parse_path(..): "d:" ne kelio pradžioje
    // turi persikelti į pradžią
    url_testas("file:///abc/../d:/../some.txt");
    // ar naršyklėse veiks (t.y. rodys failą):
    url_testas("file:///abc/../d:/some.txt");

    // UTF-8 percent encoded test
    url_testas("http://Ā©.com");
    url_testas("http://%C2%A9.com");
    url_testas("http://%C2©.com");
    url_testas("http://Ā%A9.com");
    url_testas("http://%C2%39.com");
    // https://github.com/whatwg/url/issues/215
    url_testas("http://example.com%A0");
    url_testas("http://%E2%98%83");

    // https://github.com/whatwg/url/issues/148
    url_testas("unknown://†/");
    url_testas("notspecial://H%4fSt/path");
}

#include "buffer.h"

void run_unit_tests() {
    whatwg::simple_buffer<char, 16> buff;

    std::string aaa("aaabbbccc");
    std::string bbb("-ddeXeff=");

    buff.reserve(10);
    buff.resize(3);
    std::strcpy(buff.data(), "ABC");
    //buff.shrink_to_fit();
    buff.push_back('Z');
    buff.append(aaa.cbegin(), aaa.cend());
    buff.append(bbb.cbegin(), bbb.cend());
    buff.append(bbb.cbegin(), bbb.cend());
    buff.push_back('\0');

    std::cout << buff.data();


    // IPv4 parser test
    const char* szEmpty = "";
    uint32_t ipv4 = 1;
    assert(whatwg::ipv4_parse_number(szEmpty, szEmpty, ipv4) == whatwg::RES_OK && ipv4 == 0);
    ipv4 = 1;
    assert(whatwg::ipv4_parse(szEmpty, szEmpty, ipv4) == whatwg::RES_OK && ipv4 == 0);
}
