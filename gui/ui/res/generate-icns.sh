#!/usr/bin/env bash

mkdir -p flashback.iconset

sips -z 16 16     flashback-high.png --out flashback.iconset/icon_16x16.png
sips -z 32 32     flashback-high.png --out flashback.iconset/icon_16x16@2x.png
sips -z 32 32     flashback-high.png --out flashback.iconset/icon_32x32.png
sips -z 64 64     flashback-high.png --out flashback.iconset/icon_32x32@2x.png
sips -z 120 120   flashback-high.png --out flashback.iconset/icon_120x120.png
sips -z 128 128   flashback-high.png --out flashback.iconset/icon_128x128.png
sips -z 167 167   flashback-high.png --out flashback.iconset/icon_167x167.png
sips -z 256 256   flashback-high.png --out flashback.iconset/icon_128x128@2x.png
sips -z 252 252   flashback-high.png --out flashback.iconset/icon_252x252.png
sips -z 256 256   flashback-high.png --out flashback.iconset/icon_256x256.png
sips -z 512 512   flashback-high.png --out flashback.iconset/icon_256x256@2x.png
sips -z 512 512   flashback-high.png --out flashback.iconset/icon_512x512.png
cp                flashback-high.png       flashback.iconset/icon_512x512@2x.png

iconutil -c icns flashback.iconset -o flashback.icns
