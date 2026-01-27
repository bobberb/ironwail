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

        # Windows x64 cross-compilation
        pkgsWindows = pkgs.pkgsCross.mingwW64;

        ironwail-windows = pkgsWindows.stdenv.mkDerivation {
          pname = "ironwail-windows";
          version = "0.8.1-hexen2";

          src = ./.;

          nativeBuildInputs = with pkgs; [
            cmake
            pkgsWindows.buildPackages.gcc
            pkgsWindows.buildPackages.binutils
          ];

          buildInputs = with pkgsWindows; [
            SDL2
            windows.pthreads
            windows.mcfgthreads
            windows.mingw_w64  # For winmm, ws2_32, etc.
            # curl disabled - nghttp3 doesn't support Windows cross-compile
          ];

          # CMake cross-compilation settings
          cmakeFlags = [
            "-DCMAKE_BUILD_TYPE=Release"
            "-DCMAKE_SYSTEM_NAME=Windows"
            "-DCMAKE_C_COMPILER=${pkgsWindows.stdenv.cc.targetPrefix}cc"
            "-DCMAKE_CXX_COMPILER=${pkgsWindows.stdenv.cc.targetPrefix}c++"
            "-DCMAKE_RC_COMPILER=${pkgsWindows.stdenv.cc.bintools.targetPrefix}windres"
            # Disable optional packages for simpler cross-compile
            "-DCMAKE_DISABLE_FIND_PACKAGE_PkgConfig=TRUE"
            "-DCMAKE_DISABLE_FIND_PACKAGE_CURL=TRUE"
          ];

          # Fix library name case for mingw (Winmm -> winmm)
          # Also disable post-build DLL copying which fails without codecs
          postPatch = ''
            sed -i 's/Winmm/winmm/g' CMakeLists.txt
            # Remove all the DLL copying post-build commands that fail without external codecs
            sed -i '/add_custom_command.*POST_BUILD/,/COMMAND_EXPAND_LISTS)/d' CMakeLists.txt
          '';

          # Windows doesn't have pkg-config in cross-compile, handle dependencies manually
          preConfigure = ''
            export SDL2_DIR=${pkgsWindows.SDL2}/lib/cmake/SDL2
          '';

          installPhase = ''
            mkdir -p $out/bin
            cp ironwail.exe $out/bin/

            # Copy SDL2 DLL
            cp ${pkgsWindows.SDL2}/bin/*.dll $out/bin/ || true

            # Create README
            cat > $out/bin/README.txt <<EOF
Ironwail - Windows x64 Build (with Hexen II support)

To play Quake:
1. Place your id1 folder (with pak0.pak) in the same directory as ironwail.exe
2. Run ironwail.exe

To play Hexen II:
1. Place your data1 folder (with pak0.pak, pak1.pak) in the same directory
2. Run ironwail.exe - it will auto-detect Hexen II mode

For Portal of Praevus:
1. Also place your portals folder (with pak3.pak)
2. Run: ironwail.exe -portals

For more info: https://github.com/andrei-drexler/ironwail
EOF
          '';

          meta = with pkgs.lib; {
            description = "Ironwail - Windows x64 build with Hexen II support";
            homepage = "https://github.com/andrei-drexler/ironwail";
            license = licenses.gpl2Plus;
            platforms = [ "x86_64-windows" ];
          };
        };
      in
      {
        packages.default = ironwail;
        packages.ironwail = ironwail;
        packages.windows = ironwail-windows;

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

        # Hexen II mode - run with data1 game data (auto-detects Portal of Praevus)
        apps.hexen2 = flake-utils.lib.mkApp {
          drv = pkgs.writeShellApplication {
            name = "ironwail-hexen2";
            runtimeInputs = [];
            text = ''
              GAME_DIR="''${GAME_DIR:-/tank/josh/Documents/Games/PcGames/HeXen II}"
              # Don't use -game data1, let auto-detection handle it
              # This allows Portal of Praevus to be auto-detected
              exec ${ironwail}/bin/ironwail -basedir "$GAME_DIR" "$@"
            '';
          };
        };

        # Hexen II with Portal of Praevus (mission pack)
        apps.hexen2-portals = flake-utils.lib.mkApp {
          drv = pkgs.writeShellApplication {
            name = "ironwail-hexen2-portals";
            runtimeInputs = [];
            text = ''
              GAME_DIR="''${GAME_DIR:-/tank/josh/Documents/Games/PcGames/HeXen II}"
              exec ${ironwail}/bin/ironwail -portals -basedir "$GAME_DIR" "$@"
            '';
          };
        };

        # Hexen II dedicated server (auto-detects Portal of Praevus)
        apps.hexen2-dedicated = flake-utils.lib.mkApp {
          drv = pkgs.writeShellApplication {
            name = "ironwail-hexen2-dedicated";
            runtimeInputs = [];
            text = ''
              GAME_DIR="''${GAME_DIR:-/tank/josh/Documents/Games/PcGames/HeXen II}"
              exec ${ironwail}/bin/ironwail -dedicated -nossi -nojoy -nomissing -basedir "$GAME_DIR" "$@"
            '';
          };
        };

        # Hexen II dedicated server with Portal of Praevus
        apps.hexen2-portals-dedicated = flake-utils.lib.mkApp {
          drv = pkgs.writeShellApplication {
            name = "ironwail-hexen2-portals-dedicated";
            runtimeInputs = [];
            text = ''
              GAME_DIR="''${GAME_DIR:-/tank/josh/Documents/Games/PcGames/HeXen II}"
              exec ${ironwail}/bin/ironwail -dedicated -nossi -nojoy -nomissing -portals -basedir "$GAME_DIR" "$@"
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
