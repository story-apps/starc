#include "beat_name_widget.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/icon_button/icon_button.h>
#include <ui/widgets/label/label.h>
#include <utils/helpers/color_helper.h>

#include <QBoxLayout>


namespace Ui {

class BeatNameWidget::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    void updateBeatNameTextColor();


    AbstractLabel* titleLabel = nullptr;
    IconButton* copyNameButton = nullptr;
    AbstractLabel* beatNameLabel = nullptr;
    bool hasBeatName = false;
};

BeatNameWidget::Implementation::Implementation(QWidget* _parent)
    : titleLabel(new Subtitle2Label(_parent))
    , copyNameButton(new IconButton(_parent))
    , beatNameLabel(new Subtitle2Label(_parent))
{
    copyNameButton->setIcon(u8"\U000F0CF9");
    copyNameButton->setFocusPolicy(Qt::NoFocus);
}

void BeatNameWidget::Implementation::updateBeatNameTextColor()
{
    beatNameLabel->setTextColor(
        ColorHelper::transparent(Ui::DesignSystem::color().onPrimary(),
                                 hasBeatName ? 1.0 : Ui::DesignSystem::disabledTextOpacity()));
}


// ****


BeatNameWidget::BeatNameWidget(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation(this))
{
    auto titleLayout = new QHBoxLayout;
    titleLayout->setContentsMargins({});
    titleLayout->setSpacing(0);
    titleLayout->addWidget(d->titleLabel, 1, Qt::AlignVCenter);
    titleLayout->addWidget(d->copyNameButton);

    auto layout = new QVBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addLayout(titleLayout);
    layout->addWidget(d->beatNameLabel);
    setLayout(layout);


    connect(d->copyNameButton, &IconButton::clicked, this, [this] {
        if (d->hasBeatName) {
            emit pasteBeatNamePressed(d->beatNameLabel->text());
        }
    });
}

BeatNameWidget::~BeatNameWidget() = default;

void BeatNameWidget::setBeatName(const QString& _name)
{
    if (_name.isEmpty()) {
        clearBeatName();
        return;
    }

    if (_name == d->beatNameLabel->text()) {
        return;
    }

    d->hasBeatName = true;
    d->beatNameLabel->setText(_name);
    d->updateBeatNameTextColor();
}

void BeatNameWidget::clearBeatName()
{
    d->hasBeatName = false;
    d->beatNameLabel->setText(tr("No one beat selected"));
    d->updateBeatNameTextColor();
}

void BeatNameWidget::updateTranslations()
{
    d->titleLabel->setText(tr("Current beat:"));
    d->copyNameButton->setToolTip(tr("Copy current beat text and paste it to the document"));
}

void BeatNameWidget::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    setBackgroundColor(Ui::DesignSystem::color().primary());

    for (auto widget : std::vector<Widget*>{
             d->titleLabel,
             d->copyNameButton,
         }) {
        widget->setBackgroundColor(Ui::DesignSystem::color().primary());
        widget->setTextColor(ColorHelper::transparent(Ui::DesignSystem::color().onPrimary(),
                                                      Ui::DesignSystem::inactiveTextOpacity()));
    }
    d->beatNameLabel->setBackgroundColor(Ui::DesignSystem::color().primary());
    d->updateBeatNameTextColor();
    d->beatNameLabel->setContentsMargins(0, 0, Ui::DesignSystem::layout().px12(), 0);

    layout()->setContentsMargins(Ui::DesignSystem::layout().px24(), 0, 0,
                                 Ui::DesignSystem::layout().px24());
    layout()->setSpacing(Ui::DesignSystem::layout().px4());
}

} // namespace Ui
