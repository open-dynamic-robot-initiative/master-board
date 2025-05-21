{
  description = "Hardware and Firmware of the Solo Quadruped Master Board.";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-unstable";
  };

  outputs =
    { self, nixpkgs }:
    let
      pkgs = nixpkgs.legacyPackages.x86_64-linux;
    in
    {
      packages.x86_64-linux.sdk = pkgs.stdenv.mkDerivation {

        pname = "odri-masterboard-sdk";
        version = "1.0.7";

        src = builtins.path {
          name = "sdk";
          path = ./sdk/master_board_sdk;
        };

        nativeBuildInputs = with pkgs; [
          cmake
          python312
          python312Packages.boost
        ];

        propagatedBuildInputs = with pkgs; [ python312Packages.boost ];
      };
    };
}
