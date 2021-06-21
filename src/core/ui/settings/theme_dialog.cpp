#include "theme_dialog.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/label/link_label.h>
#include <ui/widgets/radio_button/radio_button.h>
#include <ui/widgets/radio_button/radio_button_group.h>
#include <ui/widgets/text_field/text_field.h>
#include <utils/helpers/color_helper.h>

#include <QColorDialog>
#include <QGridLayout>
#include <QTimer>
#include <QUrl>
#include <QVariant>


namespace Ui {

namespace {
const char* kThemeKey = "theme";
}

class ThemeDialog::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    /**
     * @brief Разрешить/запретить редактирование палитры
     */
    void setPaletteReadOnly(bool _readOnly);

    /**
     * @brief Получить список всех кнопок тем
     */
    QVector<RadioButton*> themes() const;

    /**
     * @brief Получить список всех кнопок цветов кастомной темы
     */
    QVector<Subtitle2Label*> colors() const;


    RadioButton* light = nullptr;
    RadioButton* darkAndLight = nullptr;
    RadioButton* dark = nullptr;
    RadioButton* custom = nullptr;
    Widget* customPalette = nullptr;
    QGridLayout* customPaletteLayout = nullptr;
    Subtitle2Label* primary = nullptr;
    Subtitle2Label* onPrimary = nullptr;
    Subtitle2Label* secondary = nullptr;
    Subtitle2Label* onSecondary = nullptr;
    Subtitle2Label* background = nullptr;
    Subtitle2Label* onBackground = nullptr;
    Subtitle2Label* surface = nullptr;
    Subtitle2Label* onSurface = nullptr;
    Subtitle2Label* error = nullptr;
    Subtitle2Label* onError = nullptr;
    Subtitle2Label* shadow = nullptr;
    Subtitle2Label* onShadow = nullptr;
    TextField* customThemeHash = nullptr;

    QHBoxLayout* buttonsLayout = nullptr;
    Button* okButton = nullptr;
};

ThemeDialog::Implementation::Implementation(QWidget* _parent)
    : light(new RadioButton(_parent))
    , darkAndLight(new RadioButton(_parent))
    , dark(new RadioButton(_parent))
    , custom(new RadioButton(_parent))
    , customPalette(new Widget(_parent))
    , primary(new Subtitle2Label(_parent))
    , onPrimary(new Subtitle2Label(_parent))
    , secondary(new Subtitle2Label(_parent))
    , onSecondary(new Subtitle2Label(_parent))
    , background(new Subtitle2Label(_parent))
    , onBackground(new Subtitle2Label(_parent))
    , surface(new Subtitle2Label(_parent))
    , onSurface(new Subtitle2Label(_parent))
    , error(new Subtitle2Label(_parent))
    , onError(new Subtitle2Label(_parent))
    , shadow(new Subtitle2Label(_parent))
    , onShadow(new Subtitle2Label(_parent))
    , customThemeHash(new TextField(_parent))
    , okButton(new Button(_parent))
{
    light->setChecked(true);
    light->setProperty(kThemeKey, static_cast<int>(ApplicationTheme::Light));
    darkAndLight->setProperty(kThemeKey, static_cast<int>(ApplicationTheme::DarkAndLight));
    dark->setProperty(kThemeKey, static_cast<int>(ApplicationTheme::Dark));
    custom->setProperty(kThemeKey, static_cast<int>(ApplicationTheme::Custom));

    customPaletteLayout = new QGridLayout;
    customPaletteLayout->setContentsMargins({});
    customPaletteLayout->setSpacing(0);
    customPaletteLayout->addWidget(primary, 0, 0);
    customPaletteLayout->addWidget(onPrimary, 0, 1);
    customPaletteLayout->addWidget(secondary, 0, 2);
    customPaletteLayout->addWidget(onSecondary, 0, 3);
    customPaletteLayout->addWidget(background, 1, 0);
    customPaletteLayout->addWidget(onBackground, 1, 1);
    customPaletteLayout->addWidget(surface, 1, 2);
    customPaletteLayout->addWidget(onSurface, 1, 3);
    customPaletteLayout->addWidget(error, 2, 0);
    customPaletteLayout->addWidget(onError, 2, 1);
    customPaletteLayout->addWidget(shadow, 2, 2);
    customPaletteLayout->addWidget(onShadow, 2, 3);
    customPaletteLayout->setRowMinimumHeight(3, 1); // добавляем отступ между цветами и полем хэша
    customPaletteLayout->addWidget(customThemeHash, 4, 0, 1, 4);
    customPalette->setLayout(customPaletteLayout);
    setPaletteReadOnly(true);

    for (auto color : colors()) {
        color->setAlignment(Qt::AlignCenter);
    }

    customThemeHash->setSpellCheckPolicy(SpellCheckPolicy::Manual);

    buttonsLayout = new QHBoxLayout;
    buttonsLayout->setContentsMargins({});
    buttonsLayout->setSpacing(0);
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(okButton);

    RadioButtonGroup* projectLocationGroup = new RadioButtonGroup(_parent);
    projectLocationGroup->add(light);
    projectLocationGroup->add(darkAndLight);
    projectLocationGroup->add(dark);
    projectLocationGroup->add(custom);
}

