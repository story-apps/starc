#include "overview_widget.h"

#include <ui/design_system/design_system.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/text_helper.h>

#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>


class OverviewWidget::Implementation
{
public:
    QString icon;
    QString title;

    struct DataSet {
        QColor color;
        QString label;
        QString value;
        int progress = 0;
    };
    DataSet min;
    DataSet avg;
    DataSet max;
    QString progressTitle;
};


// ****


OverviewWidget::OverviewWidget(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation)
{
}

OverviewWidget::~OverviewWidget() = default;

void OverviewWidget::setTitle(const QString _icon, const QString& _title)
{
    if (d->icon == _icon && d->title == _title) {
        return;
    }

    d->icon = _icon;
    d->title = _title;

    updateGeometry();
    update();
}

void OverviewWidget::setColors(const QColor& _min, const QColor& _avg, const QColor& _max)
{
    d->min.color = _min;
    d->avg.color = _avg;
    d->max.color = _max;

    update();
}

void OverviewWidget::setLabels(const QString& _min, const QString& _avg, const QString& _max)
{
    d->min.label = _min;
    d->avg.label = _avg;
    d->max.label = _max;

    update();
}

void OverviewWidget::setValues(const QString& _min, const QString& _avg, const QString& _max)
{
    d->min.value = _min;
    d->avg.value = _avg;
    d->max.value = _max;

    update();
}

void OverviewWidget::setProgress(int _min, int _avg, int _max, const QString& _progressTitle)
{
    d->min.progress = _min;
    d->avg.progress = _avg;
    d->max.progress = _max;
    d->progressTitle = _progressTitle;

    update();
}

QSize OverviewWidget::sizeHint() const
{
    return QSize(
        contentsMargins().left() + Ui::DesignSystem::layout().px(164) + contentsMargins().right(),
        contentsMargins().top() + Ui::DesignSystem::layout().px(164) + contentsMargins().bottom());
}

