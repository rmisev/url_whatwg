#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest/doctest.h"
#include "url_cleanup.h"

// The main() entry point
// https://github.com/onqtam/doctest/blob/master/doc/markdown/main.md

int main(int argc, char** argv) {
    doctest::Context context;

    // apply command line
    context.applyCommandLine(argc, argv);

    // run test cases
    const int res = context.run();

    // Free memory
    whatwg::url_cleanup();

    return res;
}
