#include "text_field.h"

#include <include/custom_events.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/animations/click_animation.h>
#include <ui/widgets/context_menu/context_menu.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/icon_helper.h>
#include <utils/helpers/image_helper.h>
#include <utils/helpers/text_helper.h>

#include <QAction>
#include <QApplication>
#include <QEvent>
#include <QKeyEvent>
#include <QMimeData>
#include <QPainter>
#include <QPainterPath>
#include <QTextFrame>
#include <QToolTip>
#include <QVariantAnimation>
#include <QtMath>


class TextField::Implementation
{
public:
    explicit Implementation(TextField* _q);

    /**
     * @brief Переконфигурировать виджет при смене параметров дизайн системы
     */
    void reconfigure();

    /**
     * @brief Анимировать лейбл
     */
    /** @{ */
    void animateLabelToTop();
    void animateLabelToBottom();
    /** @} */

    /**
     * @brief Закончить анимации
     */
    void finishAnimation();
    void finishAnimationIfInvisible();

    /**
     * @brief Получить отступы в зависимости от настроек
     */
    QMarginsF contentMargins() const;
    QMarginsF margins() const;

    /**
     * @brief Определить область декорирования
     */
    /** @{ */
    QRectF decorationRect() const;
    QRectF decorationRectInFocus() const;
    /** @} */

    /**
     * @brief Определяем смещение для картинки
     */
    int trailingIconOffset() const;

    /**
     * @brief Стандартный область для текстового поля
     */
    QRectF inputTextRect() const;

    /**
     * @brief Определить область для отрисовки лейбла
     */
    QRectF labelRect(const qreal fontHeight) const;

    /**
     * @brief Определить область для отрисовки суффикса
     */
    QRectF suffixRect() const;

    /**
     * @brief Определить область для отрисовки иконки
     */
    QRectF trailingIconRect() const;


    TextField* q = nullptr;

    QColor backgroundInactiveColor = kDefaultWidgetColor;
    QColor backgroundActiveColor = kDefaultWidgetColor;
    QColor textColor = kDefaultWidgetColor;
    QColor textDisabledColor = kDefaultWidgetColor;

    QString label;
    QString helper;
    QString error;
    QString placeholder;
    QString suffix;

    QString trailingIcon;
    QColor trailingIconColor;
    QString trailingIconToolTip;

    bool isContextMenuShown = false;
    bool isPasswordModeEnabled = false;
    bool isEnterMakesNewLine = false;
    bool isUnderlineDecorationVisible = true;
    bool isTitleVisible = true;
    bool isDefaultMarginsEnabled = true;
    QMarginsF customMargins;

    QVariantAnimation labelColorAnimation;
    QVariantAnimation labelFontSizeAnimation;
    QVariantAnimation labelTopLeftAnimation;
    QVariantAnimation decorationAnimation;

    /**
     * @brief Декорация иконки при клике
     */
    ClickAnimation iconDecorationAnimation;
};

TextField::Implementation::Implementation(TextField* _q)
    : q(_q)
{
    labelColorAnimation.setDuration(200);
    labelColorAnimation.setEasingCurve(QEasingCurve::OutQuad);
    labelFontSizeAnimation.setDuration(200);
    labelFontSizeAnimation.setEasingCurve(QEasingCurve::OutQuad);
    labelTopLeftAnimation.setDuration(200);
    labelTopLeftAnimation.setEasingCurve(QEasingCurve::OutQuad);
    decorationAnimation.setDuration(200);
    decorationAnimation.setEasingCurve(QEasingCurve::OutQuad);
}

