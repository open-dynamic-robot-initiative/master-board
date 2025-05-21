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
      packages.x86_64-linux.default = pkgs.stdenv.mkDerivation {

        pname = "odri-masterboard-sdk";
        version = "1.0.7";

        src = builtins.path {
          name = "sdk";
          path = ./sdk/master_board_sdk;
        };

        doCheck = true;

        nativeBuildInputs = with pkgs; [
          jrl-cmakemodules
          cmake
          python312
          python312Packages.boost
        ];

        # from package.xml
        buildInputs = with pkgs; [ python312Packages.numpy ];

        nativeCheckInputs = with pkgs; [ catch2 ];

        propagatedBuildInputs = with pkgs; [ python312Packages.boost ];
      };
    };
}
