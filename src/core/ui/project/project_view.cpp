#include "project_view.h"

#include <business_layer/model/structure/structure_model_item.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/label/link_label.h>
#include <ui/widgets/tab_bar/tab_bar.h>
#include <utils/helpers/color_helper.h>

#include <QVBoxLayout>
#include <QVariantAnimation>


namespace Ui {

class ProjectView::Implementation
{
public:
    explicit Implementation(QWidget* _parent);


    Widget* defaultPage = nullptr;
    H6Label* defaultPageTitleLabel = nullptr;
    Body1Label* defaultPageBodyLabel = nullptr;
    Body1LinkLabel* defaultPageAddItemButton = nullptr;

    Widget* notImplementedPage = nullptr;
    H6Label* notImplementedPageTitleLabel = nullptr;
    Body1Label* notImplementedPageBodyLabel = nullptr;

    Widget* documentEditorPage = nullptr;
    TabBar* documentVersions = nullptr;
    QVariantAnimation documentVersionsHeightAnimation;
    StackWidget* documentEditor = nullptr;

    Widget* overlay = nullptr;
    QVariantAnimation overlayOpacityAnimation;
};

ProjectView::Implementation::Implementation(QWidget* _parent)
    : defaultPage(new Widget(_parent))
    , defaultPageTitleLabel(new H6Label(defaultPage))
    , defaultPageBodyLabel(new Body1Label(defaultPage))
    , defaultPageAddItemButton(new Body1LinkLabel(defaultPage))
    , notImplementedPage(new Widget(_parent))
    , notImplementedPageTitleLabel(new H6Label(notImplementedPage))
    , notImplementedPageBodyLabel(new Body1Label(notImplementedPage))
    , documentEditorPage(new Widget(_parent))
    , documentVersions(new TabBar(documentEditorPage))
    , documentEditor(new StackWidget(documentEditorPage))
    , overlay(new Widget(_parent))
{
    defaultPage->setFocusPolicy(Qt::StrongFocus);
    defaultPageBodyLabel->setAlignment(Qt::AlignCenter);
    notImplementedPage->setFocusPolicy(Qt::StrongFocus);
    notImplementedPageBodyLabel->setAlignment(Qt::AlignCenter);
    documentVersions->hide();
    documentVersions->setContextMenuPolicy(Qt::CustomContextMenu);
    documentEditor->setAnimationType(AnimationType::FadeThrough);
    overlay->setAttribute(Qt::WA_TransparentForMouseEvents);
    overlay->hide();
    overlayOpacityAnimation.setDuration(180);
    overlayOpacityAnimation.setEasingCurve(QEasingCurve::OutQuad);
    overlayOpacityAnimation.setStartValue(0.0);
    overlayOpacityAnimation.setEndValue(1.0);

    {
        QVBoxLayout* layout = new QVBoxLayout(defaultPage);
        layout->setContentsMargins({});
        layout->setSpacing(0);
        layout->addStretch();
        layout->addWidget(defaultPageTitleLabel, 0, Qt::AlignHCenter);
        QHBoxLayout* bodyLayout = new QHBoxLayout;
        bodyLayout->setContentsMargins({});
        bodyLayout->setSpacing(0);
        bodyLayout->addStretch();
        bodyLayout->addWidget(defaultPageBodyLabel, 0, Qt::AlignHCenter);
        bodyLayout->addWidget(defaultPageAddItemButton, 0, Qt::AlignHCenter);
        bodyLayout->addStretch();
        layout->addLayout(bodyLayout);
        layout->addStretch();
    }

    {
        QVBoxLayout* layout = new QVBoxLayout(notImplementedPage);
        layout->setContentsMargins({});
        layout->setSpacing(0);
        layout->addStretch();
        layout->addWidget(notImplementedPageTitleLabel, 0, Qt::AlignHCenter);
        QHBoxLayout* bodyLayout = new QHBoxLayout;
        bodyLayout->setContentsMargins({});
        bodyLayout->setSpacing(0);
        bodyLayout->addStretch();
        bodyLayout->addWidget(notImplementedPageBodyLabel, 0, Qt::AlignHCenter);
        bodyLayout->addStretch();
        layout->addLayout(bodyLayout);
        layout->addStretch();
    }

    {
        auto layout = new QVBoxLayout(documentEditorPage);
        layout->setContentsMargins({});
        layout->setSpacing(0);
        layout->addWidget(documentVersions);
        layout->addWidget(documentEditor, 1);
    }

    documentVersionsHeightAnimation.setEasingCurve(QEasingCurve::OutQuad);
    documentVersionsHeightAnimation.setDuration(160);
}


// ****


ProjectView::ProjectView(QWidget* _parent)
    : StackWidget(_parent)
    , d(new Implementation(this))
{
    setFocusPolicy(Qt::StrongFocus);
    setAnimationType(AnimationType::FadeThrough);

    addWidget(d->defaultPage);
    addWidget(d->notImplementedPage);
    addWidget(d->documentEditorPage);

    showDefaultPage();

    connect(d->defaultPageAddItemButton, &Body1LinkLabel::clicked, this,
            &ProjectView::createNewItemPressed);
    connect(d->documentVersions, &TabBar::currentIndexChanged, this,
            &ProjectView::showVersionPressed);
    connect(d->documentVersions, &TabBar::customContextMenuRequested, this,
            [this](const QPoint _position) {
                emit showVersionContextMenuPressed(d->documentVersions->tabAt(_position));
            });
    connect(
        &d->documentVersionsHeightAnimation, &QVariantAnimation::valueChanged, this,
        [this](const QVariant& _value) { d->documentVersions->setFixedHeight(_value.toInt()); });
    connect(&d->documentVersionsHeightAnimation, &QVariantAnimation::finished, this, [this] {
        if (d->documentVersionsHeightAnimation.direction() == QVariantAnimation::Backward) {
            d->documentVersions->hide();
        }
    });
    connect(&d->overlayOpacityAnimation, &QVariantAnimation::valueChanged, this,
            [this](const QVariant& _value) { d->overlay->setOpacity(_value.toReal()); });
    connect(&d->overlayOpacityAnimation, &QVariantAnimation::finished, this, [this] {
        if (d->overlayOpacityAnimation.direction() == QVariantAnimation::Backward) {
            d->overlay->hide();
        }
    });
}

ProjectView::~ProjectView() = default;

void ProjectView::showDefaultPage()
{
    setCurrentWidget(d->defaultPage);
}

void ProjectView::showNotImplementedPage()
{
    setCurrentWidget(d->notImplementedPage);
}

QWidget* ProjectView::currentEditor() const
{
    if (currentWidget() != d->documentEditorPage) {
        return nullptr;
    }

    return d->documentEditor->currentWidget();
}

void ProjectView::showEditor(QWidget* _widget)
{
    setFocus();
    d->documentEditor->setCurrentWidget(_widget);
    setCurrentWidget(d->documentEditorPage);
}

void ProjectView::setActive(bool _active)
{
    d->overlayOpacityAnimation.stop();
    d->overlayOpacityAnimation.setDirection(_active ? QVariantAnimation::Backward
                                                    : QVariantAnimation::Forward);
    d->overlayOpacityAnimation.start();
    if (!_active) {
        d->overlay->raise();
        d->overlay->show();
    }
}

void ProjectView::setDocumentVersions(const QVector<BusinessLayer::StructureModelItem*>& _versions)
{
    if (d->documentVersions->count() == 1 && _versions.isEmpty()) {
        return;
    }

    //
    // Блокируем сигналы, чтобы менеджер не думал, что мы переключаемся тут между разными версиями
    //
    QSignalBlocker blocker(this);

    const auto lastActiveVersion = d->documentVersions->currentTab();

    d->documentVersions->removeAllTabs();
    d->documentVersions->addTab(tr("Current draft"));
    for (const auto version : _versions) {
        d->documentVersions->addTab(version->name(),
                                    version->isReadOnly() ? u8"\U000F033E" : u8"\U000F0765",
                                    version->color());
    }

    d->documentVersions->setCurrentTab(lastActiveVersion);

    d->documentVersionsHeightAnimation.setStartValue(0);
    d->documentVersionsHeightAnimation.setEndValue(d->documentVersions->sizeHint().height());
}

void ProjectView::setVersionsVisible(bool _visible)
{
    if (d->documentVersionsHeightAnimation.state() == QVariantAnimation::Running) {
        if ((d->documentVersionsHeightAnimation.direction() == QVariantAnimation::Forward
             && _visible)
            || (d->documentVersionsHeightAnimation.direction() == QVariantAnimation::Backward
                && !_visible)) {
            return;
        }
        d->documentVersionsHeightAnimation.stop();
    }

    if (_visible) {
        d->documentVersions->show();
    }
    d->documentVersionsHeightAnimation.setDirection(_visible ? QVariantAnimation::Forward
                                                             : QVariantAnimation::Backward);
    d->documentVersionsHeightAnimation.start();
}

int ProjectView::currentVersion() const
{
    return d->documentVersions->currentTab();
}

void ProjectView::setCurrentVersion(int _index)
{
    d->documentVersions->setCurrentTab(_index);
}

void ProjectView::resizeEvent(QResizeEvent* _event)
{
    StackWidget::resizeEvent(_event);

    d->overlay->resize(size());
}

void ProjectView::updateTranslations()
{
    d->defaultPageTitleLabel->setText(
        tr("Here will be an editor of the document you choose in the navigator (at left)."));
    d->defaultPageBodyLabel->setText(tr("Choose an item to edit, or"));
    d->defaultPageAddItemButton->setText(tr("create a new one"));

    d->notImplementedPageTitleLabel->setText(
        tr("Ooops... looks like editor of this document not implemented yet."));
    d->notImplementedPageBodyLabel->setText(
        tr("But don't worry, it will be here in one of the future updates!"));

    d->documentVersions->setTabName(0, tr("Current draft"));
}

void ProjectView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    StackWidget::designSystemChangeEvent(_event);