void TextField::Implementation::reconfigure()
{
    QSignalBlocker signalBlocker(q);

    q->setFont(Ui::DesignSystem::font().body1());

    QPalette palette = q->palette();
    palette.setColor(QPalette::Base, Qt::transparent);
    palette.setColor(QPalette::Text, textColor);
    palette.setColor(QPalette::Disabled, QPalette::Text, textDisabledColor);
    palette.setColor(QPalette::Highlight, Ui::DesignSystem::color().secondary());
    palette.setColor(QPalette::HighlightedText, Ui::DesignSystem::color().onSecondary());
    q->setPalette(palette);

    QMarginsF frameMargins;
    frameMargins.setLeft(contentMargins().left() + margins().left()
                         + (trailingIcon.isEmpty() || q->isLeftToRight() ? 0 : trailingIconOffset())
                         + (!suffix.isEmpty() && q->isRightToLeft()
                                ? suffixRect().width() + Ui::DesignSystem::textField().spacing()
                                : 0));
    frameMargins.setTop(contentMargins().top() + margins().top());
    frameMargins.setRight(
        contentMargins().right() + margins().right()
        + (trailingIcon.isEmpty() || q->isRightToLeft() ? 0 : trailingIconOffset()));
    frameMargins.setBottom(contentMargins().bottom() + margins().bottom());
    q->setPageMarginsPx(frameMargins);

    //
    // Переконфигурируем цвет и размер лейблов
    //
    if (q->hasFocus()) {
        labelColorAnimation.setEndValue(Ui::DesignSystem::color().secondary());
    } else {
        labelColorAnimation.setEndValue(textDisabledColor);
        if (q->text().isEmpty() && q->placeholderText().isEmpty()) {
            animateLabelToBottom();
        } else {
            animateLabelToTop();
        }
    }
    finishAnimationIfInvisible();

    //
    // Анимация лейбла
    //
    const int offset = trailingIconOffset();
    const QPointF labelNewTopLeft = Ui::DesignSystem::textField().labelTopLeft()
        + QPointF(contentMargins().left() + (q->isRightToLeft() ? offset : 0),
                  contentMargins().top());
    const qreal backgroundHeight
        = q->sizeHint().height() - contentMargins().top() - contentMargins().bottom();
    const QPointF labelCurrentTopLeft(
        labelNewTopLeft.x(),
        contentMargins().top()
            + (backgroundHeight - TextHelper::fineLineSpacing(Ui::DesignSystem::font().body1()))
                / 2);
    labelTopLeftAnimation.setStartValue(labelCurrentTopLeft);
    labelTopLeftAnimation.setEndValue(labelNewTopLeft);

    iconDecorationAnimation.setRadiusInterval(Ui::DesignSystem::textField().iconSize().height()
                                                  / 2.0,
                                              Ui::DesignSystem::radioButton().height() / 2.0);
    q->document()->clearUndoRedoStacks();
}

void TextField::Implementation::animateLabelToTop()
{
    if (labelFontSizeAnimation.startValue().toInt() == Ui::DesignSystem::font().body1().pixelSize()
        && labelFontSizeAnimation.endValue().toInt()
            == Ui::DesignSystem::font().caption().pixelSize()
        && labelTopLeftAnimation.direction() == QVariantAnimation::Forward) {
        return;
    }

    labelFontSizeAnimation.setStartValue(Ui::DesignSystem::font().body1().pixelSize());
    labelFontSizeAnimation.setEndValue(Ui::DesignSystem::font().caption().pixelSize());
    labelFontSizeAnimation.start();

    labelTopLeftAnimation.stop();

    labelTopLeftAnimation.setDirection(QVariantAnimation::Forward);
    labelTopLeftAnimation.start();
}

void TextField::Implementation::animateLabelToBottom()
{
    if (labelFontSizeAnimation.startValue().toInt()
            == Ui::DesignSystem::font().caption().pixelSize()
        && labelFontSizeAnimation.endValue().toInt() == Ui::DesignSystem::font().body1().pixelSize()
        && labelTopLeftAnimation.direction() == QVariantAnimation::Backward) {
        return;
    }

    labelFontSizeAnimation.setStartValue(Ui::DesignSystem::font().caption().pixelSize());
    labelFontSizeAnimation.setEndValue(Ui::DesignSystem::font().body1().pixelSize());
    labelFontSizeAnimation.start();

    labelTopLeftAnimation.setDirection(QVariantAnimation::Backward);
    labelTopLeftAnimation.start();
}

void TextField::Implementation::finishAnimation()
{
    labelFontSizeAnimation.setCurrentTime(labelFontSizeAnimation.direction()
                                                  == QVariantAnimation::Forward
                                              ? labelFontSizeAnimation.duration()
                                              : 0);
    labelTopLeftAnimation.setCurrentTime(labelTopLeftAnimation.direction()
                                                 == QVariantAnimation::Forward
                                             ? labelTopLeftAnimation.duration()
                                             : 0);
}

void TextField::Implementation::finishAnimationIfInvisible()
{
    //
    // Если лейбл скрыт, то выполнять всю анимацию ни к чему
    //
    if (!q->isVisible()) {
        finishAnimation();
    }
}

QMarginsF TextField::Implementation::contentMargins() const
{
    return isDefaultMarginsEnabled ? Ui::DesignSystem::textField().contentsMargins()
                                   : customMargins;
}

QMarginsF TextField::Implementation::margins() const
{
    return Ui::DesignSystem::textField().margins(isTitleVisible);
}

