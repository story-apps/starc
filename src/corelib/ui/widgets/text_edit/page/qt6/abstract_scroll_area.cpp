#include "abstract_scroll_area.h"

#include "abstract_scroll_area_p.h"
#include "page_text_edit_scroll_bar.h"
#include "page_text_edit_scroll_bar_p.h"
#include "qapplication.h"
#include "qboxlayout.h"
#include "qdebug.h"
#include "qevent.h"
#include "qheaderview.h"
#include "qmargins.h"
#include "qpainter.h"
#include "qstyle.h"
#include "qstyleoption.h"

#include <private/qapplication_p.h>

#include <QDebug>
#include <qwidget.h>
#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif
QT_BEGIN_NAMESPACE
using namespace Qt::StringLiterals;
/*!
    \class AbstractScrollArea
    \brief The AbstractScrollArea widget provides a scrolling area with
    on-demand scroll bars.
    \ingroup abstractwidgets
    \inmodule QtWidgets
    AbstractScrollArea is a low-level abstraction of a scrolling
    area. The area provides a central widget called the viewport, in
    which the contents of the area is to be scrolled (i.e, the
    visible parts of the contents are rendered in the viewport).
    Next to the viewport is a vertical scroll bar, and below is a
    horizontal scroll bar. When all of the area contents fits in the
    viewport, each scroll bar can be either visible or hidden
    depending on the scroll bar's Qt::ScrollBarPolicy. When a scroll
    bar is hidden, the viewport expands in order to cover all
    available space. When a scroll bar becomes visible again, the
    viewport shrinks in order to make room for the scroll bar.
    It is possible to reserve a margin area around the viewport, see
    setViewportMargins(). The feature is mostly used to place a
    QHeaderView widget above or beside the scrolling area. Subclasses
    of AbstractScrollArea should implement margins.
    When inheriting AbstractScrollArea, you need to do the
    following:
    \list
        \li Control the scroll bars by setting their
           range, value, page step, and tracking their
           movements.
        \li Draw the contents of the area in the viewport according
           to the values of the scroll bars.
        \li Handle events received by the viewport in
           viewportEvent() - notably resize events.
        \li Use \c{viewport->update()} to update the contents of the
          viewport instead of \l{QWidget::update()}{update()}
          as all painting operations take place on the viewport.
    \endlist
    With a scroll bar policy of Qt::ScrollBarAsNeeded (the default),
    AbstractScrollArea shows scroll bars when they provide a non-zero
    scrolling range, and hides them otherwise.
    The scroll bars and viewport should be updated whenever the viewport
    receives a resize event or the size of the contents changes.
    The viewport also needs to be updated when the scroll bars
    values change. The initial values of the scroll bars are often
    set when the area receives new contents.
    We give a simple example, in which we have implemented a scroll area
    that can scroll any QWidget. We make the widget a child of the
    viewport; this way, we do not have to calculate which part of
    the widget to draw but can simply move the widget with
    QWidget::move(). When the area contents or the viewport size
    changes, we do the following:
    \snippet myscrollarea/myscrollarea.cpp 1
    When the scroll bars change value, we need to update the widget
    position, i.e., find the part of the widget that is to be drawn in
    the viewport:
    \snippet myscrollarea/myscrollarea.cpp 0
    In order to track scroll bar movements, reimplement the virtual
    function scrollContentsBy(). In order to fine-tune scrolling
    behavior, connect to a scroll bar's
    QAbstractSlider::actionTriggered() signal and adjust the \l
    QAbstractSlider::sliderPosition as you wish.
    For convenience, AbstractScrollArea makes all viewport events
    available in the virtual viewportEvent() handler. QWidget's
    specialized handlers are remapped to viewport events in the cases
    where this makes sense. The remapped specialized handlers are:
    paintEvent(), mousePressEvent(), mouseReleaseEvent(),
    mouseDoubleClickEvent(), mouseMoveEvent(), wheelEvent(),
    dragEnterEvent(), dragMoveEvent(), dragLeaveEvent(), dropEvent(),
    contextMenuEvent(),  and resizeEvent().
    QScrollArea, which inherits AbstractScrollArea, provides smooth
    scrolling for any QWidget (i.e., the widget is scrolled pixel by
    pixel). You only need to subclass AbstractScrollArea if you need
    more specialized behavior. This is, for instance, true if the
    entire contents of the area is not suitable for being drawn on a
    QWidget or if you do not want smooth scrolling.
    \sa QScrollArea
*/
AbstractScrollAreaPrivate::AbstractScrollAreaPrivate()
    : hbar(nullptr)
    , vbar(nullptr)
    , vbarpolicy(Qt::ScrollBarAsNeeded)
    , hbarpolicy(Qt::ScrollBarAsNeeded)
    , shownOnce(false)
    , inResize(false)
    , sizeAdjustPolicy(AbstractScrollArea::AdjustIgnored)
    , viewport(nullptr)
    , cornerWidget(nullptr)
    , left(0)
    , top(0)
    , right(0)
    , bottom(0)
    , xoffset(0)
    , yoffset(0)
    , viewportFilter(nullptr)
{
}
AbstractScrollAreaPrivate::~AbstractScrollAreaPrivate()
{
}
AbstractScrollAreaScrollBarContainer::AbstractScrollAreaScrollBarContainer(
    Qt::Orientation orientation, QWidget* parent)
    : QWidget(parent)
    , scrollBar(new PageTextEditScrollBar(orientation, this))
    , layout(new QBoxLayout(orientation == Qt::Horizontal ? QBoxLayout::LeftToRight
                                                          : QBoxLayout::TopToBottom))
    , orientation(orientation)
{
    setLayout(layout);
    layout->setContentsMargins(QMargins());
    layout->setSpacing(0);
    layout->addWidget(scrollBar);
    layout->setSizeConstraint(QLayout::SetMaximumSize);
}
/*! \internal
    Adds a widget to the scroll bar container.
*/
void AbstractScrollAreaScrollBarContainer::addWidget(QWidget* widget, LogicalPosition position)
{
    QSizePolicy policy = widget->sizePolicy();
    if (orientation == Qt::Vertical)
        policy.setHorizontalPolicy(QSizePolicy::Ignored);
    else
        policy.setVerticalPolicy(QSizePolicy::Ignored);
    widget->setSizePolicy(policy);
    widget->setParent(this);
    const int insertIndex = (position & LogicalLeft) ? 0 : scrollBarLayoutIndex() + 1;
    layout->insertWidget(insertIndex, widget);
}
/*! \internal
    Returns a list of scroll-bar widgets for the given position. The scroll bar
    itself is not returned.
*/
QWidgetList AbstractScrollAreaScrollBarContainer::widgets(LogicalPosition position)
{
    QWidgetList list;
    const int scrollBarIndex = scrollBarLayoutIndex();
    if (position == LogicalLeft) {
        list.reserve(scrollBarIndex);
        for (int i = 0; i < scrollBarIndex; ++i)
            list.append(layout->itemAt(i)->widget());
    } else if (position == LogicalRight) {
        const int layoutItemCount = layout->count();
        list.reserve(layoutItemCount - (scrollBarIndex + 1));
        for (int i = scrollBarIndex + 1; i < layoutItemCount; ++i)
            list.append(layout->itemAt(i)->widget());
    }
    return list;
}
/*! \internal
    Returns the layout index for the scroll bar. This needs to be
    recalculated by a linear search for each use, since items in
    the layout can be removed at any time (i.e. when a widget is
    deleted or re-parented).
*/
int AbstractScrollAreaScrollBarContainer::scrollBarLayoutIndex() const
{
    const int layoutItemCount = layout->count();
    for (int i = 0; i < layoutItemCount; ++i) {
        if (qobject_cast<PageTextEditScrollBar*>(layout->itemAt(i)->widget()))
            return i;
    }
    return -1;
}
/*! \internal
 */
