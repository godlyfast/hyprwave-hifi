{
  description = "Hyprwave Flake";

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
          description = " A music control bar for hyprland.";
          mainProgram = "hyprwave";
        };
      };
      apps.default = self.apps.${system}.hyprwave;
    });
}
