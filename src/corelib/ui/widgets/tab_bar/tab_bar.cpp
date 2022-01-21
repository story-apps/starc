#include "tab_bar.h"

#include <ui/design_system/design_system.h>
#include <utils/helpers/scroller_helper.h>

#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QResizeEvent>
#include <QTimeLine>
#include <QVariantAnimation>

namespace {

struct Tab {
    QString name;
    QPixmap icon;
    bool visible;
};

} // namespace

class TabBar::Implementation
{
public:
    Implementation();

    /**
     * @brief Иделаьная ширина вкладки
     */
    qreal tabWidthHint(const Tab& _tab) const;
    qreal tabWidthHintForFixed(int _width) const;

    /**
     * @brief Идеальная высота вкладки
     */
    qreal tabHeightHint(const Tab& _tab) const;

    /**
     * @brief Идеальный размер вкладки
     */
    QSizeF tabSizeHint(const Tab& _tab) const;

    /**
     * @brief Ширина контента
     */
    qreal contentWidth() const;

    /**
     * @brief Анимировать клик
     */
    void animateClick();


    bool isFixed = false;
    QVector<Tab> tabs;
    int currentTabIndex = 0;
    int hoverTabIndex = -1;
    struct ScrollState {
        qreal maximum = 0.0;
        qreal current = 0.0;
    } scrollState;
    QVariantAnimation scrollingAnimation;

    /**
     * @brief Декорация подчёркивания активной вкладки
     */
    QVariantAnimation activeTabIndicatorAnimation;

    /**
     * @brief  Декорации таба при клике
     */
    QPointF decorationCenterPosition;
    QVariantAnimation decorationRadiusAnimation;
    QVariantAnimation decorationOpacityAnimation;
};

TabBar::Implementation::Implementation()
{
    scrollingAnimation.setDuration(160);
    scrollingAnimation.setEasingCurve(QEasingCurve::OutQuad);
    activeTabIndicatorAnimation.setDuration(160);
    activeTabIndicatorAnimation.setEasingCurve(QEasingCurve::OutQuad);

    decorationRadiusAnimation.setEasingCurve(QEasingCurve::InQuad);
    decorationRadiusAnimation.setStartValue(1.0);
    decorationRadiusAnimation.setDuration(240);

    decorationOpacityAnimation.setEasingCurve(QEasingCurve::InQuad);
    decorationOpacityAnimation.setStartValue(0.5);
    decorationOpacityAnimation.setEndValue(0.0);
    decorationOpacityAnimation.setDuration(420);
}

qreal TabBar::Implementation::tabWidthHint(const Tab& _tab) const
{
    return qMax(Ui::DesignSystem::tab().minimumWidth(),
                QFontMetricsF(Ui::DesignSystem::font().button()).boundingRect(_tab.name).width()
                    + Ui::DesignSystem::tab().margins().left()
                    + Ui::DesignSystem::tab().margins().right());
}

qreal TabBar::Implementation::tabWidthHintForFixed(int _width) const
{
    const int visibleTabs
        = std::count_if(tabs.begin(), tabs.end(), [](const Tab& _tab) { return _tab.visible; });
    Q_ASSERT(visibleTabs != 0);
    return _width / visibleTabs;
}

qreal TabBar::Implementation::tabHeightHint(const Tab& _tab) const
{
    return !_tab.name.isEmpty() && !_tab.icon.isNull()
        ? Ui::DesignSystem::tab().heightWithTextAndIcon()
        : !_tab.name.isEmpty() ? Ui::DesignSystem::tab().heightWithText()
                               : Ui::DesignSystem::tab().heightWithIcon();
}

QSizeF TabBar::Implementation::tabSizeHint(const Tab& _tab) const
{
    return QSizeF(tabWidthHint(_tab), tabHeightHint(_tab));
}

qreal TabBar::Implementation::contentWidth() const
{
    qreal contentWidth = 0.0;
    for (const Tab& tab : tabs) {
        if (!tab.visible) {
            continue;
        }
        contentWidth += tabWidthHint(tab);
    }
    if (!isFixed) {
        contentWidth += Ui::DesignSystem::tabBar().scrollableLeftMargin();
    }

    return contentWidth;
}

void TabBar::Implementation::animateClick()
{
    decorationOpacityAnimation.setCurrentTime(0);
    decorationRadiusAnimation.start();
    decorationOpacityAnimation.start();
}


// ****


