from nailyudha/tauri:latest
copy . /src
workdir /src
run apt update && apt install -y rustc && apt upgrade -y cargo rustc
run cargo install tauri-cli --version '^2.10.1' --locked
run cargo tauri android init
run sed -i '/getByName("release")/a signingConfig = signingConfigs.getByName("release")' src-tauri/gen/android/app/build.gradle.kts
run sed -i '/^android {/a signingConfigs { create("release") { storeFile = file("keystore.jks"); storePassword = file("keystore.txt"); keyAlias = "flashback"; keyPassword = file("keystore.txt"); } }' src-tauri/gen/android/app/build.gradle.kts
run cargo tauri icon src-tauri/icons/flashback-transparent.png
run cargo tauri android build --apk --debug
run --mount=type=secret,id=keystore_base64 cp /run/secrets/keystore_base64 src-tauri/gen/android/app/keystore.jks
run --mount=type=secret,id=keystore_password cp /run/secrets/keystore_password src-tauri/gen/android/app/keystore.txt
run echo "storeFile=keystore.jks" > src-tauri/gen/android/keystore.properties
run --mount=type=secret,id=keystore_password echo "password=$(cat /run/secrets/keystore_password)" >> src-tauri/gen/android/keystore.properties
run --mount=type=secret,id=keystore_alias echo "keyAlias=$(cat /run/secrets/keystore_alias)" >> src-tauri/gen/android/keystore.properties
run --mount=type=secret,id=keystore_password echo "keyPassword=$(cat /run/secrets/keystore_password)" >> src-tauri/gen/android/keystore.properties
run cargo tauri android build