#!/bin/bash

#
# сформировать app-файл
#
cp -R ../../src/_build/starcapp.app Story\ Architect.app
find Story\ Architect.app -name "*.dSYM" -print0 | xargs -0 rm -rf
macdeployqt Story\ Architect.app

#
# проверяем наличие crashpad_handler
#
if [ ! -f "Story Architect.app/Contents/MacOS/crashpad/crashpad_handler" ]; then
    echo "Error: crashpad_handler not found in Story Architect.app/Contents/MacOS/crashpad/"
    echo "Make sure crashpad was built and copied during the build process."
    exit 1
fi
chmod +x "Story Architect.app/Contents/MacOS/crashpad/crashpad_handler"

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
