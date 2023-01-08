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
    QHBoxLayout* buttonsSideBySideLayout = nullptr;
    QVBoxLayout* buttonsStackedLayout = nullptr;
    QVector<Button*> buttons;
};

Dialog::Implementation::Implementation(QWidget* _parent)
    : supportingText(new Body1Label(_parent))
    , buttonsSideBySideLayout(new QHBoxLayout)
    , buttonsStackedLayout(new QVBoxLayout)
{
    buttonsSideBySideLayout->setContentsMargins({});
    buttonsSideBySideLayout->setSpacing(0);
    buttonsSideBySideLayout->addStretch();

    buttonsStackedLayout->setContentsMargins({});
    buttonsStackedLayout->setSpacing(0);
}

// ****

Dialog::Dialog(QWidget* _parent)
    : AbstractDialog(_parent)
    , d(new Implementation(this))
{
    contentsLayout()->addWidget(d->supportingText, 0, 0);
    contentsLayout()->addLayout(d->buttonsSideBySideLayout, 1, 0);
    contentsLayout()->addLayout(d->buttonsStackedLayout, 2, 0, Qt::AlignRight);
}

Dialog::~Dialog() = default;

void Dialog::showDialog(const QString& _title, const QString& _supportingText,
                        const QVector<ButtonInfo>& _buttons, bool _placeButtonsSideBySide)
{
    Q_ASSERT(!_buttons.isEmpty());

    setTitle(_title);
    d->supportingText->setText(_supportingText);
    if (_placeButtonsSideBySide) {
        contentsLayout()->removeItem(d->buttonsStackedLayout);
        d->buttonsStackedLayout->deleteLater();
    } else {
        contentsLayout()->removeItem(d->buttonsSideBySideLayout);
        d->buttonsSideBySideLayout->deleteLater();
    }
    for (const auto& buttonInfo : _buttons) {
        auto button = new Button(this);
        button->setText(buttonInfo.text);
        d->buttons.append(button);
        if (_placeButtonsSideBySide) {
            d->buttonsSideBySideLayout->addWidget(button);
        } else {
            d->buttonsStackedLayout->addWidget(button, 0, Qt::AlignRight);
        }
        connect(button, &Button::clicked, this, [this, buttonInfo] { emit finished(buttonInfo); });
        if (buttonInfo.type == Dialog::AcceptButton) {
            setAcceptButton(button);
        } else if (buttonInfo.type == Dialog::RejectButton) {
            setRejectButton(button);
        }
    }

    //
    // Настраиваем стили добавленных кнопок
    //
    designSystemChangeEvent(nullptr);

    AbstractDialog::showDialog();
}

QWidget* Dialog::focusedWidgetAfterShow() const
{
    return d->supportingText;
}

QWidget* Dialog::lastFocusableWidget() const
{
    return d->buttons.last();
}

void Dialog::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    AbstractDialog::designSystemChangeEvent(_event);

    d->supportingText->setContentsMargins(
        QMarginsF(Ui::DesignSystem::layout().px24(),
                  title().isEmpty() ? Ui::DesignSystem::layout().px24() : 0,
                  Ui::DesignSystem::layout().px24(), 0)
            .toMargins());
    d->supportingText->setBackgroundColor(Qt::transparent);
    d->supportingText->setTextColor(Ui::DesignSystem::color().onBackground());

    for (auto button : std::as_const(d->buttons)) {
        button->setBackgroundColor(Ui::DesignSystem::color().accent());
        button->setTextColor(Ui::DesignSystem::color().accent());
    }

    d->buttonsSideBySideLayout->setContentsMargins(
        QMarginsF(Ui::DesignSystem::layout().px12(), Ui::DesignSystem::layout().px12(),
                  Ui::DesignSystem::layout().px16(), Ui::DesignSystem::layout().px16())
            .toMargins());

    d->buttonsStackedLayout->setContentsMargins(
        QMarginsF(Ui::DesignSystem::layout().px12(), Ui::DesignSystem::layout().px12(),
                  Ui::DesignSystem::layout().px16(), Ui::DesignSystem::layout().px16())
            .toMargins());
}