void ThemeDialog::Implementation::setPaletteReadOnly(bool _readOnly)
{
    for (auto color : colors()) {
        color->setEnabled(!_readOnly);
    }
    customThemeHash->setReadOnly(_readOnly);
}

QVector<RadioButton*> ThemeDialog::Implementation::themes() const
{
    return { darkAndLight, dark, light, custom };
}

QVector<Subtitle2Label*> ThemeDialog::Implementation::colors() const
{
    return { primary, onPrimary, secondary, onSecondary, background, onBackground,
             surface, onSurface, error,     onError,     shadow,     onShadow };
}


// ****


ThemeDialog::ThemeDialog(QWidget* _parent)
    : AbstractDialog(_parent)
    , d(new Implementation(this))
{
    setRejectButton(d->okButton);

    contentsLayout()->addWidget(d->light, 0, 0);
    contentsLayout()->addWidget(d->darkAndLight, 0, 1);
    contentsLayout()->addWidget(d->dark, 0, 2);
    contentsLayout()->setColumnStretch(3, 1);
    contentsLayout()->addWidget(d->custom, 1, 0, 1, 3);
    contentsLayout()->addWidget(d->customPalette, 2, 0, 1, 4);
    contentsLayout()->setRowStretch(3, 1);
    contentsLayout()->addLayout(d->buttonsLayout, 3, 0, 1, 4);

    for (auto radioButton : d->themes()) {
        connect(radioButton, &RadioButton::checkedChanged, this,
                [this, radioButton](bool _checked) {
                    if (!_checked) {
                        return;
                    }

                    const auto theme = radioButton->property(kThemeKey).toInt();
                    emit themeChanged(static_cast<ApplicationTheme>(theme));
                });
    }
    connect(d->custom, &RadioButton::checkedChanged, this,
            [this](bool _checked) { d->setPaletteReadOnly(!_checked); });
    auto updateColorLabel = [this] {
        auto colorLabel = qobject_cast<Widget*>(sender());
        if (colorLabel == nullptr) {
            return;
        }

        const auto newColor = QColorDialog::getColor(colorLabel->backgroundColor(), colorLabel);
        if (!newColor.isValid()) {
            return;
        }

        colorLabel->setBackgroundColor(newColor);

        auto color(Ui::DesignSystem::color());
        color.setPrimary(d->primary->backgroundColor());
        color.setOnPrimary(d->onPrimary->backgroundColor());
        color.setSecondary(d->secondary->backgroundColor());
        color.setOnSecondary(d->onSecondary->backgroundColor());
        color.setBackground(d->background->backgroundColor());
        color.setOnBackground(d->onBackground->backgroundColor());
        color.setSurface(d->surface->backgroundColor());
        color.setOnSurface(d->onSurface->backgroundColor());
        color.setError(d->error->backgroundColor());
        color.setOnError(d->onError->backgroundColor());
        color.setShadow(d->shadow->backgroundColor());
        color.setOnShadow(d->onShadow->backgroundColor());
        emit customThemeColorsChanged(color);
    };
    for (auto color : d->colors()) {
        connect(color, &Subtitle2Label::clicked, this, updateColorLabel);
    }
    connect(d->customThemeHash, &TextField::textChanged, this, [this] {
        const auto hash = d->customThemeHash->text().trimmed();
        if (hash.length() != 72) {
            d->customThemeHash->setError(tr("Entered HASH has incorrect length"));
            return;
        }

        QVector<QColor> colors;
        int startIndex = 0;
        for (int colorIndex = 0; colorIndex < 12; ++colorIndex) {
            const int length = 6;
            const QColor color = "#" + hash.mid(startIndex, length);
            if (!color.isValid()) {
                d->customThemeHash->setError(tr("Entered HASH has invalid colors"));
                return;
            }

            startIndex += length;
            colors.append(color);
        }
        for (auto color : colors) {
            if (colors.count(color) > 6) {
                d->customThemeHash->setError(tr("Entered HASH has too much equal colors"));
                return;
            }
        }

        d->customThemeHash->setError({});
        emit customThemeColorsChanged(Ui::DesignSystem::Color(hash));
    });
    connect(d->okButton, &Button::clicked, this, &ThemeDialog::hideDialog);

    updateTranslations();
    designSystemChangeEvent(nullptr);
}

