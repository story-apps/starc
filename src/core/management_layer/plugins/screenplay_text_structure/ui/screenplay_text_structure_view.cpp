#include "screenplay_text_structure_view.h"

#include "beat_name_widget.h"
#include "screenplay_text_structure_delegate.h"

#include <business_layer/model/text/text_model_group_item.h>
#include <business_layer/templates/screenplay_template.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <interfaces/management_layer/i_document_manager.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/scroll_bar/scroll_bar.h>
#include <ui/widgets/shadow/shadow.h>
#include <ui/widgets/tree/tree.h>

#include <QAction>
#include <QStringListModel>
#include <QVBoxLayout>


namespace Ui {

namespace {

const QString kSettingsKey = "screenplay-text-structure";
const QString kIsBeatTextVisibleKey = kSettingsKey + "/is-beat-text-visible";

} // namespace

class ScreenplayTextStructureView::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    /**
     * @brief Обновить название текущего бита
     */
    void updateCurrentBeatName(const QModelIndex& _index);

    /**
     * @brief Обновить переводы дополнительных действий
     */
    void updateOptionsTranslations();


    IconsMidLabel* backIcon = nullptr;
    Subtitle2Label* backText = nullptr;
    Tree* content = nullptr;
    ScreenplayTextStructureDelegate* contentDelegate = nullptr;
    BeatNameWidget* beatNameWidget = nullptr;

    //
    // Действия опций редактора
    //
    QAction* showBeatTextAction = nullptr;
};

ScreenplayTextStructureView::Implementation::Implementation(QWidget* _parent)
    : backIcon(new IconsMidLabel(_parent))
    , backText(new Subtitle2Label(_parent))
    , content(new Tree(_parent))
    , contentDelegate(new ScreenplayTextStructureDelegate(content))
    , beatNameWidget(new BeatNameWidget(_parent))
    , showBeatTextAction(new QAction(_parent))
{
    backIcon->setIcon(u8"\U000F0141");

    content->setContextMenuPolicy(Qt::CustomContextMenu);
    content->setDragDropEnabled(true);
    content->setSelectionMode(QAbstractItemView::ContiguousSelection);
    content->setAutoAdjustSize(true);
    content->setItemDelegate(contentDelegate);

    new Shadow(Qt::TopEdge, content);
    new Shadow(Qt::BottomEdge, content);

    beatNameWidget->hide();

    showBeatTextAction->setCheckable(true);
    showBeatTextAction->setIconText(u8"\U000F09A8");
}

void ScreenplayTextStructureView::Implementation::updateCurrentBeatName(const QModelIndex& _index)
{
    using namespace BusinessLayer;
    if (static_cast<TextModelItemType>(_index.data(Qt::UserRole).toInt())
            == TextModelItemType::Group
        && static_cast<TextGroupType>(_index.data(TextModelGroupItem::GroupTypeRole).toInt())
            == TextGroupType::Beat) {
        beatNameWidget->setBeatName(_index.data(TextModelGroupItem::GroupHeadingRole).toString());
    } else {
        beatNameWidget->clearBeatName();
    }
}

void ScreenplayTextStructureView::Implementation::updateOptionsTranslations()
{
    showBeatTextAction->setText(showBeatTextAction->isChecked() ? tr("Hide current beat name")
                                                                : tr("Show current beat name"));
}


// ****


ScreenplayTextStructureView::ScreenplayTextStructureView(QWidget* _parent)
    : AbstractNavigator(_parent)
    , d(new Implementation(this))
{
    QHBoxLayout* topLayout = new QHBoxLayout;
    topLayout->setContentsMargins({});
    topLayout->setSpacing(0);
    topLayout->addWidget(d->backIcon);
    topLayout->addWidget(d->backText, 1);

    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addLayout(topLayout);
    layout->addWidget(d->content);
    layout->addWidget(d->beatNameWidget);
    setLayout(layout);


    connect(d->backIcon, &AbstractLabel::clicked, this, &ScreenplayTextStructureView::backPressed);
    connect(d->backText, &AbstractLabel::clicked, this, &ScreenplayTextStructureView::backPressed);
    connect(d->content, &Tree::clicked, this,
            &ScreenplayTextStructureView::currentModelIndexChanged);
    connect(d->content, &Tree::doubleClicked, this,
            &ScreenplayTextStructureView::currentModelIndexChanged);
    connect(d->content, &Tree::customContextMenuRequested, this, [this](const QPoint& _pos) {
        emit customContextMenuRequested(d->content->mapToParent(_pos));
    });
    connect(this, &ScreenplayTextStructureView::currentModelIndexChanged, this,
            [this](const QModelIndex& _index) { d->updateCurrentBeatName(_index); });
    connect(d->beatNameWidget, &BeatNameWidget::pasteBeatNamePressed, this,
            &ScreenplayTextStructureView::pasteBeatNamePressed);
    connect(d->showBeatTextAction, &QAction::toggled, this, [this](bool _checked) {
        d->updateOptionsTranslations();
        d->beatNameWidget->setVisible(_checked);
    });
}

