#include "text_generation_dialog.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/text_field/text_field.h>
#include <utils/helpers/color_helper.h>

#include <QBoxLayout>


namespace Ui {

class TextGenerationDialog::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    Body1Label* promptHintLabel = nullptr;
    TextField* promptText = nullptr;

    Body1Label* generationsInfoLabel = nullptr;
    Button* cancelButton = nullptr;
    Button* generateButton = nullptr;
    QHBoxLayout* buttonsLayout = nullptr;
};

TextGenerationDialog::Implementation::Implementation(QWidget* _parent)
    : promptHintLabel(new Body1Label(_parent))
    , promptText(new TextField(_parent))
    , generationsInfoLabel(new Body1Label(_parent))
    , cancelButton(new Button(_parent))
    , generateButton(new Button(_parent))
    , buttonsLayout(new QHBoxLayout)
{
    buttonsLayout->setContentsMargins({});
    buttonsLayout->setSpacing(0);
    buttonsLayout->addWidget(generationsInfoLabel);
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
}

TextGenerationDialog::~TextGenerationDialog() = default;

void TextGenerationDialog::setOptions(const QString& _title, int _availableGenerations,
                                      int _totalGenerations, const QString& _promptLabel,
                                      const QString& _prompt)
{
    Q_UNUSED(_totalGenerations)

    setTitle(_title);
    d->generationsInfoLabel->setText(
        tr("%n generation(s) available", 0, _availableGenerations).arg(_availableGenerations));
    d->promptHintLabel->setText(_promptLabel);
    d->promptText->setText(_prompt);
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
    d->cancelButton->setText(tr("Cancel"));
    d->generateButton->setText(tr("Generate"));
}

void TextGenerationDialog::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    AbstractDialog::designSystemChangeEvent(_event);

    for (auto label : {
             d->promptHintLabel,
             d->generationsInfoLabel,
         }) {
        label->setBackgroundColor(Ui::DesignSystem::color().background());
        label->setTextColor(Ui::DesignSystem::color().onBackground());
    }
    d->promptHintLabel->setContentsMargins(Ui::DesignSystem::layout().px24(), 0,
                                           Ui::DesignSystem::layout().px16(),
                                           Ui::DesignSystem::layout().px24());
    d->generationsInfoLabel->setTextColor(ColorHelper::transparent(
        Ui::DesignSystem::color().onBackground(), Ui::DesignSystem::disabledTextOpacity()));
    d->generationsInfoLabel->setContentsMargins(
        Ui::DesignSystem::layout().px12(), Ui::DesignSystem::layout().px8(),
        Ui::DesignSystem::layout().px8(), Ui::DesignSystem::layout().px12());

    for (auto textField : std::vector<TextField*>{
             d->promptText,
         }) {
        textField->setTextColor(Ui::DesignSystem::color().onBackground());
        textField->setBackgroundColor(Ui::DesignSystem::color().onBackground());
    }

    d->generateButton->setBackgroundColor(Ui::DesignSystem::color().accent());
    d->generateButton->setTextColor(Ui::DesignSystem::color().accent());
    d->cancelButton->setBackgroundColor(Ui::DesignSystem::color().onBackground());
    d->cancelButton->setTextColor(Ui::DesignSystem::color().onBackground());

    d->buttonsLayout->setContentsMargins(
        QMarginsF(Ui::DesignSystem::layout().px12(), Ui::DesignSystem::layout().px12(),
                  Ui::DesignSystem::layout().px12(), Ui::DesignSystem::layout().px12())
            .toMargins());
}

} // namespace Ui
