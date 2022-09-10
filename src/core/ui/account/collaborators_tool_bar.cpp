#include "collaborators_tool_bar.h"

#include <domain/starcloud_api.h>
#include <ui/design_system/design_system.h>
#include <ui/modules/avatar_generator/avatar_generator.h>
#include <utils/helpers/color_helper.h>

#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>


namespace Ui {

class CollaboratorsToolBar::Implementation
{
public:
    explicit Implementation(CollaboratorsToolBar* _q);

    qreal avatarSize() const;
    qreal spacing() const;

    void updateSize();
    void updatePosition();


    CollaboratorsToolBar* q = nullptr;

    QVector<Domain::CursorInfo> collaborators;
};

CollaboratorsToolBar::Implementation::Implementation(CollaboratorsToolBar* _q)
    : q(_q)
{
}

qreal CollaboratorsToolBar::Implementation::avatarSize() const
{
    return Ui::DesignSystem::layout().px(40);
}

qreal CollaboratorsToolBar::Implementation::spacing() const
{
    return Ui::DesignSystem::layout().px8();
}

void CollaboratorsToolBar::Implementation::updateSize()
{
    q->setFixedSize(q->sizeHint());
}

void CollaboratorsToolBar::Implementation::updatePosition()
{
    auto parentWidget = q->parentWidget();
    Q_ASSERT(parentWidget);

    q->move(q->isLeftToRight()
                ? parentWidget->width() - q->width() - Ui::DesignSystem::layout().px24()
                : Ui::DesignSystem::layout().px24(),
            parentWidget->height() - q->height() - Ui::DesignSystem::layout().px24());
}


// ****


CollaboratorsToolBar::CollaboratorsToolBar(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation(this))
{
    _parent->installEventFilter(this);

    hide();

    connect(AvatarGenerator::instance(), &AvatarGenerator::avatarUpdated, this,
            [this](const QString& _email) {
                for (const auto& collaborator : std::as_const(d->collaborators)) {
                    if (collaborator.email == _email) {
                        update();
                        break;
                    }
                }
            });
}

CollaboratorsToolBar::~CollaboratorsToolBar() = default;

void CollaboratorsToolBar::setCollaborators(const QVector<Domain::CursorInfo>& _collaborators)
{
    if (d->collaborators.isEmpty()) {
        d->collaborators = _collaborators;
    } else if (_collaborators.isEmpty()) {
        d->collaborators.clear();
    } else {
        //
        // Если список не пуст, то стараемся обновить так, чтобы аватарки не скакали
        //
        // ... сперва удалим тех, кто отвалился
        //
        for (int index = 0; index < d->collaborators.size(); ++index) {
            bool contains = false;
            for (const auto& newCollaborator : _collaborators) {
                if (newCollaborator.cursorId == d->collaborators[index].cursorId) {
                    contains = true;
                    break;
                }
            }
            if (!contains) {
                d->collaborators.removeAt(index);
                --index;
            }
        }
        //
        // ... а затем обновляем тех, кто уже был и добавляем новых
        //
        for (const auto& newCollaborator : _collaborators) {
            bool contains = false;
            for (auto& collaborator : d->collaborators) {
                if (collaborator.cursorId == newCollaborator.cursorId) {
                    contains = true;
                    collaborator = newCollaborator;
                    break;
                }
            }
            if (!contains) {
                d->collaborators.append(newCollaborator);
            }
        }
    }

    if (d->collaborators.isEmpty()) {
        hide();
    } else {
        raise();
        d->updateSize();
        d->updatePosition();
        update();
        show();
    }
}

QSize CollaboratorsToolBar::sizeHint() const
{
    const auto size = d->avatarSize();
    const auto spacing = d->spacing();
    return QSize(d->collaborators.size() * size + (d->collaborators.size() - 1) * spacing, size);
}

bool CollaboratorsToolBar::eventFilter(QObject* _watched, QEvent* _event)
{
    if (_watched == parent() && _event->type() == QEvent::Resize) {
        d->updatePosition();
    }

    return Widget::eventFilter(_watched, _event);
}

void CollaboratorsToolBar::paintEvent(QPaintEvent* _event)
{
    QPainter painter(this);

    //
    // Заливаем фон прозрачным
    //
    painter.fillRect(_event->rect(), Qt::transparent);

    //
    // Рисуем аватарки
    //
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    qreal x = 0.0;
    const auto size = d->avatarSize();
    const auto spacing = d->spacing();
    for (const auto& collaborator : std::as_const(d->collaborators)) {
        //
        // ... цветной кружок под аватаркой
        //
        const QRectF avatarBackgroundRect(x, 0.0, size, size);
        painter.setBrush(ColorHelper::forText(collaborator.name));
        painter.drawEllipse(avatarBackgroundRect);
        //
        // ... сама аватарка
        //
        const auto avatarRect = avatarBackgroundRect.adjusted(
            Ui::DesignSystem::layout().px2(), Ui::DesignSystem::layout().px2(),
            -Ui::DesignSystem::layout().px2(), -Ui::DesignSystem::layout().px2());
        const auto avatar = AvatarGenerator::avatar(collaborator.name, collaborator.email);
        painter.drawPixmap(avatarRect, avatar, avatar.rect());

        x += size + spacing;
    }
}

void CollaboratorsToolBar::mouseReleaseEvent(QMouseEvent* _event)
{
    if (rect().contains(_event->pos())) {
        return;
    }

    qreal x = 0.0;
    const auto size = d->avatarSize();
    const auto spacing = d->spacing();
    for (const auto& collaborator : std::as_const(d->collaborators)) {
        if (x <= _event->pos().x() && _event->pos().x() <= x + size) {
            emit collaboratorClicked(collaborator.cursorId);
            return;
        }

        x += size + spacing;
    }
}

void CollaboratorsToolBar::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    d->updateSize();
}

} // namespace Ui
