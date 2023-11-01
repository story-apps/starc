#include "application_view.h"

#include <ui/design_system/design_system.h>
#include <ui/settings/theme_setup_view.h>
#include <ui/widgets/app_bar/app_bar.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/shadow/shadow.h>
#include <ui/widgets/splitter/splitter.h>
#include <ui/widgets/stack_widget/stack_widget.h>
#include <ui/widgets/task_bar/task_bar.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/platform_helper.h>
#include <utils/logging.h>

#include <QCloseEvent>
#include <QPainter>
#include <QParallelAnimationGroup>
#include <QScreen>
#include <QShortcut>
#include <QTimer>
#include <QToolTip>
#include <QVBoxLayout>
#include <QVariantAnimation>


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

    ThemeSetupView* themeSetupView = nullptr;

    IconsBigLabel* turnOffFullScreenIcon = nullptr;
};

ApplicationView::Implementation::Implementation(QWidget* _parent)
    : navigationWidget(new Widget(_parent))
    , toolBar(new StackWidget(_parent))
    , navigator(new StackWidget(_parent))
    , view(new StackWidget(_parent))
    , splitter(new Splitter(_parent))
    , themeSetupView(new ThemeSetupView(_parent))
    , turnOffFullScreenIcon(new IconsBigLabel(_parent))
{
    new Shadow(view);
    auto splitterTopShadow = new Shadow(Qt::TopEdge, splitter);
    splitterTopShadow->setVisibilityAnchor(themeSetupView);

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

    d->themeSetupView->hide();

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(d->themeSetupView);
    layout->addWidget(d->splitter, 1);


    connect(d->turnOffFullScreenIcon, &IconsBigLabel::clicked, this,
            &ApplicationView::turnOffFullScreenRequested);


#ifndef Q_OS_WIN
    //
    // Добавляем шорткат для выхода по Ctrl+Q
    //
    new QShortcut(QKeySequence("Ctrl+Q"), this, this, &ApplicationView::close,
                  Qt::ApplicationShortcut);
#endif
}

ApplicationView::~ApplicationView() = default;

ThemeSetupView* ApplicationView::themeSetupView() const
{
    return d->themeSetupView;
}

QWidget* ApplicationView::view() const
{
    return d->view;
}

void ApplicationView::slideViewOut()
{
    const auto finalWidth = DesignSystem::layout().px(1200);
    auto navigatorWidthAnimation = new QVariantAnimation(this);
    navigatorWidthAnimation->setDuration(40);
    navigatorWidthAnimation->setEasingCurve(QEasingCurve::OutQuart);
    navigatorWidthAnimation->setStartValue(width());
    navigatorWidthAnimation->setEndValue(
        static_cast<int>(finalWidth / std::accumulate(kDefaultSizes.begin(), kDefaultSizes.end(), 0)
                         * kDefaultSizes.constFirst()));
    auto fullWidthAnimation = new QVariantAnimation(this);
    fullWidthAnimation->setDuration(260);
    fullWidthAnimation->setEasingCurve(QEasingCurve::OutQuad);
    fullWidthAnimation->setStartValue(width());
    fullWidthAnimation->setEndValue(static_cast<int>(finalWidth));
    connect(fullWidthAnimation, &QVariantAnimation::valueChanged, this,
            [this, navigatorWidthAnimation](const QVariant& _value) {
                const auto width = _value.toInt();
                resize(width, height());
                const auto navigatorWidth = navigatorWidthAnimation->currentValue().toInt();
                d->splitter->setSizes({ navigatorWidth, width - navigatorWidth });

                move(screen()->availableGeometry().center() - QPoint(width / 2, height() / 2));
            });
    auto animation = new QParallelAnimationGroup(this);
    animation->addAnimation(fullWidthAnimation);
    animation->addAnimation(navigatorWidthAnimation);
    connect(
        animation, &QParallelAnimationGroup::stateChanged, this,
        [this, fullWidthAnimation, navigatorWidthAnimation](QAbstractAnimation::State _newState) {
            if (_newState == QAbstractAnimation::Running) {
                d->view->setMinimumSize(fullWidthAnimation->endValue().toInt()
                                            - navigatorWidthAnimation->endValue().toInt(),
                                        height());
            } else if (_newState == QAbstractAnimation::Stopped) {
                d->view->setMinimumSize(0, 0);
            }
        });
    animation->start(QAbstractAnimation::DeleteWhenStopped);

    d->view->show();
}

void ApplicationView::setHideNavigationButtonAvailable(bool _available)
{
    d->splitter->setHidePanelButtonAvailable(_available);
}

QVariantMap ApplicationView::saveState() const
{
    QVariantMap state;
    state[kSplitterState] = d->splitter->saveState();
    state[kViewGeometry] = saveGeometry();
    return state;
}

void ApplicationView::restoreState(bool _onboaringPassed, const QVariantMap& _state)
{
    //
    // Если это первый запуск, то конфигурируем геометрию под онбординг
    //
    if (!_onboaringPassed || _state.isEmpty()) {
        d->splitter->setSizes({ 1, 0 });
        d->view->hide();
        resize(Ui::DesignSystem::layout().px(468), Ui::DesignSystem::layout().px(740));
        move(screen()->availableGeometry().center() - QPoint(width() / 2, height() / 2));
        return;
    }

    if (_state.contains(kSplitterState)) {
        d->splitter->restoreState(_state[kSplitterState].toByteArray());
    }
    if (_state.contains(kViewGeometry)) {
        restoreGeometry(_state[kViewGeometry].toByteArray());
    }

    //
    // Почему-то иногда состояние геометрии может аффектить на видимость вьюхи, поэтому тут
    // принудительно показываем её, чтобы не словить кейс, когда приложение запущено, а интерфейс не
    // отображается
    //
    if (!isVisible()) {
        setVisible(true);
    }

    //
    // Иногда бывает так, что приложение после того, как станет видимым устанавливает геометрию окна
    // невалидной, поэтому сделана данная проверка и расширение размера вьюхи
    //
    constexpr int minSize = 100;
    if (height() < minSize || width() < minSize) {
        resize(Ui::DesignSystem::layout().px(1200), Ui::DesignSystem::layout().px(740));
        move(screen()->availableGeometry().center() - QPoint(width() / 2, height() / 2));
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
    Log::debug("Show content: %1, %2, %3", _toolbar->metaObject()->className(),
               _navigator->metaObject()->className(), _view->metaObject()->className());

    d->toolBar->setCurrentWidget(_toolbar);
    d->navigator->setCurrentWidget(_navigator);
    d->view->setCurrentWidget(_view);

    //
    // Фокусируем представление, после того, как оно будет отображено пользователю
    //
    QTimer::singleShot(d->view->animationDuration() * 1.3, this, [this] { d->view->setFocus(); });
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

    Log::trace("Init design system for the application view");

    setBackgroundColor(Ui::DesignSystem::color().primary());

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

    Log::trace("Register task bar");
    TaskBar::registerTaskBar(this, Ui::DesignSystem::color().primary(),
                             Ui::DesignSystem::color().onPrimary(),
                             Ui::DesignSystem::color().accent());

    Log::trace("Init window title bar theme");
    PlatformHelper::setTitleBarTheme(
        this, ColorHelper::isColorLight(Ui::DesignSystem::color().background()));

    Log::trace("Application view design system successfully initialized");
}

} // namespace Ui
