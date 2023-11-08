#include "upa/url.h"
#include "picojson_util.h"

#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

#define ANKERL_NANOBENCH_IMPLEMENT
#include "ankerl/nanobench.h"

// -----------------------------------------------------------------------------
// Read samples from text file (URL in each line) and benchmark

int benchmark_txt(const char* file_name, uint64_t min_iters) {
    std::vector<std::string> url_strings;

    // Load URL samples
    std::cout << "Load URL samples from: " << file_name << '\n';
    std::ifstream finp(file_name);
    if (!finp.is_open()) {
        std::cout << "Failed to open " << file_name << '\n';
        return 2;
    }

    std::string line;
    while (std::getline(finp, line))
        url_strings.push_back(line);

    // Run benchmark

    ankerl::nanobench::Bench().minEpochIterations(min_iters).run("Upa URL", [&] {
        upa::url url;

        for (const auto& str_url : url_strings) {
            url.parse(str_url, nullptr);

            ankerl::nanobench::doNotOptimizeAway(url);
        }
    });

    return 0;
}

// -----------------------------------------------------------------------------
// Read samples from urltestdata.json and benchmark

template <class OnArrayItem>
class root_array_context : public picojson::deny_parse_context {
    OnArrayItem on_array_item_;
public:
    root_array_context(OnArrayItem on_array_item)
        : on_array_item_(on_array_item)
    {}

    // array as root
    bool parse_array_start() { return true; }
    bool parse_array_stop(std::size_t) { return true; }

    template <typename Iter> bool parse_array_item(picojson::input<Iter>& in, std::size_t) {
        picojson::value item;

        // parse the array item
        picojson::default_parse_context ctx(&item);
        if (!picojson::_parse(ctx, in))
            return false;

        // callback with array item
        return on_array_item_(item);
    }

    // deny object as root
    bool parse_object_start() { return false; }
    bool parse_object_stop() { return false; }
};

template <typename Context>
bool load_tests(Context& ctx, const char* file_name) {
    // Load URL samples
    std::cout << "Load URL samples from: " << file_name << '\n';
    std::ifstream file(file_name, std::ios_base::in | std::ios_base::binary);
    if (!file.is_open()) {
        std::cerr << "Can't open file: " << file_name << std::endl;
        return false;
    }

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

void benchmark_wpt(const char* file_name, uint64_t min_iters) {
    // Load URL strings
    std::vector<std::pair<std::string, std::string>> url_samples;

    root_array_context context{ [&](const picojson::value& item) {
        if (item.is<picojson::object>()) {
            try {
                const picojson::object& obj = item.get<picojson::object>();
                const auto input_val = obj.at("input");
                const auto base_val = obj.at("base");

                url_samples.emplace_back(
                    input_val.get<std::string>(),
                    base_val.is<picojson::null>() ? std::string{} : base_val.get<std::string>());
            }
            catch (const std::out_of_range& ex) {
                std::cout << "[ERR:invalid file]: " << ex.what() << std::endl;
                return false;
            }
        }
        return true;
    } };

    if (!load_tests(context, file_name))
        return;

    // Run benchmark

    ankerl::nanobench::Bench().minEpochIterations(min_iters).run("Upa URL", [&] {
        upa::url url;
        upa::url url_base;

        for (const auto& url_strings : url_samples) {
            upa::url* ptr_base = nullptr;
            if (!url_strings.second.empty()) {
                if (!upa::success(url_base.parse(url_strings.second, nullptr)))
                    continue; // invalid base
                ptr_base = &url_base;
            }
            url.parse(url_strings.first, ptr_base);

            ankerl::nanobench::doNotOptimizeAway(url);
        }
    });
}

// -----------------------------------------------------------------------------

uint64_t get_positive_or_default(const char* str, uint64_t def)
{
    const uint64_t res = std::strtoull(str, nullptr, 10);
    if (res > 0)
        return res;
    return def;
}

int main(int argc, const char* argv[])
{
    constexpr uint64_t min_iters_def = 3;

    if (argc < 2) {
        std::cerr << "Usage: bench-url <file containing URLs> [<min iterations>]\n";
        return 1;
    }

    const std::filesystem::path file_name = argv[1];
    const uint64_t min_iters = argc > 2
        ? get_positive_or_default(argv[2], min_iters_def)
        : min_iters_def;

    if (file_name.extension() == ".json") {
        benchmark_wpt(file_name.string().c_str(), min_iters);
    } else if (file_name.extension() == ".txt") {
        benchmark_txt(file_name.string().c_str(), min_iters);
    } else {
        std::cerr << "File containing URLs should have .json or .txt extension.\n";
        return 1;
    }

    return 0;
}