TabBar::TabBar(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation)
{
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);

    ScrollerHelper::addScroller(this);

    //
    // Анимируем прокручивание вьюпорта при смене текущей вкладки
    //
    connect(&d->scrollingAnimation, &QVariantAnimation::valueChanged, this,
            [this](const QVariant& _value) {
                const qreal scrollValue = _value.toReal();
                if (scrollValue < 0.0) {
                    d->scrollState.current = 0.0;
                } else if (scrollValue > d->scrollState.maximum) {
                    d->scrollState.current = d->scrollState.maximum;
                } else {
                    d->scrollState.current = scrollValue;

                    //
                    // дополнительно коррктируем позицию центра декорации, куда кликнул пользователь
                    //
                    d->decorationCenterPosition
                        += QPointF(d->scrollState.current - scrollValue, 0.0);
                }
                update();
            });

    //
    // При обновлении положения декорации обновим отрисовку
    //
    connect(&d->activeTabIndicatorAnimation, &QVariantAnimation::valueChanged, this,
            qOverload<>(&TabBar::update));
    connect(&d->decorationRadiusAnimation, &QVariantAnimation::valueChanged, this,
            qOverload<>(&TabBar::update));
    connect(&d->decorationOpacityAnimation, &QVariantAnimation::valueChanged, this,
            qOverload<>(&TabBar::update));
}

TabBar::~TabBar() = default;

void TabBar::setFixed(bool fixed)
{
    if (d->isFixed == fixed) {
        return;
    }

    d->isFixed = fixed;

    updateGeometry();
    update();
}

void TabBar::addTab(const QString& _tabName)
{
    d->tabs.append({ _tabName, QPixmap(), true });

    updateGeometry();
    update();
}

void TabBar::setTabName(int _tabIndex, const QString& _tabName)
{
    if (0 > _tabIndex || _tabIndex >= d->tabs.size()) {
        return;
    }

    if (d->tabs[_tabIndex].name == _tabName) {
        return;
    }

    d->tabs[_tabIndex].name = _tabName;

    updateGeometry();
    update();
}

void TabBar::setTabVisible(int _tabIndex, bool _visible)
{
    if (0 > _tabIndex || _tabIndex >= d->tabs.size()) {
        return;
    }

    if (d->tabs[_tabIndex].visible == _visible) {
        return;
    }

    d->tabs[_tabIndex].visible = _visible;
    //
    // Если скрывается выделенная вкладка, ищем, какую можно сделать текущей
    //
    if (d->currentTabIndex == _tabIndex && _visible == false) {
        //
        // Спарва ищем назад
        //
        int visibleTab = d->currentTabIndex - 1;
        while (visibleTab >= 0) {
            if (d->tabs[visibleTab].visible == true) {
                setCurrentTab(visibleTab);
                break;
            }

            --visibleTab;
        }

        //
        // Если там не нашлось, ищем вперёд
        //
        if (visibleTab < 0) {
            visibleTab = d->currentTabIndex + 1;
            while (visibleTab < d->tabs.size()) {
                if (d->tabs[visibleTab].visible == true) {
                    setCurrentTab(visibleTab);
                    break;
                }

                ++visibleTab;
            }
        }
    }

    //
    // Если все вкладки были скрыты, а эта становится видимой, то сделаем её текущей
    //
    if (_visible == true && d->tabs.value(d->currentTabIndex).visible == false) {
        setCurrentTab(_tabIndex);
    }

    updateGeometry();
    update();
}

void TabBar::setCurrentTab(int _index)
{
    if (0 > _index || _index >= d->tabs.size() || d->currentTabIndex == _index) {
        return;
    }

    //
    // Обновим текущий индекс вкладки
    //
    const int previousIndex = d->currentTabIndex;
    d->currentTabIndex = _index;

    //
    // Если вкладки не растянуты на всю ширину, смещаем выбранную вкладку, чтобы она была видна
    //
    if (!d->isFixed) {
        //
        // Определим текущее положение центра таба
        //
        qreal tabX = Ui::DesignSystem::tabBar().scrollableLeftMargin();
        tabX -= d->scrollState.current;
        for (int tabIndex = 0; tabIndex < _index; ++tabIndex) {
            const Tab& tab = d->tabs.at(tabIndex);
            if (!tab.visible) {
                continue;
            }
            const qreal tabWidth = d->tabWidthHint(tab);
            tabX += tabWidth;
        }
        const qreal tabWidth = d->tabWidthHint(d->tabs.at(_index));
        const qreal currentTabCenter = tabX + tabWidth / 2.0;
        //
        // ... таб должен быть расположен внутри видимой области
        //
        qreal targetTabCenter = 0.0;
        if (tabX < Ui::DesignSystem::layout().px48()) {
            targetTabCenter = Ui::DesignSystem::layout().px48() + tabWidth / 2.0;
        } else if (tabX + tabWidth > width() - Ui::DesignSystem::layout().px48()) {
            targetTabCenter = width() - Ui::DesignSystem::layout().px48() - tabWidth / 2.0;
        } else {
            targetTabCenter = currentTabCenter;
        }
        //
        // ... рассчитаем на сколько нужно сместить вьюпорт
        //
        const qreal delta = currentTabCenter - targetTabCenter;
        d->scrollingAnimation.setStartValue(d->scrollState.current);
        d->scrollingAnimation.setEndValue(d->scrollState.current + delta);
        //
        // ... и запустим анимацию смещения
        //
        d->scrollingAnimation.start();
    }

    //
    // Запустим анимацию декорации
    //
    if (isVisible()) {
        d->activeTabIndicatorAnimation.start();
    } else {
        d->activeTabIndicatorAnimation.setCurrentTime(d->activeTabIndicatorAnimation.duration());
    }

    //
    // Надо бы перерисоваться
    //
    update();

    emit currentIndexChanged(d->currentTabIndex, previousIndex);
}

