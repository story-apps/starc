#include "stepper.h"

#include <ui/design_system/design_system.h>

#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>


class Stepper::Implementation
{
public:
    QColor inactiveStepNumberBackgroundColor = Qt::red;

    QVector<QString> steps;
    int currentStepIndex = 0;
    bool isFinished = false;
};


// ****


Stepper::Stepper(QWidget* _parent)
    : Widget(_parent),
      d(new Implementation)
{
    setFocusPolicy(Qt::StrongFocus);
}

void Stepper::setInactiveStepNumberBackgroundColor(const QColor& _color)
{
    if (d->inactiveStepNumberBackgroundColor == _color) {
        return;
    }

    d->inactiveStepNumberBackgroundColor = _color;
    update();
}

Stepper::~Stepper() = default;

void Stepper::addStep(const QString& _stepName)
{
    d->steps.append(_stepName);
    update();
}

void Stepper::setStepName(int _index, const QString& _name)
{
    if (0 > _index || _index >= d->steps.size()
        || d->steps[_index] == _name) {
        return;
    }

    d->steps[_index] = _name;
    updateGeometry();
    update();
}

void Stepper::setCurrentStep(int _index)
{
    if (0 > _index || _index >= d->steps.size()
        || d->currentStepIndex == _index) {
        return;
    }

    //
    // Обновим текущий индекс шага
    //
    const int previousStepIndex = d->currentStepIndex;
    d->currentStepIndex = _index;

    //
    // Надо бы перерисоваться
    //
    update();

    //
    // Уведомим клиентов о смене текущего шага
    //
    emit currentIndexChanged(d->currentStepIndex, previousStepIndex);
}

void Stepper::setFinished(bool _finished)
{
    if (d->isFinished == _finished) {
        return;
    }

    d->isFinished = _finished;
    update();
}

void Stepper::paintEvent(QPaintEvent* _event)
{
    Q_UNUSED(_event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setFont(Ui::DesignSystem::font().subtitle2());

    //
    // Заливаем фон
    //
    painter.fillRect(_event->rect(), backgroundColor());

    //
    // Если шагов нет, больши рисовать нечего
    //
    if (d->steps.isEmpty()) {
        return;
    }

    //
    // Определим полную область отрисовки шагов
    //
    const QRectF stepsRect(0, 0, width(), Ui::DesignSystem::stepper().height() * d->steps.size());
    //
    // ... и проверим есть ли в требуемой к перерисовке области шаги
    //
    if (!stepsRect.intersects(_event->rect())) {
        return;
    }

    //
    // Отрисовываем шаги
    //
    for (int stepIndex = 0; stepIndex < d->steps.size(); ++stepIndex) {
        const QRectF stepRect(0, stepIndex * Ui::DesignSystem::stepper().height(),
                              width(), Ui::DesignSystem::stepper().height());

        //
        // Проверяем нужно ли рисовать текущий шаг
        //
        if (!stepRect.intersects(_event->rect())) {
            continue;
        }

        //
        // Собственно отрисовка шага
        //

        //
        // Кружок
        //
        const QRectF stepNumberBackgroundRect(
                    QPointF(stepRect.left() + Ui::DesignSystem::stepper().margins().left(),
                            stepRect.top() + Ui::DesignSystem::stepper().margins().top()),
                    Ui::DesignSystem::stepper().iconSize());
        const QColor stepNumberBackgroundColor
                = !d->isFinished && stepIndex > d->currentStepIndex
                  ? d->inactiveStepNumberBackgroundColor
                  : Ui::DesignSystem::color().secondary();
        painter.setPen(stepNumberBackgroundColor);
        painter.setBrush(stepNumberBackgroundColor);
        painter.drawEllipse(stepNumberBackgroundRect);
        //
        // ... и номер внутри
        //
        painter.setPen(Ui::DesignSystem::color().onSecondary());
        if (!d->isFinished && stepIndex >= d->currentStepIndex) {
            const QString stepNumber = QString::number(stepIndex + 1);
            painter.drawText(stepNumberBackgroundRect, Qt::AlignCenter, stepNumber);
        }
        //
        // ... или галочка
        //
        else {
            painter.setFont(Ui::DesignSystem::font().iconsSmall());
            painter.drawText(stepNumberBackgroundRect, Qt::AlignCenter, "\uf12c");
        }

        //
        // Текст
        //
        painter.setPen(!d->isFinished && stepIndex > d->currentStepIndex
                       ? d->inactiveStepNumberBackgroundColor
                       : textColor());
        QFont textFont = Ui::DesignSystem::font().subtitle2();
        textFont.setWeight(stepIndex == d->currentStepIndex ? QFont::Medium : QFont::Normal);
        painter.setFont(textFont);
        const qreal stepTextRectX = stepNumberBackgroundRect.right() + Ui::DesignSystem::stepper().spacing();
        const QRectF stepTextRect(stepTextRectX, stepRect.top(),
                                  stepRect.right() - stepTextRectX - Ui::DesignSystem::stepper().margins().right(),
                                  stepRect.height());
        const QString stepText = QFontMetricsF(textFont).elidedText(d->steps.at(stepIndex), Qt::ElideRight, stepTextRect.width());
        painter.drawText(stepTextRect, Qt::AlignVCenter, stepText);

        //
        // декорации
        //
        // ... сверху
        //
        painter.setPen(QPen(d->inactiveStepNumberBackgroundColor, Ui::DesignSystem::stepper().pathWidth()));
        const qreal pathX = stepNumberBackgroundRect.center().x();
        if (stepIndex > 0) {
            painter.drawLine(QPointF(pathX, stepNumberBackgroundRect.top() - Ui::DesignSystem::stepper().pathSpacing()),
                             QPointF(pathX, stepRect.top()));
        }
        //
        // ... снизу
        //
        if (stepIndex < d->steps.size() - 1) {
            painter.drawLine(QPointF(pathX, stepNumberBackgroundRect.bottom() + Ui::DesignSystem::stepper().pathSpacing()),
                             QPointF(pathX, stepRect.bottom()));
        }
    }
}

void Stepper::mouseReleaseEvent(QMouseEvent* _event)
{
    for (int stepIndex = 0; stepIndex < d->steps.size(); ++stepIndex) {
        const QRectF stepRect(0, stepIndex * Ui::DesignSystem::stepper().height(),
                              width(), Ui::DesignSystem::stepper().height());
        if (stepRect.contains(_event->pos())) {
            setCurrentStep(stepIndex);
            return;
        }
    }
}
