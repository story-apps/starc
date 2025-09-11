#include "module_promo_widget.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/icon_button/icon_button.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/stack_widget/stack_widget.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/ui_helper.h>

#include <QBoxLayout>


namespace Ui {

namespace {
const char* kPromoTypeIdKey = "promo-type-id";
}

class ModulePromoWidget::Implementation
{
public:
    Implementation(ModulePromoWidget* _q);

    /**
     * @brief Удалить виджеты превьюшек модулей и опций
     */
    void clearPreviews();
    void clearOptions();

    /**
     * @brief Добавить превью для заданного типа
     */
    void addPreview(ModulePromoType _forType, ModulePromoType _currentType);

    /**
     * @brief Добавить опцию с заданным текстом
     */
    void addOption(const QString& _text);

    void updatePreviewUi(int _currentModuleIndex);
    void showPreviousPreview();
    void showNextPreview();


    ModulePromoWidget* q = nullptr;

    H6Label* title = nullptr;
    IconButton* close = nullptr;

    StackWidget* preview = nullptr;
    QVector<QWidget*> previewModules;
    Subtitle1Label* previewTitle = nullptr;
    IconButton* previewPrevious = nullptr;
    IconButton* previewNext = nullptr;

    IconsMidLabel* advancedVersionIcon = nullptr;
    Subtitle2Label* advancedVersionTitle = nullptr;
    Body2Label* advancedVersionDescription = nullptr;
    QVBoxLayout* advancedVersionOptionsLayout = nullptr;
    struct OptionItem {
        QHBoxLayout* layout = nullptr;
        IconsSmallLabel* icon = nullptr;
        CaptionLabel* title = nullptr;
    };
    QVector<OptionItem> advancedVersionOptions;
    Button* purchase = nullptr;

    /**
     * @brief Рекламируем PRO версию (true) или CLOUD (false)
     */
    bool isPromoteProVersion = true;

