from nailyudha/tauri:latest
copy . /src
workdir /src
run cargo install tauri-cli
run cargo tauri android init
run cargo tauri android build --apk --debug