void AbstractScrollAreaPrivate::replaceScrollBar(PageTextEditScrollBar* scrollBar,
                                                 Qt::Orientation orientation)
{
    Q_Q(AbstractScrollArea);
    AbstractScrollAreaScrollBarContainer* container = scrollBarContainers[orientation];
    bool horizontal = (orientation == Qt::Horizontal);
    PageTextEditScrollBar* oldBar = horizontal ? hbar : vbar;
    if (horizontal)
        hbar = scrollBar;
    else
        vbar = scrollBar;
    scrollBar->setParent(container);
    container->scrollBar = scrollBar;
    container->layout->removeWidget(oldBar);
    container->layout->insertWidget(0, scrollBar);
    scrollBar->setVisible(oldBar->isVisibleTo(container));
    scrollBar->setInvertedAppearance(oldBar->invertedAppearance());
    scrollBar->setInvertedControls(oldBar->invertedControls());
    scrollBar->setRange(oldBar->minimum(), oldBar->maximum());
    scrollBar->setOrientation(oldBar->orientation());
    scrollBar->setPageStep(oldBar->pageStep());
    scrollBar->setSingleStep(oldBar->singleStep());
    scrollBar->d_func()->viewMayChangeSingleStep = oldBar->d_func()->viewMayChangeSingleStep;
    scrollBar->setSliderDown(oldBar->isSliderDown());
    scrollBar->setSliderPosition(oldBar->sliderPosition());
    scrollBar->setTracking(oldBar->hasTracking());
    scrollBar->setValue(oldBar->value());
    scrollBar->installEventFilter(q);
    oldBar->removeEventFilter(q);
    delete oldBar;
    QObject::connect(scrollBar, SIGNAL(valueChanged(int)), q,
                     horizontal ? SLOT(_q_hslide(int)) : SLOT(_q_vslide(int)));
    QObject::connect(scrollBar, SIGNAL(rangeChanged(int, int)),

                     q, SLOT(_q_showOrHideScrollBars()), Qt::QueuedConnection);
}
void AbstractScrollAreaPrivate::init()
{
    Q_Q(AbstractScrollArea);
    viewport = new QWidget(q);
    viewport->setObjectName("qt_scrollarea_viewport"_L1);
    viewport->setBackgroundRole(QPalette::Base);
    viewport->setAutoFillBackground(true);
    scrollBarContainers[Qt::Horizontal]
        = new AbstractScrollAreaScrollBarContainer(Qt::Horizontal, q);
    scrollBarContainers[Qt::Horizontal]->setObjectName("qt_scrollarea_hcontainer"_L1);
    hbar = scrollBarContainers[Qt::Horizontal]->scrollBar;
    hbar->setRange(0, 0);
    scrollBarContainers[Qt::Horizontal]->setVisible(false);
    hbar->installEventFilter(q);
    QObject::connect(hbar, SIGNAL(valueChanged(int)), q, SLOT(_q_hslide(int)));
    QObject::connect(hbar, SIGNAL(rangeChanged(int, int)), q, SLOT(_q_showOrHideScrollBars()),
                     Qt::QueuedConnection);
    scrollBarContainers[Qt::Vertical] = new AbstractScrollAreaScrollBarContainer(Qt::Vertical, q);
    scrollBarContainers[Qt::Vertical]->setObjectName("qt_scrollarea_vcontainer"_L1);
    vbar = scrollBarContainers[Qt::Vertical]->scrollBar;
    vbar->setRange(0, 0);
    scrollBarContainers[Qt::Vertical]->setVisible(false);
    vbar->installEventFilter(q);
    QObject::connect(vbar, SIGNAL(valueChanged(int)), q, SLOT(_q_vslide(int)));
    QObject::connect(vbar, SIGNAL(rangeChanged(int, int)), q, SLOT(_q_showOrHideScrollBars()),
                     Qt::QueuedConnection);
    viewportFilter.reset(new AbstractScrollAreaFilter(this));
    viewport->installEventFilter(viewportFilter.data());
    viewport->setFocusProxy(q);
    q->setFocusPolicy(Qt::StrongFocus);
    q->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    q->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    layoutChildren();
#ifndef Q_OS_MACOS
#ifndef QT_NO_GESTURES
    viewport->grabGesture(Qt::PanGesture);
#endif
#endif
}
void AbstractScrollAreaPrivate::layoutChildren()
{
    bool needH = false;
    bool needV = false;
    layoutChildren_helper(&needH, &needV);
    // Call a second time if one scrollbar was needed and not the other to
    // check if it needs to readjust accordingly
    if (needH != needV)
        layoutChildren_helper(&needH, &needV);
}
void AbstractScrollAreaPrivate::layoutChildren_helper(bool* needHorizontalScrollbar,
                                                      bool* needVerticalScrollbar)
{
    Q_Q(AbstractScrollArea);
    QStyleOptionSlider barOpt;
    hbar->initStyleOption(&barOpt);
    bool htransient = hbar->style()->styleHint(QStyle::SH_ScrollBar_Transient, &barOpt, hbar);
    bool needh = *needHorizontalScrollbar
        || ((hbarpolicy != Qt::ScrollBarAlwaysOff)
            && ((hbarpolicy == Qt::ScrollBarAlwaysOn && !htransient)
                || ((hbarpolicy == Qt::ScrollBarAsNeeded || htransient)
                    && hbar->minimum() < hbar->maximum() && !hbar->sizeHint().isEmpty())));
    const int hscrollOverlap
        = hbar->style()->pixelMetric(QStyle::PM_ScrollView_ScrollBarOverlap, &barOpt, hbar);
    vbar->initStyleOption(&barOpt);
    bool vtransient = vbar->style()->styleHint(QStyle::SH_ScrollBar_Transient, &barOpt, vbar);
    bool needv = *needVerticalScrollbar
        || ((vbarpolicy != Qt::ScrollBarAlwaysOff)
            && ((vbarpolicy == Qt::ScrollBarAlwaysOn && !vtransient)
                || ((vbarpolicy == Qt::ScrollBarAsNeeded || vtransient)
                    && vbar->minimum() < vbar->maximum() && !vbar->sizeHint().isEmpty())));
    const int vscrollOverlap
        = vbar->style()->pixelMetric(QStyle::PM_ScrollView_ScrollBarOverlap, &barOpt, vbar);
    QStyleOption opt(0);
    opt.initFrom(q);
    const int hsbExt = hbar->sizeHint().height();
    const int vsbExt = vbar->sizeHint().width();
    const QPoint extPoint(vsbExt, hsbExt);
    const QSize extSize(vsbExt, hsbExt);
    const QRect widgetRect = q->rect();
    const bool hasCornerWidget = (cornerWidget != nullptr);
    QPoint cornerOffset((needv && vscrollOverlap == 0) ? vsbExt : 0,
                        (needh && hscrollOverlap == 0) ? hsbExt : 0);
    QRect controlsRect;
    QRect viewportRect;
    // In FrameOnlyAroundContents mode the frame is drawn between the controls and
    // the viewport, else the frame rect is equal to the widget rect.
    if ((frameStyle != QFrame::NoFrame)
        && q->style()->styleHint(QStyle::SH_ScrollView_FrameOnlyAroundContents, &opt, q)) {
        controlsRect = widgetRect;
        const int spacing
            = q->style()->pixelMetric(QStyle::PM_ScrollView_ScrollBarSpacing, &opt, q);
        const QPoint cornerExtra(needv ? spacing + vscrollOverlap : 0,
                                 needh ? spacing + hscrollOverlap : 0);
        QRect frameRect = widgetRect;
        frameRect.adjust(0, 0, -cornerOffset.x() - cornerExtra.x(),
                         -cornerOffset.y() - cornerExtra.y());
        q->setFrameRect(QStyle::visualRect(opt.direction, opt.rect, frameRect));
        // The frame rect needs to be in logical coords, however we need to flip
        // the contentsRect back before passing it on to the viewportRect
        // since the viewportRect has its logical coords calculated later.
        viewportRect = QStyle::visualRect(opt.direction, opt.rect, q->contentsRect());
    } else {
        q->setFrameRect(QStyle::visualRect(opt.direction, opt.rect, widgetRect));
        controlsRect = q->contentsRect();
        viewportRect = QRect(controlsRect.topLeft(), controlsRect.bottomRight() - cornerOffset);
    }
    cornerOffset = QPoint(needv ? vsbExt : 0, needh ? hsbExt : 0);
    // If we have a corner widget and are only showing one scroll bar, we need to move it
    // to make room for the corner widget.
    if (hasCornerWidget && ((needv && vscrollOverlap == 0) || (needh && hscrollOverlap == 0)))
        cornerOffset = extPoint;
    // The corner point is where the scroll bar rects, the corner widget rect and the
    // viewport rect meets.
    const QPoint cornerPoint(controlsRect.bottomRight() + QPoint(1, 1) - cornerOffset);
    // Some styles paints the corner if both scorllbars are showing and there is
    // no corner widget.
    if (needv && needh && !hasCornerWidget && hscrollOverlap == 0 && vscrollOverlap == 0)
        cornerPaintingRect
            = QStyle::visualRect(opt.direction, opt.rect, QRect(cornerPoint, extSize));
    else
        cornerPaintingRect = QRect();
    // move the scrollbars away from top/left headers
    int vHeaderRight = 0;
    int hHeaderBottom = 0;
#if QT_CONFIG(itemviews)
    if ((vscrollOverlap > 0 && needv) || (hscrollOverlap > 0 && needh)) {
        const QList<QHeaderView*> headers = q->findChildren<QHeaderView*>();
        if (headers.size() <= 2) {
            for (const QHeaderView* header : headers) {
                const QRect geo = header->geometry();
                if (header->orientation() == Qt::Vertical && header->isVisible()
                    && QStyle::visualRect(opt.direction, opt.rect, geo).left()
                        <= opt.rect.width() / 2)
                    vHeaderRight = QStyle::visualRect(opt.direction, opt.rect, geo).right();
                else if (header->orientation() == Qt::Horizontal && header->isVisible()
                         && geo.top() <= q->frameWidth())
                    hHeaderBottom = geo.bottom();
            }
        }
    }
#endif // QT_CONFIG(itemviews)
    if (needh) {
        QRect horizontalScrollBarRect(QPoint(controlsRect.left() + vHeaderRight, cornerPoint.y()),
                                      QPoint(cornerPoint.x() - 1, controlsRect.bottom()));
        if (!hasCornerWidget && htransient)
            horizontalScrollBarRect.adjust(0, 0, cornerOffset.x(), 0);
        scrollBarContainers[Qt::Horizontal]->setGeometry(
            QStyle::visualRect(opt.direction, opt.rect, horizontalScrollBarRect));
        scrollBarContainers[Qt::Horizontal]->raise();
    }
    if (needv) {
        QRect verticalScrollBarRect(QPoint(cornerPoint.x(), controlsRect.top() + hHeaderBottom),
                                    QPoint(controlsRect.right(), cornerPoint.y() - 1));
        if (!hasCornerWidget && vtransient)
            verticalScrollBarRect.adjust(0, 0, 0, cornerOffset.y());
        scrollBarContainers[Qt::Vertical]->setGeometry(
            QStyle::visualRect(opt.direction, opt.rect, verticalScrollBarRect));
        scrollBarContainers[Qt::Vertical]->raise();
    }
    if (cornerWidget) {
        const QRect cornerWidgetRect(cornerPoint, controlsRect.bottomRight());
        cornerWidget->setGeometry(QStyle::visualRect(opt.direction, opt.rect, cornerWidgetRect));
    }
    scrollBarContainers[Qt::Horizontal]->setVisible(needh);
    scrollBarContainers[Qt::Vertical]->setVisible(needv);
    if (q->isRightToLeft())
        viewportRect.adjust(right, top, -left, -bottom);
    else
        viewportRect.adjust(left, top, -right, -bottom);
    viewportRect = QStyle::visualRect(opt.direction, opt.rect, viewportRect);
    viewportRect.translate(-overshoot);
    viewport->setGeometry(viewportRect); // resize the viewport last
    *needHorizontalScrollbar = needh;
    *needVerticalScrollbar = needv;
}
/*!
    \enum AbstractScrollArea::SizeAdjustPolicy
    \since 5.2
    This enum specifies how the size hint of the AbstractScrollArea should
    adjust when the size of the viewport changes.
    \value AdjustIgnored                 The scroll area will behave like before - and not do any
   adjust. \value AdjustToContents              The scroll area will always adjust to the viewport
    \value AdjustToContentsOnFirstShow   The scroll area will adjust to its viewport the first time
   it is shown.
*/
/*!
    \internal
    Creates a new AbstractScrollAreaPrivate, \a dd with the given \a parent.
*/
AbstractScrollArea::AbstractScrollArea(AbstractScrollAreaPrivate& dd, QWidget* parent)
    : Frame(dd, parent)
{
    Q_D(AbstractScrollArea);
    QT_TRY
    {
        d->init();
    }
    QT_CATCH(...)
    {
        d->viewportFilter.reset();
        QT_RETHROW;
    }
}
/*!
    Constructs a viewport.
    The \a parent argument is sent to the QWidget constructor.
*/
AbstractScrollArea::AbstractScrollArea(QWidget* parent)
    : Frame(*new AbstractScrollAreaPrivate, parent)
{
    Q_D(AbstractScrollArea);
    QT_TRY
    {
        d->init();
    }
    QT_CATCH(...)
    {
        d->viewportFilter.reset();
        QT_RETHROW;
    }
}
/*!
  Destroys the viewport.
 */