    /**
     * @brief Тип модуля, который должен в карусели показываться первым
     */
    ModulePromoType promoteType = ModulePromoType::Undefined;
};

ModulePromoWidget::Implementation::Implementation(ModulePromoWidget* _q)
    : q(_q)
    , title(new H6Label(q))
    , close(new IconButton(q))
    , preview(new StackWidget(q))
    , previewTitle(new Subtitle1Label(q))
    , previewPrevious(new IconButton(q))
    , previewNext(new IconButton(q))
    , advancedVersionIcon(new IconsMidLabel(q))
    , advancedVersionTitle(new Subtitle2Label(q))
    , advancedVersionDescription(new Body2Label(q))
    , advancedVersionOptionsLayout(UiHelper::makeVBoxLayout())
    , purchase(new Button(q))
{
    preview->setAnimationType(StackWidget::AnimationType::Slide);
    previewPrevious->setIcon(u8"\U000F0141");
    previewNext->setIcon(u8"\U000F0142");
    close->setIcon(u8"\U000F0156");
    purchase->setContained(true);

    auto titleLayout = UiHelper::makeHBoxLayout();
    titleLayout->addWidget(title, 1);
    titleLayout->addWidget(close);

    auto previewTitleLayout = UiHelper::makeHBoxLayout();
    previewTitleLayout->addWidget(previewTitle, 1);
    previewTitleLayout->addWidget(previewPrevious);
    previewTitleLayout->addWidget(previewNext);

    auto advancedVersionTitleLayout = UiHelper::makeHBoxLayout();
    advancedVersionTitleLayout->addWidget(advancedVersionIcon, 0, Qt::AlignTop | Qt::AlignHCenter);
    advancedVersionTitleLayout->addWidget(advancedVersionTitle, 1);

    auto layout = UiHelper::makeVBoxLayout();
    layout->addLayout(titleLayout);
    layout->addWidget(preview);
    layout->addLayout(previewTitleLayout);
    layout->addLayout(advancedVersionTitleLayout);
    layout->addWidget(advancedVersionDescription);
    layout->addLayout(advancedVersionOptionsLayout);
    layout->addStretch();
    layout->addWidget(purchase);
    q->setLayout(layout);
}

void ModulePromoWidget::Implementation::clearPreviews()
{
    while (!previewModules.isEmpty()) {
        auto previewModule = previewModules.takeLast();
        preview->takeWidget(previewModule);
        previewModule->deleteLater();
    }
}

void ModulePromoWidget::Implementation::clearOptions()
{
    while (!advancedVersionOptions.isEmpty()) {
        auto option = advancedVersionOptions.takeLast();
        advancedVersionOptionsLayout->removeItem(option.layout);
        option.icon->deleteLater();
        option.title->deleteLater();
        option.layout->deleteLater();
    }
}

void ModulePromoWidget::Implementation::addPreview(ModulePromoType _forType,
                                                   ModulePromoType _currentType)
{
    auto previewImagePath = [_forType] {
        switch (_forType) {
        case ModulePromoType::CharactersRelations: {
            return ":/images/promo/characters-relations";
        }
        case ModulePromoType::CharacterInformation: {
            return ":/images/promo/character-profile";
        }
        case ModulePromoType::CharacterDialogues: {
            return ":/images/promo/character-dialogues";
        }
        case ModulePromoType::LocationsMap: {
            return ":/images/promo/locations-map";
        }
        case ModulePromoType::LocationInformation: {
            return ":/images/promo/location-profile";
        }
        case ModulePromoType::LocationScenes: {
            return ":/images/promo/location-scenes";
        }
        case ModulePromoType::WorldsMap: {
            return ":/images/promo/worlds-map";
        }
        case ModulePromoType::WoldInformation: {
            return ":/images/promo/world-profile";
        }
        case ModulePromoType::ExtendedStatistics: {
            return ":/images/promo/statistics";
        }
        case ModulePromoType::IndexCards: {
            return ":/images/promo/cards";
        }
        case ModulePromoType::StoryTimeline: {
            return ":/images/promo/timeline";
        }
        case ModulePromoType::ScreenplayBreakdown: {
            return ":/images/promo/breakdown";
        }
        case ModulePromoType::ImagesGallery: {
            return ":/images/promo/image-gallery";
        }
        case ModulePromoType::MindMaps: {
            return ":/images/promo/mindmaps";
        }
        case ModulePromoType::Presentation: {
            return ":/images/promo/presentation";
        }
        case ModulePromoType::SeriesPlan: {
            return ":/images/promo/series-plan";
        }
        case ModulePromoType::SeriesStatistics: {
            return ":/images/promo/statistics";
        }
        case ModulePromoType::CloudSync: {
            return ":/images/promo/cloud";
        }
        default: {
            Q_ASSERT(false);
            return "";
        }
        }
    };

    auto previewModule = new ImageLabel(q);
    previewModule->setProperty(kPromoTypeIdKey, static_cast<int>(_forType));
    previewModule->setBackgroundColor(Qt::transparent);
    previewModule->setImage(QPixmap(previewImagePath()));
    preview->addWidget(previewModule);
    if (_forType == _currentType) {
        preview->setCurrentWidget(previewModule);
    }

    previewModules.append(previewModule);
}

void ModulePromoWidget::Implementation::addOption(const QString& _text)
{
    auto icon = new IconsSmallLabel(q);
    icon->setIcon(u8"\U000F012C");
    auto option = new CaptionLabel(q);
    option->setText(_text);
    auto layout = UiHelper::makeHBoxLayout();
    layout->addWidget(icon);
    layout->addWidget(option, 1);
    advancedVersionOptionsLayout->addLayout(layout);

    advancedVersionOptions.append({ layout, icon, option });
}

void ModulePromoWidget::Implementation::updatePreviewUi(int _currentModuleIndex)
{
    previewPrevious->setEnabled(_currentModuleIndex > 0);
    previewNext->setEnabled(_currentModuleIndex < previewModules.size() - 1);

    const auto promoType = preview->currentWidget()->property(kPromoTypeIdKey).toInt();
    auto buildPreviewTitle = [promoType] {
        switch (static_cast<ModulePromoType>(promoType)) {
        case ModulePromoType::CharactersRelations: {
            return tr("Characters relations");
        }
        case ModulePromoType::CharacterInformation: {
            return tr("Character information");
        }
        case ModulePromoType::CharacterDialogues: {
            return tr("Character dialogues");
        }
        case ModulePromoType::LocationsMap: {
            return tr("Locations map");
        }
        case ModulePromoType::LocationInformation: {
            return tr("Location information");
        }
        case ModulePromoType::LocationScenes: {
            return tr("Location scenes");
        }
        case ModulePromoType::WorldsMap: {
            return tr("Worlds map");
        }
        case ModulePromoType::WoldInformation: {
            return tr("Wold information");
        }
        case ModulePromoType::ExtendedStatistics: {
            return tr("Extended statistics");
        }
        case ModulePromoType::IndexCards: {
            return tr("Index cards");
        }
        case ModulePromoType::StoryTimeline: {
            return tr("Story timeline");
        }
        case ModulePromoType::ScreenplayBreakdown: {
            return tr("Screenplay breakdown");
        }
        case ModulePromoType::ImagesGallery: {
            return tr("Images gallery");
        }
        case ModulePromoType::MindMaps: {
            return tr("Mind maps");
        }
        case ModulePromoType::Presentation: {
            return tr("Presentation");
        }
        case ModulePromoType::SeriesPlan: {
            return tr("Series plan");
        }
        case ModulePromoType::CloudSync: {
            return tr("Cloud synchronization");
        }
        case ModulePromoType::SeriesStatistics: {
            return tr("Series statistics");
        }

        default: {
            Q_ASSERT(false);
            return QString();
        }
        }
    };
    previewTitle->setText(buildPreviewTitle());
}

void ModulePromoWidget::Implementation::showPreviousPreview()
{
    auto currentModuleIndex = previewModules.indexOf(preview->currentWidget());
    if (currentModuleIndex == 0) {
        return;
    }

    --currentModuleIndex;
    preview->setCurrentWidget(previewModules.at(currentModuleIndex));

    updatePreviewUi(currentModuleIndex);
}

void ModulePromoWidget::Implementation::showNextPreview()
{
    auto currentModuleIndex = previewModules.indexOf(preview->currentWidget());
    if (currentModuleIndex == previewModules.size() - 1) {
        return;
    }

    ++currentModuleIndex;
    preview->setCurrentWidget(previewModules.at(currentModuleIndex));

    updatePreviewUi(currentModuleIndex);
}


// ****


ModulePromoWidget::ModulePromoWidget(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation(this))
{
    connect(d->close, &IconButton::clicked, this, &ModulePromoWidget::closePressed);
    connect(d->previewPrevious, &IconButton::clicked, this, [this] { d->showPreviousPreview(); });
    connect(d->previewNext, &IconButton::clicked, this, [this] { d->showNextPreview(); });
    connect(d->purchase, &Button::clicked, this, &ModulePromoWidget::purchasePressed);
}

ModulePromoWidget::~ModulePromoWidget() = default;

void ModulePromoWidget::setupProVersionContent(ModulePromoType _forType)
{
    if (_forType == ModulePromoType::Undefined) {
        return;
    }


    auto advancedVersionDescription = [_forType] {
        QString componentName;
        switch (_forType) {
        case ModulePromoType::CharactersRelations: {
            componentName = tr("characters relations map");
            break;
        }
        case ModulePromoType::CharacterInformation: {
            componentName = tr("full character profile");
            break;
        }
        case ModulePromoType::CharacterDialogues: {
            componentName = tr("character dialogues report");
            break;
        }
        case ModulePromoType::LocationsMap: {
            componentName = tr("locations map");
            break;
        }
        case ModulePromoType::LocationInformation: {
            componentName = tr("full location profile");
            break;
        }
        case ModulePromoType::LocationScenes: {
            componentName = tr("location scenes report");
            break;
        }
        case ModulePromoType::WorldsMap: {
            componentName = tr("worlds map");
            break;
        }
        case ModulePromoType::WoldInformation: {
            componentName = tr("world profile");
            break;
        }
        case ModulePromoType::IndexCards: {
            componentName = tr("story index cards");
            break;
        }
        case ModulePromoType::StoryTimeline: {
            componentName = tr("story timeline");
            break;
        }
        case ModulePromoType::ExtendedStatistics: {
            componentName = tr("extended statistics");
            break;
        }
        case ModulePromoType::ImagesGallery: {
            componentName = tr("images gallery");
            break;
        }
        case ModulePromoType::MindMaps: {
            componentName = tr("mind maps");
            break;
        }
        case ModulePromoType::Presentation: {
            componentName = tr("presentation");
            break;
        }
        default: {
            return tr("Subscribe to PRO version and get advanced features that speed up and "
                      "simplify your work");
        }
        }

        return QString("%1 %2 %3")
            .arg(tr("Subscribe to PRO version and unlock access to the"), componentName,
                 tr("and other advanced features that speed up and simplify your work"));
    };

    if (d->isPromoteProVersion != true || d->promoteType != _forType) {
        d->isPromoteProVersion = true;
        d->promoteType = _forType;

        d->clearPreviews();
        d->addPreview(ModulePromoType::CharactersRelations, _forType);
        d->addPreview(ModulePromoType::CharacterInformation, _forType);
        d->addPreview(ModulePromoType::CharacterDialogues, _forType);
        d->addPreview(ModulePromoType::LocationsMap, _forType);
        d->addPreview(ModulePromoType::LocationInformation, _forType);
        d->addPreview(ModulePromoType::LocationScenes, _forType);
        d->addPreview(ModulePromoType::WorldsMap, _forType);
        d->addPreview(ModulePromoType::WoldInformation, _forType);
        d->addPreview(ModulePromoType::IndexCards, _forType);
        d->addPreview(ModulePromoType::StoryTimeline, _forType);
        d->addPreview(ModulePromoType::ExtendedStatistics, _forType);
        d->addPreview(ModulePromoType::ImagesGallery, _forType);
        d->addPreview(ModulePromoType::MindMaps, _forType);
        d->addPreview(ModulePromoType::Presentation, _forType);
    }
    d->updatePreviewUi(d->previewModules.indexOf(d->preview->currentWidget()));

    d->title->setText(tr("Activate PRO"));

    d->advancedVersionIcon->setIcon(u8"\U000F18BC");
    d->advancedVersionTitle->setText(tr("Get access to the best features with a PRO subscription"));
    d->advancedVersionDescription->setText(advancedVersionDescription());

    d->clearOptions();
    d->addOption(
        tr("Advanced worldbuilding tools (full character, location and world profiles and maps)"));
    d->addOption(tr("Story timeline, index cards, extended statistics"));
    d->addOption(tr("Extended research tools (mind maps, images gallery and presentations)"));

    d->purchase->setText(tr("Become a PRO user"));

    designSystemChangeEvent(nullptr);
}

void ModulePromoWidget::setupCloudVersionContent(ModulePromoType _forType)
{
    if (_forType == ModulePromoType::Undefined) {
        return;
    }


    auto advancedVersionDescription = [_forType] {
        QString componentName;
        switch (_forType) {
        case ModulePromoType::CloudSync: {
            componentName = tr("cloud sync, real time collaboration");
            break;
        }
        case ModulePromoType::ScreenplayBreakdown: {
            componentName = tr("screenplay breakdown");
            break;
        }
        case ModulePromoType::SeriesPlan: {
            componentName = tr("series plan");
            break;
        }
        case ModulePromoType::SeriesStatistics: {
            componentName = tr("series statistics");
            break;
        }
        default: {
            return tr("Subscribe to CLOUD version and get advanced features that speed up and "
                      "simplify your work");
        }
        }

        return QString("%1 %2 %3")
            .arg(tr("Subscribe to CLOUD version and unlock access to the"), componentName,
                 tr("and other advanced features that speed up and simplify your work"));
    };

    if (d->isPromoteProVersion != false || d->promoteType != _forType) {
        d->isPromoteProVersion = false;
        d->promoteType = _forType;

        d->clearPreviews();
        d->addPreview(ModulePromoType::CloudSync, _forType);
        d->addPreview(ModulePromoType::ScreenplayBreakdown, _forType);
        d->addPreview(ModulePromoType::SeriesPlan, _forType);
        d->addPreview(ModulePromoType::SeriesStatistics, _forType);
    }
    d->updatePreviewUi(d->previewModules.indexOf(d->preview->currentWidget()));

    d->title->setText(tr("Activate PRO"));

    d->advancedVersionIcon->setIcon(u8"\U000F18BC");
    d->advancedVersionTitle->setText(tr("Get access to all features with a CLOUD subscription"));
    d->advancedVersionDescription->setText(advancedVersionDescription());

    d->clearOptions();
    d->addOption(tr("Cloud sync and real time collaboration"));
    d->addOption(tr("Preproduction tools (screenplay breakdown)"));
    d->addOption(tr("Showrunner tools (series plan, series statistics)"));
    d->addOption(tr("Plus everything from the PRO version"));

    d->purchase->setText(tr("Become a CLOUD user"));

    designSystemChangeEvent(nullptr);
}

void ModulePromoWidget::resizeEvent(QResizeEvent* _event)
{
    Widget::resizeEvent(_event);

    d->preview->setMinimumHeight(width() * 0.68);
}

void ModulePromoWidget::updateTranslations()
{
    if (d->isPromoteProVersion) {
        setupProVersionContent(d->promoteType);
    } else {
        setupCloudVersionContent(d->promoteType);
    }
}

void ModulePromoWidget::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    setBackgroundColor(DesignSystem::color().primary());
    for (auto label : std::vector<Widget*>{
             d->title,
             d->close,
             d->previewTitle,
             d->advancedVersionTitle,
         }) {
        label->setBackgroundColor(DesignSystem::color().primary());
        label->setTextColor(DesignSystem::color().onPrimary());
    }
    d->title->setContentsMargins(DesignSystem::layout().px16(), DesignSystem::layout().px16(),
                                 DesignSystem::layout().px16(), DesignSystem::layout().px16());

