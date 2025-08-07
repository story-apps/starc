#pragma once

#include "frame.h"

#include <corelib_global.h>


class QMargins;
class PageTextEditScrollBar;
class AbstractScrollAreaPrivate;
class CORE_LIBRARY_EXPORT AbstractScrollArea : public Frame
{
    Q_OBJECT
    Q_PROPERTY(Qt::ScrollBarPolicy verticalScrollBarPolicy READ verticalScrollBarPolicy WRITE
                   setVerticalScrollBarPolicy)
    Q_PROPERTY(Qt::ScrollBarPolicy horizontalScrollBarPolicy READ horizontalScrollBarPolicy WRITE
                   setHorizontalScrollBarPolicy)
    Q_PROPERTY(SizeAdjustPolicy sizeAdjustPolicy READ sizeAdjustPolicy WRITE setSizeAdjustPolicy)

public:
    explicit AbstractScrollArea(QWidget* parent = nullptr);
    ~AbstractScrollArea();

    enum SizeAdjustPolicy { AdjustIgnored, AdjustToContentsOnFirstShow, AdjustToContents };
    Q_ENUM(SizeAdjustPolicy)
    Qt::ScrollBarPolicy verticalScrollBarPolicy() const;
    void setVerticalScrollBarPolicy(Qt::ScrollBarPolicy);
    Qt::ScrollBarPolicy horizontalScrollBarPolicy() const;
    void setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy);
    QWidget* cornerWidget() const;
    void setCornerWidget(QWidget* widget);
    void addScrollBarWidget(QWidget* widget, Qt::Alignment alignment);
    QWidgetList scrollBarWidgets(Qt::Alignment alignment);
    QWidget* viewport() const;
    void setViewport(QWidget* widget);
    QSize maximumViewportSize() const;
    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;
    virtual void setupViewport(QWidget* viewport);
    SizeAdjustPolicy sizeAdjustPolicy() const;
    void setSizeAdjustPolicy(SizeAdjustPolicy policy);

protected:
    AbstractScrollArea(AbstractScrollAreaPrivate& dd, QWidget* parent = nullptr);
    void setViewportMargins(int left, int top, int right, int bottom);
    void setViewportMargins(const QMargins& margins);
    QMargins viewportMargins() const;
    PageTextEditScrollBar* verticalScrollBar() const;
    PageTextEditScrollBar* horizontalScrollBar() const;
    bool eventFilter(QObject*, QEvent*) override;
    bool event(QEvent*) override;
    virtual bool viewportEvent(QEvent*);
    void resizeEvent(QResizeEvent*) override;
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void mouseDoubleClickEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
#if QT_CONFIG(wheelevent)
    void wheelEvent(QWheelEvent*) override;
#endif
#ifndef QT_NO_CONTEXTMENU
    void contextMenuEvent(QContextMenuEvent*) override;
#endif
#if QT_CONFIG(draganddrop)
    void dragEnterEvent(QDragEnterEvent*) override;
    void dragMoveEvent(QDragMoveEvent*) override;
    void dragLeaveEvent(QDragLeaveEvent*) override;
    void dropEvent(QDropEvent*) override;
#endif
    void keyPressEvent(QKeyEvent*) override;
    virtual void scrollContentsBy(int dx, int dy);
    virtual QSize viewportSizeHint() const;

private:
    Q_DECLARE_PRIVATE(AbstractScrollArea)
    Q_DISABLE_COPY(AbstractScrollArea)
    Q_PRIVATE_SLOT(d_func(), void _q_hslide(int))
    Q_PRIVATE_SLOT(d_func(), void _q_vslide(int))
    Q_PRIVATE_SLOT(d_func(), void _q_showOrHideScrollBars())
    friend class ScalableWrapper;
    friend class QStyleSheetStyle;
    friend class QWidgetPrivate;
};