AbstractScrollArea::~AbstractScrollArea()
{
    Q_D(AbstractScrollArea);
    // reset it here, otherwise we'll have a dangling pointer in ~QWidget
    d->viewportFilter.reset();
}
/*!
  \since 4.2
  Sets the viewport to be the given \a widget.
  The AbstractScrollArea will take ownership of the given \a widget.
  If \a widget is \nullptr, AbstractScrollArea will assign a new QWidget
  instance for the viewport.
  \sa viewport()
*/
void AbstractScrollArea::setViewport(QWidget* widget)
{
    Q_D(AbstractScrollArea);
    if (widget != d->viewport) {
        QWidget* oldViewport = d->viewport;
        if (!widget)
            widget = new QWidget;
        d->viewport = widget;
        d->viewport->setParent(this);
        d->viewport->setFocusProxy(this);
        d->viewport->installEventFilter(d->viewportFilter.data());
#ifndef QT_NO_GESTURES
        d->viewport->grabGesture(Qt::PanGesture);
#endif
        d->layoutChildren();
#ifndef QT_NO_OPENGL
        QWidgetPrivate::get(d->viewport)->initializeViewportFramebuffer();
#endif
        if (isVisible())
            d->viewport->show();
        setupViewport(widget);
        delete oldViewport;
    }
}
/*!
    Returns the viewport widget.
    Use the QScrollArea::widget() function to retrieve the contents of
    the viewport widget.
    \sa QScrollArea::widget()
*/
QWidget* AbstractScrollArea::viewport() const
{
    Q_D(const AbstractScrollArea);
    return d->viewport;
}
/*!
Returns the size of the viewport as if the scroll bars had no valid
scrolling range.
*/
QSize AbstractScrollArea::maximumViewportSize() const
{
    Q_D(const AbstractScrollArea);
    int f = 2 * d->frameWidth;
    QSize max = size() - QSize(f + d->left + d->right, f + d->top + d->bottom);
    // Count the sizeHint of the bar only if it is displayed.
    if (d->vbarpolicy == Qt::ScrollBarAlwaysOn)
        max.rwidth() -= d->vbar->sizeHint().width();
    if (d->hbarpolicy == Qt::ScrollBarAlwaysOn)
        max.rheight() -= d->hbar->sizeHint().height();
    return max;
}
/*!
    \property AbstractScrollArea::verticalScrollBarPolicy
    \brief the policy for the vertical scroll bar
    The default policy is Qt::ScrollBarAsNeeded.
    \sa horizontalScrollBarPolicy
*/
Qt::ScrollBarPolicy AbstractScrollArea::verticalScrollBarPolicy() const
{
    Q_D(const AbstractScrollArea);
    return d->vbarpolicy;
}
void AbstractScrollArea::setVerticalScrollBarPolicy(Qt::ScrollBarPolicy policy)
{
    Q_D(AbstractScrollArea);
    const Qt::ScrollBarPolicy oldPolicy = d->vbarpolicy;
    d->vbarpolicy = policy;
    if (isVisible())
        d->layoutChildren();
    if (oldPolicy != d->vbarpolicy)
        d->scrollBarPolicyChanged(Qt::Vertical, d->vbarpolicy);
}
/*!
    \property AbstractScrollArea::horizontalScrollBarPolicy
    \brief the policy for the horizontal scroll bar
    The default policy is Qt::ScrollBarAsNeeded.
    \sa verticalScrollBarPolicy
*/
Qt::ScrollBarPolicy AbstractScrollArea::horizontalScrollBarPolicy() const
{
    Q_D(const AbstractScrollArea);
    return d->hbarpolicy;
}
void AbstractScrollArea::setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy policy)
{
    Q_D(AbstractScrollArea);
    const Qt::ScrollBarPolicy oldPolicy = d->hbarpolicy;
    d->hbarpolicy = policy;
    if (isVisible())
        d->layoutChildren();
    if (oldPolicy != d->hbarpolicy)
        d->scrollBarPolicyChanged(Qt::Horizontal, d->hbarpolicy);
}
/*!
    \since 4.2
    Returns the widget in the corner between the two scroll bars.
    By default, no corner widget is present.
*/
QWidget* AbstractScrollArea::cornerWidget() const
{
    Q_D(const AbstractScrollArea);
    return d->cornerWidget;
}
/*!
    \since 4.2
    Sets the widget in the corner between the two scroll bars to be
    \a widget.
    You will probably also want to set at least one of the scroll bar
    modes to \c AlwaysOn.
    Passing \nullptr shows no widget in the corner.
    Any previous corner widget is hidden.
    You may call setCornerWidget() with the same widget at different
    times.
    All widgets set here will be deleted by the scroll area when it is
    destroyed unless you separately reparent the widget after setting
    some other corner widget (or \nullptr).
    Any \e newly set widget should have no current parent.
    By default, no corner widget is present.
    \sa horizontalScrollBarPolicy, horizontalScrollBarPolicy
*/
void AbstractScrollArea::setCornerWidget(QWidget* widget)
{
    Q_D(AbstractScrollArea);
    QWidget* oldWidget = d->cornerWidget;
    if (oldWidget != widget) {
        if (oldWidget)
            oldWidget->hide();
        d->cornerWidget = widget;
        if (widget && widget->parentWidget() != this)
            widget->setParent(this);
        d->layoutChildren();
        if (widget)
            widget->show();
    } else {
        d->cornerWidget = widget;
        d->layoutChildren();
    }
}
/*!
    \since 4.2
    Adds \a widget as a scroll bar widget in the location specified
    by \a alignment.
    Scroll bar widgets are shown next to the horizontal or vertical
    scroll bar, and can be placed on either side of it. If you want
    the scroll bar widgets to be always visible, set the
    scrollBarPolicy for the corresponding scroll bar to \c AlwaysOn.
    \a alignment must be one of Qt::Alignleft and Qt::AlignRight,
    which maps to the horizontal scroll bar, or Qt::AlignTop and
    Qt::AlignBottom, which maps to the vertical scroll bar.
    A scroll bar widget can be removed by either re-parenting the
    widget or deleting it. It's also possible to hide a widget with
    QWidget::hide()
    The scroll bar widget will be resized to fit the scroll bar
    geometry for the current style. The following describes the case
    for scroll bar widgets on the horizontal scroll bar:
    The height of the widget will be set to match the height of the
    scroll bar. To control the width of the widget, use
    QWidget::setMinimumWidth and QWidget::setMaximumWidth, or
    implement QWidget::sizeHint() and set a horizontal size policy.
    If you want a square widget, call
    QStyle::pixelMetric(QStyle::PM_ScrollBarExtent) and set the
    width to this value.
    \sa scrollBarWidgets()
*/
void AbstractScrollArea::addScrollBarWidget(QWidget* widget, Qt::Alignment alignment)
{
    Q_D(AbstractScrollArea);
    if (widget == nullptr)
        return;
    const Qt::Orientation scrollBarOrientation
        = ((alignment & Qt::AlignLeft) || (alignment & Qt::AlignRight)) ? Qt::Horizontal
                                                                        : Qt::Vertical;
    const AbstractScrollAreaScrollBarContainer::LogicalPosition position
        = ((alignment & Qt::AlignRight) || (alignment & Qt::AlignBottom))
        ? AbstractScrollAreaScrollBarContainer::LogicalRight
        : AbstractScrollAreaScrollBarContainer::LogicalLeft;
    d->scrollBarContainers[scrollBarOrientation]->addWidget(widget, position);
    d->layoutChildren();
    if (isHidden() == false)
        widget->show();
}
/*!
    \since 4.2
    Returns a list of the currently set scroll bar widgets. \a alignment
    can be any combination of the four location flags.
    \sa addScrollBarWidget()
*/
QWidgetList AbstractScrollArea::scrollBarWidgets(Qt::Alignment alignment)
{
    Q_D(AbstractScrollArea);
    QWidgetList list;
    if (alignment & Qt::AlignLeft)
        list += d->scrollBarContainers[Qt::Horizontal]->widgets(
            AbstractScrollAreaScrollBarContainer::LogicalLeft);
    if (alignment & Qt::AlignRight)
        list += d->scrollBarContainers[Qt::Horizontal]->widgets(
            AbstractScrollAreaScrollBarContainer::LogicalRight);
    if (alignment & Qt::AlignTop)
        list += d->scrollBarContainers[Qt::Vertical]->widgets(
            AbstractScrollAreaScrollBarContainer::LogicalLeft);
    if (alignment & Qt::AlignBottom)
        list += d->scrollBarContainers[Qt::Vertical]->widgets(
            AbstractScrollAreaScrollBarContainer::LogicalRight);
    return list;
}
/*!
    Sets the margins around the scrolling area to \a left, \a top, \a
    right and \a bottom. This is useful for applications such as
    spreadsheets with "locked" rows and columns. The marginal space
    is left blank; put widgets in the unused area.
    Note that this function is frequently called by QTreeView and
    QTableView, so margins must be implemented by AbstractScrollArea
    subclasses. Also, if the subclasses are to be used in item views,
    they should not call this function.
    By default all margins are zero.
    \sa viewportMargins()
*/
void AbstractScrollArea::setViewportMargins(int left, int top, int right, int bottom)
{
    Q_D(AbstractScrollArea);
    d->left = left;
    d->top = top;
    d->right = right;
    d->bottom = bottom;
    d->layoutChildren();
}
/*!
    \since 4.6
    Sets \a margins around the scrolling area. This is useful for
    applications such as spreadsheets with "locked" rows and columns.
    The marginal space is is left blank; put widgets in the unused
    area.
    By default all margins are zero.
    \sa viewportMargins()
*/
void AbstractScrollArea::setViewportMargins(const QMargins& margins)
{
    setViewportMargins(margins.left(), margins.top(), margins.right(), margins.bottom());
}
/*!
    \since 5.5
    Returns the margins around the scrolling area.
    By default all the margins are zero.
    \sa setViewportMargins()
*/
QMargins AbstractScrollArea::viewportMargins() const
{
    Q_D(const AbstractScrollArea);
    return QMargins(d->left, d->top, d->right, d->bottom);
}
/*!
  Returns the vertical scroll bar.
  \sa verticalScrollBarPolicy, horizontalScrollBar()
 */
