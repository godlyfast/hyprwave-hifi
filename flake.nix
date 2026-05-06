{
  description = "HyprWave HiFi";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils, ... }:
    flake-utils.lib.eachDefaultSystem (system: let
      pkgs = nixpkgs.legacyPackages.${system};
      hyprwave = pkgs.callPackage ./default.nix {};
    in {
      packages.hyprwave = hyprwave;
      packages.default = hyprwave;
      apps.hyprwave = {
        type = "app";
        program = "${hyprwave}/bin/hyprwave";
        meta = {
          description = "GTK4 Wayland music overlay with PipeWire visualization.";
          mainProgram = "hyprwave";
        };
      };
      apps.default = {
        type = "app";
        program = "${hyprwave}/bin/hyprwave";
      };
    });
}
