#include <expected>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>
namespace fs = std::filesystem;

struct Args {
  // positional argument: フィールドの宣言順に割り当てられる
  cli::Positional<  //
      cli::nargs::one_or_more,
      cli::action(cli::conversion::integer)         //
          .validate(cli::validation::range<1, 10>)  //
      >
      numbers{
          .help = "One or more integers between 1 and 10",
          .presence = cli::presence::required,
      };

  cli::Arg<"filename", 'f', cli::nargs::one_or_more,
           cli::action(cli::conversion::path)
               .validate(cli::validation::file_exists)
               .validate(cli::validation::readable)
               .validate(cli::validation::extension<".txt">)>
      filename{
          .help = "The input files to process",
          .presence = cli::presence::required,
      };

  // short name なしのオプション引数
  cli::Arg<"output", cli::nargs::one |
                         cli::action(cli::conversion::path)
                             .validate(cli::validation::parent_directory_exists)
                             .validate(cli::validation::writable_target)>
      output{
          .help = "The output file",
          .presence = cli::presence::optional,
      };

  // envvar フォールバック付きデフォルト値
  cli::Arg<"threads", 't',
           cli::nargs::one | cli::action(cli::conversion::integer)
                                 .validate(cli::validation::range<1, 10>)>
      threads{
          .help = "The number of worker threads",
          .presence = cli::presence::defaulted,
          .default_value = cli::getenvvar<int>("THREADS").value_or(4),
      };

  // flag: nargs::none で bool に変換される
  cli::Arg<"verbose", 'v', cli::nargs::none | cli::action(cli::conversion::flag)>
      verbose{
          .help = "Enable verbose logging",
          .presence = cli::presence::optional,
      };

  cli::Arg<"help", 'h',
           cli::nargs::none | cli::action(cli::conversion::flag)
                                  .action(cli::action::print_help)
                                  .action(cli::action::exit_success)>
      help{
          .help = "Show this help message and exit",
      };

  // サブコマンドは別の struct で定義
  struct Build {
    cli::Arg<"config", 'c',
             cli::nargs::one | cli::action(cli::conversion::path)
                                   .validate(cli::validation::file_exists)
                                   .validate(cli::validation::readable)>
        config{
            .help = "The configuration file for the build",
            .presence = cli::presence::required,
        };

    cli::Arg<"release", cli::nargs::none | cli::action(cli::conversion::flag)>
        release{
            .help = "Build in release mode",
            .presence = cli::presence::optional,
        };
  };

  // Command はオプショナル: args.build が nullopt なら build サブコマンド未使用
  cli::Command<"build", Build> build{
      .help = "Build the project with the specified configuration",
  };
};

int main(int argc, char** argv) {
  auto result = cli::parse<Args>(argc, argv);
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
