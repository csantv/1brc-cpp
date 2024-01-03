#include "sample.hpp"

#include <iostream>

void say_hello()
{
    std::cout << "Hello, World!\n";
}

auto main() -> int
{
    say_hello();
    return 0;
}
