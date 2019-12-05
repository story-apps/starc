#include "settings_view.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/card/card.h>
#include <ui/widgets/check_box/check_box.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/scroll_bar/scroll_bar.h>
#include <ui/widgets/slider/slider.h>
#include <ui/widgets/text_field/text_field.h>

#include <QGridLayout>
#include <QLocale>
#include <QScrollArea>


namespace Ui
{

namespace {

/**
 * @brief Получить название языка установленной локали
 */
QString currentLanguage() {
    switch (QLocale().language()) {
        case QLocale::English: {
            return "English";
        }
        case QLocale::Russian: {
            return "Русский";
        }
        default: {
            return QLocale::languageToString(QLocale().language());
        }
    }
}

/**
 * @brief Получить название текущей темы
 */
QString currentTheme() {
    switch (Ui::DesignSystem::theme()) {
        case Ui::ApplicationTheme::Dark: {
            return SettingsView::tr("dark");
        }
        case Ui::ApplicationTheme::Light: {
            return SettingsView::tr("light");
        }
        case Ui::ApplicationTheme::DarkAndLight: {
            return SettingsView::tr("dark and light");
        }
        default: {
            return SettingsView::tr("custom");
        }
    }
}
}

class SettingsView::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    QScrollArea* content = nullptr;

    Card* applicationCard = nullptr;
    QGridLayout* applicationCardLayout = nullptr;
    //
    H5Label* applicationTitle = nullptr;
    Body1Label* language = nullptr;
    Button* changeLanuage = nullptr;
    CheckBox* useSpellChecker = nullptr;
    //
    H6Label* applicationUserInterfaceTitle = nullptr;
    Body1Label* theme = nullptr;
    Button* changeTheme = nullptr;
    Body1Label* scaleFactorTitleLabel = nullptr;
    Slider* scaleFactorSlider = nullptr;
    Body2Label* scaleFactorSmallInfoLabel = nullptr;
    Body2Label* scaleFactorBigInfoLabel = nullptr;
    //
    H6Label* applicationSaveAndBackupTitle = nullptr;
    CheckBox* saveBackups = nullptr;
    TextField* backupsFolderPath = nullptr;
    //
    int applicationCardBottomSpacerIndex = 0;
};

SettingsView::Implementation::Implementation(QWidget* _parent)
    : content(new QScrollArea(_parent)),
      applicationCard(new Card(content)),
      applicationCardLayout(new QGridLayout),
      applicationTitle(new H5Label(applicationCard)),
      language(new Body1Label(applicationCard)),
      changeLanuage(new Button(applicationCard)),
      useSpellChecker(new CheckBox(applicationCard)),
      applicationUserInterfaceTitle(new H6Label(applicationCard)),
      theme(new Body1Label(applicationCard)),
      changeTheme(new Button(applicationCard)),
      applicationSaveAndBackupTitle(new H6Label(applicationCard)),
      saveBackups(new CheckBox(applicationCard)),
      backupsFolderPath(new TextField(applicationCard))
{
    QPalette palette;
    palette.setColor(QPalette::Base, Qt::transparent);
    palette.setColor(QPalette::Window, Qt::transparent);
    content->setPalette(palette);
    content->setVerticalScrollBar(new ScrollBar);
    content->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto makeLayout = [] {
        auto layout = new QHBoxLayout;
        layout->setContentsMargins({});
        layout->setSpacing(0);
        return layout;
    };

    applicationCardLayout->setContentsMargins({});
    applicationCardLayout->setSpacing(0);
    int itemIndex = 0;
    //
    // Приложение
    //
    applicationCardLayout->addWidget(applicationTitle, itemIndex++, 0);
    {
        auto layout = makeLayout();
        layout->addWidget(language, 0, Qt::AlignCenter);
        layout->addWidget(changeLanuage);
        layout->addStretch();
        applicationCardLayout->addLayout(layout, itemIndex++, 0);
    }
    {
        auto layout = makeLayout();
        layout->addWidget(useSpellChecker);
        layout->addStretch();
        applicationCardLayout->addLayout(layout, itemIndex++, 0);
    }
    //
    // ... интерфейс
    //
    applicationCardLayout->addWidget(applicationUserInterfaceTitle, itemIndex++, 0);
    {
        auto layout = makeLayout();
        layout->addWidget(theme, 0, Qt::AlignCenter);
        layout->addWidget(changeTheme);
        layout->addStretch();
        applicationCardLayout->addLayout(layout, itemIndex++, 0);
    }
    //
    // ... сохранение и бэкапы
    //
    applicationCardLayout->addWidget(applicationSaveAndBackupTitle, itemIndex++, 0);
    {
        backupsFolderPath->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

        auto layout = makeLayout();
        layout->addWidget(saveBackups, 0, Qt::AlignCenter);
        layout->addWidget(backupsFolderPath);
        applicationCardLayout->addLayout(layout, itemIndex++, 0);
    }
    applicationCardBottomSpacerIndex = itemIndex;
    applicationCard->setLayoutReimpl(applicationCardLayout);

    QWidget* contentWidget = new QWidget;
    content->setWidget(contentWidget);
    content->setWidgetResizable(true);
    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(applicationCard);
    layout->addStretch();
    contentWidget->setLayout(layout);
}