PageTextEditScrollBar* AbstractScrollArea::verticalScrollBar() const
{
    Q_D(const AbstractScrollArea);
    return d->vbar;
}
/*!
  Returns the horizontal scroll bar.
  \sa horizontalScrollBarPolicy, verticalScrollBar()
 */
PageTextEditScrollBar* AbstractScrollArea::horizontalScrollBar() const
{
    Q_D(const AbstractScrollArea);
    return d->hbar;
}
/*! \internal */
bool AbstractScrollArea::eventFilter(QObject* o, QEvent* e)
{
    Q_D(AbstractScrollArea);
    if ((o == d->hbar || o == d->vbar)
        && (e->type() == QEvent::HoverEnter || e->type() == QEvent::HoverLeave)) {
        if (d->hbarpolicy == Qt::ScrollBarAsNeeded && d->vbarpolicy == Qt::ScrollBarAsNeeded) {
            PageTextEditScrollBar* sbar = static_cast<PageTextEditScrollBar*>(o);
            PageTextEditScrollBar* sibling = sbar == d->hbar ? d->vbar : d->hbar;
            if (sbar->style()->styleHint(QStyle::SH_ScrollBar_Transient, nullptr, sbar)
                && sibling->style()->styleHint(QStyle::SH_ScrollBar_Transient, nullptr, sibling))
                d->setScrollBarTransient(sibling, e->type() == QEvent::HoverLeave);
        }
    }
    return Frame::eventFilter(o, e);
}
/*!
    \fn bool AbstractScrollArea::event(QEvent *event)
    \reimp
    This is the main event handler for the AbstractScrollArea widget (\e not
    the scrolling area viewport()). The specified \a event is a general event
    object that may need to be cast to the appropriate class depending on its
    type.
    \sa QEvent::type()
*/
bool AbstractScrollArea::event(QEvent* e)
{
    Q_D(AbstractScrollArea);
    switch (e->type()) {
    case QEvent::AcceptDropsChange:
        // There was a chance that with accessibility client we get an
        // event before the viewport was created.
        // Also, in some cases we might get here from QWidget::event() virtual function which is
        // (indirectly) called from the viewport constructor at the time when the d->viewport is not
        // yet initialized even without any accessibility client. See AbstractScrollArea autotest
        // for a test case.
        if (d->viewport)
            d->viewport->setAcceptDrops(acceptDrops());
        break;
    case QEvent::MouseTrackingChange:
        d->viewport->setMouseTracking(hasMouseTracking());
        break;
    case QEvent::Resize:
        if (!d->inResize) {
            d->inResize = true;
            d->layoutChildren();
            d->inResize = false;
        }
        break;
    case QEvent::Show:
        if (!d->shownOnce
            && d->sizeAdjustPolicy == AbstractScrollArea::AdjustToContentsOnFirstShow) {
            d->sizeHint = QSize();
            updateGeometry();
        }
        d->shownOnce = true;
        return Frame::event(e);
    case QEvent::Paint: {
        QStyleOption option;
        option.initFrom(this);
        if (d->cornerPaintingRect.isValid()) {
            option.rect = d->cornerPaintingRect;
            QPainter p(this);
            style()->drawPrimitive(QStyle::PE_PanelScrollAreaCorner, &option, &p, this);
        }
    }
        Frame::paintEvent((QPaintEvent*)e);
        break;
#ifndef QT_NO_CONTEXTMENU
    case QEvent::ContextMenu:
        if (static_cast<QContextMenuEvent*>(e)->reason() == QContextMenuEvent::Keyboard)
            return Frame::event(e);
        e->ignore();
        break;
#endif // QT_NO_CONTEXTMENU
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
    case QEvent::MouseMove:
    case QEvent::Wheel:
#if QT_CONFIG(draganddrop)
    case QEvent::Drop:
    case QEvent::DragEnter:
    case QEvent::DragMove:
    case QEvent::DragLeave:
#endif
        // ignore touch events in case they have been propagated from the viewport
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
        return false;
#ifndef QT_NO_GESTURES
    case QEvent::Gesture: {
        QGestureEvent* ge = static_cast<QGestureEvent*>(e);
        QPanGesture* g = static_cast<QPanGesture*>(ge->gesture(Qt::PanGesture));
        if (g) {
            PageTextEditScrollBar* hBar = horizontalScrollBar();
            PageTextEditScrollBar* vBar = verticalScrollBar();
            QPointF delta = g->delta();
            if (!delta.isNull()) {
                if (QGuiApplication::isRightToLeft())
                    delta.rx() *= -1;
                int newX = hBar->value() - delta.x();
                int newY = vBar->value() - delta.y();
                hBar->setValue(newX);
                vBar->setValue(newY);
            }
            return true;
        }
        return false;
    }
#endif // QT_NO_GESTURES
    case QEvent::ScrollPrepare: {
        QScrollPrepareEvent* se = static_cast<QScrollPrepareEvent*>(e);
        if (d->canStartScrollingAt(se->startPos().toPoint())) {
            PageTextEditScrollBar* hBar = horizontalScrollBar();
            PageTextEditScrollBar* vBar = verticalScrollBar();
            se->setViewportSize(QSizeF(viewport()->size()));
            se->setContentPosRange(QRectF(0, 0, hBar->maximum(), vBar->maximum()));
            se->setContentPos(QPointF(hBar->value(), vBar->value()));
            se->accept();
            return true;
        }
        return false;
    }
    case QEvent::Scroll: {
        QScrollEvent* se = static_cast<QScrollEvent*>(e);
        PageTextEditScrollBar* hBar = horizontalScrollBar();
        PageTextEditScrollBar* vBar = verticalScrollBar();
        hBar->setValue(se->contentPos().x());
        vBar->setValue(se->contentPos().y());
        QPoint delta = d->overshoot - se->overshootDistance().toPoint();
        if (!delta.isNull())
            viewport()->move(viewport()->pos() + delta);
        d->overshoot = se->overshootDistance().toPoint();
        return true;
    }
    case QEvent::StyleChange:
    case QEvent::LayoutDirectionChange:
    case QEvent::ApplicationLayoutDirectionChange:
    case QEvent::LayoutRequest:
        d->layoutChildren();
        Q_FALLTHROUGH();
    default:
        return Frame::event(e);
    }
    return true;
}
/*!
  \fn bool AbstractScrollArea::viewportEvent(QEvent *event)
  The main event handler for the scrolling area (the viewport() widget).
  It handles the \a event specified, and can be called by subclasses to
  provide reasonable default behavior.
  Returns \c true to indicate to the event system that the event has been
  handled, and needs no further processing; otherwise returns \c false to
  indicate that the event should be propagated further.
  You can reimplement this function in a subclass, but we recommend
  using one of the specialized event handlers instead.
  Specialized handlers for viewport events are: paintEvent(),
  mousePressEvent(), mouseReleaseEvent(), mouseDoubleClickEvent(),
  mouseMoveEvent(), wheelEvent(), dragEnterEvent(), dragMoveEvent(),
  dragLeaveEvent(), dropEvent(), contextMenuEvent(), and
  resizeEvent().
*/
bool AbstractScrollArea::viewportEvent(QEvent* e)
{
    switch (e->type()) {
    case QEvent::Resize:
    case QEvent::Paint:
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
    case QEvent::MouseMove:
    case QEvent::ContextMenu:
#if QT_CONFIG(wheelevent)
    case QEvent::Wheel:
#endif
#if QT_CONFIG(draganddrop)
    case QEvent::Drop:
    case QEvent::DragEnter:
    case QEvent::DragMove:
    case QEvent::DragLeave:
#endif
#ifndef QT_NO_OPENGL
        // QOpenGLWidget needs special support because it has to know
        // its size has changed, so that it can resize its fbo.
        if (e->type() == QEvent::Resize)
            QWidgetPrivate::get(viewport())->resizeViewportFramebuffer();
#endif
        return Frame::event(e);
    case QEvent::LayoutRequest:
#ifndef QT_NO_GESTURES
    case QEvent::Gesture:
    case QEvent::GestureOverride:
        return event(e);
#endif
    case QEvent::ScrollPrepare:
    case QEvent::Scroll:
        return event(e);
    default:
        break;
    }
    return false; // let the viewport widget handle the event
}
/*!
    \fn void AbstractScrollArea::resizeEvent(QResizeEvent *event)
    This event handler can be reimplemented in a subclass to receive
    resize events (passed in \a event), for the viewport() widget.
    When resizeEvent() is called, the viewport already has its new
    geometry: Its new size is accessible through the
    QResizeEvent::size() function, and the old size through
    QResizeEvent::oldSize().
    \sa QWidget::resizeEvent()
 */
