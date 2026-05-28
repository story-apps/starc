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
APP_MACOS_DIR="${APP_BUNDLE}/Contents/MacOS"
APP_FRAMEWORKS_DIR="${APP_BUNDLE}/Contents/Frameworks"
CORELIB="${APP_FRAMEWORKS_DIR}/libcorelib.dylib"

print_otool_info() {
  local target="$1"

  echo "--- otool -L ${target}"
  otool -L "${target}" 2>&1 || true
  echo "--- otool -l ${target} (LC_RPATH only)"
  otool -l "${target}" 2>/dev/null | awk '
    /cmd LC_RPATH/ { in_rpath = 1; print }
    in_rpath && /path / { print; in_rpath = 0 }
  ' || true
}

print_openssl_diagnostics() {
  echo "--- OpenSSL diagnostics"
  echo "APP_BUNDLE=${APP_BUNDLE}"
  echo "APP_FRAMEWORKS_DIR=${APP_FRAMEWORKS_DIR}"
  echo "CORELIB=${CORELIB}"
  echo "OPENSSL_PREFIX=${OPENSSL_PREFIX}"

  echo "--- Frameworks OpenSSL files"
  find "${APP_FRAMEWORKS_DIR}" -maxdepth 1 \( -name 'libcrypto*.dylib' -o -name 'libssl*.dylib' \) -print 2>/dev/null || true

  echo "--- Common Homebrew OpenSSL files"
  for openssl_dir in \
    "${OPENSSL_PREFIX}/lib" \
    /opt/homebrew/opt/openssl@3/lib \
    /opt/homebrew/opt/openssl/lib \
    /usr/local/opt/openssl@3/lib \
    /usr/local/opt/openssl/lib; do
    [ -n "${openssl_dir}" ] || continue
    [ -d "${openssl_dir}" ] || continue
    echo "${openssl_dir}:"
    find "${openssl_dir}" -maxdepth 1 \( -name 'libcrypto*.dylib' -o -name 'libssl*.dylib' \) -print 2>/dev/null || true
  done
}

find_dependencies() {
  local target="$1"
  local library_name="$2"

  otool -L "${target}" 2>/dev/null \
    | awk -v library_name="${library_name}" 'NR > 1 && $1 ~ library_name "([.][0-9]+)*[.]dylib$" { print $1 }' \
    | sort -u
}

find_dependency() {
  local target="$1"
  local library_name="$2"

  find_dependencies "${target}" "${library_name}" | head -1
}

find_resolved_dependency() {
  local target="$1"
  local library_name="$2"
  local dependency=""
  local resolved_dependency=""

  find_dependencies "${target}" "${library_name}" | while read -r dependency; do
    resolved_dependency=$(resolve_dependency_path "${target}" "${dependency}")
    [ -f "${resolved_dependency}" ] || continue
    echo "${resolved_dependency}"
    break
  done
}

relink_dependencies() {
  local target="$1"
  local library_name="$2"
  local new_dependency="$3"
  local dependency=""

  find_dependencies "${target}" "${library_name}" | while read -r dependency; do
    [ -n "${dependency}" ] || continue
    [ "${dependency}" != "${new_dependency}" ] || continue
    install_name_tool -change "${dependency}" "${new_dependency}" "${target}"
  done
}

find_openssl_file_by_name() {
  local library_file="$1"

  for openssl_dir in \
    "${OPENSSL_PREFIX}/lib" \
    /opt/homebrew/opt/openssl@3/lib \
    /opt/homebrew/opt/openssl/lib \
    /usr/local/opt/openssl@3/lib \
    /usr/local/opt/openssl/lib; do
    [ -n "${openssl_dir}" ] || continue
    [ -f "${openssl_dir}/${library_file}" ] || continue
    echo "${openssl_dir}/${library_file}"
    return
  done
}

