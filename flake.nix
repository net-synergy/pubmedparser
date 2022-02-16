{
  description = "R package to parse pubmed xml files into a set of graphes.";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
      in {
        devShell = pkgs.mkShell {
          buildInputs = [
            (pkgs.rWrapper.override {
              packages = with pkgs.rPackages;
                [ tidyverse lintr styler xml2 devtools furrr ];
            })
          ];
        };
      });
}
