#include "comparison_toolbar.h"

#include "floating_toolbar_animator.h"

#include <ui/design_system/design_system.h>
#include <utils/helpers/color_helper.h>

#include <QAction>
#include <QEvent>
#include <QFontMetrics>
#include <QTimer>

namespace Ui {

ComparisonToolbar::ComparisonToolbar(FloatingToolBar* _toolbar, FloatingToolbarAnimator* _animator,
                                     const QString& _title, const std::function<void()>& _close)
    : FloatingToolBar(_toolbar->parentWidget())
    , m_toolbar(_toolbar)
    , m_back(new QAction(this))
    , m_title(new QAction(this))
    , m_fullTitle(_title)
{
    setObjectName(QStringLiteral("comparisonToolbar"));
    setCurtain(true);
    m_back->setIconText(u8"\U000F004D");
    addAction(m_back);
    addAction(m_title);
    connect(m_back, &QAction::triggered, this, _close);
    m_toolbar->installEventFilter(this);
    parentWidget()->installEventFilter(this);
    hide();
    updateTranslations();

    // Дождёмся компоновки редактора перед тем же переходом, который используется для поиска.
    QTimer::singleShot(0, this, [this, _animator] {
        updateGeometryAndTitle();
        setBackgroundColor(ColorHelper::nearby(DesignSystem::color().background()));
        setTextColor(DesignSystem::color().onBackground());
        _animator->switchToolbars(u8"\U000F1492", {}, m_toolbar, this);
    });
}

bool ComparisonToolbar::eventFilter(QObject* _watched, QEvent* _event)
{
    if (_event->type() == QEvent::Resize || _event->type() == QEvent::Move) {
        updateGeometryAndTitle();
    } else if (_watched == m_toolbar && _event->type() == QEvent::Show) {
        m_toolbar->hide();
    }
    return FloatingToolBar::eventFilter(_watched, _event);
}

void ComparisonToolbar::updateGeometryAndTitle()
{
    const auto& style = DesignSystem::floatingToolBar();
    const int availableWidth = qMax(0, parentWidget()->width());
    const QFontMetrics metrics(DesignSystem::font().subtitle2());
    const int padding = style.shadowMargins().left() + style.shadowMargins().right()
        + style.margins().left() + style.margins().right() + style.iconSize().width()
        + style.spacing() * 2;
    const int textWidth
        = qMax(0, qMin(metrics.horizontalAdvance(m_fullTitle), availableWidth - padding));
    m_title->setText(metrics.elidedText(m_fullTitle, Qt::ElideMiddle, textWidth));
    m_title->setToolTip(m_fullTitle);
    setActionCustomWidth(m_title, textWidth);
    resize(qMin(sizeHint().width(), availableWidth), sizeHint().height());
    move(qMax(0, m_toolbar->geometry().center().x() - width() / 2), m_toolbar->y());
}

void ComparisonToolbar::updateTranslations()
{
    m_back->setText(tr("Back to draft"));
}

void ComparisonToolbar::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    FloatingToolBar::designSystemChangeEvent(_event);
    setBackgroundColor(ColorHelper::nearby(DesignSystem::color().background()));
    setTextColor(DesignSystem::color().onBackground());
    updateGeometryAndTitle();
}

} // namespace Ui
