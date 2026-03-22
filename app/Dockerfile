from nailyudha/tauri:latest
arg KEYSTORE_BASE64
arg KEYSTORE_ALIAS
arg KEYSTORE_PASSWORD
copy . /src
workdir /src
run cargo install tauri-cli --version '^2.10.1' --locked
run cargo tauri android init
run sed -i '/getByName("release")/a signingConfig = signingConfigs.getByName("release")' src-tauri/gen/android/app/build.gradle.kts
run sed -i '/^android {/a signingConfigs { create("release") { storeFile = file("keystore.jks"); storePassword = System.getenv("KEYSTORE_PASSWORD"); keyAlias = "flashback"; keyPassword = System.getenv("KEYSTORE_PASSWORD"); } }' src-tauri/gen/android/app/build.gradle.kts
run cargo tauri icon src-tauri/icon/flashback-transparent.png
run cargo tauri android build --apk --debug
run echo "$KEYSTORE_BASE64" > src-tauri/gen/android/app/keystore.jks
run echo "keyAlias=$KEYSTORE_ALIAS" > src-tauri/gen/android/keystore.properties
run echo "keyPassword=$KEYSTORE_PASSWORD" >> src-tauri/gen/android/keystore.properties
run echo "password=$KEYSTORE_PASSWORD" >> src-tauri/gen/android/keystore.properties
run echo "storeFile=keystore.jks" >> src-tauri/gen/android/keystore.properties
run cargo tauri android build
copy src-tauri/gen/android/app/build/outputs/apk/universal/release/app-universal-release.apk /packages