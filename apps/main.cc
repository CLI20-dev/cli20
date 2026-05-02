#include <iostream>

// #include "argon.hh"
#include "argon/argument.hh"

// template <class T>
// struct String {};
//
// struct Args {
//   consteval Args(std::string_view str) : option1(str) {
//     if (str.starts_with("Hello")) {
//       throw std::runtime_error("String cannot start with 'Hello'");
//     }
//   }
//
//   std::string_view option1;
//   auto print() const { std::cout << "A: " << option1 << std::endl; }
// };
//
// struct Argument {
//   argon::Arg<int> arg1 = {"arg1", "a"};
//   argon::Arg<int> arg2 = {"arg2", "b"};
//   struct Argument2 {
//     argon::Arg<int> arg1 = {"arg1", "a"};
//     argon::Arg<int> arg2 = {"arg2", "b"};
//   } sub_parser;
// };

auto main([[maybe_unused]] int argc, [[maybe_unused]] char** argv) -> int {
  // auto _ = argon::Failure<true>{};

  // auto option_name = "Hello, World!";
  //
  // // auto a = A("Hell, World!");
  // auto a = A(option_name);
  // a.print();
  //
  // // auto res = argon::parse<Argument>(argc, argv);
  // //
  // // if (!res) {
  // //   std::cerr << "Error parsing arguments: " << res.error().message() << std::endl;
  // //   return 1;
  // // }
  // //
  // // std::cout << "arg1: " << res->arg1.value << std::endl;
  // // std::cout << "arg2: " << res->arg2.value << std::endl;
  // //
  return 0;
}
