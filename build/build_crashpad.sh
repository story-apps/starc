#!/usr/bin/env bash
set -euo pipefail

# Парсинг параметров
BUILD_MODE="release"
TARGET_CPU="x64"
UNIVERSAL_BUILD=false

while [[ $# -gt 0 ]]; do
    case $1 in
        -d)
            BUILD_MODE="debug"
            shift
            ;;
        -a|--arch)
            TARGET_CPU="$2"
            shift 2
            ;;
        -u|--universal)
            UNIVERSAL_BUILD=true
            shift
            ;;
        *)
            echo "Error: Unexpected parameter: $1"
            echo "Usage: $0 [-d] [-a x86|x64|arm64] [-u]"
            echo "  -d: debug build (default: release)"
            echo "  -a, --arch: target architecture x86, x64, or arm64 (default: x64)"
            echo "  -u, --universal: build universal binary for macOS (x64 + arm64)"
            exit 1
            ;;
    esac
done

if [ "$TARGET_CPU" != "x86" ] && [ "$TARGET_CPU" != "x64" ] && [ "$TARGET_CPU" != "arm64" ]; then
    echo "Error: Invalid architecture '$TARGET_CPU'. Use x86, x64, or arm64"
    exit 1
fi

SCRIPT_DIR="$(pwd)"
echo "[*] Script directory: $SCRIPT_DIR"
echo "[*] build: $BUILD_MODE"
echo "[*] target_cpu: $TARGET_CPU"

# ---- НАСТРОЙКИ ----
ROOT_DIR="$SCRIPT_DIR/../src/3rd_party/chromium"
CRASHPAD_DIR="$ROOT_DIR/crashpad/crashpad"
DEPOT_TOOLS_DIR="$ROOT_DIR/depot_tools"

echo "[*] Crashpad build script"
echo "    CRASHPAD_DIR: $CRASHPAD_DIR"
echo "    DEPOT_TOOLS_DIR: $DEPOT_TOOLS_DIR"
echo

# 1. Определяем ОС
OS="$(uname -s)"
case "$OS" in
    Linux*) PLATFORM=linux ;;
    Darwin*) PLATFORM=mac ;;
    MINGW*|MSYS*|CYGWIN*) PLATFORM=win ;;
    *) echo "Unsupported OS: $OS" && exit 1 ;;
esac

echo "[*] Detected platform: $PLATFORM"
echo

# 2. Скачиваем depot_tools, если нет
if [ ! -d "$DEPOT_TOOLS_DIR" ]; then
    echo "[*] Installing depot_tools..."
    git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git "$DEPOT_TOOLS_DIR"
else
    echo "[*] depot_tools already exists, updating..."
    cd "$DEPOT_TOOLS_DIR"
	git checkout main
	git pull
fi
export PATH="$DEPOT_TOOLS_DIR:$PATH"

if [ "$PLATFORM" = "win" ]; then
    echo "[*] Updating depot_tools..."
    cd "$DEPOT_TOOLS_DIR"
    ./update_depot_tools.bat
fi

# 3. Скачиваем crashpad, если нет
if [ ! -d "$CRASHPAD_DIR" ]; then
	echo "[*] Getting the crashpad source..."
    mkdir -p "$(dirname "$CRASHPAD_DIR")"
    cd "$(dirname "$CRASHPAD_DIR")"
	fetch crashpad
else
    echo "[*] crashpad already exists"
fi

# 4. Обновляем crashpad
echo "[*] Syncing crashpad..."
cd "$CRASHPAD_DIR"
git checkout main
git pull -r
gclient syn

# Устанавливаем deployment target для macOS
if [ "$PLATFORM" = "mac" ]; then
    export MACOSX_DEPLOYMENT_TARGET=10.13
    echo "[*] Set MACOSX_DEPLOYMENT_TARGET=10.13"
fi

# 5. Генерируем билд через gn
BASE_OUT_DIR="$SCRIPT_DIR/../bin/crashpad"

