#!/bin/zsh

set -euo pipefail

if [[ $# -ne 2 ]]; then
    print -u2 "Usage: $0 <built .app> <installed .app>"
    exit 2
fi

built_app="$1"
installed_app="$2"
built_corelib="$built_app/Contents/Frameworks/libcorelib.1.0.0.dylib"
built_coreplugin="$built_app/Contents/PlugIns/libcoreplugin.dylib"
built_screenplay_plugin="$built_app/Contents/PlugIns/libscreenplaytextplugin.dylib"
installed_corelib="$installed_app/Contents/Frameworks/libcorelib.1.0.0.dylib"
installed_coreplugin="$installed_app/Contents/PlugIns/libcoreplugin.dylib"
installed_screenplay_plugin="$installed_app/Contents/PlugIns/libscreenplaytextplugin.dylib"

for required_file in "$built_corelib" "$built_coreplugin" "$installed_corelib" \
    "$installed_coreplugin" "$built_screenplay_plugin" "$installed_screenplay_plugin"; do
    if [[ ! -f "$required_file" ]]; then
        print -u2 "Required file is missing: $required_file"
        exit 1
    fi
done

/bin/cp "$built_corelib" "$installed_corelib"
/bin/cp "$built_coreplugin" "$installed_coreplugin"
/bin/cp "$built_screenplay_plugin" "$installed_screenplay_plugin"

patch_qt_dependencies() {
    local binary="$1"
    local dependency framework bundled_dependency

    while IFS= read -r dependency; do
        [[ "$dependency" == /*/Qt*.framework/Versions/A/Qt* ]] || continue
        framework="${dependency:t}"
        bundled_dependency="@executable_path/../Frameworks/$framework.framework/Versions/A/$framework"
        /usr/bin/install_name_tool -change "$dependency" "$bundled_dependency" "$binary"
    done < <(/usr/bin/otool -L "$binary" | /usr/bin/tail -n +2 | /usr/bin/awk '{ print $1 }')
}

patch_qt_dependencies "$installed_corelib"
patch_qt_dependencies "$installed_coreplugin"
patch_qt_dependencies "$installed_screenplay_plugin"

if /usr/bin/otool -L "$installed_corelib" "$installed_coreplugin" \
    "$installed_screenplay_plugin" \
    | /usr/bin/grep -Eq '^[[:space:]]+/.*Qt[^/]*\.framework/Versions/A/Qt'; then
    print -u2 "A non-bundled Qt framework dependency remains after patching."
    exit 1
fi

/usr/bin/codesign --force --deep --sign - "$installed_app"
/usr/bin/codesign --verify --deep --strict --verbose=2 "$installed_app"

print "Installed and verified: $installed_app"