resolve_dependency_path() {
  local target="$1"
  local dependency="$2"
  local dependency_file=""

  case "${dependency}" in
    /*)
      echo "${dependency}"
      ;;
    @executable_path/../Frameworks/*)
      echo "${APP_FRAMEWORKS_DIR}/$(basename "${dependency}")"
      ;;
    @executable_path/*)
      echo "${APP_MACOS_DIR}/${dependency#@executable_path/}"
      ;;
    @loader_path/*)
      echo "$(dirname "${target}")/${dependency#@loader_path/}"
      ;;
    @rpath/*)
      dependency_file=$(basename "${dependency}")
      if [ -f "${APP_FRAMEWORKS_DIR}/${dependency_file}" ]; then
        echo "${APP_FRAMEWORKS_DIR}/${dependency_file}"
      else
        find_openssl_file_by_name "${dependency_file}"
      fi
      ;;
    *)
      echo "${dependency}"
      ;;
  esac
}

copy_to_frameworks() {
  local source="$1"
  local destination="${APP_FRAMEWORKS_DIR}/$(basename "${source}")"

  if [ -e "${destination}" ] && [ "${source}" -ef "${destination}" ]; then
    echo "▶ $(basename "${source}") уже находится в Frameworks"
  else
    cp -fL "${source}" "${APP_FRAMEWORKS_DIR}/"
  fi
}

echo "▶ Создаём Frameworks директорию..."
mkdir -p "${APP_FRAMEWORKS_DIR}"

if [ ! -f "${CORELIB}" ]; then
  CORELIB=$(find "${APP_FRAMEWORKS_DIR}" -maxdepth 1 -name 'libcorelib*.dylib' -type f 2>/dev/null | head -1)
fi
if [ -z "${CORELIB}" ] || [ ! -f "${CORELIB}" ]; then
  echo "Error! corelib was not found in ${APP_FRAMEWORKS_DIR}."
  print_openssl_diagnostics
  exit 1
fi

echo "▶ Диагностика corelib перед обработкой OpenSSL..."
print_otool_info "${CORELIB}"
print_openssl_diagnostics

echo "▶ Определяем исходный путь OpenSSL через otool в $(basename "${CORELIB}")..."
OLD_CRYPTO=$(find_dependencies "${CORELIB}" "libcrypto" | paste -sd ' ' -)
CRYPTO_SOURCE=$(find_resolved_dependency "${CORELIB}" "libcrypto")
if [ -z "${OLD_CRYPTO}" ] || [ -z "${CRYPTO_SOURCE}" ] || [ ! -f "${CRYPTO_SOURCE}" ]; then
  echo "Error! OpenSSL libcrypto dependency was not found in ${CORELIB}."
  echo "OLD_CRYPTO=${OLD_CRYPTO}"
  echo "CRYPTO_SOURCE=${CRYPTO_SOURCE}"
  print_otool_info "${CORELIB}"
  print_openssl_diagnostics
  exit 1
fi

OPENSSL_LIB_DIR=$(dirname "${CRYPTO_SOURCE}")
OLD_SSL=$(find_dependencies "${CORELIB}" "libssl" | paste -sd ' ' -)
SSL_SOURCE=$(find_resolved_dependency "${CORELIB}" "libssl")
if [ -z "${SSL_SOURCE}" ] || [ ! -f "${SSL_SOURCE}" ]; then
  SSL_SOURCE=$(ls "${OPENSSL_LIB_DIR}"/libssl*.dylib 2>/dev/null | head -1)
fi

echo "▶ OpenSSL найден в ${OPENSSL_LIB_DIR}"
echo "▶ OLD_CRYPTO=${OLD_CRYPTO}"
echo "▶ CRYPTO_SOURCE=${CRYPTO_SOURCE}"
echo "▶ OLD_SSL=${OLD_SSL}"
echo "▶ SSL_SOURCE=${SSL_SOURCE}"

echo "▶ Копируем OpenSSL библиотеки..."
copy_to_frameworks "${CRYPTO_SOURCE}"
CRYPTO_LIB=$(basename "${CRYPTO_SOURCE}")

if [ -n "${SSL_SOURCE}" ] && [ -f "${SSL_SOURCE}" ]; then
  copy_to_frameworks "${SSL_SOURCE}"
  SSL_LIB=$(basename "${SSL_SOURCE}")
  echo "▶ Найдены: ${CRYPTO_LIB}, ${SSL_LIB}"
else
  SSL_LIB=""
  echo "▶ Найдена: ${CRYPTO_LIB}"
  echo "▶ libssl не найдена рядом с libcrypto, пропускаем её копирование"
fi

echo "▶ Диагностика скопированных OpenSSL библиотек..."
print_otool_info "${APP_FRAMEWORKS_DIR}/${CRYPTO_LIB}"
if [ -n "${SSL_LIB}" ]; then
  print_otool_info "${APP_FRAMEWORKS_DIR}/${SSL_LIB}"
fi

echo "▶ Обновляем install_name (id)..."
install_name_tool -id \
  "@executable_path/../Frameworks/${CRYPTO_LIB}" \
  "${APP_FRAMEWORKS_DIR}/${CRYPTO_LIB}"

if [ -n "${SSL_LIB}" ]; then
  install_name_tool -id \
    "@executable_path/../Frameworks/${SSL_LIB}" \
    "${APP_FRAMEWORKS_DIR}/${SSL_LIB}"

  echo "▶ Правим зависимость ${SSL_LIB} → ${CRYPTO_LIB}..."
  relink_dependencies \
    "${APP_FRAMEWORKS_DIR}/${SSL_LIB}" \
    "libcrypto" \
    "@executable_path/../Frameworks/${CRYPTO_LIB}"
fi

echo "▶ Правим ссылки на OpenSSL в corelib..."
relink_dependencies \
  "${CORELIB}" \
  "libcrypto" \
  "@executable_path/../Frameworks/${CRYPTO_LIB}"

if [ -n "${SSL_LIB}" ]; then
  relink_dependencies \
    "${CORELIB}" \
    "libssl" \
    "@executable_path/../Frameworks/${SSL_LIB}"
fi

echo "▶ Диагностика corelib после обработки OpenSSL..."
print_otool_info "${CORELIB}"

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
