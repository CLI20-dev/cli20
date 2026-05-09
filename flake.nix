{
  description = "cli20: a C++20-native command line parser";

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
        let
          version = builtins.replaceStrings [ "\n" ] [ "" ] (builtins.readFile ./VERSION);
        in
        rec {
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
              pkgs.nodejs_latest
              pkgs.clang-tools
              pkgs.doxygen
              pkgs.emscripten
              config.treefmt.build.wrapper
            ];
          };

          packages.default = pkgs.stdenvNoCC.mkDerivation {
            pname = "cli20";
            inherit version;
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
              "-DCXX_CLI20_ENABLE_TEST=ON"
              "-DCXX_CLI20_ENABLE_CLANG_TIDY=ON"
              "-DCXX_CLI20_ENABLE_SANITIZERS=OFF"
            ];
            doCheck = true;
            checkPhase = ''
              runHook preCheck
              ctest --output-on-failure
              runHook postCheck
            '';
          };

          legacyPackages.buildDoc =
            { baseUrl }:
            pkgs.buildNpmPackage {
              pname = "cli20-docs";
              inherit version;
              # Full repo source: the remark plugin reads examples/*.cc and
              # executes build/examples/* at build time, so it needs access to
              # the whole repository, not just the docs/ subdirectory.
              src = ./.;
              postUnpack = ''sourceRoot="$sourceRoot/docs"'';
              DOCUSAURUS_BASE_URL = baseUrl;
              # npmDeps must reference docs/ explicitly so fetchNpmDeps finds
              # package-lock.json there, not at the repo root.
              npmDeps = pkgs.fetchNpmDeps {
                name = "cli20-docs-0.0.0-npm-deps";
                src = ./docs;
                hash = "sha256-DOI2KP2qq8zbUjX3Kktgfe16n567pNkkGz5ABGSgSdo=";
              };
              nativeBuildInputs = [
                pkgs.emscripten
                pkgs.doxygen
              ];
              buildPhase = ''
                runHook preBuild
                mkdir -p .emcc-cache
                export EM_CACHE=$(pwd)/.emcc-cache
                npm run build
                runHook postBuild
              '';
              installPhase = ''
                runHook preInstall
                cp -r build/. $out/
                runHook postInstall
              '';
            };

          packages.doc = legacyPackages.buildDoc {
            baseUrl = "/cli20/";
          };

          apps.build = {
            type = "app";
            program =
              (pkgs.writeShellScript "build-cli20" ''
                set -euo pipefail
                if [ ! -d build ]; then
                  nix develop --command cmake -S . -B build -G Ninja \
                    -DCMAKE_BUILD_TYPE=Debug \
                    -DCXX_CLI20_ENABLE_TEST=ON \
                    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
                fi
                nix develop --command cmake --build build
              '').outPath;
          };

          apps.doc = {
            type = "app";
            program = "${pkgs.writeShellScriptBin "build-cli20-doc" ''
              set -eu

              if [ "$#" -lt 1 ] || [ "$#" -gt 2 ]; then
                echo "usage: nix run .#doc -- BASE_URL [OUT_LINK]" >&2
                exit 2
              fi

              base_url="$1"
              out_link="''${2:-result}"

              nix build --impure -L --out-link "$out_link" --expr "
                (builtins.getFlake (toString ./.)).legacyPackages.${pkgs.system}.buildDoc {
                  baseUrl = \"$base_url\";
                }
              "
            ''}/bin/build-cli20-doc";
          };
        };
    };
}
