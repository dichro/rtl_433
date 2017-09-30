#include <iostream>

extern "C" int c_main(int, char**);

int main(int argc, char **argv) {
    std::cout << "hello world!\n";
    return c_main(argc, argv);
}