void AbstractScrollArea::resizeEvent(QResizeEvent*)
{
}
/*!
    \fn void AbstractScrollArea::paintEvent(QPaintEvent *event)
    This event handler can be reimplemented in a subclass to receive
    paint events (passed in \a event), for the viewport() widget.
    \note If you create a QPainter, it must operate on the viewport().
    \sa QWidget::paintEvent()
*/
void AbstractScrollArea::paintEvent(QPaintEvent*)
{
}
/*!
    This event handler can be reimplemented in a subclass to receive
    mouse press events for the viewport() widget. The event is passed
    in \a e.
    The default implementation calls QWidget::mousePressEvent() for
    default popup handling.
    \sa QWidget::mousePressEvent()
*/
void AbstractScrollArea::mousePressEvent(QMouseEvent* e)
{
    QWidget::mousePressEvent(e);
}
/*!
    This event handler can be reimplemented in a subclass to receive
    mouse release events for the viewport() widget. The event is
    passed in \a e.
    \sa QWidget::mouseReleaseEvent()
*/
void AbstractScrollArea::mouseReleaseEvent(QMouseEvent* e)
{
    e->ignore();
}
/*!
    This event handler can be reimplemented in a subclass to receive
    mouse double click events for the viewport() widget. The event is
    passed in \a e.
    \sa QWidget::mouseDoubleClickEvent()
*/
void AbstractScrollArea::mouseDoubleClickEvent(QMouseEvent* e)
{
    e->ignore();
}
/*!
    This event handler can be reimplemented in a subclass to receive
    mouse move events for the viewport() widget. The event is passed
    in \a e.
    \sa QWidget::mouseMoveEvent()
*/
void AbstractScrollArea::mouseMoveEvent(QMouseEvent* e)
{
    e->ignore();
}
/*!
    This event handler can be reimplemented in a subclass to receive
    wheel events for the viewport() widget. The event is passed in \a
    e.
    \sa QWidget::wheelEvent()
*/
#if QT_CONFIG(wheelevent)
void AbstractScrollArea::wheelEvent(QWheelEvent* e)
{
    Q_D(AbstractScrollArea);
    if (qAbs(e->angleDelta().x()) > qAbs(e->angleDelta().y()))
        QCoreApplication::sendEvent(d->hbar, e);
    else
        QCoreApplication::sendEvent(d->vbar, e);
}
#endif
#ifndef QT_NO_CONTEXTMENU
/*!
    This event handler can be reimplemented in a subclass to receive
    context menu events for the viewport() widget. The event is passed
    in \a e.
    \sa QWidget::contextMenuEvent()
*/
void AbstractScrollArea::contextMenuEvent(QContextMenuEvent* e)
{
    e->ignore();
}
#endif // QT_NO_CONTEXTMENU
/*!
    This function is called with key event \a e when key presses
    occur. It handles PageUp, PageDown, Up, Down, Left, and Right, and
    ignores all other key presses.
*/
void AbstractScrollArea::keyPressEvent(QKeyEvent* e)
{
    Q_D(AbstractScrollArea);
    if (false) {
#ifndef QT_NO_SHORTCUT
    } else if (e == QKeySequence::MoveToPreviousPage) {
        d->vbar->triggerAction(PageTextEditScrollBar::SliderPageStepSub);
    } else if (e == QKeySequence::MoveToNextPage) {
        d->vbar->triggerAction(PageTextEditScrollBar::SliderPageStepAdd);
#endif
    } else {
#ifdef QT_KEYPAD_NAVIGATION
        if (QApplicationPrivate::keypadNavigationEnabled() && !hasEditFocus()) {
            e->ignore();
            return;
        }
#endif
        switch (e->key()) {
        case Qt::Key_Up:
            d->vbar->triggerAction(PageTextEditScrollBar::SliderSingleStepSub);
            break;
        case Qt::Key_Down:
            d->vbar->triggerAction(PageTextEditScrollBar::SliderSingleStepAdd);
            break;
        case Qt::Key_Left:
#ifdef QT_KEYPAD_NAVIGATION
            if (QApplicationPrivate::keypadNavigationEnabled() && hasEditFocus()
                && (!d->hbar->isVisible() || d->hbar->value() == d->hbar->minimum())) {
                // if we aren't using the hbar or we are already at the leftmost point ignore
                e->ignore();
                return;
            }
#endif
            d->hbar->triggerAction(

                layoutDirection() == Qt::LeftToRight ? PageTextEditScrollBar::SliderSingleStepSub
                                                     : PageTextEditScrollBar::SliderSingleStepAdd);
            break;
        case Qt::Key_Right:
#ifdef QT_KEYPAD_NAVIGATION
            if (QApplicationPrivate::keypadNavigationEnabled() && hasEditFocus()
                && (!d->hbar->isVisible() || d->hbar->value() == d->hbar->maximum())) {
                // if we aren't using the hbar or we are already at the rightmost point ignore
                e->ignore();
                return;
            }
#endif
            d->hbar->triggerAction(

                layoutDirection() == Qt::LeftToRight ? PageTextEditScrollBar::SliderSingleStepAdd
                                                     : PageTextEditScrollBar::SliderSingleStepSub);
            break;
        default:
            e->ignore();
            return;
        }
    }
    e->accept();
}
#if QT_CONFIG(draganddrop)
/*!
    \fn void AbstractScrollArea::dragEnterEvent(QDragEnterEvent *event)
    This event handler can be reimplemented in a subclass to receive
    drag enter events (passed in \a event), for the viewport() widget.
    \sa QWidget::dragEnterEvent()
*/
void AbstractScrollArea::dragEnterEvent(QDragEnterEvent*)
{
}
/*!
    \fn void AbstractScrollArea::dragMoveEvent(QDragMoveEvent *event)
    This event handler can be reimplemented in a subclass to receive
    drag move events (passed in \a event), for the viewport() widget.
    \sa QWidget::dragMoveEvent()
*/
void AbstractScrollArea::dragMoveEvent(QDragMoveEvent*)
{
}
/*!
    \fn void AbstractScrollArea::dragLeaveEvent(QDragLeaveEvent *event)
    This event handler can be reimplemented in a subclass to receive
    drag leave events (passed in \a event), for the viewport() widget.
    \sa QWidget::dragLeaveEvent()
*/
void AbstractScrollArea::dragLeaveEvent(QDragLeaveEvent*)
{
}
/*!
    \fn void AbstractScrollArea::dropEvent(QDropEvent *event)
    This event handler can be reimplemented in a subclass to receive
    drop events (passed in \a event), for the viewport() widget.
    \sa QWidget::dropEvent()
*/
void AbstractScrollArea::dropEvent(QDropEvent*)
{
}
#endif
/*!
    This virtual handler is called when the scroll bars are moved by
    \a dx, \a dy, and consequently the viewport's contents should be
    scrolled accordingly.
    The default implementation simply calls update() on the entire
    viewport(), subclasses can reimplement this handler for
    optimization purposes, or - like QScrollArea - to move a contents
    widget. The parameters \a dx and \a dy are there for convenience,
    so that the class knows how much should be scrolled (useful
    e.g. when doing pixel-shifts). You may just as well ignore these
    values and scroll directly to the position the scroll bars
    indicate.
    Calling this function in order to scroll programmatically is an
    error, use the scroll bars instead (e.g. by calling
    ScrollBar::setValue() directly).
*/
void AbstractScrollArea::scrollContentsBy(int, int)
{
    viewport()->update();
}
bool AbstractScrollAreaPrivate::canStartScrollingAt(const QPoint& startPos) const
{
    Q_Q(const AbstractScrollArea);
    // don't start scrolling on a QAbstractSlider
    if (qobject_cast<QAbstractSlider*>(q->viewport()->childAt(startPos)))
        return false;
    return true;
}
void AbstractScrollAreaPrivate::flashScrollBars()
{
    QStyleOptionSlider opt;
    hbar->initStyleOption(&opt);
    bool htransient = hbar->style()->styleHint(QStyle::SH_ScrollBar_Transient, &opt, hbar);
    if ((hbarpolicy != Qt::ScrollBarAlwaysOff)
        && (hbarpolicy == Qt::ScrollBarAsNeeded || htransient))
        hbar->d_func()->flash();
    vbar->initStyleOption(&opt);
    bool vtransient = vbar->style()->styleHint(QStyle::SH_ScrollBar_Transient, &opt, vbar);
    if ((vbarpolicy != Qt::ScrollBarAlwaysOff)
        && (vbarpolicy == Qt::ScrollBarAsNeeded || vtransient))
        vbar->d_func()->flash();
}
void AbstractScrollAreaPrivate::setScrollBarTransient(PageTextEditScrollBar* scrollBar,
                                                      bool transient)
{
    scrollBar->d_func()->setTransient(transient);
}
void AbstractScrollAreaPrivate::_q_hslide(int x)
{
    Q_Q(AbstractScrollArea);
    int dx = xoffset - x;
    xoffset = x;
    q->scrollContentsBy(dx, 0);
    flashScrollBars();
}
void AbstractScrollAreaPrivate::_q_vslide(int y)
{
    Q_Q(AbstractScrollArea);
    int dy = yoffset - y;
    yoffset = y;
    q->scrollContentsBy(0, dy);
    flashScrollBars();
}
void AbstractScrollAreaPrivate::_q_showOrHideScrollBars()
{
    layoutChildren();
}
QPoint AbstractScrollAreaPrivate::contentsOffset() const
{
    Q_Q(const AbstractScrollArea);
    QPoint offset;
    if (vbar->isVisible())
        offset.setY(vbar->value());
    if (hbar->isVisible()) {
        if (q->isRightToLeft())
            offset.setX(hbar->maximum() - hbar->value());
        else
            offset.setX(hbar->value());
    }
    return offset;
}
/*!
    \reimp
*/
QSize AbstractScrollArea::minimumSizeHint() const
{
    Q_D(const AbstractScrollArea);
    int hsbExt = d->hbar->sizeHint().height();
    int vsbExt = d->vbar->sizeHint().width();
    int extra = 2 * d->frameWidth;
    QStyleOption opt;
    opt.initFrom(this);
    if ((d->frameStyle != QFrame::NoFrame)
        && style()->styleHint(QStyle::SH_ScrollView_FrameOnlyAroundContents, &opt, this)) {
        extra += style()->pixelMetric(QStyle::PM_ScrollView_ScrollBarSpacing, &opt, this);
    }
    return QSize(d->scrollBarContainers[Qt::Horizontal]->sizeHint().width() + vsbExt + extra,
                 d->scrollBarContainers[Qt::Vertical]->sizeHint().height() + hsbExt + extra);
}
/*!
    Returns the sizeHint property of the scroll area. The size is determined by using
    viewportSizeHint() plus some extra space for scroll bars, if needed.
    \reimp
*/
QSize AbstractScrollArea::sizeHint() const
{
    Q_D(const AbstractScrollArea);
    if (d->sizeAdjustPolicy == AbstractScrollArea::AdjustIgnored)
        return QSize(256, 192);
    if (!d->sizeHint.isValid() || d->sizeAdjustPolicy == AbstractScrollArea::AdjustToContents) {
        const int f = 2 * d->frameWidth;
        const QSize frame(f, f);
        const bool vbarHidden
            = !d->vbar->isVisibleTo(this) || d->vbarpolicy == Qt::ScrollBarAlwaysOff;
        const bool hbarHidden
            = !d->hbar->isVisibleTo(this) || d->hbarpolicy == Qt::ScrollBarAlwaysOff;
        const QSize scrollbars(vbarHidden ? 0 : d->vbar->sizeHint().width(),
                               hbarHidden ? 0 : d->hbar->sizeHint().height());
        d->sizeHint = frame + scrollbars + viewportSizeHint();
    }
    return d->sizeHint;
}
/*!
   \since 5.2
   Returns the recommended size for the viewport.
   The default implementation returns viewport()->sizeHint().
   Note that the size is just the viewport's size, without any scroll bars visible.
 */