QRectF TextField::Implementation::decorationRect() const
{
    qreal top
        = q->height() - Ui::DesignSystem::textField().underlineHeight() - contentMargins().bottom();
    if (!helper.isEmpty() || !error.isEmpty()) {
        top -= Ui::DesignSystem::textField().helperHeight();
    }
    return QRectF(contentMargins().left(), top,
                  q->width() - contentMargins().left() - contentMargins().right(),
                  Ui::DesignSystem::textField().underlineHeight());
}

QRectF TextField::Implementation::decorationRectInFocus() const
{
    qreal top = q->height() - Ui::DesignSystem::textField().underlineHeightInFocus()
        - contentMargins().bottom();
    if (!helper.isEmpty() || !error.isEmpty()) {
        top -= Ui::DesignSystem::textField().helperHeight();
    }
    return QRectF(contentMargins().left(), top,
                  q->width() - contentMargins().left() - contentMargins().right(),
                  Ui::DesignSystem::textField().underlineHeightInFocus());
}

int TextField::Implementation::trailingIconOffset() const
{
    return trailingIcon.isEmpty() ? 0
                                  : Ui::DesignSystem::textField().iconSize().width()
            + Ui::DesignSystem::textField().spacing();
}

QRectF TextField::Implementation::inputTextRect() const
{
    const int offset = trailingIconOffset();
    QPointF labelTopLeft = QPointF(Ui::DesignSystem::textField().labelTopLeft().x(), 0)
        + QPointF(contentMargins().left() + (q->isRightToLeft() ? offset : 0),
                  contentMargins().top() + margins().top());
    const qreal additionalBottomMargin = !error.isEmpty() || !helper.isEmpty()
        ? Ui::DesignSystem::textField().helperHeight()
        : 0.0;

    return QRectF(labelTopLeft,
                  QSizeF(q->width() - labelTopLeft.x() - contentMargins().left() - margins().right()
                             - (q->isRightToLeft() ? 0 : offset),
                         q->height() - contentMargins().top() - margins().top()
                             - contentMargins().bottom() - margins().bottom()
                             - additionalBottomMargin));
}

QRectF TextField::Implementation::labelRect(const qreal fontHeight) const
{
    QPointF labelTopLeft;
    if (!labelTopLeftAnimation.currentValue().isNull()) {
        labelTopLeft = labelTopLeftAnimation.currentValue().toPointF();
    } else if (!q->hasFocus() && q->text().isEmpty() && placeholder.isEmpty()) {
        labelTopLeft = labelTopLeftAnimation.startValue().toPointF();
    } else {
        labelTopLeft = labelTopLeftAnimation.endValue().toPointF();
    }

    const int offset = trailingIconOffset();
    return QRectF(labelTopLeft,
                  QSizeF(q->width() - labelTopLeft.x() - contentMargins().right()
                             - margins().right() - (q->isRightToLeft() ? 0 : offset),
                         fontHeight));
}

QRectF TextField::Implementation::suffixRect() const
{
    QPointF topLeft;
    const qreal suffixWidth = TextHelper::fineTextWidthF(suffix, Ui::DesignSystem::font().body1());

    if (q->isRightToLeft()) {
        topLeft = QPointF(contentMargins().right() + margins().right(), margins().top());
    } else {
        topLeft = QPointF(q->width() - contentMargins().right() - margins().right() - suffixWidth,
                          margins().top());
    }
    const qreal additionalBottomMargin = !error.isEmpty() || !helper.isEmpty()
        ? Ui::DesignSystem::textField().helperHeight()
        : 0.0;

    return QRectF(
        topLeft,
        QSizeF(suffixWidth,
               q->height() - margins().top() - margins().bottom() - additionalBottomMargin));
}

QRectF TextField::Implementation::trailingIconRect() const
{
    if (trailingIcon.isEmpty()) {
        return {};
    }

    int topLeft = 0;
    if (q->isRightToLeft()) {
        topLeft = contentMargins().left() + margins().left();
    } else {
        topLeft = q->width() - contentMargins().right() - margins().right()
            - Ui::DesignSystem::textField().iconSize().width();
    }
    return QRectF(
        QPointF(topLeft, contentMargins().top() + Ui::DesignSystem::textField().iconTop()),
        Ui::DesignSystem::textField().iconSize());
}


// ****


