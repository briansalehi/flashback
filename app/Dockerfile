from rust:1.94.0
label authors="Brian Salehi <briansalehi@proton.me>"

arg TAURI_CLI_VERSION=2.10.1
arg NDK_VERSION=29.0.13846066

run apt update && apt install --yes --no-install-recommends ca-certificates build-essential file curl wget rustup libwebkit2gtk-4.1-dev libxdo-dev libssl-dev libayatana-appindicator3-dev librsvg2-dev openjdk-21-jdk-headless unzip && apt clean

env JAVA_HOME=/usr/lib/jvm/java-21-openjdk-amd64
env ANDROID_HOME=/opt/android-sdk
env NDK_HOME=${ANDROID_HOME}/ndk/${NDK_VERSION}
env PATH=${PATH}:${ANDROID_HOME}/cmdline-tools/latest/bin:${ANDROID_HOME}/platform-tools

run mkdir -p ${ANDROID_HOME}/cmdline-tools
run wget -q https://dl.google.com/android/repository/commandlinetools-linux-11076708_latest.zip -O /tmp/cmdline-tools.zip
run unzip -q /tmp/cmdline-tools.zip -d ${ANDROID_HOME}/cmdline-tools
run mv ${ANDROID_HOME}/cmdline-tools/cmdline-tools ${ANDROID_HOME}/cmdline-tools/latest
run rm /tmp/cmdline-tools.zip

run yes | sdkmanager --licenses
run sdkmanager "platforms;android-36" "build-tools;36.0.0" "ndk;${NDK_VERSION}" "platform-tools"

run curl -L --proto '=https' --tlsv1.2 -sSf https://raw.githubusercontent.com/cargo-bins/cargo-binstall/main/install-from-binstall-release.sh | bash
run cargo binstall -y --locked --version "^${TAURI_CLI_VERSION}" tauri-cli
run rm -rf ~/.cargo/{registry,.crates.toml,.crates2.json,.global-cache,.package-cache,.package-cache-mutate}

cmd ["cargo", "tauri", "build"]
