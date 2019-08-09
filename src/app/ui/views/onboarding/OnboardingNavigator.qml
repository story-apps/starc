import QtQuick 2.12

import app.starc 1.0


Item {
    id: root

    //
    // Обновить индекс текущего шага
    //
    function updateStepIndex(_index) {
        //
        // Отнимаем единицу, т.к. пользователю отображаются номера начиная с единицы, а не с нуля
        //
        OnboardingViewModel.stepIndex = (_index - 1);
    }

    Rectangle {
        anchors.fill: parent
        color: DesignSystem.color.primary
    }

    OnboardingStep {
        id: languageStep

        anchors {
            left: parent.left
            top: parent.top
            right: parent.right
        }
        checked: true
        index: "1"
        text: "Choose preffered language"
        pathAtBottom: true

        onClicked: {
            updateStepIndex(index);
        }
    }

    OnboardingStep {
        id: themeStep

        anchors {
            left: parent.left
            top: languageStep.bottom
            right: parent.right
        }
        index: "2"
        text: "Choose application theme"
        pathAtTop: true

        onClicked: {
            updateStepIndex(index);
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
                languageStep.checked = true;
                languageStep.finished = false;
                themeStep.checked = false;
                themeStep.finished = false;
                break;

            case 1:
                languageStep.checked = false;
                languageStep.finished = true;
                themeStep.checked = true;
                themeStep.finished = false;
                break;

            default:
                languageStep.checked = false;
                languageStep.finished = true;
                themeStep.checked = false;
                themeStep.finished = true;
                break;
            }
        }
    }
}