TextField::TextField(QWidget* _parent)
    : BaseTextEdit(_parent)
    , d(new Implementation(this))
{
    setAttribute(Qt::WA_Hover);
    setAddSpaceToBottom(false);
    setContextMenuPolicy(Qt::CustomContextMenu);
    setFrameShape(QFrame::NoFrame);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setPageSpacing(0);
    setTabChangesFocus(true);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    viewport()->setMouseTracking(true);

    QSizePolicy policy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    policy.setHeightForWidth(true);
    setSizePolicy(policy);

    TextField::reconfigure();

    connect(this, &TextField::customContextMenuRequested, this, [this](const QPoint& _position) {
        auto menu = createContextMenu(_position);
        if (!menu) {
            return;
        }

        //
        // Отслеживаем отображение контекстного меню, чтобы не мельтишить анимацией лейбла,
        // т.к. после скрытия контекстного меню фокус опять вернётся к полю ввода, безопасно
        // оставить лейбл в активированном состоянии (поднятым кверху)
        //
        d->isContextMenuShown = true;
        connect(menu, &ContextMenu::disappeared, this, [this] { d->isContextMenuShown = false; });

        auto actions = menu->actions().toVector();
        //
        // Убираем возможности форматирования (пока)
        //
        if (isMispelledWordUnderCursor(_position)) {
            actions.takeAt(1)->deleteLater();
        } else {
            actions.takeFirst()->deleteLater();
            actions.first()->setSeparator(false);
        }
        menu->setActions(actions);
        menu->showContextMenu(mapToGlobal(_position));
    });
    connect(&d->labelColorAnimation, &QVariantAnimation::valueChanged, this,
            qOverload<>(&TextField::update));
    connect(&d->labelFontSizeAnimation, &QVariantAnimation::valueChanged, this,
            qOverload<>(&TextField::update));
    connect(&d->labelTopLeftAnimation, &QVariantAnimation::valueChanged, this,
            qOverload<>(&TextField::update));
    connect(&d->decorationAnimation, &QVariantAnimation::valueChanged, this,
            qOverload<>(&TextField::update));
    connect(&d->iconDecorationAnimation, &ClickAnimation::valueChanged, this,
            qOverload<>(&TextField::update));
    connect(document(), &QTextDocument::contentsChange, this, &TextField::updateGeometry);
}

TextField::~TextField() = default;

void TextField::setBackgroundColor(const QColor& _color)
{
    if (d->backgroundInactiveColor == _color) {
        return;
    }

    d->backgroundInactiveColor = _color;
    d->backgroundInactiveColor.setAlphaF(
        Ui::DesignSystem::textField().backgroundInactiveColorOpacity());
    d->backgroundActiveColor = _color;
    d->backgroundActiveColor.setAlphaF(
        Ui::DesignSystem::textField().backgroundActiveColorOpacity());
    reconfigure();
}

void TextField::setTextColor(const QColor& _color)
{
    if (d->textColor == _color) {
        return;
    }

    d->textColor = _color;
    d->textDisabledColor = d->textColor;
    d->textDisabledColor.setAlphaF(Ui::DesignSystem::disabledTextOpacity());
    reconfigure();
}

QColor TextField::textColor() const
{
    return d->textColor;
}

void TextField::setLabel(const QString& _text)
{
    if (d->label == _text) {
        return;
    }

    d->label = _text;
    update();
}

void TextField::setHelper(const QString& _text)
{
    if (d->helper == _text) {
        return;
    }

    d->helper = _text;
    updateGeometry();
    update();
}

void TextField::setError(const QString& _text)
{
    if (d->error == _text) {
        return;
    }

    d->error = _text;
    updateGeometry();
    update();
}

void TextField::clearError()
{
    setError({});
}

void TextField::setPlaceholderText(const QString& _placeholder)
{
    if (d->placeholder == _placeholder) {
        return;
    }

    d->placeholder = _placeholder;
    updateGeometry();
    if (text().isEmpty()) {
        update();
    }

    //
    // Анимируем только в случае, когда текста не было и его установили
    //
    if (text().isEmpty() && !d->placeholder.isEmpty()) {
        d->animateLabelToTop();
        d->finishAnimationIfInvisible();
    }
}

QString TextField::placeholderText() const
{
    return d->placeholder;
}

void TextField::setSuffix(const QString& _suffix)
{
    if (d->suffix == _suffix) {
        return;
    }

    d->suffix = _suffix;
    reconfigure();
    updateGeometry();
    update();
}

QString TextField::text() const
{
    return toPlainText();
}

void TextField::setText(const QString& _text)
{
    if (_text.isEmpty()) {
        clear();
        return;
    }

    const bool needAnimate = text().isEmpty() && !_text.isEmpty();

    QTextCursor cursor = textCursor();
    cursor.beginEditBlock();
    cursor.select(QTextCursor::Document);
    cursor.insertText(_text);
    cursor.endEditBlock();
    document()->clearUndoRedoStacks();

    //
    // Анимируем только в случае, когда текста не было и его установили
    //
    if (needAnimate) {
        d->animateLabelToTop();
        d->finishAnimationIfInvisible();
    }
}

