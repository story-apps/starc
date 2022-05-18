#include "icon_helper.h"

#include <QApplication>


namespace {
}

QString IconHelper::directedIcon(const QString& _icon)
{
    if (QApplication::isLeftToRight()) {
        return _icon;
    }

    const QHash<QString, QString> iconsMapping = {
        { u8"\U000F004D", u8"\U000F0054" }, // arrow left
        { u8"\U000F0054", u8"\U000F004D" }, // arrow right
        { u8"\U000F0141", u8"\U000F0142" }, // chevron left
        { u8"\U000F0142", u8"\U000F0141" }, // chevron right
        { u8"\U000F035F", u8"\U000F035E" }, // menu left
        { u8"\U000F035E", u8"\U000F035F" }, // menu right
        { u8"\U000F054C", u8"\U000F044E" }, // undo
        { u8"\U000F044E", u8"\U000F054C" }, // redo
    };

    return iconsMapping.value(_icon, _icon);
}

QString IconHelper::chevronRight()
{
    return directedIcon(u8"\U000F0142");
}

QString IconHelper::menuRight()
{
    return directedIcon(u8"\U000F035F");
}
