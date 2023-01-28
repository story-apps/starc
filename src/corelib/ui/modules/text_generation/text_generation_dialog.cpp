#include "text_generation_dialog.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/label/link_label.h>
#include <ui/widgets/text_field/text_field.h>
#include <utils/helpers/color_helper.h>

#include <QBoxLayout>
#include <QSettings>


namespace Ui {

namespace {
const QString kLastPromptKey = "widgets/text-generation-dialog/last-prompt";
}


class TextGenerationDialog::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    Body1Label* promptHintLabel = nullptr;
    TextField* promptText = nullptr;

    Body1Label* creditsInfoLabel = nullptr;
    Body1LinkLabel* buyCreditsLabel = nullptr;
    Button* cancelButton = nullptr;
    Button* generateButton = nullptr;
    QHBoxLayout* buttonsLayout = nullptr;

    int credits = 0;
};

TextGenerationDialog::Implementation::Implementation(QWidget* _parent)
    : promptHintLabel(new Body1Label(_parent))
    , promptText(new TextField(_parent))
    , creditsInfoLabel(new Body1Label(_parent))
    , buyCreditsLabel(new Body1LinkLabel(_parent))
    , cancelButton(new Button(_parent))
    , generateButton(new Button(_parent))
    , buttonsLayout(new QHBoxLayout)
{
    buttonsLayout->setContentsMargins({});
    buttonsLayout->setSpacing(0);
    buttonsLayout->addWidget(creditsInfoLabel);
    buttonsLayout->addWidget(buyCreditsLabel);
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(cancelButton);
    buttonsLayout->addWidget(generateButton);
}


// ****


TextGenerationDialog::TextGenerationDialog(QWidget* _parent)
    : AbstractDialog(_parent)
    , d(new Implementation(this))
{
    setAcceptButton(d->generateButton);
    setRejectButton(d->cancelButton);

    int row = 0;
    contentsLayout()->addWidget(d->promptHintLabel, row++, 0);
    contentsLayout()->addWidget(d->promptText, row++, 0);
    contentsLayout()->addLayout(d->buttonsLayout, row++, 0);

    connect(d->promptText, &TextField::textChanged, this, [this] {
        if (d->promptText->text().split(' ').size() > 50) {
            d->promptText->setError(tr("Prompt is too big, shorten it to 50 words"));
            return;
        }
        d->promptText->setError({});
    });
    connect(d->buyCreditsLabel, &Body1LinkLabel::clicked, this,
            &TextGenerationDialog::buyCreditsPressed);
    connect(d->cancelButton, &Button::clicked, this, &TextGenerationDialog::hideDialog);
    connect(d->generateButton, &Button::clicked, this, [this] {
        if (d->promptText->text().isEmpty()) {
            d->promptText->setError(tr("Please, enter prompt for text generation"));
            return;
        }

        if (!d->promptText->error().isEmpty()) {
            return;
        }

        emit generatePressed(d->promptText->text());
    });

    QSettings settings;
    d->promptText->setText(settings.value(kLastPromptKey).toString());
}

TextGenerationDialog::~TextGenerationDialog()
{
    QSettings settings;
    settings.setValue(kLastPromptKey, d->promptText->text());
}

void TextGenerationDialog::setOptions(const QString& _title, int _credits,
                                      const QString& _promptLabel, const QString& _prompt)
{
    d->credits = _credits;

    setTitle(_title + " [beta]");
    d->promptHintLabel->setText(_promptLabel);
    if (!_prompt.trimmed().isEmpty()) {
        d->promptText->setText(_prompt);
    }

    d->creditsInfoLabel->setText(d->credits > 0 ? tr("%n credit(s) available", 0, _credits)
                                                : tr("No credits available"));
    d->buyCreditsLabel->setVisible(d->credits == 0);
    d->generateButton->setEnabled(d->credits > 0);

    designSystemChangeEvent(nullptr);
}

QWidget* TextGenerationDialog::focusedWidgetAfterShow() const
{
    return d->promptText;
}

QWidget* TextGenerationDialog::lastFocusableWidget() const
{
    return d->generateButton;
}

void TextGenerationDialog::updateTranslations()
{
    d->promptText->setLabel(tr("Prompt"));
    d->buyCreditsLabel->setText(tr("purchase"));
    d->cancelButton->setText(tr("Cancel"));
    d->generateButton->setText(tr("Generate"));
}

void TextGenerationDialog::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    AbstractDialog::designSystemChangeEvent(_event);

    for (auto label : std::vector<Widget*>{
             d->promptHintLabel,
             d->creditsInfoLabel,
             d->buyCreditsLabel,
         }) {
        label->setBackgroundColor(Ui::DesignSystem::color().background());
        label->setTextColor(Ui::DesignSystem::color().onBackground());
    }
    d->promptHintLabel->setContentsMargins(Ui::DesignSystem::layout().px24(), 0,
                                           Ui::DesignSystem::layout().px16(),
                                           Ui::DesignSystem::layout().px24());
    d->creditsInfoLabel->setTextColor(ColorHelper::transparent(
        Ui::DesignSystem::color().onBackground(), Ui::DesignSystem::disabledTextOpacity()));
    d->creditsInfoLabel->setContentsMargins(
        Ui::DesignSystem::layout().px12(), Ui::DesignSystem::layout().px8(),
        Ui::DesignSystem::layout().px8(), Ui::DesignSystem::layout().px12());
    d->buyCreditsLabel->setTextColor(Ui::DesignSystem::color().accent());
    d->buyCreditsLabel->setContentsMargins(
        Ui::DesignSystem::layout().px4(), Ui::DesignSystem::layout().px8(),
        Ui::DesignSystem::layout().px4(), Ui::DesignSystem::layout().px12());

    for (auto textField : std::vector<TextField*>{
             d->promptText,
         }) {
        textField->setTextColor(Ui::DesignSystem::color().onBackground());
        textField->setBackgroundColor(Ui::DesignSystem::color().onBackground());
    }

    d->generateButton->setBackgroundColor(d->credits == 0 ? Ui::DesignSystem::color().onBackground()
                                                          : Ui::DesignSystem::color().accent());
    d->generateButton->setTextColor(d->credits == 0 ? Ui::DesignSystem::color().onBackground()
                                                    : Ui::DesignSystem::color().accent());
    d->cancelButton->setBackgroundColor(Ui::DesignSystem::color().onBackground());
    d->cancelButton->setTextColor(Ui::DesignSystem::color().onBackground());

    d->buttonsLayout->setContentsMargins(
        QMarginsF(Ui::DesignSystem::layout().px12(), Ui::DesignSystem::layout().px12(),
                  Ui::DesignSystem::layout().px12(), Ui::DesignSystem::layout().px12())
            .toMargins());
}

} // namespace Ui
