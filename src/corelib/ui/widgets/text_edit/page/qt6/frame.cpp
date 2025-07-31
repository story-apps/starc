#pragma once

#include "frame.h"

#include "frame_p.h"
#include "qapplication.h"
#include "qbitmap.h"
#include "qdrawutil.h"
#include "qevent.h"
#include "qpainter.h"
#include "qstyle.h"
#include "qstyleoption.h"
#include "qstylepainter.h"


FramePrivate::FramePrivate()
    : frect(0, 0, 0, 0)
    , frameStyle(QFrame::NoFrame | QFrame::Plain)
    , lineWidth(1)
    , midLineWidth(0)
    , frameWidth(0)
    , leftFrameWidth(0)
    , rightFrameWidth(0)
    , topFrameWidth(0)
    , bottomFrameWidth(0)
{
}
FramePrivate::~FramePrivate()
{
}
inline void FramePrivate::init()
{
    Q_Q(Frame);
    setLayoutItemMargins(QStyle::SE_FrameLayoutItem);
    // The frameRect property is implemented in terms of the widget's
    // contentsRect, which conflicts with the implicit inclusion of
    // the safe area margins in the contentsRect.
    q->setAttribute(Qt::WA_ContentsMarginsRespectsSafeArea, false);
}
/*!
    \class Frame
    \brief The Frame class is the base class of widgets that can have a frame.
    \ingroup abstractwidgets
    \inmodule QtWidgets
    QMenu uses this to "raise" the menu above the surrounding
    screen. QProgressBar has a "sunken" look. QLabel has a flat look.
    The frames of widgets like these can be changed.
    \snippet code/src_gui_widgets_Frame.cpp 0
    The Frame class can also be used directly for creating simple
    placeholder frames without any contents.
    The frame style is specified by a \l{Frame::Shape}{frame shape} and
    a \l{Frame::Shadow}{shadow style} that is used to visually separate
    the frame from surrounding widgets. These properties can be set
    together using the setFrameStyle() function and read with frameStyle().
    The frame shapes are \l NoFrame, \l Box, \l Panel, \l StyledPanel,
    HLine and \l VLine; the shadow styles are \l Plain, \l Raised and
    \l Sunken.
    A frame widget has three attributes that describe the thickness of the
    border: \l lineWidth, \l midLineWidth, and \l frameWidth.
    \list
    \li The line width is the width of the frame border. It can be modified
       to customize the frame's appearance.
    \li The mid-line width specifies the width of an extra line in the
       middle of the frame, which uses a third color to obtain a special
       3D effect. Notice that a mid-line is only drawn for \l Box, \l
       HLine and \l VLine frames that are raised or sunken.
    \li The frame width is determined by the frame style, and the frameWidth()
       function is used to obtain the value defined for the style used.
    \endlist
    The margin between the frame and the contents of the frame can be
    customized with the QWidget::setContentsMargins() function.
    \target picture
    This table shows some of the combinations of styles and line widths:
    \image frames.png Table of frame styles
*/
/*!
    \enum Frame::Shape
    This enum type defines the shapes of frame available.
    \value NoFrame  Frame draws nothing
    \value Box  Frame draws a box around its contents
    \value Panel  Frame draws a panel to make the contents appear
    raised or sunken
    \value StyledPanel  draws a rectangular panel with a look that
    depends on the current GUI style. It can be raised or sunken.
    \value HLine  Frame draws a horizontal line that frames nothing
    (useful as separator)
    \value VLine  Frame draws a vertical line that frames nothing
    (useful as separator)
    \value WinPanel draws a rectangular panel that can be raised or
    sunken like those in Windows 2000. Specifying this shape sets
    the line width to 2 pixels. WinPanel is provided for compatibility.
    For GUI style independence we recommend using StyledPanel instead.
    When it does not call QStyle, Shape interacts with Frame::Shadow,
    the lineWidth() and the midLineWidth() to create the total result.
    See the picture of the frames in the main class documentation.
    \sa Frame::Shadow, Frame::style(), QStyle::drawPrimitive()
*/
/*!
    \enum Frame::Shadow
    This enum type defines the types of shadow that are used to give
    a 3D effect to frames.
    \value Plain  the frame and contents appear level with the
    surroundings; draws using the palette QPalette::WindowText color
    (without any 3D effect)
    \value Raised the frame and contents appear raised; draws a 3D
    raised line using the light and dark colors of the current color
    group
    \value Sunken the frame and contents appear sunken; draws a 3D
    sunken line using the light and dark colors of the current color
    group
    Shadow interacts with Frame::Shape, the lineWidth() and the
    midLineWidth(). See the picture of the frames in the main class
    documentation.
    \sa Frame::Shape, lineWidth(), midLineWidth()
*/
/*!
    \enum Frame::StyleMask
    This enum defines two constants that can be used to extract the
    two components of frameStyle():
    \value Shadow_Mask The \l Shadow part of frameStyle()
    \value Shape_Mask  The \l Shape part of frameStyle()
    Normally, you don't need to use these, since frameShadow() and
    frameShape() already extract the \l Shadow and the \l Shape parts
    of frameStyle().
    \sa frameStyle(), setFrameStyle()
*/
/*!
    Constructs a frame widget with frame style \l NoFrame and a
    1-pixel frame width.
    The \a parent and \a f arguments are passed to the QWidget
    constructor.
*/
Frame::Frame(QWidget* parent, Qt::WindowFlags f)
    : QWidget(*new FramePrivate, parent, f)
{
    Q_D(Frame);
    d->init();
}
/*! \internal */
Frame::Frame(FramePrivate& dd, QWidget* parent, Qt::WindowFlags f)
    : QWidget(dd, parent, f)
{
    Q_D(Frame);
    d->init();
}
/*!
    \since 5.5
    Initializes \a option with the values from this Frame. This method is
    useful for subclasses when they need a QStyleOptionFrame but don't want to
    fill in all the information themselves.
    \sa QStyleOption::initFrom()
*/
void Frame::initStyleOption(QStyleOptionFrame* option) const
{
    if (!option)
        return;
    Q_D(const Frame);
    option->initFrom(this);
    int frameShape = d->frameStyle & QFrame::Shape_Mask;
    int frameShadow = d->frameStyle & QFrame::Shadow_Mask;
    option->frameShape = QFrame::Shape(int(option->frameShape) | frameShape);
    option->rect = frameRect();
    switch (frameShape) {
    case QFrame::Box:
    case QFrame::HLine:
    case QFrame::VLine:
    case QFrame::StyledPanel:
    case QFrame::Panel:
        option->lineWidth = d->lineWidth;
        option->midLineWidth = d->midLineWidth;
        break;
    default:
        // most frame styles do not handle customized line and midline widths
        // (see updateFrameWidth()).
        option->lineWidth = d->frameWidth;
        break;
    }
    if (frameShadow == QFrame::Sunken)
        option->state |= QStyle::State_Sunken;
    else if (frameShadow == QFrame::Raised)
        option->state |= QStyle::State_Raised;
}
/*!
  Destroys the frame.
 */