ThemeDialog::~ThemeDialog() = default;

void ThemeDialog::setCurrentTheme(ApplicationTheme _theme)
{
    for (auto radioButton : d->themes()) {
        if (static_cast<ApplicationTheme>(radioButton->property(kThemeKey).toInt()) == _theme) {
            radioButton->setChecked(true);
            break;
        }
    }
}

QWidget* ThemeDialog::focusedWidgetAfterShow() const
{
    return d->light;
}

QWidget* ThemeDialog::lastFocusableWidget() const
{
    return d->okButton;
}

void ThemeDialog::updateTranslations()
{
    setTitle(tr("Change application theme"));

    d->darkAndLight->setText(tr("Dark and light"));
    d->dark->setText(tr("Dark"));
    d->light->setText(tr("Light"));
    d->custom->setText(tr("Create your own color theme of the application"));
    d->primary->setText(tr("primary"));
    d->onPrimary->setText(tr("text on primary"));
    d->secondary->setText(tr("accent"));
    d->onSecondary->setText(tr("text on accent"));
    d->background->setText(tr("background"));
    d->onBackground->setText(tr("text on background"));
    d->surface->setText(tr("surface"));
    d->onSurface->setText(tr("text on surface"));
    d->error->setText(tr("error"));
    d->onError->setText(tr("text on error"));
    d->shadow->setText(tr("shadow"));
    d->onShadow->setText(tr("text on shadow"));

    d->customThemeHash->setLabel(tr("Theme HASH"));
    d->customThemeHash->setHelper(tr(
        "Copy theme HASH to share your custom theme with others, or paste HASH here to apply it"));

    d->okButton->setText(tr("Close"));
}

void ThemeDialog::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    AbstractDialog::designSystemChangeEvent(_event);

    for (auto radioButton : d->themes()) {
        radioButton->setBackgroundColor(Ui::DesignSystem::color().background());
        radioButton->setTextColor(Ui::DesignSystem::color().onBackground());
    }

    d->customPalette->setBackgroundColor(Ui::DesignSystem::color().background());

    auto initColorLabel = [](const QColor& _color, Widget* _label) {
        _label->setBackgroundColor(_color);
        _label->setTextColor(ColorHelper::contrasted(_color));
    };
    initColorLabel(Ui::DesignSystem::color().primary(), d->primary);
    initColorLabel(Ui::DesignSystem::color().onPrimary(), d->onPrimary);
    initColorLabel(Ui::DesignSystem::color().secondary(), d->secondary);
    initColorLabel(Ui::DesignSystem::color().onSecondary(), d->onSecondary);
    initColorLabel(Ui::DesignSystem::color().background(), d->background);
    initColorLabel(Ui::DesignSystem::color().onBackground(), d->onBackground);
    initColorLabel(Ui::DesignSystem::color().surface(), d->surface);
    initColorLabel(Ui::DesignSystem::color().onSurface(), d->onSurface);
    initColorLabel(Ui::DesignSystem::color().error(), d->error);
    initColorLabel(Ui::DesignSystem::color().onError(), d->onError);
    initColorLabel(Ui::DesignSystem::color().shadow(), d->shadow);
    initColorLabel(Ui::DesignSystem::color().onShadow(), d->onShadow);

    const auto minimumSize = static_cast<int>(Ui::DesignSystem::layout().px24() * 4);
    for (auto color : d->colors()) {
        color->setMinimumSize(minimumSize * 2, minimumSize);
    }
    d->customPaletteLayout->setRowMinimumHeight(
        3, static_cast<int>(Ui::DesignSystem::layout().px24()));
    d->customThemeHash->setBackgroundColor(Ui::DesignSystem::color().background());
    d->customThemeHash->setTextColor(Ui::DesignSystem::color().onBackground());
    {
        QSignalBlocker blocker(d->customThemeHash);
        d->customThemeHash->setText(Ui::DesignSystem::color().toString());
    }

    d->okButton->setBackgroundColor(Ui::DesignSystem::color().secondary());
    d->okButton->setTextColor(Ui::DesignSystem::color().secondary());

    contentsLayout()->setSpacing(static_cast<int>(Ui::DesignSystem::layout().px8()));
    d->buttonsLayout->setContentsMargins(
        QMarginsF(Ui::DesignSystem::layout().px12(), Ui::DesignSystem::layout().px12(),
                  Ui::DesignSystem::layout().px16(), Ui::DesignSystem::layout().px8())
            .toMargins());
}

} // namespace Ui
