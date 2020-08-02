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

}

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


    bool isFixed = false;
    QVector<Tab> tabs;
    int currentTabIndex = 0;
    struct ScrollState {
        qreal maximum = 0.0;
        qreal current = 0.0;
    } scrollState;
    QVariantAnimation scrollingAnimation;
    QVariantAnimation decorationAnimation;
};

TabBar::Implementation::Implementation()
{
    scrollingAnimation.setDuration(160);
    scrollingAnimation.setEasingCurve(QEasingCurve::OutQuad);
    decorationAnimation.setDuration(160);
    decorationAnimation.setEasingCurve(QEasingCurve::OutQuad);
}

qreal TabBar::Implementation::tabWidthHint(const Tab& _tab) const
{
    return qMax(Ui::DesignSystem::tab().minimumWidth(),
                QFontMetricsF(Ui::DesignSystem::font().button()).boundingRect(_tab.name).width()
                + Ui::DesignSystem::tab().margins().top()
                + Ui::DesignSystem::tab().margins().top());
}

qreal TabBar::Implementation::tabWidthHintForFixed(int _width) const
{
    const int visibleTabs = std::count_if(tabs.begin(), tabs.end(),
                                          [] (const Tab& _tab) { return _tab.visible; });
    return _width / visibleTabs;
}

qreal TabBar::Implementation::tabHeightHint(const Tab& _tab) const
{
    return !_tab.name.isEmpty() && !_tab.icon.isNull()
            ? Ui::DesignSystem::tab().heightWithTextAndIcon()
            : !_tab.name.isEmpty()
              ? Ui::DesignSystem::tab().heightWithText()
              : Ui::DesignSystem::tab().heightWithIcon();
}

QSizeF TabBar::Implementation::tabSizeHint(const Tab& _tab) const
{
    return QSizeF(tabWidthHint(_tab), tabHeightHint(_tab));
}

qreal TabBar::Implementation::contentWidth() const
{
    qreal contentWidth;
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


// ****


TabBar::TabBar(QWidget* _parent)
    : Widget(_parent),
      d(new Implementation)
{
    setFocusPolicy(Qt::StrongFocus);

    ScrollerHelper::addScroller(this);

    //
    // Анимируем прокручивание вьюпорта при смене текущей вкладки
    //
    connect(&d->scrollingAnimation, &QVariantAnimation::valueChanged, this, [this] (const QVariant& _value) {
        const qreal scrollValue = _value.toReal();
        if (scrollValue < 0.0) {
            d->scrollState.current = 0.0;
        } else if (scrollValue > d->scrollState.maximum) {
            d->scrollState.current = d->scrollState.maximum;
        } else {
            d->scrollState.current = scrollValue;
        }
        update();
    });

    //
    // При обновлении положения декорации обновим отрисовку
    //
    connect(&d->decorationAnimation, &QVariantAnimation::valueChanged, this, [this] { update(); });
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
    d->tabs.append({_tabName, QPixmap(), true});

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
    if (d->currentTabIndex == _tabIndex
        && _visible == false) {
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
    if (_visible == true
        && d->tabs.value(d->currentTabIndex).visible == false) {
        setCurrentTab(_tabIndex);
    }

    updateGeometry();
    update();
}

void TabBar::setCurrentTab(int _index)
{
    if (0 > _index || _index >= d->tabs.size()
        || d->currentTabIndex == _index) {
        return;
    }

    //
    // Обновим текущий индекс вкладки
    //
    const int previousIndex = d->currentTabIndex;
    d->currentTabIndex = _index;

    //
    // Если вкладки не растянуты на всю ширину, пробуем сместить выбранную вкладку в центр
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
        // ... таб должен быть расположен в центре экрана
        //
        const qreal targetTabCenter = width() / 2.0;
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
    d->decorationAnimation.start();

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
        const qreal tabWidth = d->isFixed
                               ? d->tabWidthHintForFixed(width())
                               : d->tabWidthHint(tab);
        const QRectF tabBoundingRect = QRectF(QPointF(tabX, 0.0), QSizeF(tabWidth, height()));

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
        const QRectF tabTextRect = QRectF(QPointF(tabBoundingRect.x(), tabBoundingRect.y() + heightDelta),
                                          tabBoundingRect.adjusted(0, 0, 0, -heightDelta).size());
        painter.drawText(tabTextRect, Qt::AlignCenter, tab.name);
        painter.setOpacity(1.0);

        //
        // Подчёркивание
        //
        if (isTabCurrent) {
            const QRectF decorationRect = QRectF(QPointF(tabBoundingRect.left(),
                                                         tabBoundingRect.bottom()
                                                         - Ui::DesignSystem::tabBar().underlineHeight()),
                                                 tabBoundingRect.bottomRight());

            if (d->decorationAnimation.state() != QAbstractAnimation::Running) {
                d->decorationAnimation.setStartValue(decorationRect);
            } else {
                d->decorationAnimation.setEndValue(decorationRect);
            }

            const QRectF currentDecorationRect
                    = d->decorationAnimation.state() == QAbstractAnimation::Running
                      && d->decorationAnimation.currentValue().isValid()
                        ? d->decorationAnimation.currentValue().toRectF()
                        : decorationRect;
            painter.fillRect(currentDecorationRect, painter.pen().color());

        }

        tabX += tabBoundingRect.width();
    }

    painter.setOpacity(Ui::DesignSystem::disabledTextOpacity());
    painter.setPen(Ui::DesignSystem::color().shadow());
    painter.drawLine(rect().bottomLeft(), rect().bottomRight());
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
