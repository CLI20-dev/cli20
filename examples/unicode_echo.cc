// unicode_echo.cc — integration-test helper for Windows wmain Unicode support.
// Parses --name and positional file arguments, then echoes each value to
// stdout in "key=value\n" format so CI scripts can compare output.
//
// Windows converts wmain's UTF-16 argv explicitly, then uses the normal UTF-8
// parser path. Non-Windows builds use main directly.

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
  auto argv_utf8 = cli::utf16_to_utf8(argc, argv);
  const auto args = cli::parse_or_exit<Args>(argv_utf8.size(), argv_utf8.data());
#else
auto main(int argc, char* argv[]) -> int {
  const auto args = cli::parse_or_exit<Args>(argc, argv);
#endif

  if (args.name) {
    std::cout << "name=" << *args.name << '\n';
  }
  for (const auto& f : args.files.value()) {
    std::cout << "file=" << f << '\n';
  }
  return 0;
}
