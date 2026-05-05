{
  description = "C++ template with CMake, Ninja, and Nix";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
    flake-parts.url = "github:hercules-ci/flake-parts";
    treefmt-nix.url = "github:numtide/treefmt-nix";
    treefmt-nix.inputs.nixpkgs.follows = "nixpkgs";
  };

  outputs =
    inputs@{
      flake-parts,
      treefmt-nix,
      ...
    }:
    flake-parts.lib.mkFlake { inherit inputs; } {
      systems = [
        "x86_64-linux"
        "aarch64-linux"
        "x86_64-darwin"
        "aarch64-darwin"
      ];

      imports = [
        treefmt-nix.flakeModule
      ];

      perSystem =
        { pkgs, config, ... }:
        {
          treefmt = {
            projectRootFile = "flake.nix";
            programs.clang-format = {
              enable = true;
              package = pkgs.clang-tools;
            };
          };

          devShells.default = pkgs.mkShellNoCC {
            packages = [
              pkgs.llvmPackages.libcxxClang
              pkgs.cmake
              pkgs.ninja
              (pkgs.gtest.override {
                stdenv = pkgs.libcxxStdenv;
              })
              pkgs.clang-tools
              config.treefmt.build.wrapper
            ];
          };

          packages.default = pkgs.stdenvNoCC.mkDerivation {
            name = "argon";
            src = ./.;
            nativeBuildInputs = [
              pkgs.llvmPackages.libcxxClang
              pkgs.cmake
              pkgs.ninja
              pkgs.clang-tools
            ];
            buildInputs = [
              (pkgs.gtest.override {
                stdenv = pkgs.libcxxStdenv;
              })
            ];
            cmakeFlags = [
              "-G Ninja"
              "-DCMAKE_BUILD_TYPE=Release"
              "-DCXX_ARGON_ENABLE_TEST=ON"
              "-DCXX_TEMPLATE_ENABLE_CLANG_TIDY=ON"
              "-DCXX_TEMPLATE_ENABLE_SANITIZERS=ON"
            ];
            doCheck = true;
            checkPhase = ''
              runHook preCheck
              ctest --output-on-failure
              runHook postCheck
            '';
          };

          apps.build = {
            type = "app";
            program =
              (pkgs.writeShellScript "build-cxx-template" ''
                set -euo pipefail
                if [ ! -d build ]; then
                  nix develop --command cmake -S . -B build -G Ninja \
                    -DCMAKE_BUILD_TYPE=Debug \
                    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
                fi
                nix develop --command cmake --build build
              '').outPath;
          };
        };
    };
}
