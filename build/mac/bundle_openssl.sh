#!/bin/bash
set -e

APP="Story Architect.app/Contents"
BINARY="${APP}/MacOS/starcapp"
ARCHS=$(lipo -archs "${BINARY}")
FRAMEWORKS="${APP}/Frameworks"

cp "${OPENSSL_PREFIX}/lib/libcrypto.3.dylib" "${FRAMEWORKS}/"

install_name_tool -id \
    "@executable_path/../Frameworks/libcrypto.3.dylib" \
    "${FRAMEWORKS}/libcrypto.3.dylib"

# Правим ссылки для каждой архитектуры
for ARCH in $ARCHS; do
    OLD=$(otool -arch $ARCH -L "${FRAMEWORKS}/libcorelib.dylib" | grep libcrypto | awk '{print $1}')
    if [ -n "$OLD" ]; then
        echo "   [$ARCH] $OLD → @executable_path/../Frameworks/libcrypto.3.dylib"
        install_name_tool -change \
            "$OLD" \
            "@executable_path/../Frameworks/libcrypto.3.dylib" \
            "${FRAMEWORKS}/libcorelib.dylib"
    fi
done