QString TextField::selectedText() const
{
    return textCursor().selectedText();
}

void TextField::setTrailingIcon(const QString& _icon)
{
    if (d->trailingIcon == _icon) {
        return;
    }

    const bool needReconfigure = d->trailingIcon.isEmpty() != _icon.isEmpty();

    d->trailingIcon = _icon;
    if (needReconfigure) {
        reconfigure();
    }
    update();
}

void TextField::setTrailingIconColor(const QColor& _color)
{
    if (d->trailingIconColor == _color) {
        return;
    }

    d->trailingIconColor = _color;
    update();
}

QColor TextField::trailingIconColor() const
{
    return d->trailingIconColor;
}

void TextField::setTrailingIconToolTip(const QString& _toolTip)
{
    d->trailingIconToolTip = _toolTip;
}

void TextField::setPasswordModeEnabled(bool _enable)
{
    if (d->isPasswordModeEnabled == _enable) {
        return;
    }

    d->isPasswordModeEnabled = _enable;
    update();
}

bool TextField::isPasswordModeEnabled() const
{
    return d->isPasswordModeEnabled;
}

void TextField::setUnderlineDecorationVisible(bool _visible)
{
    if (d->isUnderlineDecorationVisible == _visible) {
        return;
    }

    d->isUnderlineDecorationVisible = _visible;
    update();
}

void TextField::setLabelVisible(bool _visible)
{
    if (d->isTitleVisible == _visible) {
        return;
    }

    d->isTitleVisible = _visible;
    update();
}

bool TextField::isDefaultMarginsEnabled() const
{
    return d->isDefaultMarginsEnabled;
}

void TextField::setDefaultMarginsEnabled(bool _enable)
{
    if (d->isDefaultMarginsEnabled == _enable) {
        return;
    }

    d->isDefaultMarginsEnabled = _enable;
    reconfigure();
    updateGeometry();
    update();
}

void TextField::setCustomMargins(const QMarginsF& _margins)
{
    if (d->customMargins == _margins) {
        return;
    }

    d->customMargins = _margins;
    if (d->isDefaultMarginsEnabled) {
        d->isDefaultMarginsEnabled = false;
    }
    reconfigure();
    updateGeometry();
    update();
}

QMarginsF TextField::customMargins() const
{
    return d->customMargins;
}

void TextField::setEnterMakesNewLine(bool _make)
{
    if (d->isEnterMakesNewLine == _make) {
        return;
    }

    d->isEnterMakesNewLine = _make;
}

void TextField::clear()
{
    if (text().isEmpty()) {
        return;
    }

    QTextCursor cursor = textCursor();
    cursor.select(QTextCursor::Document);
    cursor.deleteChar();
    document()->clearUndoRedoStacks();

    d->animateLabelToBottom();
    d->finishAnimationIfInvisible();
}

QSize TextField::minimumSizeHint() const
{
    return {};
}

QSize TextField::sizeHint() const
{
    QSizeF size(TextHelper::fineTextWidth(!text().isEmpty() ? text() : d->placeholder,
                                          Ui::DesignSystem::font().body1()),
                TextHelper::fineLineSpacing(Ui::DesignSystem::font().body1()));
    if (!d->suffix.isEmpty()) {
        const auto suffixWidth
            = TextHelper::fineTextWidthF(d->suffix, Ui::DesignSystem::font().body1())
            + Ui::DesignSystem::textField().spacing();
        size += { suffixWidth, 0 };
    }
    if (!d->label.isEmpty()) {
        const auto labelWidth
            = TextHelper::fineTextWidthF(d->label, Ui::DesignSystem::font().caption());
        size = size.expandedTo({ labelWidth, 1 });
    }
    if (!d->error.isEmpty()) {
        const auto errorWidth
            = TextHelper::fineTextWidthF(d->error, Ui::DesignSystem::font().caption());
        size = size.expandedTo({ errorWidth, 1 });
    } else if (!d->helper.isEmpty()) {
        const auto helperWidth
            = TextHelper::fineTextWidthF(d->helper, Ui::DesignSystem::font().caption());
        size = size.expandedTo({ helperWidth, 1 });
    }
    size += QSizeF(d->margins().left() + d->margins().right(),
                   d->margins().top() + d->margins().bottom())
                .toSize();
    size += QSizeF(d->contentMargins().left() + d->contentMargins().right(),
                   d->contentMargins().top() + d->contentMargins().bottom())
                .toSize();
    if (!d->trailingIcon.isEmpty()) {
        size += QSizeF(Ui::DesignSystem::textField().iconSize().width()
                           + Ui::DesignSystem::textField().spacing(),
                       0.0)
                    .toSize();
    }
    return size.toSize();
}

