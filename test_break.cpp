#include <iostream>
#include "solix/parser.hpp"
int main() {
    solix::parser::AstTree tree;
    try {
        tree.include(std::string_view("class Test { public void method() { break; } }"));
        std::cout << "Parsed OK" << std::endl;
    } catch(const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
    }
    return 0;
}
