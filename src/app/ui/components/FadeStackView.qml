import QtQuick 2.12
import QtQuick.Controls 2.13

//
// Стэк представлений, компоненты в котором плавно переходят из одного в другой
//
StackView {
    replaceEnter: Transition {
        NumberAnimation { property: "opacity"; from: 0; to: 1; }
    }
    replaceExit: Transition {
        NumberAnimation { property: "opacity"; from: 1; to: 0; }
    }
}
