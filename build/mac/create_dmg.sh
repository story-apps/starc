#!/bin/bash

#
# сформировать app-файл
#
cp -R ../../src/_build/starcapp.app starcapp.app
macdeployqt starcapp.app

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
codesign --force --entitlements entitlements.plist --deep --sign "$APPLE_SIGNING_IDENTITY" --options "runtime" "starcapp.app/" -v

#
# сформировать архив с приложением
#
ditto -ck --keepParent starcapp.app starcapp.zip

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
xcrun stapler staple starcapp.app

#
# Создаём dmg-файл
#
./make_dmg.sh -i starc.app/Contents/Resources/icon.icns -b cover.png -c "462:252:176:258" -s "640:400"  starcapp.app
mv -f starcapp.dmg starc-setup.dmg
