#include "avatar.h"

#include <QPainter>
#include <QResizeEvent>


namespace Ui
{

class Avatar::Implementation
{
public:
    void prepreAvatarForPainting(const QSize& _size);

    QPixmap avatar;
    QPixmap preparedAvatar;
};

void Avatar::Implementation::prepreAvatarForPainting(const QSize& _size)
{
    if (preparedAvatar.size() == _size) {
        return;
    }

    preparedAvatar = avatar.scaled(_size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}


// ****


Avatar::Avatar(QWidget* _parent)
    : Widget(_parent),
      d(new Implementation)
{
    d->avatar = QPixmap(":/images/default-avatar");
}

Avatar::~Avatar() = default;

void Avatar::setAvatar(const QPixmap& _avatar)
{
    if (d->avatar == _avatar) {
        return;
    }

    d->avatar = _avatar;
    d->prepreAvatarForPainting(size());
    updateGeometry();
    update();
}

QSize Avatar::sizeHint() const
{
    return QSize(height(), height());
}

void Avatar::paintEvent(QPaintEvent* _event)
{
    Q_UNUSED(_event)

    QPainter painter(this);
    painter.drawPixmap(0, 0, d->preparedAvatar);
}

void Avatar::resizeEvent(QResizeEvent* _event)
{
    Widget::resizeEvent(_event);

    if (_event->size().height() != _event->size().width()) {
        setFixedWidth(_event->size().height());
        return;
    }

    d->prepreAvatarForPainting(_event->size());
}

} // namespace Ui
