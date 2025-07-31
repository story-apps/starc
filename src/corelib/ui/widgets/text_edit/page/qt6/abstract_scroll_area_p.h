#pragma once

#include "abstract_scroll_area.h"
#include "frame_p.h"

#include <QtGui/private/qgridlayoutengine_p.h>
#include <QtWidgets/private/qtwidgetsglobal_p.h>


class PageTextEditScrollBar;
class AbstractScrollAreaScrollBarContainer;
class Q_AUTOTEST_EXPORT AbstractScrollAreaPrivate : public FramePrivate
{
    Q_DECLARE_PUBLIC(AbstractScrollArea)

public:
    AbstractScrollAreaPrivate();
    ~AbstractScrollAreaPrivate();

    void replaceScrollBar(PageTextEditScrollBar* scrollBar, Qt::Orientation orientation);
    QHVContainer<AbstractScrollAreaScrollBarContainer*> scrollBarContainers;
    PageTextEditScrollBar *hbar, *vbar;
    Qt::ScrollBarPolicy vbarpolicy, hbarpolicy;
    bool shownOnce;
    bool inResize;
    mutable QSize sizeHint;
    AbstractScrollArea::SizeAdjustPolicy sizeAdjustPolicy;
    QWidget* viewport;
    QWidget* cornerWidget;
    QRect cornerPaintingRect;
    int left, top, right, bottom; // viewport margin
    int xoffset, yoffset;
    QPoint overshoot;
    void init();
    void layoutChildren();
    void layoutChildren_helper(bool* needHorizontalScrollbar, bool* needVerticalScrollbar);
    virtual void scrollBarPolicyChanged(Qt::Orientation, Qt::ScrollBarPolicy)
    {
    }
    virtual bool canStartScrollingAt(const QPoint& startPos) const;
    void flashScrollBars();
    void setScrollBarTransient(PageTextEditScrollBar* scrollBar, bool transient);
    void _q_hslide(int);
    void _q_vslide(int);
    void _q_showOrHideScrollBars();
    virtual QPoint contentsOffset() const;
    inline bool viewportEvent(QEvent* event)
    {
        return q_func()->viewportEvent(event);
    }
    QScopedPointer<QObject> viewportFilter;
};
class AbstractScrollAreaFilter : public QObject
{
    Q_OBJECT
public:
    AbstractScrollAreaFilter(AbstractScrollAreaPrivate* p)
        : d(p)
    {
        setObjectName(QLatin1StringView("qt_abstractscrollarea_filter"));
    }
    bool eventFilter(QObject* o, QEvent* e) override
    {
        return (o == d->viewport ? d->viewportEvent(e) : false);
    }

private:
    AbstractScrollAreaPrivate* d;
};
class QBoxLayout;
class AbstractScrollAreaScrollBarContainer : public QWidget
{
public:
    enum LogicalPosition { LogicalLeft = 1, LogicalRight = 2 };
    AbstractScrollAreaScrollBarContainer(Qt::Orientation orientation, QWidget* parent);
    void addWidget(QWidget* widget, LogicalPosition position);
    QWidgetList widgets(LogicalPosition position);
    void removeWidget(QWidget* widget);
    PageTextEditScrollBar* scrollBar;
    QBoxLayout* layout;

private:
    int scrollBarLayoutIndex() const;
    Qt::Orientation orientation;
};
