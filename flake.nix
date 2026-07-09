  {
  description = "SDK for the ODRI Master board";

  inputs = {
    # TODO: drop `/module` after https://github.com/Gepetto/nix/pull/54
    gepetto.url = "github:gepetto/nix/module";
    flake-parts.follows = "gepetto/flake-parts";
    nixpkgs.follows = "gepetto/nixpkgs";
    nix-ros-overlay.follows = "gepetto/nix-ros-overlay";
    systems.follows = "gepetto/systems";
    treefmt-nix.follows = "gepetto/treefmt-nix";
    utils.url = "github:Gepetto/nix-lib";
  };

  outputs =
    inputs:
    inputs.flake-parts.lib.mkFlake { inherit inputs; } {
      systems = import inputs.systems;
      imports = [ inputs.gepetto.flakeModule ];
      perSystem =
        {
          lib,
          pkgs,
          self',
          ...
        }:
        {
          packages = {
            default = self'.packages.odri-masterboard-sdk;
            odri-masterboard-sdk = pkgs.odri-masterboard-sdk.overrideAttrs {
              version = inputs.utils.lib.rosVersion pkgs ./sdk/master_board_sdk/package.xml;
              src = lib.fileset.toSource {
                root = ./.;
                fileset = lib.fileset.unions [
                  ./sdk/master_board_sdk/src
                  ./sdk/master_board_sdk/example
                  ./sdk/master_board_sdk/include
                  ./sdk/master_board_sdk/IMU_vs_ENC.kst
                  ./sdk/master_board_sdk/Makefile
                  ./sdk/master_board_sdk/package.xml
                  ./sdk/master_board_sdk/srcpy
                  ./sdk/master_board_sdk/CMakeLists.txt
                  ./sdk/master_board_sdk/tests
                ];
              };
            };
          };
        };
    };
}
