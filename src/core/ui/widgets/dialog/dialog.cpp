#include "dialog.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/label/label.h>

#include <QHBoxLayout>


class Dialog::Implementation
{
public:
    Implementation(QWidget* _parent);

    Body1Label* supportingText;
    QHBoxLayout* buttonsLayout = nullptr;
    QVector<Button*> buttons;
};

Dialog::Implementation::Implementation(QWidget* _parent)
    : supportingText(new Body1Label(_parent)),
      buttonsLayout(new QHBoxLayout)
{
    buttonsLayout->setContentsMargins({});
    buttonsLayout->setSpacing(0);
    buttonsLayout->addStretch();
}


// ****

Dialog::Dialog(QWidget* _parent)
    : AbstractDialog(_parent),
      d(new Implementation(this))
{
    contentsLayout()->addWidget(d->supportingText, 0, 0);
    contentsLayout()->addLayout(d->buttonsLayout, 1, 0);
}

Dialog::~Dialog() = default;

void Dialog::showDialog(const QString& _title, const QString& _supportingText,
    const QVector<ButtonInfo>& _buttons)
{
    Q_ASSERT(!_buttons.isEmpty());

    setTitle(_title);
    d->supportingText->setText(_supportingText);
    for (auto buttonInfo : _buttons) {
        auto button = new Button(this);
        button->setText(buttonInfo.text);
        d->buttons.append(button);
        d->buttonsLayout->addWidget(button);
        connect(button, &Button::clicked,  this, [this, buttonInfo] { emit finished(buttonInfo); });
    }

    designSystemChangeEvent(nullptr);

    AbstractDialog::showDialog();
}

QWidget* Dialog::focusedWidgetAfterShow() const
{
    return d->supportingText;
}

void Dialog::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    AbstractDialog::designSystemChangeEvent(_event);

    d->supportingText->setContentsMargins(
                QMarginsF(Ui::DesignSystem::layout().px24(),
                          Ui::DesignSystem::layout().px12(),
                          Ui::DesignSystem::layout().px24(),
                          0)
                .toMargins());
    d->supportingText->setBackgroundColor(Ui::DesignSystem::color().background());
    d->supportingText->setTextColor(Ui::DesignSystem::color().onBackground());

    for (auto button : d->buttons) {
        button->setBackgroundColor(Ui::DesignSystem::color().secondary());
        button->setTextColor(Ui::DesignSystem::color().secondary());
    }

    d->buttonsLayout->setContentsMargins(
                QMarginsF(Ui::DesignSystem::layout().px12(),
                          Ui::DesignSystem::layout().px12(),
                          Ui::DesignSystem::layout().px16(),
                          Ui::DesignSystem::layout().px8())
                .toMargins());
}
