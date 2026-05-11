// unicode_echo.cc — integration-test helper for Windows wmain Unicode support.
// Parses --name and positional file arguments, then echoes each value to
// stdout in "key=value\n" format so CI scripts can compare output.
//
// Build and run via wmain on Windows; falls back to main elsewhere.

#include <iostream>

#include "cli/argument.hh"
#include "cli/parser.hh"

struct Args {
  cli::StringOption<"name", 'n'> name{{.help = "Name to echo"}};
  cli::Positional<std::string, cli::nargs::zero_or_more> files{
      {.help = "Files to echo"}};
};

#ifdef _WIN32
auto wmain(int argc, wchar_t* argv[]) -> int {
#else
auto main(int argc, char* argv[]) -> int {
#endif
  const auto args = cli::parse_or_exit<Args>(argc, argv);

  if (args.name) {
    std::cout << "name=" << *args.name << '\n';
  }
  for (const auto& f : args.files.value()) {
    std::cout << "file=" << f << '\n';
  }
  return 0;
}
