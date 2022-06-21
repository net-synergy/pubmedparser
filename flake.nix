{
  description = "Convert pubmed xml files to tables";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs";
    flake-utils = {
      url = "github:numtide/flake-utils";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
        src = self;
        version = "1.1.0";
      in {
        packages.pubmedparser = pkgs.callPackage ./. { inherit src version; };
        defaultPackage = self.packages.${system}.pubmedparser;
        devShell =
          pkgs.mkShell { buildInputs = (with pkgs; [ gcc gdb astyle zlib ]); };
      });
}
