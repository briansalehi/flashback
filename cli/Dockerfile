from fedora:latest
label authors="Brian Salehi <briansalehi@proton.me>"
run dnf upgrade -y
run dnf install -y cmake make g++ git boost-devel ftxui-devel grpc-devel protobuf-compiler gtest-devel
run dnf clean all
run useradd -m flashback -s /usr/bin/bash -u 1000
run git clone --branch master --depth 1 https://github.com/briansalehi/flashback
run cmake -G 'Unix Makefiles' -S flashback -B build -D CMAKE_BUILD_TYPE=Release
run cmake --build build --parallel $(nproc) --target cli
run cmake --install build
user flashback
workdir /home/flashback
entrypoint ["flashback"]