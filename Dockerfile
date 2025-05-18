from fedora:42
run dnf upgrade -y && dnf install -y bash cmake git g++ boost-devel qt6-qtbase-devel qt6-qtquick3d-devel grpc-devel protobuf-compiler
run git clone --depth 1 --branch develop https://github.com/briansalehi/flashback.git /src
run cmake -S /src -B /workdir -G "Unix Makefiles" --workflows --preset all
entrypoint ["/workdir/flashback"]