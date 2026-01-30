{ pkgs ? import <nixpkgs> {} }:

pkgs.stdenv.mkDerivation rec {
  pname = "hyprwave";
  version = "0.8";

  src = pkgs.fetchgit {
    url = "https://github.com/shantanubaddar/hyprwave";
    rev = "b3e7507d6dadc23e819ebd8cd6beb4ae00922376";
    sha256 = "sha256-RR0uCvpIE7M15QCvaqKAA3NuirigoNY3vc1RvZVFH60=";
  };

  buildInputs = with pkgs; [
    gnumake
    glib
    glibc
    gtk4
    gtk4-layer-shell
    pkg-config
    libpulseaudio
  ];

  makeFlags = [
    "PREFIX=$(out)"
  ];

  postPatch = ''
    substituteInPlace paths.c \
      --replace-fail '"/usr/share/hyprwave' '"${placeholder "out"}/share/hyprwave'
  '';

  buildPhase = ''
    make
  '';

  installPhase = ''
    mkdir -p $out/bin
    mkdir -p $out/share/hyprwave
    make PREFIX=$out BINDIR=$out/bin DESTDIR=$out/bin DATADIR=$out/share/hyprwave install
  '';

  meta = with pkgs.lib; {
    homepage = "https://github.com/shantanubaddar/hyprwave";
    description = "A sleek, modern music control bar for Wayland compositors with MPRIS integration.";
    platforms = [
      "x86_64-linux"
    ];
    maintainers = [ maintainers.nixpup ];
    license = licenses.gpl3Only;
    sourceProvenance = with sourceTypes; [ binaryNativeCode ];
    mainProgram = "hyprwave";
  };
}
