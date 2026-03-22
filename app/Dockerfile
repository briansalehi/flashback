from rust:1.94.0
label authors="Brian Salehi <briansalehi@proton.me>"

arg TAURI_CLI_VERSION=2.10.1

run apt update && apt install --yes --no-install-recommends ca-certificates build-essential file curl wget rustup libwebkit2gtk-4.1-dev libxdo-dev libssl-dev libayatana-appindicator3-dev librsvg2-dev && apt clean

run cargo install tauri-cli --version "^${TAURI_CLI_VERSION}" --locked
run rm -rf ~/.cargo/{registry,.crates.toml,.crates2.json,.global-cache,.package-cache,.package-cache-mutate}

cmd ["cargo", "tauri", "build"]
