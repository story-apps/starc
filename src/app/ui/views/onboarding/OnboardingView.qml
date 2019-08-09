import QtQuick 2.0
import QtQuick.Controls 2.13
import QtQuick.Controls.Material 2.13
import QtQuick.Layouts 1.13

import app.starc 1.0
import "../../components"


Item {
    Rectangle {
        anchors.fill: parent
        color: DesignSystem.color.background
    }

    FadeStackView {
        id: content

        anchors.fill: parent
        anchors.margins: DesignSystem.layout.padding
        initialItem: languagePage
    }

    RowLayout {
        id: navigationButtons

        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
            margins: DesignSystem.layout.padding
        }
        spacing: DesignSystem.layout.spacing

        Item {
            Layout.fillWidth: true
        }

        Button {
            id: goToNext

            font: DesignSystem.font.button
            highlighted: true
            text: qsTr("Continue")

            onClicked: {
                OnboardingViewModel.stepIndex += 1;
            }
        }

        Button {
            id: goToEnd

            font: DesignSystem.font.button
            text: qsTr("Skip onboarding")

            onClicked: {
                OnboardingViewModel.finish();
            }
        }
    }

    //
    // Страница выбора языка
    //

    ButtonGroup {
        id: languageGroup
    }

    Item {
        id: languagePage

        visible: false

        Column {
            spacing: DesignSystem.layout.spacing

            Text {
                font: DesignSystem.font.h5
                text: qsTr("Choose preferred language")
            }

            RadioButton {
                ButtonGroup.group: languageGroup
                font: DesignSystem.font.body1
                checked: true
                text: qsTr("Detect language from system")
            }

            Row {
                spacing: DesignSystem.layout.spacing

                RadioButton {
                    ButtonGroup.group: languageGroup
                    font: DesignSystem.font.body1
                    text: "English"
                }

                RadioButton {
                    ButtonGroup.group: languageGroup
                    font: DesignSystem.font.body1
                    text: "Русский"
                }
            }
        }
    }

    //
    // Страница настройки внешнего вида
    //

    ButtonGroup {
        id: themeGroup
    }

    Item {
        id: themePage

        visible: false

        Column {
            spacing: DesignSystem.layout.spacing

            Text {
                font: DesignSystem.font.h5
                text: qsTr("Choose application theme")
            }

            RadioButton {
                ButtonGroup.group: themeGroup
                font: DesignSystem.font.body1
                checked: true
                text: qsTr("Light theme")
            }

            Text {
                font: DesignSystem.font.body2
                wrapMode: Text.WordWrap
                text: qsTr("Theme is convenient for work with sufficient light")
            }

            RadioButton {
                ButtonGroup.group: themeGroup
                font: DesignSystem.font.body1
                text: qsTr("Dark theme")
            }

            Text {
                font: DesignSystem.font.body2
                text: qsTr("Theme is more suitable for work in dimly lit rooms, and also in the evening or at night.")
            }
        }
    }

    //
    // Страница завершения посадки
    //

    Item {
        id: lastPage

        visible: false

        Column {
            spacing: DesignSystem.layout.spacing

            Text {
                font: DesignSystem.font.h5
                text: qsTr("Starting application...")
            }
        }

        BusyIndicator {
            anchors.centerIn: parent
            running: true
        }
    }

    //
    // Реакция на изменение текущего шага посадки
    //
    Connections {
        target: OnboardingViewModel
        onStepIndexChanged: {
            switch (_index) {
            case 0:
                content.replace(languagePage);
                break;

            case 1:
                content.replace(themePage);
                break;

            default:
                content.replace(lastPage);
                navigationButtons.visible = false;
                break;
            }
        }
    }
}
