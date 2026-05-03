// example.cc — argon argument parsing demo
//
// Try it:
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

// `build` sub-command
struct BuildArgs {
  // --target / -t <string>  : build target  (optional, default empty)
  argon::StrArg<"target", 't'> target;

  // --jobs / -j <int>       : parallel job count
  argon::IntArg<"jobs", 'j'> jobs;

  // --feature <str...>      : features to enable (zero or more)
  argon::StrListArg<"feature", 'f'> features{argon::nargs::zero_or_more};

  // --dry-run               : print what would happen, don't build
  argon::FlagArg<"dry-run"> dry_run;
};

// `push` sub-command  — --remote is required
struct PushArgs {
  argon::StrArg<"remote", 'r'> remote{argon::required};
  argon::FlagArg<"force", 'f'> force;
  argon::IntArg<"depth"> depth;
};

// `cp` sub-command  — both positionals are required
struct CpArgs {
  argon::StrPositional src{argon::required};
  argon::StrPositional dst{argon::required};
  argon::FlagArg<"recursive", 'r'> recursive;
};

// ---------------------------------------------------------------------------
// Top-level argument struct  ← the struct IS the schema
// ---------------------------------------------------------------------------

struct Args {
  argon::FlagArg<"verbose", 'v'> verbose;
  argon::IntArg<"jobs", 'j'> jobs;
  argon::StrArg<"config", 'c'> config;
  argon::BoolArg<"color"> color;  // --color true/false

  argon::Command<BuildArgs, "build"> build;
  argon::Command<PushArgs, "push"> push;
  argon::Command<CpArgs, "cp"> cp;
};

// ---------------------------------------------------------------------------
// main  — uses argon::parse<Args> free function
// ---------------------------------------------------------------------------

auto main(int argc, char** argv) -> int {
  auto res = argon::parse<Args>(argc, argv);

  if (!res) {
    std::cerr << "error: " << res.error().message() << '\n';
    return 1;
  }

  const auto& a = *res;

  // ---- global options ----
  if (a.verbose.provided()) {
    std::cout << "[global] verbose = true\n";
  }
  if (a.jobs.provided()) {
    std::cout << "[global] jobs    = " << a.jobs.value() << '\n';
  }
  if (a.config.provided()) {
    std::cout << "[global] config  = " << a.config.value() << '\n';
  }
  if (a.color.provided()) {
    std::cout << "[global] color   = " << (a.color.value() ? "true" : "false") << '\n';
  }

  // ---- build sub-command ----
  if (a.build.provided()) {
    std::cout << "[build] invoked\n";
    if (a.build.target.provided()) {
      std::cout << "[build] target   = " << a.build.target.value() << '\n';
    } else {
      std::cout << "[build] target   = (default)\n";
    }
    if (a.build.jobs.provided()) {
      std::cout << "[build] jobs     = " << a.build.jobs.value() << '\n';
    }
    if (!a.build.features.value().empty()) {
      std::cout << "[build] features =";
      for (const auto& f : a.build.features.value()) std::cout << ' ' << f;
      std::cout << '\n';
    }
    if (a.build.dry_run.provided()) {
      std::cout << "[build] dry-run  = true\n";
    }
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
    std::cout << "(no sub-command given)\n";
  }

  return 0;
}
