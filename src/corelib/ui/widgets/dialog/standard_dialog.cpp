#include "standard_dialog.h"

#include "dialog.h"

#include <ui/design_system/design_system.h>

#include <QApplication>
#include <QRandomGenerator>

namespace {
QString generateOkTerm()
{
    QVector<QString> terms = { QApplication::translate("StandardDialog", "Got it"),
                               QApplication::translate("StandardDialog", "Ah, perfect"),
                               QApplication::translate("StandardDialog", "This is fine"),
                               QApplication::translate("StandardDialog", "Not bad"),
                               QApplication::translate("StandardDialog", "I see"),
                               QApplication::translate("StandardDialog", "Wonderful"),
                               QApplication::translate("StandardDialog", "Alright"),
                               QApplication::translate("StandardDialog", "So-so"),
                               QApplication::translate("StandardDialog", "Amazing"),
                               QApplication::translate("StandardDialog", "Cool"),
                               QApplication::translate("StandardDialog", "Gorgeous"),
                               QApplication::translate("StandardDialog", "Fine") };
    const auto _index = QRandomGenerator::global()->bounded(0, terms.size());
    return terms[_index];
}
} // namespace


void StandardDialog::information(QWidget* _parent, const QString& _title, const QString& _text)
{
    auto dialog = new Dialog(_parent);
    dialog->setContentMaximumWidth(static_cast<int>(Ui::DesignSystem::dialog().infoMaximumWidth()));
    dialog->showDialog(_title, _text, { { 0, generateOkTerm(), Dialog::RejectButton } });
    QObject::connect(dialog, &Dialog::finished, dialog, &Dialog::hideDialog);
    QObject::connect(dialog, &Dialog::disappeared, dialog, &Dialog::deleteLater);
}
