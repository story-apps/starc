import QtQuick 2.12

import app.starc 1.0

Item {
    id: root

    property bool checked: false
    property bool finished: false
    property alias index: stepIndex.text
    property alias text: stepName.text
    property alias pathAtTop: topPath.visible
    property alias pathAtBottom: bottomPath.visible

    signal clicked()

    height: DesignSystem.stepper.height

    Rectangle {
        id: stepIcon

        anchors {
            left: parent.left
            leftMargin: DesignSystem.stepper.padding
            top: parent.top
            topMargin: DesignSystem.stepper.padding
        }
        height: DesignSystem.stepper.iconSize
        width: DesignSystem.stepper.iconSize
        radius: height / 2
        color: (root.checked || root.finished)
               ? DesignSystem.color.secondary
               : DesignSystem.color.inactiveColor

        Text {
            id: stepIndex

            visible: !root.finished
            anchors.centerIn: parent
            font: DesignSystem.font.caption
            color: DesignSystem.color.primaryContrast
        }

        Text {
            id: stepIndexFinished

            visible: root.finished
            anchors.centerIn: parent
            font: DesignSystem.font.iconsSmall
            color: DesignSystem.color.primaryContrast
            text: "\uf12c"
        }


        Behavior on color { ColorAnimation {} }
    }

    Text {
        id: stepName

        anchors {
            left: stepIcon.right
            leftMargin: DesignSystem.stepper.textSpacing
            top: parent.top
            right: parent.right
            bottom: parent.bottom
        }
        font: DesignSystem.font.subtitle2
        color: DesignSystem.color.primaryContrast
        verticalAlignment: Qt.AlignVCenter
    }

    Rectangle {
        id: topPath

        visible: false
        anchors {
            left: parent.left
            leftMargin: DesignSystem.stepper.padding + ((DesignSystem.stepper.iconSize - DesignSystem.stepper.pathWidth) / 2)
            top: parent.top
        }
        width: DesignSystem.stepper.pathWidth
        height: DesignSystem.stepper.padding - DesignSystem.stepper.pathSpacing
        color: DesignSystem.color.inactiveColor
    }

    Rectangle {
        id: bottomPath

        visible: false
        anchors {
            left: parent.left
            leftMargin: topPath.anchors.leftMargin
            bottom: parent.bottom
        }
        width: DesignSystem.stepper.pathWidth
        height: DesignSystem.stepper.padding - DesignSystem.stepper.pathSpacing
        color: DesignSystem.color.inactiveColor
    }

    MouseArea {
        anchors.fill: parent

        onClicked: {
            root.clicked()
        }
    }
}
