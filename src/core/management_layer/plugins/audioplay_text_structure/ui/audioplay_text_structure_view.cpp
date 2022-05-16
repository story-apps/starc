#include "audioplay_text_structure_view.h"

#include "audioplay_text_structure_delegate.h"

#include <business_layer/model/text/text_model_group_item.h>
#include <business_layer/templates/audioplay_template.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/scroll_bar/scroll_bar.h>
#include <ui/widgets/shadow/shadow.h>
#include <ui/widgets/tree/tree.h>

#include <QAction>
#include <QStringListModel>
#include <QVBoxLayout>


namespace Ui {

class AudioplayTextStructureView::Implementation
{
public:
    explicit Implementation(QWidget* _parent);


    IconsMidLabel* backIcon = nullptr;
    Subtitle2Label* backText = nullptr;
    Tree* content = nullptr;
    AudioplayTextStructureDelegate* contentDelegate = nullptr;
};

AudioplayTextStructureView::Implementation::Implementation(QWidget* _parent)
    : backIcon(new IconsMidLabel(_parent))
    , backText(new Subtitle2Label(_parent))
    , content(new Tree(_parent))
    , contentDelegate(new AudioplayTextStructureDelegate(content))
{
    backIcon->setText(u8"\U000F0141");

    content->setContextMenuPolicy(Qt::CustomContextMenu);
    content->setDragDropEnabled(true);
    content->setSelectionMode(QAbstractItemView::ContiguousSelection);
    content->setItemDelegate(contentDelegate);

    new Shadow(Qt::TopEdge, content);
}


// ****


AudioplayTextStructureView::AudioplayTextStructureView(QWidget* _parent)
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
    setLayout(layout);


    connect(d->backIcon, &AbstractLabel::clicked, this, &AudioplayTextStructureView::backPressed);
    connect(d->backText, &AbstractLabel::clicked, this, &AudioplayTextStructureView::backPressed);
    connect(d->content, &Tree::clicked, this,
            &AudioplayTextStructureView::currentModelIndexChanged);
    connect(d->content, &Tree::doubleClicked, this,
            &AudioplayTextStructureView::currentModelIndexChanged);
    connect(d->content, &Tree::customContextMenuRequested, this, [this](const QPoint& _pos) {
        emit customContextMenuRequested(d->content->mapToParent(_pos));
    });


    updateTranslations();
    designSystemChangeEvent(nullptr);
}

AudioplayTextStructureView::~AudioplayTextStructureView() = default;

QWidget* AudioplayTextStructureView::asQWidget()
{
    return this;
}

void AudioplayTextStructureView::reconfigure()
{
    const bool showSceneNumber
        = settingsValue(DataStorageLayer::kComponentsAudioplayNavigatorShowSceneNumberKey).toBool();
    d->contentDelegate->showSceneNumber(showSceneNumber);

    const bool showSceneText
        = settingsValue(DataStorageLayer::kComponentsAudioplayNavigatorShowSceneTextKey).toBool();
    if (showSceneText == false) {
        d->contentDelegate->setTextLinesSize(0);
    } else {
        const int sceneTextLines
            = settingsValue(DataStorageLayer::kComponentsAudioplayNavigatorSceneTextLinesKey)
                  .toInt();
        d->contentDelegate->setTextLinesSize(sceneTextLines);
    }

    d->content->setItemDelegate(nullptr);
    d->content->setItemDelegate(d->contentDelegate);
}

void AudioplayTextStructureView::setTitle(const QString& _title)
{
    d->backText->setText(_title);
}

void AudioplayTextStructureView::setModel(QAbstractItemModel* _model)
{
    d->content->setModel(_model);
}

void AudioplayTextStructureView::setCurrentModelIndex(const QModelIndex& _sourceIndex,
                                                      const QModelIndex& _mappedIndex)
{
    d->content->setCurrentIndex(_mappedIndex);
}

QModelIndexList AudioplayTextStructureView::selectedIndexes() const
{
    return d->content->selectedIndexes();
}

void AudioplayTextStructureView::updateTranslations()
{
    d->backText->setText("Back to navigator");
}

void AudioplayTextStructureView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
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

    d->backIcon->setContentsMargins(
        QMarginsF(Ui::DesignSystem::layout().px12(), Ui::DesignSystem::layout().px8(),
                  Ui::DesignSystem::layout().px4(), Ui::DesignSystem::layout().px8())
            .toMargins());
    d->backText->setContentsMargins(QMarginsF(0, Ui::DesignSystem::layout().px12(),
                                              Ui::DesignSystem::layout().px16(),
                                              Ui::DesignSystem::layout().px12())
                                        .toMargins());
}

} // namespace Ui
