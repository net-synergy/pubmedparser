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
      let pkgs = nixpkgs.legacyPackages.${system};
      in {
        packages.pubmedparser = pkgs.callPackage ./pubmedparser.nix { };
        defaultPackage = self.packages.${system}.pubmedparser;
        devShell = pkgs.mkShell {
          buildInputs =
            (with pkgs; [ gcc gdb astyle zlib ]);
        };
      });
}