int TextField::heightForWidth(int _width) const
{
    qreal width = _width;
    width -= d->margins().left() + d->contentMargins().right();
    width -= d->contentMargins().left() + d->margins().right();
    if (!d->trailingIcon.isEmpty()) {
        width -= Ui::DesignSystem::textField().iconSize().width()
            + Ui::DesignSystem::textField().spacing();
    }

    qreal height = TextHelper::heightForWidth(text(), Ui::DesignSystem::font().body1(),
                                              static_cast<int>(width));
    height += d->margins().top() + d->margins().bottom();
    height += d->contentMargins().top() + d->contentMargins().bottom();
    if (!d->helper.isEmpty() || !d->error.isEmpty()) {
        height += Ui::DesignSystem::textField().helperHeight();
    }
    return qCeil(height);
}

ContextMenu* TextField::createContextMenu(const QPoint& _position, QWidget* _parent)
{
    const QRectF iconRect = d->trailingIconRect();
    if (!isReadOnly() && iconRect.contains(mapFromGlobal(QCursor::pos()))) {
        emit trailingIconContextMenuRequested();
        return nullptr;
    }

    return BaseTextEdit::createContextMenu(_position, _parent);
}

void TextField::reconfigure()
{
    d->reconfigure();
}

bool TextField::event(QEvent* _event)
{
    switch (static_cast<int>(_event->type())) {
    case static_cast<QEvent::Type>(EventType::DesignSystemChangeEvent): {
        reconfigure();
        updateGeometry();
        update();
        return true;
    }

    case QEvent::ToolTip: {
        QHelpEvent* event = static_cast<QHelpEvent*>(_event);
        const QRectF iconRect = d->trailingIconRect();
        if (iconRect.contains(event->pos())) {
            QToolTip::showText(event->globalPos(), d->trailingIconToolTip);
        } else {
            QToolTip::hideText();
        }
        return true;
    }

    default: {
        return BaseTextEdit::event(_event);
    }
    }
}