QSize AbstractScrollArea::viewportSizeHint() const
{
    Q_D(const AbstractScrollArea);
    if (d->viewport) {
        const QSize sh = d->viewport->sizeHint();
        if (sh.isValid()) {
            return sh;
        }
    }
    const int h = qMax(10, fontMetrics().height());
    return QSize(6 * h, 4 * h);
}
/*!
    \since 5.2
    \property AbstractScrollArea::sizeAdjustPolicy
    \brief the policy describing how the size of the scroll area changes when the
    size of the viewport changes.
    The default policy is AbstractScrollArea::AdjustIgnored.
    Changing this property might actually resize the scrollarea.
*/
AbstractScrollArea::SizeAdjustPolicy AbstractScrollArea::sizeAdjustPolicy() const
{
    Q_D(const AbstractScrollArea);
    return d->sizeAdjustPolicy;
}
void AbstractScrollArea::setSizeAdjustPolicy(SizeAdjustPolicy policy)
{
    Q_D(AbstractScrollArea);
    if (d->sizeAdjustPolicy == policy)
        return;
    d->sizeAdjustPolicy = policy;
    d->sizeHint = QSize();
    updateGeometry();
}
/*!
    This slot is called by AbstractScrollArea after setViewport(\a
    viewport) has been called. Reimplement this function in a
    subclass of AbstractScrollArea to initialize the new \a viewport
    before it is used.
    \sa setViewport()
*/
void AbstractScrollArea::setupViewport(QWidget* viewport)
{
    Q_UNUSED(viewport);
}
QT_END_NAMESPACE
#include "moc_abstract_scroll_area.cpp"
#include "moc_abstract_scroll_area_p.cpp"