    d->preview->setBackgroundColor(DesignSystem::color().primary());
    for (auto previewModule : d->previewModules) {
        previewModule->setContentsMargins(DesignSystem::layout().px16(),
                                          DesignSystem::layout().px4(),
                                          DesignSystem::layout().px16(), 0);
    }
    d->previewTitle->setContentsMargins(
        DesignSystem::layout().px24(), DesignSystem::layout().px12(), DesignSystem::layout().px16(),
        DesignSystem::layout().px12());
    d->previewPrevious->setBackgroundColor(DesignSystem::color().primary());
    d->previewPrevious->setTextColor(DesignSystem::color().onPrimary());
    d->previewPrevious->setContentsMargins(DesignSystem::layout().px16(), 0, 0, 0);
    d->previewNext->setBackgroundColor(DesignSystem::color().primary());
    d->previewNext->setTextColor(DesignSystem::color().onPrimary());
    d->previewNext->setContentsMargins(0, 0, DesignSystem::layout().px16(), 0);

    d->advancedVersionIcon->setBackgroundColor(DesignSystem::color().primary());
    d->advancedVersionIcon->setTextColor(DesignSystem::color().accent());
    d->advancedVersionIcon->setContentsMargins(
        DesignSystem::layout().px12(), DesignSystem::layout().px24(), DesignSystem::layout().px8(),
        DesignSystem::layout().px16());
    d->advancedVersionTitle->setContentsMargins(0, DesignSystem::layout().px24(),
                                                DesignSystem::layout().px16(),
                                                DesignSystem::layout().px16());
    d->advancedVersionDescription->setBackgroundColor(DesignSystem::color().primary());
    d->advancedVersionDescription->setTextColor(ColorHelper::transparent(
        DesignSystem::color().onPrimary(), DesignSystem::inactiveTextOpacity()));
    d->advancedVersionDescription->setContentsMargins(DesignSystem::layout().px16(), 0,
                                                      DesignSystem::layout().px16(),
                                                      DesignSystem::layout().px12());
    for (auto option : d->advancedVersionOptions) {
        option.icon->setBackgroundColor(DesignSystem::color().primary());
        option.icon->setTextColor(ColorHelper::forNumber(5));
        option.icon->setContentsMargins(DesignSystem::layout().px12(), 0,
                                        DesignSystem::layout().px8(),
                                        DesignSystem::layout().px12());
        option.title->setBackgroundColor(DesignSystem::color().primary());
        option.title->setTextColor(DesignSystem::color().onPrimary());
        option.title->setContentsMargins(0, 0, DesignSystem::layout().px16(),
                                         DesignSystem::layout().px12());
    }

    d->purchase->setBackgroundColor(DesignSystem::color().accent());
    d->purchase->setTextColor(DesignSystem::color().onAccent());
    d->purchase->setContentsMargins(DesignSystem::layout().px8(), DesignSystem::layout().px16(),
                                    DesignSystem::layout().px8(), DesignSystem::layout().px8());
}

} // namespace Ui
