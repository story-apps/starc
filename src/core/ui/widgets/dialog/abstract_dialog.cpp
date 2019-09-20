#include "abstract_dialog.h"

#include "dialog_content.h"

#include <ui/design_system/design_system.h>

#include <ui/widgets/label/label.h>

#include <QGridLayout>
#include <QPainter>
#include <QPaintEvent>
#include <QTimer>
#include <QVariantAnimation>


class AbstractDialog::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    /**
     * @brief Анимировать отображение
     */
    void animateShow(const QPoint& _pos);

    /**
     * @brief Анимировать скрытие
     */
    void animateHide();


    DialogContent* content = nullptr;
    H6Label* title = nullptr;
    QGridLayout* contentsLayout = nullptr;

    QVariantAnimation opacityAnimation;
    QVariantAnimation contentPosAnimation;
    QPixmap contentPixmap;
};

AbstractDialog::Implementation::Implementation(QWidget* _parent)
    : content(new DialogContent(_parent)),
      title(new H6Label(_parent)),
      contentsLayout(new QGridLayout)
{
    content->setBackgroundColor(Qt::white);
    content->setTextColor(Qt::black);

    title->setBackgroundColor(Qt::white);
    title->setTextColor(Qt::black);

    QVBoxLayout* layout = new QVBoxLayout(content);
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(title);
    layout->addLayout(contentsLayout);

    opacityAnimation.setDuration(220);
    opacityAnimation.setEasingCurve(QEasingCurve::OutQuad);
    contentPosAnimation.setDuration(160);
    contentPosAnimation.setEasingCurve(QEasingCurve::OutQuad);
}

void AbstractDialog::Implementation::animateShow(const QPoint& _pos)
{
    //
    // Прозрачность
    //
    opacityAnimation.setStartValue(0.0);
    opacityAnimation.setEndValue(1.0);
    opacityAnimation.start();

    //
    // Позиция контента
    //
    const qreal movementDelta = 40;
    contentPosAnimation.setStartValue(_pos - QPoint(0, static_cast<int>(movementDelta)));
    contentPosAnimation.setEndValue(_pos);
    contentPosAnimation.start();
}

void AbstractDialog::Implementation::animateHide()
{
    opacityAnimation.setStartValue(1.0);
    opacityAnimation.setEndValue(0.0);
    opacityAnimation.start();
}


// ****


AbstractDialog::AbstractDialog(QWidget* _parent)
    : QWidget(_parent),
      d(new Implementation(this))
{
    Q_ASSERT(_parent);

    _parent->installEventFilter(this);

    QGridLayout* layout = new QGridLayout(this);
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->setRowStretch(0, 1);
    layout->setColumnStretch(0, 1);
    layout->addWidget(d->content, 1, 1);
    layout->setRowStretch(2, 1);
    layout->setColumnStretch(2, 1);

    connect(&d->opacityAnimation, &QVariantAnimation::valueChanged, this, [this] { update(); });
    connect(&d->contentPosAnimation, &QVariantAnimation::valueChanged, this, [this] { update(); });
    connect(&d->contentPosAnimation, &QVariantAnimation::finished, d->content, &DialogContent::show);
}

void AbstractDialog::showDialog()
{
    if (parentWidget() == nullptr) {
        return;
    }

    //
    // Конфигурируем геометрию диалога
    //
    move(0, 0);
    resize(parentWidget()->size());

    //
    // Сохраняем изображение контента и прячем сам виджет
    //
    d->contentPixmap = d->content->grab(QRect({0, 0}, d->content->sizeHint()));
    d->content->hide();

    //
    // Запускаем отображение
    //
    d->animateShow(d->content->pos());
    setFocus();
    show();
}

void AbstractDialog::hideDialog()
{
    d->content->hide();
    d->animateHide();
    QTimer::singleShot(d->opacityAnimation.duration(), this, &AbstractDialog::hide);
}

void AbstractDialog::setTitle(const QString& _title)
{
    d->title->setText(_title);
}

QGridLayout* AbstractDialog::contentsLayout() const
{
    return d->contentsLayout;
}

void AbstractDialog::paintEvent(QPaintEvent* _event)
{
    QPainter painter(this);
    painter.setOpacity(d->opacityAnimation.currentValue().toReal());
    painter.fillRect(_event->rect(), Ui::DesignSystem::color().shadow());

    if (!d->content->isVisible()) {
        painter.drawPixmap(d->contentPosAnimation.currentValue().toPoint(), d->contentPixmap);
    }
}

bool AbstractDialog::eventFilter(QObject* _watched, QEvent* _event)
{
    if (_watched == parentWidget()
        && _event->type() == QEvent::Resize) {
        auto resizeEvent = static_cast<QResizeEvent*>(_event);
        resize(resizeEvent->size());
    }

    return QWidget::eventFilter(_watched, _event);
}

void AbstractDialog::show()
{
    QWidget::show();
}

void AbstractDialog::hide()
{
    QWidget::hide();
}

AbstractDialog::~AbstractDialog() = default;
