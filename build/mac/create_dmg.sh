#!/bin/bash

#
# сформировать app-файл
#
cp ../../src/_build/starcapp.app starc.app
$Qt5_Dir/bin/macdeployqt starcapp.app

#
# подпишем app-файл
#
codesign --force --entitlements entitlements.plist --deep --sign "$APPLE_SIGNING_IDENTITY" --options "runtime" "starcapp.app/"

#
# сформировать архив с приложением
#
ditto -ck --keepParent starcapp.app starcapp.zip

#
# отправить архив на проверку
#
xcrun notarytool submit starcapp.zip --apple-id $APPLE_ID --password $APPLE_PASSWORD --team-id $APPLE_TEAM_ID --wait

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
mv -f starcapp.dmg starc-setup-$1.dmg
