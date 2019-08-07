pragma Singleton
import QtQuick 2.12

//
// Дизайн система приложения
//
QtObject {
    //
    // Цвета
    //
    property QtObject color: QtObject {
        //
        // ... базовые цвета
        //
        property color primary: "#1F1F1F"
        property color primaryDark: "#0A0A0A"
        property color secondary: "#448AFF"
        property color background: "#FFFFFF"
        property color surface: "#FFFFFF"
        property color error: "#B00020"
        property color shadow: Qt.rgba(0, 0, 0, 0.56)
        //
        // ... контрастные цвета для базовых
        //
        property color primaryContrast: "#FFFFFF"
        property color secondaryContrast: "#FFFFFF"
        property color backgroundContrast: "#000000"
        property color surfaceContrast: "#000000"
        property color errorContrast: "#FFFFFF"
        //
        // ... дополнительные цвета
        //
        property color inactiveColor: Qt.darker(background, 3.0)
    }

    //
    // Шрифты
    //
    property QtObject font: QtObject {
        property var h1: Qt.font({ family: "Roboto", weight: Font.Light, pixelSize: 96, letterSpacing: -1.5 })
        property var h2: Qt.font({ family: "Roboto", weight: Font.Light, pixelSize: 60, letterSpacing: -0.5 })
        property var h3: Qt.font({ family: "Roboto", weight: Font.Regular, pixelSize: 48, letterSpacing: 0.0 })
        property var h4: Qt.font({ family: "Roboto", weight: Font.Regular, pixelSize: 34, letterSpacing: 0.25 })
        property var h5: Qt.font({ family: "Roboto", weight: Font.Regular, pixelSize: 24, letterSpacing: 0.0 })
        property var h6: Qt.font({ family: "Roboto", weight: Font.Medium, pixelSize: 20, letterSpacing: 0.15 })
        property var subtitle1: Qt.font({ family: "Roboto", weight: Font.Regular, pixelSize: 16, letterSpacing: 0.15 })
        property var subtitle2: Qt.font({ family: "Roboto", weight: Font.Medium, pixelSize: 14, letterSpacing: 0.1 })
        property var body1: Qt.font({ family: "Roboto", weight: Font.Regular, pixelSize: 16, letterSpacing: 0.5 })
        property var body2: Qt.font({ family: "Roboto", weight: Font.Regular, pixelSize: 14, letterSpacing: 0.25 })
        property var button: Qt.font({ family: "Roboto", weight: Font.Medium, pixelSize: 14, capitalization: Font.AllUppercase, letterSpacing: 1.25 })
        property var caption: Qt.font({ family: "Roboto", weight: Font.Regular, pixelSize: 12, letterSpacing: 0.4 })
        property var overline: Qt.font({ family: "Roboto", weight: Font.Regular, pixelSize: 10, capitalization: Font.AllUppercase, letterSpacing: 1.5 })

        // Cheat sheet - https://cdn.materialdesignicons.com/3.8.95/
        property var iconsSmall: Qt.font({ family: "Material Design Icons", pixelSize: 16 })
    }

    //
    // Компоновка элементов
    //
    property QtObject layout: QtObject {
        property real spacing: 8
        property real padding: 24
    }

    //
    // Панель разделяющая экран на части
    //
    property QtObject splitView: QtObject {
        property int handleWidth: 4
        property int handleHeight: 4
    }

    //
    // Тулбар
    //
    property QtObject toolBar: QtObject {
        property real height: 40
    }

    //
    // Степпер
    //
    property QtObject stepper: QtObject {
        property real height: 72
        property real padding: 24
        property real textSpacing: 12
        property real pathSpacing: 8

        property real iconSize: 24
        property real pathWidth: 2
    }
}
