auto main() -> int { return 0; }
// // example_single_header.cc — cli20 single-header demo
//
// #include <iostream>
//
// #include "cli20.hh"
//
// struct BuildArgs {
//   cli::StrArg<"target", 't'> target;
//   cli::IntArg<"jobs", 'j'> jobs;
//   cli::StrListArg<"feature", 'f'> features{cli::nargs::zero_or_more};
//   cli::FlagArg<"dry-run"> dry_run;
// };
//
// struct PushArgs {
//   cli::StrArg<"remote", 'r'> remote{cli::required};
//   cli::FlagArg<"force", 'f'> force;
//   cli::IntArg<"depth"> depth;
// };
//
// struct CpArgs {
//   cli::StrPositional src{cli::required};
//   cli::StrPositional dst{cli::required};
//   cli::FlagArg<"recursive", 'r'> recursive;
// };
//
// struct Args {
//   cli::FlagArg<"verbose", 'v'> verbose;
//   cli::IntArg<"jobs", 'j'> jobs;
//   cli::StrArg<"config", 'c'> config;
//   cli::BoolArg<"color"> color;
//
//   cli::Command<BuildArgs, "build"> build;
//   cli::Command<PushArgs, "push"> push;
//   cli::Command<CpArgs, "cp"> cp;
// };
//
// auto main(int argc, char** argv) -> int {
//   auto res = cli::parse<Args>(argc, argv);
//
//   if (!res) {
//     std::cerr << "error: " << res.error().message() << '\n';
//     return 1;
//   }
//
//   const auto& a = *res;
//
//   if (a.verbose.provided()) {
//     std::cout << "[global] verbose = true\n";
//   }
//   if (a.jobs.provided()) {
//     std::cout << "[global] jobs    = " << a.jobs.value() << '\n';
//   }
//   if (a.config.provided()) {
//     std::cout << "[global] config  = " << a.config.value() << '\n';
//   }
//   if (a.color.provided()) {
//     std::cout << "[global] color   = " << (a.color.value() ? "true" : "false")
//     << '\n';
//   }
//
//   if (a.build.provided()) {
//     std::cout << "[build] invoked\n";
//     if (a.build.target.provided()) {
//       std::cout << "[build] target   = " << a.build.target.value() << '\n';
//     } else {
//       std::cout << "[build] target   = (default)\n";
//     }
//     if (a.build.jobs.provided()) {
//       std::cout << "[build] jobs     = " << a.build.jobs.value() << '\n';
//     }
//     if (!a.build.features.value().empty()) {
//       std::cout << "[build] features =";
//       for (const auto& f : a.build.features.value()) std::cout << ' ' << f;
//       std::cout << '\n';
//     }
//     if (a.build.dry_run.provided()) {
//       std::cout << "[build] dry-run  = true\n";
//     }
//   }
//
//   if (a.push.provided()) {
//     std::cout << "[push]  remote   = " << a.push.remote.value() << '\n';
//     if (a.push.force.provided()) std::cout << "[push]  force    = true\n";
//     if (a.push.depth.provided()) std::cout << "[push]  depth    = " <<
//     a.push.depth.value() <<
//     '\n';
//   }
//
//   if (a.cp.provided()) {
//     std::cout << "[cp]    src      = " << a.cp.src.value() << '\n';
//     std::cout << "[cp]    dst      = " << a.cp.dst.value() << '\n';
//     if (a.cp.recursive.provided()) std::cout << "[cp]    recursive = true\n";
//   }
//
//   if (!a.build.provided() && !a.push.provided() && !a.cp.provided()) {
//     std::cout << "(no sub-command given)\n";
//   }
//
//   return 0;
// }
