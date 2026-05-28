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
APP_BUNDLE="Story Architect.app"
APP_FRAMEWORKS_DIR="${APP_BUNDLE}/Contents/Frameworks"
CORELIB="${APP_FRAMEWORKS_DIR}/libcorelib.dylib"

find_dependency() {
  local target="$1"
  local library_name="$2"

  otool -L "${target}" 2>/dev/null \
    | awk -v library_name="${library_name}" 'NR > 1 && $1 ~ library_name "([.][0-9]+)*[.]dylib$" { print $1; exit }'
}

echo "▶ Создаём Frameworks директорию..."
mkdir -p "${APP_FRAMEWORKS_DIR}"

if [ ! -f "${CORELIB}" ]; then
  CORELIB=$(find "${APP_FRAMEWORKS_DIR}" -maxdepth 1 -name 'libcorelib*.dylib' -type f 2>/dev/null | head -1)
fi
if [ -z "${CORELIB}" ] || [ ! -f "${CORELIB}" ]; then
  echo "Error! corelib was not found in ${APP_FRAMEWORKS_DIR}."
  exit 1
fi

echo "▶ Определяем исходный путь OpenSSL через otool в $(basename "${CORELIB}")..."
OLD_SSL=$(find_dependency "${CORELIB}" "libssl")
if [ -z "${OLD_SSL}" ] || [ ! -f "${OLD_SSL}" ]; then
  echo "Error! OpenSSL libssl dependency was not found in ${CORELIB}."
  exit 1
fi

OPENSSL_LIB_DIR=$(dirname "${OLD_SSL}")
OLD_CRYPTO_IN_SSL=$(find_dependency "${OLD_SSL}" "libcrypto")
OLD_CRYPTO_IN_CORELIB=$(find_dependency "${CORELIB}" "libcrypto")
OLD_CRYPTO="${OLD_CRYPTO_IN_SSL}"
if [ ! -f "${OLD_CRYPTO}" ]; then
  OLD_CRYPTO="${OLD_CRYPTO_IN_CORELIB}"
fi
if [ ! -f "${OLD_CRYPTO}" ]; then
  OLD_CRYPTO=$(ls "${OPENSSL_LIB_DIR}"/libcrypto*.dylib 2>/dev/null | head -1)
fi
if [ -z "${OLD_CRYPTO}" ] || [ ! -f "${OLD_CRYPTO}" ]; then
  echo "Error! OpenSSL libcrypto dependency was not found in ${CORELIB} or ${OPENSSL_LIB_DIR}."
  exit 1
fi

echo "▶ OpenSSL найден в ${OPENSSL_LIB_DIR}"

echo "▶ Копируем OpenSSL библиотеки..."
cp -fL "${OLD_SSL}" "${APP_FRAMEWORKS_DIR}/"
cp -fL "${OLD_CRYPTO}" "${APP_FRAMEWORKS_DIR}/"

SSL_LIB=$(basename "${OLD_SSL}")
CRYPTO_LIB=$(basename "${OLD_CRYPTO}")

echo "▶ Найдены: ${SSL_LIB}, ${CRYPTO_LIB}"

echo "▶ Обновляем install_name (id)..."
install_name_tool -id \
  "@executable_path/../Frameworks/${SSL_LIB}" \
  "${APP_FRAMEWORKS_DIR}/${SSL_LIB}"
install_name_tool -id \
  "@executable_path/../Frameworks/${CRYPTO_LIB}" \
  "${APP_FRAMEWORKS_DIR}/${CRYPTO_LIB}"

echo "▶ Правим зависимость ${SSL_LIB} → ${CRYPTO_LIB}..."
OLD_CRYPTO_IN_SSL=$(find_dependency "${APP_FRAMEWORKS_DIR}/${SSL_LIB}" "libcrypto")
if [ -n "${OLD_CRYPTO_IN_SSL}" ]; then
  install_name_tool -change \
    "${OLD_CRYPTO_IN_SSL}" \
    "@executable_path/../Frameworks/${CRYPTO_LIB}" \
    "${APP_FRAMEWORKS_DIR}/${SSL_LIB}"
fi

echo "▶ Правим ссылки на OpenSSL в corelib..."
install_name_tool -change \
  "${OLD_SSL}" \
  "@executable_path/../Frameworks/${SSL_LIB}" \
  "${CORELIB}"
if [ -n "${OLD_CRYPTO_IN_CORELIB}" ]; then
  install_name_tool -change \
    "${OLD_CRYPTO_IN_CORELIB}" \
    "@executable_path/../Frameworks/${CRYPTO_LIB}" \
    "${CORELIB}"
fi

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
