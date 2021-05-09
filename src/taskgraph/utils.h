#pragma once

#include <iostream>

namespace utils {
    template<typename ...Args>
    void print(Args&& ...args) {
        (std::cout << ... << args) << std::endl;
    }
}