if [ "$UNIVERSAL_BUILD" = true ] && [ "$PLATFORM" = "mac" ]; then
    echo "[*] Building universal binary (x64 + arm64)..."
    
    # Собираем для x64
    echo "[*] Building for x64..."
    OUT_DIR_X64="$BASE_OUT_DIR/${BUILD_MODE}_x64"
    mkdir -p "$OUT_DIR_X64"
    echo target_cpu=\"x64\" > "$OUT_DIR_X64/args.gn"
    echo "[*] Contents of args.gn for x64:"
    cat "$OUT_DIR_X64/args.gn"
    gn gen "$OUT_DIR_X64"
    ninja -C "$OUT_DIR_X64"
    
    # Собираем для arm64
    echo "[*] Building for arm64..."
    OUT_DIR_ARM64="$BASE_OUT_DIR/${BUILD_MODE}_arm64"
    mkdir -p "$OUT_DIR_ARM64"
    echo target_cpu=\"arm64\" > "$OUT_DIR_ARM64/args.gn"
    echo "[*] Contents of args.gn for arm64:"
    cat "$OUT_DIR_ARM64/args.gn"
    gn gen "$OUT_DIR_ARM64"
    ninja -C "$OUT_DIR_ARM64"
    
    # Объединяем библиотеки в универсальные бинарники
    echo "[*] Creating universal binaries..."
    OUT_DIR="$BASE_OUT_DIR/${BUILD_MODE}_universal"
    mkdir -p "$OUT_DIR/obj/client"
    mkdir -p "$OUT_DIR/obj/util"
    mkdir -p "$OUT_DIR/obj/third_party/mini_chromium/mini_chromium/base"
    
    # Объединяем статические библиотеки
    for lib in common client util base; do
        if [ "$lib" = "base" ]; then
            LIB_PATH="third_party/mini_chromium/mini_chromium/base"
        elif [ "$lib" = "common" ] || [ "$lib" = "client" ]; then
            LIB_PATH="client"
        else
            LIB_PATH="util"
        fi
        
        LIB_X64="$OUT_DIR_X64/obj/$LIB_PATH/lib${lib}.a"
        LIB_ARM64="$OUT_DIR_ARM64/obj/$LIB_PATH/lib${lib}.a"
        LIB_UNIVERSAL="$OUT_DIR/obj/$LIB_PATH/lib${lib}.a"
        
        if [ -f "$LIB_X64" ] && [ -f "$LIB_ARM64" ]; then
            lipo -create "$LIB_X64" "$LIB_ARM64" -output "$LIB_UNIVERSAL"
            echo "    Created universal lib${lib}.a"
        else
            echo "    Warning: Missing library for $lib (x64: $([ -f "$LIB_X64" ] && echo "yes" || echo "no"), arm64: $([ -f "$LIB_ARM64" ] && echo "yes" || echo "no"))"
        fi
    done
    
    # Объединяем mig_output
    MIG_X64="$OUT_DIR_X64/obj/util/libmig_output.a"
    MIG_ARM64="$OUT_DIR_ARM64/obj/util/libmig_output.a"
    MIG_UNIVERSAL="$OUT_DIR/obj/util/libmig_output.a"
    if [ -f "$MIG_X64" ] && [ -f "$MIG_ARM64" ]; then
        lipo -create "$MIG_X64" "$MIG_ARM64" -output "$MIG_UNIVERSAL"
        echo "    Created universal libmig_output.a"
    fi
    
    # Объединяем crashpad_handler
    HANDLER_X64="$OUT_DIR_X64/crashpad_handler"
    HANDLER_ARM64="$OUT_DIR_ARM64/crashpad_handler"
    HANDLER_UNIVERSAL="$OUT_DIR/crashpad_handler"
    if [ -f "$HANDLER_X64" ] && [ -f "$HANDLER_ARM64" ]; then
        lipo -create "$HANDLER_X64" "$HANDLER_ARM64" -output "$HANDLER_UNIVERSAL"
        chmod +x "$HANDLER_UNIVERSAL"
        echo "    Created universal crashpad_handler"
    fi
    
    TARGET_CPU="universal"
else
    # Обычная сборка для одной архитектуры
    echo "[*] Creating output directory..."
    OUT_DIR="$BASE_OUT_DIR/${BUILD_MODE}_${TARGET_CPU}"
    mkdir -p "$OUT_DIR"
    echo "[*] Output directory: $OUT_DIR (absolute: $(pwd)/$OUT_DIR)"

    if [ "$PLATFORM" = "win" ]; then
        if [ "$BUILD_MODE" = "debug" ]; then
            echo extra_cflags=\"/MDd\" > "$OUT_DIR/args.gn"
        else
            echo extra_cflags=\"/MD\" > "$OUT_DIR/args.gn"
        fi
    fi
    echo target_cpu=\"$TARGET_CPU\" >> "$OUT_DIR/args.gn"

    # Выводим содержимое args.gn для отладки 
    echo "[*] Contents of args.gn:"
    cat "$OUT_DIR/args.gn"

    echo "[*] Generating build configuration..."
    gn gen "$OUT_DIR"

    # 6. Собираем через ninja
    echo "[*] Building crashpad..."
    ninja -v -C "$OUT_DIR"
fi

echo
echo "[*] Build completed successfully!"
if [ "$PLATFORM" = "win" ]; then
    echo "    crashpad_handler: $OUT_DIR/crashpad_handler.exe"
else
    echo "    crashpad_handler: $OUT_DIR/crashpad_handler"
    echo "    Architecture: $TARGET_CPU"
fi
