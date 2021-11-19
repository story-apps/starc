#include "application_view.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/shadow/shadow.h>
#include <ui/widgets/splitter/splitter.h>
#include <ui/widgets/stack_widget/stack_widget.h>
#include <ui/widgets/task_bar/task_bar.h>

#include <QAction>
#include <QCloseEvent>
#include <QPainter>
#include <QToolTip>
#include <QVBoxLayout>


namespace Ui {

namespace {
const QString kSplitterState = "splitter/state";
const QString kViewGeometry = "view/geometry";
const QVector<int> kDefaultSizes = { 3, 7 };
} // namespace

class ApplicationView::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    Widget* navigationWidget = nullptr;
    StackWidget* toolBar = nullptr;
    StackWidget* navigator = nullptr;
    StackWidget* view = nullptr;

    QByteArray lastSplitterState;
    Splitter* splitter = nullptr;

    IconsBigLabel* turnOffFullScreenIcon = nullptr;
    Widget* accountBar = nullptr;
};

ApplicationView::Implementation::Implementation(QWidget* _parent)
    : navigationWidget(new Widget(_parent))
    , toolBar(new StackWidget(_parent))
    , navigator(new StackWidget(_parent))
    , view(new StackWidget(_parent))
    , splitter(new Splitter(_parent))
    , turnOffFullScreenIcon(new IconsBigLabel(_parent))
{
    new Shadow(view);

    turnOffFullScreenIcon->setIcon(u8"\U000F0294");
    turnOffFullScreenIcon->hide();
}


// ****


ApplicationView::ApplicationView(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation(this))
{
    d->view->installEventFilter(this);

    QVBoxLayout* navigationLayout = new QVBoxLayout(d->navigationWidget);
    navigationLayout->setContentsMargins({});
    navigationLayout->setSpacing(0);
    navigationLayout->addWidget(d->toolBar);
    navigationLayout->addWidget(d->navigator);

    d->splitter->setWidgets(d->navigationWidget, d->view);
    d->splitter->setSizes(kDefaultSizes);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(d->splitter);


    connect(d->turnOffFullScreenIcon, &IconsBigLabel::clicked, this,
            &ApplicationView::turnOffFullScreenRequested);


    updateTranslations();
    designSystemChangeEvent(nullptr);
}

ApplicationView::~ApplicationView() = default;

QVariantMap ApplicationView::saveState() const
{
    QVariantMap state;
    state[kSplitterState] = d->splitter->saveState();
    state[kViewGeometry] = saveGeometry();
    return state;
}

void ApplicationView::restoreState(const QVariantMap& _state)
{
    if (_state.contains(kSplitterState)) {
        d->splitter->restoreState(_state[kSplitterState].toByteArray());
    }
    if (_state.contains(kViewGeometry)) {
        restoreGeometry(_state[kViewGeometry].toByteArray());
    }

    //
    // Если пользователь закрыл приложение в полноэкранном состоянии, то при старте выходим из него
    //
    if (isFullScreen()) {
        toggleFullScreen(true);
        showMaximized();
    }
}

void ApplicationView::showContent(QWidget* _toolbar, QWidget* _navigator, QWidget* _view)
{
    d->toolBar->setCurrentWidget(_toolbar);
    d->navigator->setCurrentWidget(_navigator);
    d->view->setCurrentWidget(_view);
}

void ApplicationView::toggleFullScreen(bool _isFullScreen)
{
    if (!_isFullScreen) {
        d->lastSplitterState = d->splitter->saveState();
        d->turnOffFullScreenIcon->show();
    }

    d->navigationWidget->setVisible(_isFullScreen);

    if (_isFullScreen) {
        d->turnOffFullScreenIcon->hide();
        if (!d->lastSplitterState.isEmpty()) {
            d->splitter->restoreState(d->lastSplitterState);
        } else {
            d->splitter->setSizes(kDefaultSizes);
        }
    }
}

void ApplicationView::setAccountBar(Widget* _accountBar)
{
    d->accountBar = _accountBar;
}

bool ApplicationView::eventFilter(QObject* _target, QEvent* _event)
{
    if (d->accountBar != nullptr && _target == d->view && _event->type() == QEvent::Resize) {
        QResizeEvent* event = static_cast<QResizeEvent*>(_event);
        d->accountBar->move(d->view->mapTo(this, QPoint())
                            + QPointF(event->size().width() - d->accountBar->width()
                                          - Ui::DesignSystem::layout().px24(),
                                      Ui::DesignSystem::layout().px24())
                                  .toPoint());
    }

    return Widget::eventFilter(_target, _event);
}

void ApplicationView::closeEvent(QCloseEvent* _event)
{
    //
    // Вместо реального закрытия формы сигнализируем об этом намерении
    //

    _event->ignore();
    emit closeRequested();
}

void ApplicationView::updateTranslations()
{
    d->turnOffFullScreenIcon->setToolTip(tr("Turn off full screen"));
}

void ApplicationView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Q_UNUSED(_event)

    QPalette toolTipPalette;
    toolTipPalette.setColor(QPalette::ToolTipBase, Ui::DesignSystem::color().onSurface());
    toolTipPalette.setColor(QPalette::ToolTipText, Ui::DesignSystem::color().surface());
    QToolTip::setPalette(toolTipPalette);
    QToolTip::setFont(Ui::DesignSystem::font().subtitle2());

    d->navigationWidget->setBackgroundColor(DesignSystem::color().primary());

    d->toolBar->setBackgroundColor(DesignSystem::color().primary());
    d->toolBar->setFixedHeight(static_cast<int>(DesignSystem::appBar().heightRegular()));

    d->navigator->setBackgroundColor(DesignSystem::color().primary());

    d->view->setBackgroundColor(DesignSystem::color().surface());

    d->turnOffFullScreenIcon->raise();
    d->turnOffFullScreenIcon->setTextColor(Ui::DesignSystem::color().onSurface());
    d->turnOffFullScreenIcon->setBackgroundColor(Qt::transparent);
    d->turnOffFullScreenIcon->resize(d->turnOffFullScreenIcon->sizeHint());
    d->turnOffFullScreenIcon->move(Ui::DesignSystem::layout().px24(),
                                   Ui::DesignSystem::layout().px24());

    if (d->accountBar != nullptr) {
        d->accountBar->resize(d->accountBar->sizeHint());
        d->accountBar->move(
            QPointF(size().width() - d->accountBar->width() - Ui::DesignSystem::layout().px24(),
                    Ui::DesignSystem::layout().px24())
                .toPoint());
        d->accountBar->setBackgroundColor(Ui::DesignSystem::color().primary());
        d->accountBar->setTextColor(Ui::DesignSystem::color().onPrimary());
    }

    TaskBar::registerTaskBar(this, Ui::DesignSystem::color().primary(),
                             Ui::DesignSystem::color().onPrimary(),
                             Ui::DesignSystem::color().secondary());
}

} // namespace Ui
