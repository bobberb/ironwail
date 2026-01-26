{
  description = "Ironwail - Advanced Quake engine with Hexen II support";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};

        ironwail = pkgs.stdenv.mkDerivation {
          pname = "ironwail";
          version = "0.8.1-hexen2";

          src = ./.;

          nativeBuildInputs = with pkgs; [
            cmake
            pkg-config
          ];

          buildInputs = with pkgs; [
            SDL2
            libGL
            libvorbis
            mpg123
            flac
            opusfile
            curl
          ];

          cmakeFlags = [
            "-DCMAKE_BUILD_TYPE=Release"
          ];

          installPhase = ''
            mkdir -p $out/bin
            cp ironwail $out/bin/
          '';

          meta = with pkgs.lib; {
            description = "Advanced Quake engine based on QuakeSpasm with Hexen II support";
            homepage = "https://github.com/andrei-drexler/ironwail";
            license = licenses.gpl2Plus;
            platforms = platforms.linux;
            mainProgram = "ironwail";
          };
        };
      in
      {
        packages.default = ironwail;
        packages.ironwail = ironwail;

        apps = flake-utils.lib.mkApp {
          drv = pkgs.writeShellApplicationBin "ironwail-dedicated-test" {
            text = ''
              #!/bin/sh
              exec ${ironwail}/bin/ironwail -dedicated -nossi -nojoy -nomissing "$@"
            '';
          };
        };

        devShells.default = pkgs.mkShell {
          inputsFrom = [ ironwail ];

          buildInputs = with pkgs; [
            gdb
            valgrind
          ];

          shellHook = ''
            echo "Ironwail development environment"
            echo "Build with: nix build"
            echo "Run with: ./result/bin/ironwail"
            echo "Run dedicated server test: nix run ."
          '';
        };
      }
    );
}
