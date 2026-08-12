#include "dialog.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/label/label.h>
#include <utils/helpers/ui_helper.h>

#include <QHBoxLayout>
#include <QScrollArea>
#include <QVBoxLayout>

namespace {
const char* kButtonRole = "button-role";
}


class Dialog::Implementation
{
public:
    Implementation(QWidget* _parent);

    Body1Label* supportingText;
    QScrollArea* supportingTextScrollArea = nullptr;
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

void Dialog::enableSupportingTextScrolling()
{
    if (d->supportingTextScrollArea != nullptr) {
        return;
    }

    contentsLayout()->removeWidget(d->supportingText);
    d->supportingTextScrollArea = UiHelper::createScrollArea(this);
    d->supportingTextScrollArea->setFocusPolicy(Qt::StrongFocus);
    d->supportingTextScrollArea->setSizeAdjustPolicy(QAbstractScrollArea::AdjustIgnored);
    d->supportingTextScrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    d->supportingText->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    auto scrollContentLayout
        = qobject_cast<QVBoxLayout*>(d->supportingTextScrollArea->widget()->layout());
    Q_ASSERT(scrollContentLayout);
    scrollContentLayout->addWidget(d->supportingText);
    scrollContentLayout->addStretch();
    contentsLayout()->addWidget(d->supportingTextScrollArea, 0, 0);
}

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
        button->setProperty(kButtonRole, buttonInfo.type);
        d->buttons.append(button);
        if (_placeButtonsSideBySide) {
            d->buttonsSideBySideLayout->addWidget(button);
        } else {
            d->buttonsStackedLayout->addWidget(button, 0, Qt::AlignRight);
        }
        connect(button, &Button::clicked, this, [this, buttonInfo] { emit finished(buttonInfo); });
        if (buttonInfo.type == Dialog::AcceptButton
            || buttonInfo.type == Dialog::AcceptCriticalButton) {
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
    return d->supportingTextScrollArea != nullptr
        ? static_cast<QWidget*>(d->supportingTextScrollArea)
        : static_cast<QWidget*>(d->supportingText);
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
        UiHelper::ButtonRole buttonRole = UiHelper::DialogDefault;
        switch (button->property(kButtonRole).toInt()) {
        case AcceptButton: {
            buttonRole = UiHelper::DialogAccept;
            break;
        }
        case AcceptCriticalButton: {
            buttonRole = UiHelper::DialogCritical;
            break;
        }
        default: {
            break;
        }
        }
        UiHelper::initColorsFor(button, buttonRole);
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
