{ stdenv, zlib, bats, cmocka  }:

stdenv.mkDerivation {
    src = ./.;
    pname = "pubmedparser";
    version = "0.1";

    nativeBuildInputs = [ zlib ];
    preBuild = "mkdir bin";
    doCheck = true;
    checkPhase = ''
      runHook preCheck

      make check

      runHook postCheck
    '';

    checkInputs = [ bats cmocka ];

    installPhase = ''
      runHook preInstall

      mkdir -p "$out"/bin
      mv bin "$out"

      runHook postInstall
    '';
}