int TabBar::currentTab() const
{
    return d->currentTabIndex;
}

QSize TabBar::minimumSizeHint() const
{
    QSizeF sizeHint;
    for (const Tab& tab : d->tabs) {
        if (!tab.visible) {
            continue;
        }

        const QSizeF tabSizeHint = d->tabSizeHint(tab);
        if (sizeHint.height() < tabSizeHint.height()) {
            sizeHint.setHeight(tabSizeHint.height());
        }
        if (sizeHint.width() < tabSizeHint.width()) {
            sizeHint.setWidth(tabSizeHint.width());
        }
    }

    if (!d->isFixed) {
        sizeHint.setWidth(sizeHint.width() + Ui::DesignSystem::tabBar().scrollableLeftMargin());
    }

    return sizeHint.toSize();
}

QSize TabBar::sizeHint() const
{
    return minimumSizeHint();
}

bool TabBar::event(QEvent* _event)
{
    switch (static_cast<int>(_event->type())) {
    case QEvent::Scroll: {
        QScrollEvent* event = static_cast<QScrollEvent*>(_event);
        d->scrollState.current = event->contentPos().x();
        update();
        event->accept();
        return true;
    }

    case QEvent::ScrollPrepare: {
        QScrollPrepareEvent* event = static_cast<QScrollPrepareEvent*>(_event);
        event->setViewportSize(QSizeF(d->contentWidth(), height()));
        event->setContentPosRange(QRectF(0, 0, d->scrollState.maximum, 0));
        event->setContentPos(QPointF(d->scrollState.current, 0));
        event->accept();
        return true;
    }

    default: {
        return QWidget::event(_event);
    }
    }
}

void TabBar::resizeEvent(QResizeEvent* _event)
{
    QWidget::resizeEvent(_event);

    d->scrollState.maximum = std::max(d->contentWidth() - _event->size().width(), 0.0);
    d->scrollState.current = std::min(d->scrollState.current, d->scrollState.maximum);
}

