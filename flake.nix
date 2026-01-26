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

        # Quake mode - run with id1 game data
        apps.quake = flake-utils.lib.mkApp {
          drv = pkgs.writeShellApplication {
            name = "ironwail-quake";
            runtimeInputs = [];
            text = ''
              GAME_DIR="''${GAME_DIR:-/tank/josh/Documents/Games/PcGames/Quake/Quake/paks}"
              exec ${ironwail}/bin/ironwail -basedir "$GAME_DIR" "$@"
            '';
          };
        };

        # Quake dedicated server
        apps.quake-dedicated = flake-utils.lib.mkApp {
          drv = pkgs.writeShellApplication {
            name = "ironwail-quake-dedicated";
            runtimeInputs = [];
            text = ''
              GAME_DIR="''${GAME_DIR:-/tank/josh/Documents/Games/PcGames/Quake/Quake/paks}"
              exec ${ironwail}/bin/ironwail -dedicated -nossi -nojoy -nomissing -basedir "$GAME_DIR" "$@"
            '';
          };
        };

        # Hexen II mode - run with data1 game data
        apps.hexen2 = flake-utils.lib.mkApp {
          drv = pkgs.writeShellApplication {
            name = "ironwail-hexen2";
            runtimeInputs = [];
            text = ''
              GAME_DIR="''${GAME_DIR:-/tank/josh/Documents/Games/PcGames/HeXen II}"
              exec ${ironwail}/bin/ironwail -game data1 -basedir "$GAME_DIR" "$@"
            '';
          };
        };

        # Hexen II dedicated server
        apps.hexen2-dedicated = flake-utils.lib.mkApp {
          drv = pkgs.writeShellApplication {
            name = "ironwail-hexen2-dedicated";
            runtimeInputs = [];
            text = ''
              GAME_DIR="''${GAME_DIR:-/tank/josh/Documents/Games/PcGames/HeXen II}"
              exec ${ironwail}/bin/ironwail -dedicated -nossi -nojoy -nomissing -game data1 -basedir "$GAME_DIR" "$@"
            '';
          };
        };

        # Legacy test app (now quake-dedicated)
        apps.default = flake-utils.lib.mkApp {
          drv = pkgs.writeShellApplication {
            name = "ironwail-dedicated-test";
            runtimeInputs = [];
            text = ''
              GAME_DIR="''${GAME_DIR:-/tank/josh/Documents/Games/PcGames/Quake/Quake/paks}"
              exec ${ironwail}/bin/ironwail -dedicated -nossi -nojoy -nomissing -basedir "$GAME_DIR" "$@"
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
