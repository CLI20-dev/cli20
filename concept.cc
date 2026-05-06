#include <expected>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>
namespace fs = std::filesystem;

struct Args {
  // positional argument: フィールドの宣言順に割り当てられる
  argon::Positional<  //
      argon::nargs::one_or_more,
      argon::action(argon::conversion::integer)       //
          .validate(argon::validation::range<1, 10>)  //
      >
      numbers{
          .help = "One or more integers between 1 and 10",
          .presence = argon::presence::required,
      };

  argon::Arg<"filename", 'f', argon::nargs::one_or_more,
             argon::action(argon::conversion::path)
                 .validate(argon::validation::file_exists)
                 .validate(argon::validation::readable)
                 .validate(argon::validation::extension<".txt">)>
      filename{
          .help = "The input files to process",
          .presence = argon::presence::required,
      };

  // short name なしのオプション引数
  argon::Arg<"output",
             argon::nargs::one |
                 argon::action(argon::conversion::path)
                     .validate(argon::validation::parent_directory_exists)
                     .validate(argon::validation::writable_target)>
      output{
          .help = "The output file",
          .presence = argon::presence::optional,
      };

  // envvar フォールバック付きデフォルト値
  argon::Arg<"threads", 't',
             argon::nargs::one | argon::action(argon::conversion::integer)
                                     .validate(argon::validation::range<1, 10>)>
      threads{
          .help = "The number of worker threads",
          .presence = argon::presence::defaulted,
          .default_value = argon::getenvvar<int>("THREADS").value_or(4),
      };

  // flag: nargs::none で bool に変換される
  argon::Arg<"verbose", 'v',
             argon::nargs::none | argon::action(argon::conversion::flag)>
      verbose{
          .help = "Enable verbose logging",
          .presence = argon::presence::optional,
      };

  argon::Arg<"help", 'h',
             argon::nargs::none | argon::action(argon::conversion::flag)
                                      .action(argon::action::print_help)
                                      .action(argon::action::exit_success)>
      help{
          .help = "Show this help message and exit",
      };

  // サブコマンドは別の struct で定義
  struct Build {
    argon::Arg<"config", 'c',
               argon::nargs::one | argon::action(argon::conversion::path)
                                       .validate(argon::validation::file_exists)
                                       .validate(argon::validation::readable)>
        config{
            .help = "The configuration file for the build",
            .presence = argon::presence::required,
        };

    argon::Arg<"release",
               argon::nargs::none | argon::action(argon::conversion::flag)>
        release{
            .help = "Build in release mode",
            .presence = argon::presence::optional,
        };
  };

  // Command はオプショナル: args.build が nullopt なら build サブコマンド未使用
  argon::Command<"build", Build> build{
      .help = "Build the project with the specified configuration",
  };
};

int main(int argc, char** argv) {
  auto result = argon::parse<Args>(argc, argv);
  if (!result) {
    std::cerr << "error: " << result.error() << '\n';
    return 1;
  }
  auto const& args = *result;

  if (args.verbose) {
    std::cerr << "verbose mode enabled\n";
  }

  std::cout << "numbers:";
  for (int n : args.numbers) {
    std::cout << ' ' << n;
  }
  std::cout << '\n';

  std::cout << "input files:\n";
  for (fs::path const& file : args.filename) {
    std::cout << "  " << file << '\n';
  }

  if (args.output) {
    std::cout << "output: " << *args.output << '\n';
  }

  std::cout << "threads: " << args.threads << '\n';

  if (args.build) {
    std::cout << "build config: " << args.build->config << '\n';
    if (args.build->release) {
      std::cout << "release mode\n";
    }
  }

  return 0;
}
