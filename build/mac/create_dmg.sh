#!/bin/bash

#
# сформировать app-файл
#
cp -R ../../src/_build/starcapp.app Story\ Architect.app
find Story\ Architect.app -name "*.dSYM" -print0 | xargs -0 rm -rf
macdeployqt Story\ Architect.app

#
# копируем OpenSSL в бандл приложения (macdeployqt не всегда подтягивает эти библиотеки)
#
APP_FRAMEWORKS_DIR="Story Architect.app/Contents/Frameworks"
mkdir -p "${APP_FRAMEWORKS_DIR}"

OPENSSL_PREFIX=""
if command -v brew >/dev/null 2>&1; then
  OPENSSL_PREFIX=$(brew --prefix openssl@3 2>/dev/null || true)
  if [ -z "${OPENSSL_PREFIX}" ]; then
    OPENSSL_PREFIX=$(brew --prefix openssl@1.1 2>/dev/null || true)
  fi
fi

if [ -n "${OPENSSL_PREFIX}" ] && [ -d "${OPENSSL_PREFIX}/lib" ]; then
  cp -f "${OPENSSL_PREFIX}"/lib/libcrypto*.dylib "${APP_FRAMEWORKS_DIR}/" 2>/dev/null || true
  cp -f "${OPENSSL_PREFIX}"/lib/libssl*.dylib "${APP_FRAMEWORKS_DIR}/" 2>/dev/null || true
fi

#
# принудительно переключаем зависимости на OpenSSL из бандла, а не из системы/Homebrew
#
for dylib in "${APP_FRAMEWORKS_DIR}"/libcrypto*.dylib "${APP_FRAMEWORKS_DIR}"/libssl*.dylib; do
  [ -f "${dylib}" ] || continue
  install_name_tool -id "@executable_path/../Frameworks/$(basename "${dylib}")" "${dylib}" || true
done

for target in "Story Architect.app/Contents/MacOS/"* \
              "Story Architect.app/Contents/Frameworks/"*.dylib; do
  [ -f "${target}" ] || continue
  otool -L "${target}" 2>/dev/null | awk '/lib(ssl|crypto).*\.dylib/{print $1}' | while read -r dep; do
    [ -n "${dep}" ] || continue
    install_name_tool -change "${dep}" "@executable_path/../Frameworks/$(basename "${dep}")" "${target}" || true
  done
done

#
# подпишем app-файл
#
echo $APPLE_CERTIFICATE | base64 --decode > certificate.p12
security create-keychain -p $APPLE_KEYCHAIN_PASSWORD build.keychain
security default-keychain -s build.keychain
security unlock-keychain -p $APPLE_KEYCHAIN_PASSWORD build.keychain
security import certificate.p12 -k build.keychain -P $APPLE_CERTIFICATE_PASSWORD -T /usr/bin/codesign
security set-key-partition-list -S apple-tool:,apple:,codesign: -s -k $APPLE_KEYCHAIN_PASSWORD build.keychain
security find-identity
codesign --force --entitlements entitlements.plist --deep --sign "$APPLE_SIGNING_IDENTITY" --options "runtime" "Story Architect.app/" -v

#
# сформировать архив с приложением
#
ditto -ck --keepParent Story\ Architect.app starcapp.zip

#
# отправить архив на проверку
#
xcrun notarytool submit starcapp.zip --apple-id $APPLE_ID --password $APPLE_ID_APP_PASSWORD --team-id $APPLE_TEAM_ID --wait

#
# удалить архив
#
rm -R starcapp.zip

#
# поместить тикет в приложение
#
xcrun stapler staple Story\ Architect.app

#
# Создаём dmg-файл
#
./make_dmg.sh -i Story\ Architect.app/Contents/Resources/icon.icns -b cover.png -c "462:252:176:258" -s "640:400" Story\ Architect.app
mv -f Story\ Architect.dmg starc-setup.dmg
