{
  description = "R package to parse pubmed xml files into a set of graphes.";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs";
    flake-utils.url = "github:numtide/flake-utils";
    neo4j = {
      url = "/home/voidee/packages/nixpkgs/neo4j";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs = { self, nixpkgs, flake-utils, neo4j }:
    flake-utils.lib.eachDefaultSystem (system:
      let pkgs = nixpkgs.legacyPackages.${system};
      in {
        devShell = pkgs.mkShell {
          buildInputs =
            (with pkgs; [ gcc gdb valgrind astyle zlib bats cmocka ])
            ++ [ neo4j.packages.${system}.neo4j ];
          shellHook = ''
            export OMP_NUM_THREADS=4
          '';
        };
      });
}
