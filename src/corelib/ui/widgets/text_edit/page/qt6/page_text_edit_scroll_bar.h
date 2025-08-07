#pragma once

#include "abstract_slider.h"

#include <QtWidgets/qwidget.h>

#include <corelib_global.h>


class PageTextEditScrollBarPrivate;
class QStyleOptionSlider;
class CORE_LIBRARY_EXPORT PageTextEditScrollBar : public AbstractSlider
{
    Q_OBJECT
public:
    explicit PageTextEditScrollBar(QWidget* parent = nullptr);
    explicit PageTextEditScrollBar(Qt::Orientation, QWidget* parent = nullptr);
    ~PageTextEditScrollBar();
    QSize sizeHint() const override;
    bool event(QEvent* event) override;

protected:
#if QT_CONFIG(wheelevent)
    void wheelEvent(QWheelEvent*) override;
#endif
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void hideEvent(QHideEvent*) override;
    void sliderChange(SliderChange change) override;
#ifndef QT_NO_CONTEXTMENU
    void contextMenuEvent(QContextMenuEvent*) override;
#endif
    virtual void initStyleOption(QStyleOptionSlider* option) const;

private:
    friend class AbstractScrollAreaPrivate;
    Q_DISABLE_COPY(PageTextEditScrollBar)
    Q_DECLARE_PRIVATE(PageTextEditScrollBar)
#if QT_CONFIG(itemviews)
    friend class QTableView;
    friend class QTreeViewPrivate;
    friend class QCommonListViewBase;
    friend class QListModeViewBase;
    friend class QAbstractItemView;
#endif
};
