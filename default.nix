{ pkgs ? import <nixpkgs> {} }:

pkgs.stdenv.mkDerivation rec {
  pname = "hyprwave-hifi";
  version = "unstable";

  src = ./.;

  nativeBuildInputs = with pkgs; [
    pkg-config
  ];

  buildInputs = with pkgs; [
    gdk-pixbuf
    glib
    gtk4
    gtk4-layer-shell
    libsoup_3
    pipewire
  ];

  makeFlags = [
    "PREFIX=$(out)"
  ];

  postPatch = ''
    substituteInPlace paths.c \
      --replace-fail '"/usr/share/hyprwave' '"${placeholder "out"}/share/hyprwave'
  '';

  installPhase = ''
    runHook preInstall

    install -Dm755 hyprwave "$out/bin/hyprwave"
    install -Dm755 hyprwave-toggle.sh "$out/bin/hyprwave-toggle"

    install -Dm644 style.css "$out/share/hyprwave/style.css"
    install -Dm644 icons/*.svg -t "$out/share/hyprwave/icons"
    install -Dm644 themes/*.css -t "$out/share/hyprwave/themes"
    install -Dm644 fonts/VT323-Regular.ttf \
      "$out/share/fonts/truetype/hyprwave/VT323-Regular.ttf"

    runHook postInstall
  '';

  meta = with pkgs.lib; {
    homepage = "https://github.com/godlyfast/hyprwave-hifi";
    description = "HiFi fork of HyprWave with PipeWire visualizer and per-application volume control";
    platforms = platforms.linux;
    license = licenses.gpl3Only;
    mainProgram = "hyprwave";
  };
}
