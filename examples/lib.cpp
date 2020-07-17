#include <iostream>
#include "../runtime/runtime.hpp"

extern "C" {
    void println(char *str) {
        std::cout << str << std::endl;
    }

    void print_int(int i) {
        std::cout << i << std::endl;
    }

    void print_float(double f) {
        std::cout << f << std::endl;
    }

    void run_collect() {
        collect();
    }
}
