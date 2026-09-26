#include "test_helper.hpp"

using namespace solix::test;

TEST_CASE("InterfaceDeclaration - Declarations",
          "[declarations][interface][!mayfail]") {
  SECTION("Case 3.1: Multiple Interface Conformance") {
    std::string code = R"(
interface Printable { void print(); }
interface Serializable { void save(); }

class Doc implements Printable, Serializable {
    void print() { Console.println("print"); }
    void save() { Console.println("save"); }
}
)";
    assert_compile_success(code);
  }

  SECTION("Case 4.1: Interface Method with Body") {
    std::string code = R"(
interface Reader {
    int32 read() { return 0; }
}
)";
    assert_compile_error(code, "Interface methods cannot have a body");
  }
}
