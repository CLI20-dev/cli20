// example.cc — argon argument parsing demo
//
// Try it:
//   ./example --help
//   ./example --verbose -j 4 build --target release --jobs 8 --feature sse4 avx2
//   ./example push --remote origin --force
//   ./example --config myconf.toml push -r upstream
//   ./example cp src.txt dst.txt                     (required positionals)
//   ./example build -- --not-a-flag positional-value (-- separator)
//   ./example --color true -v                        (bool option)

#include <iostream>

#include "argon/arithmetic_argument.hh"
#include "argon/bool_argument.hh"
#include "argon/flag_argument.hh"
#include "argon/parser.hh"
#include "argon/string_argument.hh"

// ---------------------------------------------------------------------------
// Sub-command argument structs
// ---------------------------------------------------------------------------

struct BuildArgs {
  argon::description desc{"Compile the project"};
  argon::StrArg<"target", 't'> target{"Build target (e.g. release, debug)"};
  argon::IntArg<"jobs", 'j'> jobs{"Number of parallel jobs"};
  argon::StrListArg<"feature", 'f'> features{argon::nargs::zero_or_more,
                                             "Features to enable (repeatable)"};
  argon::FlagArg<"dry-run"> dry_run{"Print what would happen without building.\nNo files are written or deleted."
                                   "\nUseful for verifying flags before a real run."};
};

struct PushArgs {
  argon::StrArg<"remote", 'r'> remote{argon::required, "Remote name (required)"};
  argon::FlagArg<"force", 'f'> force{"Force push even if not fast-forward"};
  argon::IntArg<"depth"> depth{"Shallow-clone depth"};
};

struct CpArgs {
  argon::StrPositional src{argon::required, "Source file"};
  argon::StrPositional dst{argon::required, "Destination file"};
  argon::FlagArg<"recursive", 'r'> recursive{"Copy directories recursively"};
};

// ---------------------------------------------------------------------------
// Top-level argument struct
// ---------------------------------------------------------------------------

struct Args {
  argon::HelpFlag<> help{"Show this help message and exit"};
  argon::FlagArg<"verbose", 'v'> verbose{"Enable verbose output"};
  argon::IntArg<"jobs", 'j'> jobs{"Global job limit"};
  argon::StrArg<"config", 'c'> config{"Path to configuration file"};
  argon::BoolArg<"color"> color{"Enable or disable color output (true/false)"};

  argon::Command<BuildArgs, "build"> build;
  argon::Command<PushArgs, "push"> push{"Push commits to a remote"};
  argon::Command<CpArgs, "cp"> cp{"Copy a file"};
};

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

auto main(int argc, char** argv) -> int {
  argon::Parser<Args> parser;
  auto res = parser.parse(argc, argv);

  if (!res) {
    std::cerr << "error: " << res.error().message() << '\n';
    std::cerr << '\n' << parser.formatHelp();
    return 1;
  }

  const auto& a = *res;

  // --help: print help and exit successfully
  if (a.help.provided()) {
    std::cout << parser.formatHelp(argon::recurseHelp);
    return 0;
  }

  // ---- global options ----
  if (a.verbose.provided()) std::cout << "[global] verbose = true\n";
  if (a.jobs.provided()) std::cout << "[global] jobs    = " << a.jobs.value() << '\n';
  if (a.config.provided()) std::cout << "[global] config  = " << a.config.value() << '\n';
  if (a.color.provided())
    std::cout << "[global] color   = " << (a.color.value() ? "true" : "false") << '\n';

  // ---- build sub-command ----
  if (a.build.provided()) {
    std::cout << "[build] invoked\n";
    if (a.build.target.provided()) {
      std::cout << "[build] target   = " << a.build.target.value() << '\n';
    } else {
      std::cout << "[build] target   = (default)\n";
    }
    if (a.build.jobs.provided()) std::cout << "[build] jobs     = " << a.build.jobs.value() << '\n';
    if (!a.build.features.value().empty()) {
      std::cout << "[build] features =";
      for (const auto& f : a.build.features.value()) std::cout << ' ' << f;
      std::cout << '\n';
    }
    if (a.build.dry_run.provided()) std::cout << "[build] dry-run  = true\n";
  }

  // ---- push sub-command ----
  if (a.push.provided()) {
    std::cout << "[push]  remote   = " << a.push.remote.value() << '\n';
    if (a.push.force.provided()) std::cout << "[push]  force    = true\n";
    if (a.push.depth.provided()) std::cout << "[push]  depth    = " << a.push.depth.value() << '\n';
  }

  // ---- cp sub-command ----
  if (a.cp.provided()) {
    std::cout << "[cp]    src      = " << a.cp.src.value() << '\n';
    std::cout << "[cp]    dst      = " << a.cp.dst.value() << '\n';
    if (a.cp.recursive.provided()) std::cout << "[cp]    recursive = true\n";
  }

  if (!a.build.provided() && !a.push.provided() && !a.cp.provided()) {
    std::cout << "(no sub-command given — try --help)\n";
  }

  return 0;
}
