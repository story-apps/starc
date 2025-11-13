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

    Widget* documentLoadingPage = nullptr;
    H6Label* documentLoadingPageTitleLabel = nullptr;
    Body1Label* documentLoadingPageBodyLabel = nullptr;

    Widget* notImplementedPage = nullptr;
    H6Label* notImplementedPageTitleLabel = nullptr;
    Body1Label* notImplementedPageBodyLabel = nullptr;

    Widget* documentEditorPage = nullptr;
    TabBar* documentDrafts = nullptr;
    QVariantAnimation documentDraftsHeightAnimation;
    StackWidget* documentEditor = nullptr;

    Widget* overlay = nullptr;
    QVariantAnimation overlayOpacityAnimation;
};

ProjectView::Implementation::Implementation(QWidget* _parent)
    : defaultPage(new Widget(_parent))
    , defaultPageTitleLabel(new H6Label(defaultPage))
    , defaultPageBodyLabel(new Body1Label(defaultPage))
    , defaultPageAddItemButton(new Body1LinkLabel(defaultPage))
    , documentLoadingPage(new Widget(_parent))
    , documentLoadingPageTitleLabel(new H6Label(documentLoadingPage))
    , documentLoadingPageBodyLabel(new Body1Label(documentLoadingPage))
    , notImplementedPage(new Widget(_parent))
    , notImplementedPageTitleLabel(new H6Label(notImplementedPage))
    , notImplementedPageBodyLabel(new Body1Label(notImplementedPage))
    , documentEditorPage(new Widget(_parent))
    , documentDrafts(new TabBar(documentEditorPage))
    , documentEditor(new StackWidget(documentEditorPage))
    , overlay(new Widget(_parent))
{
    defaultPage->setFocusPolicy(Qt::StrongFocus);
    defaultPageBodyLabel->setAlignment(Qt::AlignCenter);
    documentLoadingPage->setFocusPolicy(Qt::StrongFocus);
    documentLoadingPageBodyLabel->setAlignment(Qt::AlignCenter);
    notImplementedPage->setFocusPolicy(Qt::StrongFocus);
    notImplementedPageBodyLabel->setAlignment(Qt::AlignCenter);
    documentDrafts->hide();
    documentDrafts->setContextMenuPolicy(Qt::CustomContextMenu);
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
        QVBoxLayout* layout = new QVBoxLayout(documentLoadingPage);
        layout->setContentsMargins({});
        layout->setSpacing(0);
        layout->addStretch();
        layout->addWidget(documentLoadingPageTitleLabel, 0, Qt::AlignHCenter);
        QHBoxLayout* bodyLayout = new QHBoxLayout;
        bodyLayout->setContentsMargins({});
        bodyLayout->setSpacing(0);
        bodyLayout->addStretch();
        bodyLayout->addWidget(documentLoadingPageBodyLabel, 0, Qt::AlignHCenter);
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
        layout->addWidget(documentDrafts);
        layout->addWidget(documentEditor, 1);
    }

    documentDraftsHeightAnimation.setEasingCurve(QEasingCurve::OutQuad);
    documentDraftsHeightAnimation.setDuration(160);
}


// ****