void TabBar::paintEvent(QPaintEvent* _event)
{
    Q_UNUSED(_event);

    QPainter painter(this);
    painter.setFont(Ui::DesignSystem::font().button());

    //
    // Заливаем фон
    //
    painter.fillRect(rect(), backgroundColor());

    //
    // Рисуем вкладки
    //
    qreal tabX = d->isFixed ? 0 : Ui::DesignSystem::tabBar().scrollableLeftMargin();
    tabX -= d->scrollState.current;
    for (int tabIndex = 0; tabIndex < d->tabs.size(); ++tabIndex) {
        //
        // Определим таб и является ли он текущим
        //
        const Tab& tab = d->tabs.at(tabIndex);
        if (!tab.visible) {
            continue;
        }
        const bool isTabCurrent = tabIndex == d->currentTabIndex;

        //
        // Определим область для отрисовки таба
        //
        const qreal tabWidth = d->isFixed ? d->tabWidthHintForFixed(width()) : d->tabWidthHint(tab);
        const QRectF tabBoundingRect = QRectF(QPointF(tabX, 0.0), QSizeF(tabWidth, height()));

        //
        // Ховер
        //
        if (isTabCurrent && hasFocus()) {
            painter.setOpacity(Ui::DesignSystem::focusBackgroundOpacity());
            painter.fillRect(tabBoundingRect, Ui::DesignSystem::color().secondary());
        } else if (underMouse() && tabBoundingRect.contains(mapFromGlobal(QCursor::pos()))) {
            painter.setOpacity(Ui::DesignSystem::hoverBackgroundOpacity());
            painter.fillRect(tabBoundingRect, Ui::DesignSystem::color().secondary());
        }

        //
        // Декорация
        //
        if (tabBoundingRect.contains(d->decorationCenterPosition)
            && (d->decorationRadiusAnimation.state() == QVariantAnimation::Running
                || d->decorationOpacityAnimation.state() == QVariantAnimation::Running)) {
            painter.setClipRect(tabBoundingRect);
            painter.setPen(Qt::NoPen);
            painter.setBrush(Ui::DesignSystem::color().secondary());
            painter.setOpacity(d->decorationOpacityAnimation.currentValue().toReal());
            painter.drawEllipse(d->decorationCenterPosition,
                                d->decorationRadiusAnimation.currentValue().toReal(),
                                d->decorationRadiusAnimation.currentValue().toReal());
            painter.setOpacity(1.0);
            painter.setClipRect(QRect(), Qt::NoClip);
        }

        //
        // Иконка и текст
        //
        painter.setPen(isTabCurrent ? Ui::DesignSystem::color().secondary() : textColor());
        painter.setOpacity(isTabCurrent ? 1.0 : Ui::DesignSystem::inactiveTextOpacity());
        //
        // TODO: ... иконка
        //

        //
        // ... текст
        //
        const qreal heightDelta = height() - d->tabHeightHint(tab);
        const QRectF tabTextRect
            = QRectF(QPointF(tabBoundingRect.x(), tabBoundingRect.y() + heightDelta),
                     tabBoundingRect.adjusted(0, 0, 0, -heightDelta).size());
        painter.drawText(tabTextRect, Qt::AlignCenter, tab.name);
        painter.setOpacity(1.0);

        //
        // Подчёркивание
        //
        if (isTabCurrent) {
            const QRectF decorationRect = QRectF(
                QPointF(tabBoundingRect.left(),
                        tabBoundingRect.bottom() - Ui::DesignSystem::tabBar().underlineHeight()),
                tabBoundingRect.bottomRight());

            if (d->activeTabIndicatorAnimation.state() != QAbstractAnimation::Running) {
                d->activeTabIndicatorAnimation.setStartValue(decorationRect);
            } else {
                d->activeTabIndicatorAnimation.setEndValue(decorationRect);
            }

            const QRectF currentDecorationRect
                = d->activeTabIndicatorAnimation.state() == QAbstractAnimation::Running
                    && d->activeTabIndicatorAnimation.currentValue().isValid()
                ? d->activeTabIndicatorAnimation.currentValue().toRectF()
                : decorationRect;
            painter.fillRect(currentDecorationRect, painter.pen().color());
        }

        tabX += tabBoundingRect.width();
    }

    painter.setOpacity(Ui::DesignSystem::disabledTextOpacity());
    painter.setPen(Ui::DesignSystem::color().shadow());
    painter.drawLine(rect().bottomLeft(), rect().bottomRight());
}

#if (QT_VERSION > QT_VERSION_CHECK(6, 0, 0))
void TabBar::enterEvent(QEnterEvent* _event)
#else
void TabBar::enterEvent(QEvent* _event)
#endif
{
    Q_UNUSED(_event);
    update();
}

void TabBar::leaveEvent(QEvent* _event)
{
    Q_UNUSED(_event)
    d->hoverTabIndex = -1;
    update();
}

void TabBar::mouseMoveEvent(QMouseEvent* _event)
{
    qreal tabX = d->isFixed ? 0 : Ui::DesignSystem::tabBar().scrollableLeftMargin();
    tabX -= d->scrollState.current;
    for (int tabIndex = 0; tabIndex < d->tabs.size(); ++tabIndex) {
        const Tab& tab = d->tabs.at(tabIndex);
        if (!tab.visible) {
            continue;
        }

        const qreal tabWidth = d->isFixed ? width() / d->tabs.size() : d->tabWidthHint(tab);
        if (tabX < _event->pos().x() && _event->pos().x() < tabX + tabWidth) {
            if (d->hoverTabIndex != tabIndex) {
                d->hoverTabIndex = tabIndex;
                update();
            }
            return;
        }

        tabX += tabWidth;
    }

    if (d->hoverTabIndex != -1) {
        d->hoverTabIndex = -1;
        update();
    }
}

void TabBar::mousePressEvent(QMouseEvent* _event)
{
    d->decorationCenterPosition = _event->pos();
    d->decorationRadiusAnimation.setEndValue(
        static_cast<qreal>(d->tabSizeHint(d->tabs[d->currentTabIndex]).width()));
    d->animateClick();
}

void TabBar::mouseReleaseEvent(QMouseEvent* _event)
{
    qreal tabX = d->isFixed ? 0 : Ui::DesignSystem::tabBar().scrollableLeftMargin();
    tabX -= d->scrollState.current;
    for (int tabIndex = 0; tabIndex < d->tabs.size(); ++tabIndex) {
        const Tab& tab = d->tabs.at(tabIndex);
        if (!tab.visible) {
            continue;
        }

        const qreal tabWidth = d->isFixed ? width() / d->tabs.size() : d->tabWidthHint(tab);
        if (tabX < _event->pos().x() && _event->pos().x() < tabX + tabWidth) {
            setCurrentTab(tabIndex);
            return;
        }

        tabX += tabWidth;
    }
}
