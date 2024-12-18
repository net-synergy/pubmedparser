{
  description = "Convert pubmed xml files to tables";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs";
    poetry2nix = {
      url = "github:nix-community/poetry2nix";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs = { self, nixpkgs, poetry2nix }:
    let
      system = "x86_64-linux";
      pkgs = import nixpkgs {
        inherit system;
        overlays = [ poetry2nix.overlays.default ];
      };

      pythonEnv = pkgs.poetry2nix.mkPoetryEnv {
        projectDir = ./.;
        editablePackageSources = { pubmedparser = ./.; };
        preferWheels = true;
      };
    in {
      devShells.${system}.default =
        (pkgs.mkShell.override { stdenv = pkgs.clangStdenv; }) {
          packages = (with pkgs; [
            llvm
            clang-tools
            gdb
            valgrind
            bear
            zlib
            bats
            cmocka
            poetry
            pythonEnv
          ]);
        };
    };
}
