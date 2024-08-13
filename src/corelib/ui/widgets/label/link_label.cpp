#include "link_label.h"

#include <ui/design_system/design_system.h>

#include <QDesktopServices>
#include <QMouseEvent>
#include <QUrl>


class AbstractLinkLabel::Implementation
{
public:
    QUrl link;
};


// ****


AbstractLinkLabel::AbstractLinkLabel(QWidget* _parent)
    : AbstractLabel(_parent)
    , d(new Implementation)
{
    setCursor(Qt::PointingHandCursor);
    setClickable(true);

    connect(this, &AbstractLinkLabel::clicked, this, [this] {
        if (!d->link.isEmpty()) {
            QDesktopServices::openUrl(d->link);
        }
    });
}

AbstractLinkLabel::~AbstractLinkLabel() = default;

void AbstractLinkLabel::setLink(const QUrl& _link)
{
    d->link = _link;
}


// ****


Body1LinkLabel::Body1LinkLabel(QWidget* _parent)
    : AbstractLinkLabel(_parent)
{
}

const QFont& Body1LinkLabel::textFontImpl() const
{
    return Ui::DesignSystem::font().body1();
}


// ****


Body2LinkLabel::Body2LinkLabel(QWidget* _parent)
    : AbstractLinkLabel(_parent)
{
}

const QFont& Body2LinkLabel::textFontImpl() const
{
    return Ui::DesignSystem::font().body2();
}


// ****


Subtitle2LinkLabel::Subtitle2LinkLabel(QWidget* _parent)
    : AbstractLinkLabel(_parent)
{
}

const QFont& Subtitle2LinkLabel::textFontImpl() const
{
    return Ui::DesignSystem::font().subtitle2();
}
