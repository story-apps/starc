#include "radio_button_group.h"

#include "radio_button.h"


class RadioButtonGroup::Implementation
{
public:
    void updateRadioButtonsState(RadioButton* _checked);

    QVector<RadioButton*> radioButtons;
};

void RadioButtonGroup::Implementation::updateRadioButtonsState(RadioButton* _checked)
{
    for (RadioButton* radioButton : radioButtons) {
        if (radioButton == _checked) {
            continue;
        }

        radioButton->setChecked(false);
    }
}


// ****


RadioButtonGroup::RadioButtonGroup(QObject* _parent)
    : QObject(_parent),
      d(new Implementation)
{
}

void RadioButtonGroup::add(RadioButton* _radioButton)
{
    if (d->radioButtons.contains(_radioButton)) {
        return;
    }

    d->radioButtons.append(_radioButton);
    connect(_radioButton, &RadioButton::checkedChanged, this, [this] (bool _checked) {
        if (!_checked) {
            return;
        }

        RadioButton* checkedRadioButton = qobject_cast<RadioButton*>(sender());
        if (checkedRadioButton == nullptr) {
            return;
        }

        d->updateRadioButtonsState(checkedRadioButton);
    });

    if (_radioButton->isChecked()) {
        d->updateRadioButtonsState(_radioButton);
    }
}

RadioButtonGroup::~RadioButtonGroup() = default;
