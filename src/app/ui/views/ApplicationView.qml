import QtQuick 2.13
import QtQuick.Controls 2.13
import QtQuick.Controls.Material 2.13

import app.starc 1.0
import "../components"

ApplicationWindow {
    id: root

    visible: true
    width: 640
    height: 480
    title: qsTr("Hello World")

    Material.theme: Material.Light
    Material.primary: DesignSystem.color.primary
    Material.background: DesignSystem.color.background
    Material.foreground: DesignSystem.color.backgroundContrast
    Material.accent: DesignSystem.color.secondary

    SplitView {
        id: content

        anchors.fill: parent
        orientation: Qt.Horizontal

        handle: Rectangle {
            implicitWidth: DesignSystem.splitView.handleWidth
            color: DesignSystem.color.primary
        }

        Item {
            id: sideBar

            SplitView.fillWidth: true
            SplitView.preferredWidth: root.width / 2

            MouseArea {
                anchors.fill: parent
            }

            Rectangle {
                anchors.fill: parent
                color: DesignSystem.color.primary
            }

            FadeStackView {
                id: toolBar

                anchors {
                    left: parent.left
                    top: parent.top
                    right: parent.right
                }
                height: DesignSystem.toolBar.height
                initialItem: ApplicationViewModel.initialToolBar

                Connections {
                    target: ApplicationViewModel
                    onCurrentToolBarChanged: { toolBar.replace(_toolBar); }
                }
            }

            FadeStackView {
                id: navigator

                anchors {
                    left: parent.left
                    top: toolBar.bottom
                    right: parent.right
                    bottom: parent.bottom
                }
                initialItem: ApplicationViewModel.initialNavigator

                Connections {
                    target: ApplicationViewModel
                    onCurrentNavigatorChanged: { navigator.replace(_navigator); }
                }
            }
        }

        FadeStackView {
            id: view

            SplitView.fillWidth: true
            SplitView.preferredWidth: root.width / 2
            initialItem: ApplicationViewModel.initialView

            MouseArea {
                anchors.fill: parent
            }

            Rectangle {
                anchors.fill: parent
                color: DesignSystem.color.background
            }

            Connections {
                target: ApplicationViewModel
                onCurrentViewChanged: { view.replace(_view); }
            }
        }
    }
}