ProjectView::ProjectView(QWidget* _parent)
    : StackWidget(_parent)
    , d(new Implementation(this))
{
    setFocusPolicy(Qt::StrongFocus);
    setAnimationType(AnimationType::FadeThrough);

    addWidget(d->defaultPage);
    addWidget(d->documentLoadingPage);
    addWidget(d->notImplementedPage);
    addWidget(d->documentEditorPage);

    showDefaultPage();

    connect(d->defaultPageAddItemButton, &Body1LinkLabel::clicked, this,
            &ProjectView::createNewItemPressed);
    connect(d->documentDrafts, &TabBar::currentIndexChanged, this, &ProjectView::showDraftPressed);
    connect(d->documentDrafts, &TabBar::customContextMenuRequested, this,
            [this](const QPoint _position) {
                emit showDraftContextMenuPressed(d->documentDrafts->tabAt(_position));
            });
    connect(&d->documentDraftsHeightAnimation, &QVariantAnimation::valueChanged, this,
            [this](const QVariant& _value) { d->documentDrafts->setFixedHeight(_value.toInt()); });
    connect(&d->documentDraftsHeightAnimation, &QVariantAnimation::finished, this, [this] {
        if (d->documentDraftsHeightAnimation.direction() == QVariantAnimation::Backward) {
            d->documentDrafts->hide();
        }
        d->documentDrafts->setMinimumHeight(0);
        d->documentDrafts->setMaximumHeight(QWIDGETSIZE_MAX);
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

void ProjectView::showDocumentLoadingPage()
{
    setCurrentWidget(d->documentLoadingPage);
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

void ProjectView::addEditor(QWidget* _widget)
{
    d->documentEditor->addWidget(_widget);
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

void ProjectView::setDocumentDrafts(const QVector<BusinessLayer::StructureModelItem*>& _drafts)
{
    if (d->documentDrafts->count() == 1 && _drafts.isEmpty()) {
        return;
    }

    //
    // Блокируем сигналы, чтобы менеджер не думал, что мы переключаемся тут между разными версиями
    //
    QSignalBlocker blocker(this);

    const auto lastActiveDraft = d->documentDrafts->currentTab();

    d->documentDrafts->removeAllTabs();
    d->documentDrafts->addTab(tr("Current draft"));
    for (const auto draft : _drafts) {
        d->documentDrafts->addTab(draft->name(),
                                  draft->isComparison()
                                      ? u8"\U000F1492"
                                      : (draft->isReadOnly() ? u8"\U000F033E" : u8"\U000F0765"),
                                  draft->color());
    }

    d->documentDrafts->setCurrentTab(lastActiveDraft);

    d->documentDraftsHeightAnimation.setStartValue(0);
    d->documentDraftsHeightAnimation.setEndValue(d->documentDrafts->sizeHint().height());
}

void ProjectView::setDraftsVisible(bool _visible)
{
    if (d->documentDraftsHeightAnimation.state() == QVariantAnimation::Running) {
        if ((d->documentDraftsHeightAnimation.direction() == QVariantAnimation::Forward && _visible)
            || (d->documentDraftsHeightAnimation.direction() == QVariantAnimation::Backward
                && !_visible)) {
            return;
        }
        d->documentDraftsHeightAnimation.stop();
    }

    if (d->documentDrafts->isVisible() == _visible) {
        return;
    }

    if (_visible) {
        d->documentDrafts->setFixedHeight(0);
        d->documentDrafts->show();
    }
    d->documentDraftsHeightAnimation.setDirection(_visible ? QVariantAnimation::Forward
                                                           : QVariantAnimation::Backward);
    d->documentDraftsHeightAnimation.start();
}

int ProjectView::currentDraft() const
{
    return d->documentDrafts->currentTab();
}

void ProjectView::setCurrentDraft(int _index)
{
    d->documentDrafts->setCurrentTab(_index);
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

    d->documentLoadingPageTitleLabel->setText(tr("Document content loading..."));
    d->documentLoadingPageBodyLabel->setText(
        tr("Please, wait a while and document editor will be activated."));

    d->notImplementedPageTitleLabel->setText(
        tr("Ooops... looks like editor of this document not implemented yet."));
    d->notImplementedPageBodyLabel->setText(
        tr("But don't worry, it will be here in one of the future updates!"));

    d->documentDrafts->setTabName(0, tr("Current draft"));
}

void ProjectView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    StackWidget::designSystemChangeEvent(_event);

    setBackgroundColor(DesignSystem::color().surface());

    d->defaultPage->setBackgroundColor(DesignSystem::color().surface());
    d->defaultPageBodyLabel->setContentsMargins(0, static_cast<int>(DesignSystem::layout().px16()),
                                                static_cast<int>(DesignSystem::layout().px4()), 0);
    for (auto label : std::vector<Widget*>{
             d->defaultPageTitleLabel,
             d->defaultPageBodyLabel,
         }) {
        label->setBackgroundColor(DesignSystem::color().surface());
        label->setTextColor(DesignSystem::color().onSurface());
    }
    d->defaultPageAddItemButton->setContentsMargins(
        0, static_cast<int>(DesignSystem::layout().px16()), 0, 0);
    d->defaultPageAddItemButton->setBackgroundColor(DesignSystem::color().surface());
    d->defaultPageAddItemButton->setTextColor(DesignSystem::color().accent());

    d->documentLoadingPage->setBackgroundColor(DesignSystem::color().surface());
    d->documentLoadingPageBodyLabel->setContentsMargins(
        0, static_cast<int>(DesignSystem::layout().px16()),
        static_cast<int>(DesignSystem::layout().px4()), 0);
    for (auto label : std::vector<Widget*>{
             d->documentLoadingPageTitleLabel,
             d->documentLoadingPageBodyLabel,
         }) {
        label->setBackgroundColor(DesignSystem::color().surface());
        label->setTextColor(DesignSystem::color().onSurface());
    }

    d->notImplementedPage->setBackgroundColor(DesignSystem::color().surface());
    d->notImplementedPageBodyLabel->setContentsMargins(
        0, static_cast<int>(DesignSystem::layout().px16()),
        static_cast<int>(DesignSystem::layout().px4()), 0);
    for (auto label : std::vector<Widget*>{
             d->notImplementedPageTitleLabel,
             d->notImplementedPageBodyLabel,
         }) {
        label->setBackgroundColor(DesignSystem::color().surface());
        label->setTextColor(DesignSystem::color().onSurface());
    }

    d->documentEditorPage->setBackgroundColor(DesignSystem::color().surface());
    d->documentDrafts->setBackgroundColor(ColorHelper::nearby(DesignSystem::color().background()));
    d->documentDrafts->setTextColor(DesignSystem::color().onBackground());
    d->documentEditor->setBackgroundColor(DesignSystem::color().surface());

    d->overlay->setBackgroundColor(backgroundColor());
    d->overlayOpacityAnimation.setEndValue(DesignSystem::focusBackgroundOpacity());
}

void ProjectView::setCurrentWidget(QWidget* _widget)
{
    StackWidget::setCurrentWidget(_widget);
}

} // namespace Ui