Frame::~Frame()
{
}
/*!
    Returns the frame style.
    The default value is Frame::Plain.
    \sa setFrameStyle(), frameShape(), frameShadow()
*/
int Frame::frameStyle() const
{
    Q_D(const Frame);
    return d->frameStyle;
}
/*!
    \property Frame::frameShape
    \brief the frame shape value from the frame style
    \sa frameStyle(), frameShadow()
*/
QFrame::Shape Frame::frameShape() const
{
    Q_D(const Frame);
    return (QFrame::Shape)(d->frameStyle & QFrame::Shape_Mask);
}
void Frame::setFrameShape(QFrame::Shape s)
{
    Q_D(Frame);
    setFrameStyle((d->frameStyle & QFrame::Shadow_Mask) | s);
}
/*!
    \property Frame::frameShadow
    \brief the frame shadow value from the frame style
    \sa frameStyle(), frameShape()
*/
QFrame::Shadow Frame::frameShadow() const
{
    Q_D(const Frame);
    return (QFrame::Shadow)(d->frameStyle & QFrame::Shadow_Mask);
}
void Frame::setFrameShadow(QFrame::Shadow s)
{
    Q_D(Frame);
    setFrameStyle((d->frameStyle & QFrame::Shape_Mask) | s);
}
/*!
    Sets the frame style to \a style.
    The \a style is the bitwise OR between a frame shape and a frame
    shadow style. See the picture of the frames in the main class
    documentation.
    The frame shapes are given in \l{Frame::Shape} and the shadow
    styles in \l{Frame::Shadow}.
    If a mid-line width greater than 0 is specified, an additional
    line is drawn for \l Raised or \l Sunken \l Box, \l HLine, and \l
    VLine frames. The mid-color of the current color group is used for
    drawing middle lines.
    \sa frameStyle()
*/
void Frame::setFrameStyle(int style)
{
    Q_D(Frame);
    if (!testAttribute(Qt::WA_WState_OwnSizePolicy)) {
        QSizePolicy sp;
        switch (style & QFrame::Shape_Mask) {
        case QFrame::HLine:
            sp = QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed, QSizePolicy::Line);
            break;
        case QFrame::VLine:
            sp = QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum, QSizePolicy::Line);
            break;
        default:
            sp = QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred, QSizePolicy::Frame);
        }
        setSizePolicy(sp);
        setAttribute(Qt::WA_WState_OwnSizePolicy, false);
    }
    d->frameStyle = (short)style;
    update();
    d->updateFrameWidth();
}
/*!
    \property Frame::lineWidth
    \brief the line width
    Note that the \e total line width for frames used as separators
    (\l HLine and \l VLine) is specified by \l frameWidth.
    The default value is 1.
    \sa midLineWidth, frameWidth
*/
void Frame::setLineWidth(int w)
{
    Q_D(Frame);
    if (short(w) == d->lineWidth)
        return;
    d->lineWidth = short(w);
    d->updateFrameWidth();
}
int Frame::lineWidth() const
{
    Q_D(const Frame);
    return d->lineWidth;
}
/*!
    \property Frame::midLineWidth
    \brief the width of the mid-line
    The default value is 0.
    \sa lineWidth, frameWidth
*/
void Frame::setMidLineWidth(int w)
{
    Q_D(Frame);
    if (short(w) == d->midLineWidth)
        return;
    d->midLineWidth = short(w);
    d->updateFrameWidth();
}
int Frame::midLineWidth() const
{
    Q_D(const Frame);
    return d->midLineWidth;
}
/*!
  \internal
  Updates the frame widths from the style.
*/
void FramePrivate::updateStyledFrameWidths()
{
    Q_Q(const Frame);
    QStyleOptionFrame opt;
    q->initStyleOption(&opt);
    QRect cr = q->style()->subElementRect(QStyle::SE_ShapedFrameContents, &opt, q);
    leftFrameWidth = cr.left() - opt.rect.left();
    topFrameWidth = cr.top() - opt.rect.top();
    rightFrameWidth = opt.rect.right() - cr.right(),
    bottomFrameWidth = opt.rect.bottom() - cr.bottom();
    frameWidth = qMax(qMax(leftFrameWidth, rightFrameWidth), qMax(topFrameWidth, bottomFrameWidth));
}
/*!
  \internal
  Updated the frameWidth parameter.
*/
void FramePrivate::updateFrameWidth()
{
    Q_Q(Frame);
    QRect fr = q->frameRect();
    updateStyledFrameWidths();
    q->setFrameRect(fr);
    setLayoutItemMargins(QStyle::SE_FrameLayoutItem);
}
/*!
    \property Frame::frameWidth
    \brief the width of the frame that is drawn.
    Note that the frame width depends on the \l{Frame::setFrameStyle()}{frame style},
    not only the line width and the mid-line width. For example, the style specified
    by \l NoFrame always has a frame width of 0, whereas the style \l Panel has a
    frame width equivalent to the line width.
    \sa lineWidth(), midLineWidth(), frameStyle()
*/
int Frame::frameWidth() const
{
    Q_D(const Frame);
    return d->frameWidth;
}
/*!
    \property Frame::frameRect
    \brief the frame's rectangle
    The frame's rectangle is the rectangle the frame is drawn in. By
    default, this is the entire widget. Setting the rectangle \e doesn't
    cause a widget update. The frame rectangle is automatically adjusted
    when the widget changes size.
    If you set the rectangle to a null rectangle (for example,
    QRect(0, 0, 0, 0)), then the resulting frame rectangle is
    equivalent to the \l{QWidget::rect()}{widget rectangle}.
*/
QRect Frame::frameRect() const
{
    Q_D(const Frame);
    QRect fr = contentsRect();
    fr.adjust(-d->leftFrameWidth, -d->topFrameWidth, d->rightFrameWidth, d->bottomFrameWidth);
    return fr;
}
void Frame::setFrameRect(const QRect& r)
{
    Q_D(Frame);
    QRect cr = r.isValid() ? r : rect();
    cr.adjust(d->leftFrameWidth, d->topFrameWidth, -d->rightFrameWidth, -d->bottomFrameWidth);
    setContentsMargins(cr.left(), cr.top(), rect().right() - cr.right(),
                       rect().bottom() - cr.bottom());
}
/*!\reimp
 */
