{
  description = "Convert pubmed xml files to tables";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
        src = self;
        version = "2.0.0-alpha";
        pythonEnv = pkgs.poetry2nix.mkPoetryEnv {
          projectDir = ./.;
          editablePackageSources = { pubmedparser = ./pubmedparser; };
          preferWheels = true;
          extraPackages = (ps:
            with ps; [
              ipython
              python-lsp-server
              pyls-isort
              python-lsp-black
              pylsp-mypy
            ]);
          groups = [ ];
        };
        pythonPackage = pkgs.poetry2nix.mkPoetryPackage { projectDir = ./.; };
      in {
        packages.pubmedparser = pkgs.callPackage ./. { inherit src version; };
        packages.python = pythonPackage;
        defaultPackage = self.packages.${system}.pubmedparser;
        devShell = pkgs.mkShell {
          packages =
            (with pkgs; [ gcc gdb astyle zlib bats cmocka poetry pythonEnv ]);
          shellHook = ''
            export C_INCLUDE_PATH=${pythonEnv}/include
          '';
        };
      });
}
