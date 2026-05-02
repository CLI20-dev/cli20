{
  description = "C++ template with CMake, Ninja, and Nix";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
    flake-parts.url = "github:hercules-ci/flake-parts";
  };

  outputs =
    inputs@{ flake-parts, nixpkgs, ... }:
    flake-parts.lib.mkFlake { inherit inputs; } {
      systems = nixpkgs.lib.platforms.all;

      perSystem =
        { pkgs, ... }:
        {
          devShells.default = pkgs.mkShellNoCC {
            packages = [
              pkgs.llvmPackages.libcxxClang
              pkgs.cmake
              pkgs.ninja
              pkgs.gtest
              pkgs.clang-tools
            ];
          };

          packages.default = pkgs.stdenvNoCC.mkDerivation {
            name = "cxx-template";
            src = ./.;
            nativeBuildInputs = [
              pkgs.llvmPackages.libcxxClang
              pkgs.cmake
              pkgs.ninja
              pkgs.clang-tools
            ];
            buildInputs = [
              pkgs.gtest
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

          apps.format = {
            type = "app";
            program =
              (pkgs.writeShellScript "format-cxx-template" ''
                set -euo pipefail
                CPU_COUNT=$(${pkgs.coreutils}/bin/nproc)
                echo "Running clang-format with $CPU_COUNT parallel processes..."
                ${pkgs.fd}/bin/fd -0 -t f -e hh -e cc . include src tests apps | \
                  xargs -0 -n 1 -P "$CPU_COUNT" clang-format -i
                echo "OK"
              '').outPath;
          };

          apps.build = {
            type = "app";
            program =
              (pkgs.writeShellScript "build-cxx-template" ''
                set -euo pipefail
                if [ ! -d build ]; then
                  nix develop --command cmake -S . -B build -G Ninja \
                    -DCMAKE_BUILD_TYPE=Debug \
                    -DCXX_ARGON_ENABLE_TEST=ON \
                    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
                fi
                nix develop --command cmake --build build
              '').outPath;
          };
        };
    };
}
