#include "button.h"

#include <ui/design_system/design_system.h>

#include <utils/helpers/image_helper.h>

#include <QPainter>
#include <QPaintEvent>


class Button::Implementation
{
public:
    QString text;
    bool isContained = false;
};

Button::Button(QWidget* _parent)
    : Widget(_parent),
      d(new Implementation)
{
    designSysemChangeEvent(nullptr);
}

Button::~Button() = default;

void Button::setText(const QString& _text)
{
    if (d->text == _text) {
        return;
    }

    d->text = _text;
    updateGeometry();
    update();
}

void Button::setContained(bool _contained)
{
    if (d->isContained == _contained) {
        return;
    }

    d->isContained = _contained;
    update();
}

QSize Button::sizeHint() const
{
    const qreal width = Ui::DesignSystem::button().shadowMargins().top()
                        + std::max(Ui::DesignSystem::button().minimumWidth(),
                                   Ui::DesignSystem::button().margins().left()
                                   + QFontMetrics(Ui::DesignSystem::font().button()).horizontalAdvance(d->text)
                                   + Ui::DesignSystem::button().margins().right())
                        + Ui::DesignSystem::button().shadowMargins().bottom();
    return QSize(static_cast<int>(width), static_cast<int>(Ui::DesignSystem::button().height()));
}

void Button::paintEvent(QPaintEvent* _event)
{
    Q_UNUSED(_event);

    const QRect backgroundRect = rect().marginsRemoved(Ui::DesignSystem::button().shadowMargins().toMargins());
    if (!backgroundRect.isValid()) {
        return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    //
    // Заливаем фон
    //
    if (d->isContained) {
        QPixmap image(backgroundRect.size());
        image.fill(Qt::transparent);
        QPainter imagePainter(&image);
        imagePainter.setPen(Qt::NoPen);
        imagePainter.setBrush(backgroundColor());
        imagePainter.drawRoundedRect(QRect({0,0}, image.size()),
                                     Ui::DesignSystem::button().borderRadius(),
                                     Ui::DesignSystem::button().borderRadius());
        const QPixmap shadow
                = ImageHelper::dropShadow(image,
                                          QMarginsF(Ui::DesignSystem::button().shadowMargins().left(),
                                                    Ui::DesignSystem::button().shadowMargins().top(),
                                                    Ui::DesignSystem::button().shadowMargins().right(),
                                                    Ui::DesignSystem::button().minimumShadowHeight()),
                                          Ui::DesignSystem::button().shadowBlurRadius(),
                                          Ui::DesignSystem::color().shadow());
        painter.drawPixmap(0, 0, shadow);
        painter.drawPixmap(backgroundRect.topLeft(), image);
    }

    //
    // Рисуем текст
    //
    painter.setFont(Ui::DesignSystem::font().button());
    painter.setPen(textColor());
    painter.drawText(contentsRect(), Qt::AlignCenter, d->text);
}

void Button::mouseReleaseEvent(QMouseEvent* _event)
{
    Q_UNUSED(_event);
    emit clicked();
}

void Button::designSysemChangeEvent(DesignSystemChangeEvent* _event)
{
    Q_UNUSED(_event);

    setContentsMargins(Ui::DesignSystem::button().shadowMargins().toMargins());
    updateGeometry();
    update();
}
