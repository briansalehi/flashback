from fedora:latest
run dnf install -y bash g++ ninja wget tar gz boost-devel grpc-devel protobuf-compiler npm python3 git libsodium-devel libpq-devel inotify-tools
run wget https://github.com/Kitware/CMake/releases/download/v4.2.0/cmake-4.2.0-linux-x86_64.tar.gz -O /opt/cmake.tar.gz && tar -xzf /opt/cmake.tar.gz -C /opt && ln -s /opt/cmake-4.2.0-linux-x86_64/bin/cmake /usr/local/bin/cmake && rm /opt/cmake.tar.gz
copy . /src
user root
run cmake -S /src -B /build -G Ninja -D CMAKE_BUILD_TYPE=Release -D BUILD_TESTING=OFF
run cmake --build /build --parallel --target flashbackd
run cmake --install /build --component flashbackd
cmd ["/usr/local/bin/flashbackd"]
