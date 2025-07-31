#pragma once

#include <QtWidgets/qframe.h>
#include <QtWidgets/qtwidgetsglobal.h>
#include <QtWidgets/qwidget.h>

#include <corelib_global.h>

class FramePrivate;
class QStyleOptionFrame;
class CORE_LIBRARY_EXPORT Frame : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QFrame::Shape frameShape READ frameShape WRITE setFrameShape)
    Q_PROPERTY(QFrame::Shadow frameShadow READ frameShadow WRITE setFrameShadow)
    Q_PROPERTY(int lineWidth READ lineWidth WRITE setLineWidth)
    Q_PROPERTY(int midLineWidth READ midLineWidth WRITE setMidLineWidth)
    Q_PROPERTY(int frameWidth READ frameWidth)
    Q_PROPERTY(QRect frameRect READ frameRect WRITE setFrameRect DESIGNABLE false)
public:
    explicit Frame(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
    ~Frame();
    int frameStyle() const;
    void setFrameStyle(int);
    int frameWidth() const;
    QSize sizeHint() const override;
    QFrame::Shape frameShape() const;
    void setFrameShape(QFrame::Shape);
    QFrame::Shadow frameShadow() const;
    void setFrameShadow(QFrame::Shadow);
    int lineWidth() const;
    void setLineWidth(int);
    int midLineWidth() const;
    void setMidLineWidth(int);
    QRect frameRect() const;
    void setFrameRect(const QRect&);

protected:
    bool event(QEvent* e) override;
    void paintEvent(QPaintEvent*) override;
    void changeEvent(QEvent*) override;
    void drawFrame(QPainter*);

protected:
    Frame(FramePrivate& dd, QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
    virtual void initStyleOption(QStyleOptionFrame* option) const;

private:
    Q_DISABLE_COPY(Frame)
    Q_DECLARE_PRIVATE(Frame)
};