// ****


SettingsView::SettingsView(QWidget* _parent)
    : Widget(_parent),
      d(new Implementation(this))
{
    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(d->content);
    setLayout(layout);

    designSystemChangeEvent(nullptr);
}

void SettingsView::updateTranslations()
{
    d->applicationTitle->setText(tr("Application"));
    d->language->setText(QString("%1: %2").arg(tr("Language")).arg(currentLanguage()));
    d->changeLanuage->setText(tr("Select language"));
    d->useSpellChecker->setText(tr("Spell check"));
    d->applicationUserInterfaceTitle->setText(tr("User interface"));
    d->theme->setText(QString("%1: %2").arg(tr("Theme")).arg(currentTheme()));
    d->changeTheme->setText(tr("Change theme"));
    d->applicationSaveAndBackupTitle->setText(tr("Save changes/backups"));
    d->saveBackups->setText(tr("Save backups"));
    d->backupsFolderPath->setLabel(tr("Backups folder path"));
}

SettingsView::~SettingsView() = default;

void SettingsView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    setBackgroundColor(DesignSystem::color().surface());
    d->content->widget()->layout()->setContentsMargins(QMarginsF(Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().topContentMargin(),
                                                                 Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().px24()).toMargins());

    for (auto card : { d->applicationCard }) {
        card->setBackgroundColor(DesignSystem::color().background());
    }

    for (auto title : QVector<Widget*>{ d->applicationTitle,
                                        d->applicationUserInterfaceTitle,
                                        d->applicationSaveAndBackupTitle }) {
        title->setBackgroundColor(DesignSystem::color().background());
        title->setTextColor(DesignSystem::color().onBackground());
        title->setContentsMargins(Ui::DesignSystem::label().margins().toMargins());
    }

    auto labelMargins = Ui::DesignSystem::label().margins().toMargins();
    labelMargins.setTop(0);
    labelMargins.setBottom(0);
    for (auto label : { d->language,
                        d->theme }) {
        label->setBackgroundColor(DesignSystem::color().background());
        label->setTextColor(DesignSystem::color().onBackground());
        label->setContentsMargins(labelMargins);
    }

    for (auto checkBox : { d->useSpellChecker,
                           d->saveBackups }) {
        checkBox->setBackgroundColor(DesignSystem::color().background());
        checkBox->setTextColor(DesignSystem::color().onBackground());
    }

    for (auto button : { d->changeLanuage,
                         d->changeTheme }) {
        button->setBackgroundColor(DesignSystem::color().secondary());
        button->setTextColor(DesignSystem::color().secondary());
    }

    d->applicationCardLayout->setRowMinimumHeight(d->applicationCardBottomSpacerIndex,
                                                  static_cast<int>(Ui::DesignSystem::layout().px24()));
}

} // namespace Ui