    setBackgroundColor(Ui::DesignSystem::color().surface());

    d->defaultPage->setBackgroundColor(Ui::DesignSystem::color().surface());
    d->defaultPageBodyLabel->setContentsMargins(
        0, static_cast<int>(Ui::DesignSystem::layout().px16()),
        static_cast<int>(Ui::DesignSystem::layout().px4()), 0);
    for (auto label : std::vector<Widget*>{
             d->defaultPageTitleLabel,
             d->defaultPageBodyLabel,
         }) {
        label->setBackgroundColor(Ui::DesignSystem::color().surface());
        label->setTextColor(Ui::DesignSystem::color().onSurface());
    }
    d->defaultPageAddItemButton->setContentsMargins(
        0, static_cast<int>(Ui::DesignSystem::layout().px16()), 0, 0);
    d->defaultPageAddItemButton->setBackgroundColor(Ui::DesignSystem::color().surface());
    d->defaultPageAddItemButton->setTextColor(Ui::DesignSystem::color().accent());

    d->notImplementedPage->setBackgroundColor(Ui::DesignSystem::color().surface());
    d->notImplementedPageBodyLabel->setContentsMargins(
        0, static_cast<int>(Ui::DesignSystem::layout().px16()),
        static_cast<int>(Ui::DesignSystem::layout().px4()), 0);
    for (auto label : std::vector<Widget*>{
             d->notImplementedPageTitleLabel,
             d->notImplementedPageBodyLabel,
         }) {
        label->setBackgroundColor(Ui::DesignSystem::color().surface());
        label->setTextColor(Ui::DesignSystem::color().onSurface());
    }

    d->documentEditorPage->setBackgroundColor(Ui::DesignSystem::color().surface());
    d->documentVersions->setBackgroundColor(
        ColorHelper::nearby(Ui::DesignSystem::color().background()));
    d->documentVersions->setTextColor(Ui::DesignSystem::color().onBackground());
    d->documentEditor->setBackgroundColor(Ui::DesignSystem::color().surface());

    d->overlay->setBackgroundColor(backgroundColor());
    d->overlayOpacityAnimation.setEndValue(Ui::DesignSystem::focusBackgroundOpacity());
}

void ProjectView::setCurrentWidget(QWidget* _widget)
{
    StackWidget::setCurrentWidget(_widget);
}

} // namespace Ui