void TextField::paintEvent(QPaintEvent* _event)
{
    QPainter painter;

    //
    // Рисуем фон
    //
    painter.begin(viewport());
    QRectF backgroundRect = rect().marginsRemoved(d->contentMargins().toMargins());
    if (!d->helper.isEmpty() || !d->error.isEmpty()) {
        backgroundRect.setBottom(backgroundRect.bottom()
                                 - Ui::DesignSystem::textField().helperHeight());
    }
    painter.setPen(Qt::NoPen);
    painter.setBrush(underMouse() || hasFocus()
                             || findChildren<QWidget*>().contains(QApplication::focusWidget())
                         ? d->backgroundActiveColor
                         : d->backgroundInactiveColor);
    QPainterPath backgroundPath;
    backgroundPath.setFillRule(Qt::WindingFill);
    backgroundPath.addRoundedRect(backgroundRect, Ui::DesignSystem::textField().borderRadius(),
                                  Ui::DesignSystem::textField().borderRadius());
    backgroundPath.addRect(
        backgroundRect.adjusted(0, Ui::DesignSystem::textField().borderRadius(), 0, 0));
    painter.drawPath(backgroundPath);
    painter.end();

    //
    // Если не включён режим отображения пароля, то отрисовкой текста и курсора занимается сам
    // BaseTextEdit
    //
    if (!d->isPasswordModeEnabled) {
        BaseTextEdit::paintEvent(_event);
    }
    //
    // В противном случае, самостоятельно рисуем звёздочки вместо букв
    //
    else if (!text().isEmpty()) {
        painter.begin(viewport());
        painter.setFont(Ui::DesignSystem::font().body1());
        painter.setPen(d->textColor);
        const auto passwdRect = d->inputTextRect();
        painter.drawText(passwdRect, Qt::AlignLeft | Qt::AlignBottom,
                         QString(text().length(), QString("●").front()));
        painter.end();
    }


    //
    // Рисуем декорации
    //
    painter.begin(viewport());
    //
    // ... лейбл
    //
    if (!d->label.isEmpty()) {
        QFont labelFont = Ui::DesignSystem::font().caption();
        if (!d->labelFontSizeAnimation.currentValue().isNull()) {
            labelFont.setPixelSize(d->labelFontSizeAnimation.currentValue().toInt());
        } else if (!hasFocus() && text().isEmpty() && d->placeholder.isEmpty()) {
            labelFont.setPixelSize(Ui::DesignSystem::font().body1().pixelSize());
        }
        painter.setFont(labelFont);

        QColor labelColor = d->textDisabledColor;
        if (!d->error.isEmpty()) {
            labelColor = Ui::DesignSystem::color().error();
        } else if (!d->labelColorAnimation.currentValue().isNull()) {
            labelColor = d->labelColorAnimation.currentValue().value<QColor>();
        }
        painter.setPen(labelColor);
        const auto labelRect = d->labelRect(TextHelper::fineLineSpacing(labelFont));
        painter.drawText(labelRect, Qt::AlignLeft | Qt::AlignVCenter, d->label);
    }
    //
    // ... плейсхолдер
    //
    if (text().isEmpty() && !d->placeholder.isEmpty()) {
        painter.setFont(Ui::DesignSystem::font().body1());
        painter.setPen(d->textDisabledColor);
        const auto placeholderRect = d->inputTextRect();
        painter.drawText(placeholderRect, Qt::AlignLeft | Qt::AlignVCenter, d->placeholder);
    }
    //
    // ... суффикс
    //
    if (!d->suffix.isEmpty()) {
        painter.setFont(Ui::DesignSystem::font().body1());
        painter.setPen(d->textDisabledColor);
        const auto suffixRect = d->suffixRect();
        painter.drawText(suffixRect, Qt::AlignLeft | Qt::AlignVCenter, d->suffix);
    }
    //
    // ... иконка действия
    //
    if (!d->trailingIcon.isEmpty()) {
        painter.setFont(Ui::DesignSystem::font().iconsMid());
        painter.setPen(d->trailingIconColor.isValid() ? d->trailingIconColor
                                                      : palette().color(QPalette::Text));
        const auto iconRect = d->trailingIconRect();
        painter.drawText(iconRect.toRect(), Qt::AlignCenter,
                         IconHelper::directedIcon(d->trailingIcon));
    }
    //
    // ... подчёркивание
    //
    if (d->isUnderlineDecorationVisible) {
        //
        // ... в обычном состоянии
        //
        {
            const QRectF decorationRect = d->decorationRect();
            const QColor decorationColor = !d->error.isEmpty() ? Ui::DesignSystem::color().error()
                                                               : palette().color(QPalette::Text);
            painter.fillRect(decorationRect, decorationColor);
        }
        //
        // ... в выделенном
        //
        if (d->decorationAnimation.currentValue().isValid()) {
            QRectF decorationRect = d->decorationAnimation.currentValue().toRectF();
            decorationRect.moveTop(d->decorationRectInFocus().top());
            const QColor decorationColor = !d->error.isEmpty()
                ? Ui::DesignSystem::color().error()
                : Ui::DesignSystem::color().secondary();
            painter.fillRect(decorationRect, decorationColor);
        }
    }
    //
    // ... ошибка или подсказка
    //
    if (!d->error.isEmpty() || !d->helper.isEmpty()) {
        painter.setFont(Ui::DesignSystem::font().caption());
        const QColor color
            = !d->error.isEmpty() ? Ui::DesignSystem::color().error() : d->textDisabledColor;
        painter.setPen(color);
        const QRectF textRect(d->contentMargins().left() + d->margins().left(),
                              height() - d->contentMargins().bottom()
                                  - Ui::DesignSystem::textField().helperHeight(),
                              width() - d->contentMargins().left() - d->margins().left()
                                  - d->margins().right() - d->contentMargins().right(),
                              Ui::DesignSystem::textField().helperHeight());
        const QString text = !d->error.isEmpty() ? d->error : d->helper;
        painter.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, text);
    }
    //
    // ... декорация нажатии на кнопке
    //
    if (d->iconDecorationAnimation.state() == ClickAnimation::Running) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(d->trailingIconColor.isValid() ? d->trailingIconColor
                                                        : palette().color(QPalette::Text));
        painter.setOpacity(d->iconDecorationAnimation.opacity());
        const auto radius = d->iconDecorationAnimation.radius();
        painter.drawEllipse(d->trailingIconRect().center(), radius, radius);
        painter.setOpacity(1.0);
    }
    //
    painter.end();
}

void TextField::resizeEvent(QResizeEvent* _event)
{
    BaseTextEdit::resizeEvent(_event);

    //
    // Если виджет в фокусе, корректируем размер полосы декорации под текстом
    //
    if (hasFocus()) {
        const QRectF decorationRect = d->decorationRectInFocus();
        d->decorationAnimation.setEndValue(decorationRect);
    }
}