ScreenplayTextStructureView::~ScreenplayTextStructureView() = default;

QWidget* ScreenplayTextStructureView::asQWidget()
{
    return this;
}

QVector<QAction*> ScreenplayTextStructureView::options() const
{
    return {
        d->showBeatTextAction,
    };
}

void ScreenplayTextStructureView::setEditingMode(ManagementLayer::DocumentEditingMode _mode)
{
    const auto readOnly = _mode != ManagementLayer::DocumentEditingMode::Edit;
    d->content->setContextMenuPolicy(readOnly ? Qt::NoContextMenu : Qt::CustomContextMenu);
    d->content->setDragDropEnabled(!readOnly);
}

void ScreenplayTextStructureView::reconfigure()
{
    const bool showFullBeatText
        = settingsValue(DataStorageLayer::kComponentsScreenplayNavigatorShowFullBeatTextKey)
              .toBool();
    d->contentDelegate->setShowFullBeatText(showFullBeatText);

    const bool showSceneNumber
        = settingsValue(DataStorageLayer::kComponentsScreenplayNavigatorShowSceneNumberKey)
              .toBool();
    d->contentDelegate->showSceneNumber(showSceneNumber);

    const bool showSceneText
        = settingsValue(DataStorageLayer::kComponentsScreenplayNavigatorShowSceneTextKey).toBool();
    if (showSceneText == false) {
        d->contentDelegate->setTextLinesSize(0);
    } else {
        const int sceneTextLines
            = settingsValue(DataStorageLayer::kComponentsScreenplayNavigatorSceneTextLinesKey)
                  .toInt();
        d->contentDelegate->setTextLinesSize(sceneTextLines);
    }

    d->content->setItemDelegate(nullptr);
    d->content->setItemDelegate(d->contentDelegate);
}

void ScreenplayTextStructureView::loadViewSettings()
{
    const auto isBeatTextVisible = settingsValue(kIsBeatTextVisibleKey, false).toBool();
    d->showBeatTextAction->setChecked(isBeatTextVisible);
}

void ScreenplayTextStructureView::saveViewSettings()
{
    setSettingsValue(kIsBeatTextVisibleKey, d->showBeatTextAction->isChecked());
}

void ScreenplayTextStructureView::setTitle(const QString& _title)
{
    d->backText->setText(_title);
}

void ScreenplayTextStructureView::setModel(QAbstractItemModel* _model)
{
    d->content->setModel(_model);
}

void ScreenplayTextStructureView::setCurrentModelIndex(const QModelIndex& _sourceIndex,
                                                       const QModelIndex& _mappedIndex)
{
    d->content->setCurrentIndex(_mappedIndex);
    d->updateCurrentBeatName(_sourceIndex);
}

QModelIndexList ScreenplayTextStructureView::selectedIndexes() const
{
    return d->content->selectedIndexes();
}

void ScreenplayTextStructureView::updateTranslations()
{
    d->updateOptionsTranslations();
}

void ScreenplayTextStructureView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    setBackgroundColor(DesignSystem::color().primary());
    auto backTextColor = DesignSystem::color().onPrimary();
    backTextColor.setAlphaF(Ui::DesignSystem::inactiveTextOpacity());
    for (auto widget : std::vector<Widget*>{
             d->backIcon,
             d->backText,
         }) {
        widget->setBackgroundColor(DesignSystem::color().primary());
        widget->setTextColor(backTextColor);
    }
    d->content->setBackgroundColor(DesignSystem::color().primary());
    d->content->setTextColor(DesignSystem::color().onPrimary());

    d->backIcon->setContentsMargins(QMarginsF(isLeftToRight() ? Ui::DesignSystem::layout().px12()
                                                              : Ui::DesignSystem::layout().px4(),
                                              Ui::DesignSystem::layout().px8(),
                                              isLeftToRight() ? Ui::DesignSystem::layout().px4()
                                                              : Ui::DesignSystem::layout().px12(),
                                              Ui::DesignSystem::layout().px8())
                                        .toMargins());
    d->backText->setContentsMargins(
        QMarginsF(isLeftToRight() ? 0.0 : Ui::DesignSystem::layout().px16(),
                  Ui::DesignSystem::layout().px12(),
                  isLeftToRight() ? Ui::DesignSystem::layout().px16() : 0.0,
                  Ui::DesignSystem::layout().px12())
            .toMargins());
}

} // namespace Ui
