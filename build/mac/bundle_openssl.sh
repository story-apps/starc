#!/bin/bash

APP_BUNDLE="$1"

if [ -z "${APP_BUNDLE}" ]; then
  echo "Error! App bundle path is not specified."
  exit 1
fi

APP_MACOS_DIR="${APP_BUNDLE}/Contents/MacOS"
APP_FRAMEWORKS_DIR="${APP_BUNDLE}/Contents/Frameworks"
CORELIB="${APP_FRAMEWORKS_DIR}/libcorelib.dylib"

find_dependencies() {
  local target="$1"
  local library_name="$2"

  otool -L "${target}" 2>/dev/null \
    | awk -v library_name="${library_name}" 'NR > 1 && $1 ~ library_name "([.][0-9]+)*[.]dylib$" { print $1 }' \
    | sort -u
}

find_openssl_file() {
  local library_file="$1"

  for openssl_dir in \
    "${OPENSSL_PREFIX}/lib" \
    /opt/homebrew/opt/openssl@3/lib \
    /opt/homebrew/opt/openssl/lib \
    /usr/local/opt/openssl@3/lib \
    /usr/local/opt/openssl/lib; do
    [ -n "${openssl_dir}" ] || continue
    [ "${openssl_dir}" != "/lib" ] || continue
    [ -f "${openssl_dir}/${library_file}" ] || continue
    echo "${openssl_dir}/${library_file}"
    return
  done
}

find_first_openssl_file() {
  local library_pattern="$1"

  for openssl_dir in \
    "${OPENSSL_PREFIX}/lib" \
    /opt/homebrew/opt/openssl@3/lib \
    /opt/homebrew/opt/openssl/lib \
    /usr/local/opt/openssl@3/lib \
    /usr/local/opt/openssl/lib; do
    [ -n "${openssl_dir}" ] || continue
    [ "${openssl_dir}" != "/lib" ] || continue
    [ -d "${openssl_dir}" ] || continue
    find "${openssl_dir}" -maxdepth 1 -name "${library_pattern}" -print 2>/dev/null | head -1
  done | head -1
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
        find_openssl_file "${dependency_file}"
      fi
      ;;
  esac
}

find_resolved_dependency() {
  local target="$1"
  local library_name="$2"
  local dependency=""
  local resolved_dependency=""
  local fallback_dependency=""

  while read -r dependency; do
    resolved_dependency=$(resolve_dependency_path "${target}" "${dependency}")
    [ -f "${resolved_dependency}" ] || continue
    case "${resolved_dependency}" in
      "${APP_FRAMEWORKS_DIR}"/*)
        echo "${resolved_dependency}"
        return
        ;;
    esac
    [ -n "${fallback_dependency}" ] || fallback_dependency="${resolved_dependency}"
  done <<EOF_DEPENDENCIES
$(find_dependencies "${target}" "${library_name}")
EOF_DEPENDENCIES

  [ -n "${fallback_dependency}" ] && echo "${fallback_dependency}"
}

copy_to_frameworks() {
  local source="$1"
  local destination="${APP_FRAMEWORKS_DIR}/$(basename "${source}")"

  if [ -e "${destination}" ] && [ "${source}" -ef "${destination}" ]; then
    return
  fi

  cp -fL "${source}" "${APP_FRAMEWORKS_DIR}/"
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

echo "▶ Подготавливаем OpenSSL библиотеки..."
mkdir -p "${APP_FRAMEWORKS_DIR}"

if [ ! -f "${CORELIB}" ]; then
  CORELIB=$(find "${APP_FRAMEWORKS_DIR}" -maxdepth 1 -name 'libcorelib*.dylib' -type f 2>/dev/null | head -1)
fi
if [ -z "${CORELIB}" ] || [ ! -f "${CORELIB}" ]; then
  echo "Error! corelib was not found in ${APP_FRAMEWORKS_DIR}."
  exit 1
fi

CRYPTO_SOURCE=$(find_resolved_dependency "${CORELIB}" "libcrypto")
if [ -z "${CRYPTO_SOURCE}" ] || [ ! -f "${CRYPTO_SOURCE}" ]; then
  echo "Error! OpenSSL libcrypto dependency was not found in ${CORELIB}."
  otool -L "${CORELIB}" || true
  exit 1
fi

SSL_SOURCE=$(find_resolved_dependency "${CORELIB}" "libssl")
if [ -z "${SSL_SOURCE}" ] || [ ! -f "${SSL_SOURCE}" ]; then
  SSL_SOURCE=$(find_first_openssl_file 'libssl*.dylib')
fi

copy_to_frameworks "${CRYPTO_SOURCE}"
CRYPTO_LIB=$(basename "${CRYPTO_SOURCE}")
install_name_tool -id \
  "@executable_path/../Frameworks/${CRYPTO_LIB}" \
  "${APP_FRAMEWORKS_DIR}/${CRYPTO_LIB}"

if [ -n "${SSL_SOURCE}" ] && [ -f "${SSL_SOURCE}" ]; then
  copy_to_frameworks "${SSL_SOURCE}"
  SSL_LIB=$(basename "${SSL_SOURCE}")
  install_name_tool -id \
    "@executable_path/../Frameworks/${SSL_LIB}" \
    "${APP_FRAMEWORKS_DIR}/${SSL_LIB}"
  relink_dependencies \
    "${APP_FRAMEWORKS_DIR}/${SSL_LIB}" \
    "libcrypto" \
    "@executable_path/../Frameworks/${CRYPTO_LIB}"
fi

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

echo "▶ OpenSSL готов: ${CRYPTO_LIB}${SSL_LIB:+, ${SSL_LIB}}"