void TextField::focusInEvent(QFocusEvent* _event)
{
    BaseTextEdit::focusInEvent(_event);

    //
    // Некоторые наследники (например комбобокс) пропускают событие выхода фокуса, и форсят
    // сохранение фокуса внутри, пока пользователь взаимодействует с дочерними виджетами,
    // поэтому, когда по факту фокус возвращается в виджет проверяем действительно ли нужно
    // анимировать отображение декораций
    //

    if (d->labelColorAnimation.endValue() != Ui::DesignSystem::color().secondary()) {
        d->labelColorAnimation.setStartValue(d->textDisabledColor);
        d->labelColorAnimation.setEndValue(Ui::DesignSystem::color().secondary());
        d->labelColorAnimation.start();
    }

    if (text().isEmpty() && d->placeholder.isEmpty()) {
        d->animateLabelToTop();
    }

    const QRectF decorationRect = d->decorationRectInFocus();
    const qreal contentsWidth
        = (width() - d->contentMargins().left() - d->contentMargins().right()) / 2;
    if (d->decorationAnimation.endValue() != decorationRect) {
        d->decorationAnimation.setStartValue(
            decorationRect.adjusted(contentsWidth, 0, -1 * contentsWidth, 0));
        d->decorationAnimation.setEndValue(decorationRect);
        d->decorationAnimation.start();
    }
}

void TextField::focusOutEvent(QFocusEvent* _event)
{
    BaseTextEdit::focusOutEvent(_event);

    if (d->isContextMenuShown) {
        return;
    }

    d->labelColorAnimation.setStartValue(Ui::DesignSystem::color().secondary());
    d->labelColorAnimation.setEndValue(d->textDisabledColor);
    d->labelColorAnimation.start();

    if (text().isEmpty() && d->placeholder.isEmpty()) {
        d->animateLabelToBottom();
    }

    const QRectF decorationRect = d->decorationRectInFocus();
    d->decorationAnimation.setStartValue(decorationRect);
    const qreal contentsWidth
        = (width() - d->contentMargins().left() - d->contentMargins().right()) / 2;
    d->decorationAnimation.setEndValue(
        decorationRect.adjusted(contentsWidth, 0, -1 * contentsWidth, 0));
    d->decorationAnimation.start();
}

void TextField::mousePressEvent(QMouseEvent* _event)
{
    const QRectF iconRect = d->trailingIconRect();
    if (iconRect.contains(_event->pos())) {
        d->iconDecorationAnimation.start();
    }

    BaseTextEdit::mousePressEvent(_event);
}

void TextField::mouseReleaseEvent(QMouseEvent* _event)
{
    const QRectF iconRect = d->trailingIconRect();
    if (!isReadOnly() && _event->button() != Qt::RightButton && iconRect.contains(_event->pos())) {
        emit trailingIconPressed();
        _event->accept();
    } else {
        BaseTextEdit::mouseReleaseEvent(_event);
    }
}

void TextField::mouseMoveEvent(QMouseEvent* _event)
{
    BaseTextEdit::mouseMoveEvent(_event);

    if (d->trailingIcon.isEmpty()) {
        if (viewport()->cursor() != Qt::IBeamCursor) {
            viewport()->setCursor(Qt::IBeamCursor);
        }
        return;
    }

    const QRectF iconRect = d->trailingIconRect();
    if (iconRect.contains(_event->pos())) {
        viewport()->setCursor(Qt::ArrowCursor);
    } else {
        viewport()->setCursor(Qt::IBeamCursor);
    }
}

void TextField::keyPressEvent(QKeyEvent* _event)
{
    if ((_event->key() == Qt::Key_Enter || _event->key() == Qt::Key_Return)
        && !d->isEnterMakesNewLine) {
        _event->ignore();
        emit enterPressed();
        return;
    }

    if (keyPressEventReimpl(_event)) {
        _event->accept();
    } else {
        BaseTextEdit::keyPressEvent(_event);
    }

    updateEnteredText(_event->text());
}

void TextField::changeEvent(QEvent* _event)
{
    switch (_event->type()) {
    case QEvent::LayoutDirectionChange:
        reconfigure();
        break;
    default:
        break;
    }

    return BaseTextEdit::changeEvent(_event);
}

void TextField::insertFromMimeData(const QMimeData* _source)
{
    if (_source->hasText()) {
        insertPlainText(_source->text());
    }
}
