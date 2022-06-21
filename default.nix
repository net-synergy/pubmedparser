{ src, version, stdenv, zlib, bats, cmocka }:

stdenv.mkDerivation {
  inherit src version;
  pname = "pubmedparser";

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
    mv helpers/* "$out/bin"

    runHook postInstall
  '';
}
