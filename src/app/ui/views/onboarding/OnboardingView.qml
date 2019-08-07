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
        initialItem: languageView
    }

    RowLayout {
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
            font: DesignSystem.font.button
            highlighted: true
            text: qsTr("Continue")

            onClicked: {
                OnboardingViewModel.stepIndex += 1;
            }
        }

        Button {
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
        id: languageView

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
        id: themeView

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
    // Реакция на изменение текущего шага посадки
    //
    Connections {
        target: OnboardingViewModel
        onStepIndexChanged: {
            let isFinalStep = false;
            switch (_index) {
            case 0:
                content.replace(languageView);
                break;

            case 1:
                content.replace(themeView);
                break;

            case 2:
                break;

            default:
                isFinalStep = true;
                break;
            }
        }
    }
}