QSize Frame::sizeHint() const
{
    Q_D(const Frame);
    //   Returns a size hint for the frame - for HLine and VLine
    //   shapes, this is stretchable one way and 3 pixels wide the
    //   other.  For other shapes, QWidget::sizeHint() is used.
    switch (d->frameStyle & QFrame::Shape_Mask) {
    case QFrame::HLine:
        return QSize(-1, 3);
    case QFrame::VLine:
        return QSize(3, -1);
    default:
        return QWidget::sizeHint();
    }
}
/*!\reimp
 */
void Frame::paintEvent(QPaintEvent*)
{
    QStylePainter p(this);
    drawFrame(&p);
}
/*!
    \internal
    Used by QLabel and QLCDNumber
 */
void Frame::drawFrame(QPainter* p)
{
    QStyleOptionFrame opt;
    initStyleOption(&opt);
    style()->drawControl(QStyle::CE_ShapedFrame, &opt, p, this);
}
/*!\reimp
 */
void Frame::changeEvent(QEvent* ev)
{
    Q_D(Frame);
    if (ev->type() == QEvent::StyleChange
#ifdef Q_OS_MAC
        || ev->type() == QEvent::MacSizeChange
#endif
    )
        d->updateFrameWidth();
    QWidget::changeEvent(ev);
}
/*! \reimp */
bool Frame::event(QEvent* e)
{
    if (e->type() == QEvent::ParentChange)
        d_func()->updateFrameWidth();
    bool result = QWidget::event(e);
    // this has to be done after the widget has been polished
    if (e->type() == QEvent::Polish)
        d_func()->updateFrameWidth();
    return result;
}

#include "moc_frame.cpp"