void OverviewWidget::paintEvent(QPaintEvent* _event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    //
    // Рисуем фон
    //
    painter.fillRect(_event->rect(), backgroundColor());
    painter.translate(contentsRect().topLeft());
    auto backgroundRect = contentsRect();
    backgroundRect.moveTopLeft({ 0, 0 });

    //
    // Заголовок с иконкой
    //
    const auto titleColor
        = ColorHelper::transparent(textColor(), Ui::DesignSystem::inactiveTextOpacity());
    painter.setPen(titleColor);
    painter.setFont(Ui::DesignSystem::font().iconsMid());
    const QRectF iconRect(0, 0, Ui::DesignSystem::layout().px24(),
                          Ui::DesignSystem::layout().px24());
    painter.drawText(iconRect, Qt::AlignCenter, d->icon);
    //
    painter.setFont(Ui::DesignSystem::font().button());
    const qreal titleLeft = iconRect.right() + Ui::DesignSystem::layout().px8();
    const QRectF titleRect(titleLeft, 0, backgroundRect.width() - titleLeft, iconRect.height());
    painter.drawText(titleRect, Qt::AlignLeft | Qt::AlignVCenter, d->title);

    //
    // Круги
    //
    auto drawProgress = [this, &painter](const QRectF& _rect, const QColor& _barColor,
                                         const QColor& _backgroundColor, int _progress,
                                         const QString& _text) {
        //
        // Фон
        //
        if (_backgroundColor.isValid()) {
            painter.setBrush(_backgroundColor);
            painter.setPen(Qt::NoPen);
            painter.drawEllipse(_rect);
        }

        //
        // Заполнение
        //
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(_barColor, Ui::DesignSystem::progressBar().circularTrackHeight()));
        const auto startAngle = 90 * 16;
        qreal progressCorrected = static_cast<qreal>(_progress) / 100.0;
        while (progressCorrected > 1.0) {
            progressCorrected -= 1.0;
        }
        const auto spanAngle = static_cast<int>(progressCorrected * 360 * 16);
        painter.drawArc(_rect, startAngle, spanAngle);

        //
        // Текст
        //
        if (!_text.isEmpty()) {
            painter.setPen(textColor());
            auto font = Ui::DesignSystem::font().body1();
            font.setWeight(QFont::Medium);
            painter.setFont(font);
            painter.drawText(_rect, Qt::AlignCenter, _text);
        }
    };
    const QRectF minProgressRect(0, iconRect.bottom() + Ui::DesignSystem::layout().px16(),
                                 Ui::DesignSystem::layout().px(118),
                                 Ui::DesignSystem::layout().px(118));
    drawProgress(minProgressRect, d->min.color, {}, d->min.progress, {});
    //
    const qreal progressSpacing = Ui::DesignSystem::layout().px4();
    const QRectF avgProgressRect(minProgressRect.left() + progressSpacing,
                                 minProgressRect.top() + progressSpacing,
                                 minProgressRect.width() - progressSpacing * 2,
                                 minProgressRect.height() - progressSpacing * 2);
    drawProgress(avgProgressRect, d->avg.color, {}, d->avg.progress, {});
    //
    const QRectF maxProgressRect(avgProgressRect.left() + progressSpacing,
                                 avgProgressRect.top() + progressSpacing,
                                 avgProgressRect.width() - progressSpacing * 2,
                                 avgProgressRect.height() - progressSpacing * 2);
    drawProgress(maxProgressRect, d->max.color,
                 ColorHelper::transparent(d->max.color, Ui::DesignSystem::hoverBackgroundOpacity()),
                 d->max.progress, d->progressTitle);

    //
    // Значения
    //
    auto drawValue = [this, &painter](const QRectF& _rect, const QString& _value,
                                      const QString& _label, const QString& _icon = {},
                                      const QColor& _iconColor = {}) {
        const QRectF valueRect(_rect.left(), _rect.top(), _rect.width(), _rect.height() / 2);
        painter.setPen(textColor());
        painter.setFont(Ui::DesignSystem::font().subtitle2());
        painter.drawText(valueRect, Qt::AlignLeft | Qt::AlignVCenter, _value);

        const QRectF labelRect(valueRect.left(), valueRect.bottom(), valueRect.width(),
                               valueRect.height());
        painter.setPen(
            ColorHelper::transparent(textColor(), Ui::DesignSystem::inactiveTextOpacity()));
        painter.setFont(Ui::DesignSystem::font().caption());
        painter.drawText(labelRect, Qt::AlignLeft | Qt::AlignVCenter, _label);

        if (!_icon.isEmpty()) {
            const auto labelWidth = TextHelper::fineTextWidthF(_label, painter.font());
            const QRectF iconRect(labelRect.left() + labelWidth + Ui::DesignSystem::layout().px4(),
                                  labelRect.top(), labelRect.height(), labelRect.height());
            painter.setPen(_iconColor);
            painter.setFont(Ui::DesignSystem::font().iconsSmall());
            painter.drawText(iconRect, Qt::AlignCenter, _icon);
        }
    };
    const QRectF maxValueRect(
        minProgressRect.right() + Ui::DesignSystem::layout().px24(), minProgressRect.top(),
        backgroundRect.width() - minProgressRect.width() - Ui::DesignSystem::layout().px24(),
        Ui::DesignSystem::layout().px(36));
    drawValue(maxValueRect, d->max.value, d->max.label, u8"\U000f0360", d->max.color);
    //
    const QRectF minValueRect(maxValueRect.left(),
                              maxValueRect.bottom() + Ui::DesignSystem::layout().px12(),
                              maxValueRect.width(), maxValueRect.height());
    drawValue(minValueRect, d->min.value, d->min.label, u8"\U000f035d", d->min.color);
    //
    const QRectF avgValueRect(minValueRect.left(),
                              minValueRect.bottom() + Ui::DesignSystem::layout().px12(),
                              minValueRect.width(), minValueRect.height());
    drawValue(avgValueRect, d->avg.value, d->avg.label);
}
