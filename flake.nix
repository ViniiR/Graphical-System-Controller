{
    inputs.nixpkgs.url = "github:nixos/nixpkgs/95ca1e203c0750115fd4a6f17d5a245dfe6b1edd";
    outputs = {
        self,
        nixpkgs,
    }: let
        system = "x86_64-linux";
        pkgs = import nixpkgs {inherit system;};
        lib = pkgs.lib;
        interface_name = "com.vinii.vgsc";
        binary_name = "vgscd";
        #
        runtimePackages = with pkgs; [
            gawk
            jq
            pipewire
            wireplumber
        ];
    in {
        packages.${system}.default = pkgs.stdenv.mkDerivation rec {
            pname = binary_name;
            version = "0.1";

            src = pkgs.nix-gitignore.gitignoreSource [] ./.;

            cmakeFlags = [
                "--preset"
                "release"
            ];

            nativeBuildInputs = with pkgs; [
                pkg-config
                cmake
                gcc

                # TODO: integrate rust stuff here
                # rustc
                # cargo
            ];
            buildInputs = with pkgs;
                [
                    # Libraries
                    systemd
                    dbus
                ]
                ++ runtimePackages;

            installPhase = ''
                runHook preInstall

                mkdir -p $out/bin
                mv vgsc $out/bin/${pname}

                runHook postInstall
            '';

            postInstall = ''
                # Launch D-Bus service
                mkdir -p $out/share/dbus-1/system-services
                cat <<END > $out/share/dbus-1/system-services/${interface_name}.service
                [D-BUS Service]
                Name=${interface_name}
                Exec=$out/bin/${pname}
                User=root
                SystemdService=${pname}.service
                END

                # D-Bus config file
                mkdir -p $out/share/dbus-1/system.d
                cp ${./dbus.conf.xml} $out/share/dbus-1/system.d/${interface_name}.conf
            '';
        };
        nixosModules.default = {...}: let
            package = self.packages.${pkgs.stdenv.hostPlatform.system}.default;
        in {
            services.dbus.packages = [package];
            # environment.systemPackages = [package];

            systemd.services.${binary_name} = {
                description = "Vinii's Graphical System Controller Daemon";
                wantedBy = ["multi-user.target"];
                after = ["dbus.service"];
                wants = ["dbus.service"];
                environment = {
                    DISPLAY = ":0";
                    XDG_RUNTIME_DIR = "/run/user/1000";
                    DBUS_SESSION_BUS_ADDRESS = "unix:path=/run/user/1000/bus";
                };
                path = runtimePackages;
                serviceConfig = {
                    Type = "dbus";
                    BusName = interface_name;
                    ExecStart = "${package}/bin/${binary_name}";
                    Restart = "always";
                };
            };
        };
        devShells.${system}.default = pkgs.mkShell rec {
            NIX_ENFORCE_PURITY = 0;
            LD_LIBRARY_PATH = lib.makeLibraryPath packages;

            RUST_SRC_PATH = "${pkgs.rust.packages.stable.rustPlatform.rustLibSrc}";

            # Packages available in the User's shell
            packages = with pkgs; [
                pkg-config
                systemd
                cmake
                gcc
                gtk4
                rustc
                cargo
                clippy
                rust-analyzer
                icon-library
                zsh
            ];

            shellHook = ''
                export SHELL=${pkgs.zsh}/bin/zsh
                exec zsh;
            '';
        };
    };
}
