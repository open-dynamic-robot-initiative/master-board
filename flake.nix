{
  description = "Hardware and Firmware of the Solo Quadruped Master Board.";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-unstable";

    utils = {
      url = "github:Gepetto/nix-lib";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs =
    {
      self,
      nixpkgs,
      utils,
    }:
    let
      pkgs = nixpkgs.legacyPackages.x86_64-linux;
    in
    {
      packages.x86_64-linux.default = pkgs.stdenv.mkDerivation {

        pname = "odri-masterboard-sdk";
        version = utils.lib.rosVersion pkgs ./sdk/master_board_sdk/package.xml;

        src = builtins.path {
          name = "sdk";
          path = ./sdk/master_board_sdk;
        };

        doCheck = true;

        nativeBuildInputs = with pkgs; [
          jrl-cmakemodules
          cmake
          python312
        ];

        # from package.xml
        buildInputs = with pkgs; [ python312Packages.numpy ];

        nativeCheckInputs = with pkgs; [ catch2_3 ];

        propagatedBuildInputs = with pkgs; [ python312Packages.boost ];
      };
    };
}
