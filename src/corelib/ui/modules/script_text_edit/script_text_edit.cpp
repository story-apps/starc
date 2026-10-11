#include "script_text_edit.h"

#include <business_layer/document/text/text_cursor.h>
#include <business_layer/templates/text_template.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <domain/starcloud_api.h>
#include <include/custom_events.h>
#include <ui/design_system/design_system.h>
#include <ui/modules/script_text_edit/vim_text_edit_controller.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/text_edit/page/page_text_edit_scroll_bar.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/text_helper.h>
#include <utils/tools/debouncer.h>

#include <QFocusEvent>
#include <QFontMetricsF>
#include <QGraphicsProxyWidget>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QRegularExpression>
#include <QResizeEvent>
#include <QTextTable>

using BusinessLayer::TextBlockStyle;


namespace Ui {

class ScriptTextEdit::Implementation
{
public:
    Implementation(ScriptTextEdit* _q);

    /**
     * @brief Обновить внешний вид редактора в соответствии с состоянием Vim-режима
     */
    void refreshVimModeUi();
    void updateVimModeLabel();
    void updateVimStatusLabel();
    void updateVimModeLabelGeometry();
    QRect visibleViewportRect() const;
    bool isVimBlockCursorVisible() const;
    void updateVimBlockCursor();
    void configureVimLabel(AbstractLabel* _label, const QColor& _backgroundColor,
                           const QColor& _textColor);
    QSize vimLabelSize(AbstractLabel* _label) const;
    QColor vimModeColor() const;
    QColor vimModeTextColor() const;


    ScriptTextEdit* q = nullptr;

    /**
     * @brief Показывать автодополения в пустых блоках
     */
    bool showSuggestionsInEmptyBlocks = true;

    /**
     * @brief Vim-режим редактирования текста
     */
    VimTextEditController vimMode;
    AbstractLabel* vimModeLabel = nullptr;
    AbstractLabel* vimStatusLabel = nullptr;

    /**
     * @brief Применены ли к редактору настройки внешнего вида Vim-режима
     * @note Нужно, чтобы не выполнять лишнюю работу, когда Vim-режим отключён
     */
    bool vimModeUiApplied = false;

    /**
     * @brief Нужно ли перенастроить виджет с названием режима
     */
    bool vimModeLabelNeedUpdate = true;

    /**
     * @brief Последнее положение мыши
     */
    QPoint lastMousePos;

    /**
     * @brief Курсоры соавторов
     */
    QVector<Domain::CursorInfo> collaboratorsCursorInfo;
    QVector<Domain::CursorInfo> pendingCollaboratorsCursorInfo;
    Debouncer collaboratorCursorInfoUpdateDebouncer;
};

ScriptTextEdit::Implementation::Implementation(ScriptTextEdit* _q)
    : q(_q)
    , vimMode(_q)
    , collaboratorCursorInfoUpdateDebouncer(500)
{
    vimModeLabel = new OverlineLabel(q->viewport());
    vimModeLabel->hide();
    vimModeLabel->setAttribute(Qt::WA_TransparentForMouseEvents);

    vimStatusLabel = new CaptionLabel(q->viewport());
    vimStatusLabel->hide();
    vimStatusLabel->setAttribute(Qt::WA_TransparentForMouseEvents);

    connect(q, &PageTextEdit::cursorPositionChanged, q, [this] { refreshVimModeUi(); });
    connect(q, &PageTextEdit::selectionChanged, q, [this] {
        vimMode.handleSelectionChange();
        refreshVimModeUi();
    });

    connect(&collaboratorCursorInfoUpdateDebouncer, &Debouncer::gotWork, q, [this] {
        std::swap(collaboratorsCursorInfo, pendingCollaboratorsCursorInfo);
        q->update();
    });
}

void ScriptTextEdit::Implementation::refreshVimModeUi()
{
    if (!vimMode.isEnabled()) {
        //
        // Если Vim-режим и не был включён, то и восстанавливать нечего
        //
        if (!vimModeUiApplied) {
            return;
        }

        vimModeUiApplied = false;
        vimModeLabel->hide();
        vimStatusLabel->hide();
        q->setCursorWidth(DesignSystem::layout().px2());
        q->setExtraSelections({});
        q->viewport()->update();
        return;
    }

    vimModeUiApplied = true;
    q->setCursorWidth(vimMode.usesBlockCursor() ? 0 : DesignSystem::layout().px2());

    //
    // ... положение обновляем каждый раз, ведь видимая часть редактора может измениться и без
    //     изменения его размера, например когда появляется шкала хронометража
    //
    updateVimModeLabel();
    updateVimStatusLabel();
    vimModeLabelNeedUpdate = false;
    updateVimModeLabelGeometry();

    updateVimBlockCursor();
}

void ScriptTextEdit::Implementation::updateVimModeLabel()
{
    //
    // Перенастраиваем виджет только когда действительно поменялся режим работы, или дизайн-система,
    // т.к. настройка стилей - довольно затратная операция, а обновляемся мы после каждого нажатия
    //
    const auto modeName = vimMode.modeName();
    const bool needReconfigure = vimModeLabelNeedUpdate || vimModeLabel->text() != modeName;
    if (needReconfigure) {
        vimModeLabel->setText(modeName);
        configureVimLabel(vimModeLabel, vimModeColor(), vimModeTextColor());
    }

    vimModeLabel->show();
    vimModeLabel->raise();
}

void ScriptTextEdit::Implementation::updateVimStatusLabel()
{
    const auto statusText = vimMode.statusText();
    if (statusText.isEmpty()) {
        vimStatusLabel->hide();
        return;
    }

    const bool needReconfigure = vimModeLabelNeedUpdate || vimStatusLabel->text() != statusText;
    if (needReconfigure) {
        vimStatusLabel->setText(statusText);
        configureVimLabel(vimStatusLabel, DesignSystem::color().background(),
                          DesignSystem::color().onBackground());
    }

    vimStatusLabel->show();
    vimStatusLabel->raise();
}

void ScriptTextEdit::Implementation::configureVimLabel(AbstractLabel* _label,
                                                       const QColor& _backgroundColor,
                                                       const QColor& _textColor)
{
    //
    // ... отступы вокруг текста задаём размером самого виджета, т.к. фон рисуется в области
    //     содержимого
    //
    _label->setContentsMargins({});
    _label->setAlignment(Qt::AlignCenter);
    _label->setBorderRadius(DesignSystem::layout().px4());
    _label->setBackgroundColor(_backgroundColor);
    _label->setTextColor(_textColor);
}

QSize ScriptTextEdit::Implementation::vimLabelSize(AbstractLabel* _label) const
{
    return _label->sizeHint()
        + QSize(static_cast<int>(DesignSystem::layout().px16()),
                static_cast<int>(DesignSystem::layout().px8()));
}

void ScriptTextEdit::Implementation::updateVimModeLabelGeometry()
{
    if (vimModeLabel == nullptr || vimStatusLabel == nullptr) {
        return;
    }

    const int margin = static_cast<int>(DesignSystem::layout().px8());
    const QRect visibleRect = visibleViewportRect();
    const QSize labelSize = vimLabelSize(vimModeLabel);
    vimModeLabel->setGeometry(visibleRect.left() + margin,
                              visibleRect.bottom() + 1 - labelSize.height() - margin,
                              labelSize.width(), labelSize.height());

    //
    // ... строка ввода команды располагается справа от индикатора режима
    //
    const QSize statusSize = vimLabelSize(vimStatusLabel);
    const int statusLeft = vimModeLabel->geometry().right() + 1 + margin;
    const int statusWidth
        = qMin(statusSize.width(), qMax(0, visibleRect.right() + 1 - statusLeft - margin));
    vimStatusLabel->setGeometry(statusLeft, visibleRect.bottom() + 1 - statusSize.height() - margin,
                                statusWidth, statusSize.height());
}

QRect ScriptTextEdit::Implementation::visibleViewportRect() const
{
    //
    // Редактор может быть встроен в ScalableWrapper, который показывает его не целиком: часть
    // вьюпорта закрывают полосы прокрутки обёртки, поэтому берём только видимую в ней область
    //
    QRect visibleRect = q->viewport()->rect();
    const auto proxy = q->graphicsProxyWidget();
    if (proxy == nullptr || proxy->scene() == nullptr || proxy->scene()->views().isEmpty()) {
        return visibleRect;
    }

    //
    // ... а поверх нижнего края вьюпорта обёртки бывают и другие виджеты, например шкала
    //     хронометража сценария, их тоже исключаем
    //
    const auto view = proxy->scene()->views().constFirst();
    auto viewRect = view->viewport()->rect();
    const auto viewportPosition = view->viewport()->pos();
    for (const auto child : view->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly)) {
        if (child == view->viewport() || !child->isVisible()) {
            continue;
        }
        const auto childRect = child->geometry().translated(-viewportPosition);
        if (childRect.top() > viewRect.top() && childRect.top() <= viewRect.bottom()
            && childRect.bottom() >= viewRect.bottom()
            && childRect.width() > viewRect.width() / 2) {
            viewRect.setBottom(childRect.top() - 1);
        }
    }
    const auto visibleInScene = view->mapToScene(viewRect).boundingRect();
    const auto visibleInEditor = proxy->mapFromScene(visibleInScene).boundingRect().toRect();
    const auto visibleInViewport = visibleInEditor.translated(-q->viewport()->pos());
    const auto intersection = visibleRect.intersected(visibleInViewport);
    return intersection.isEmpty() ? visibleRect : intersection;
}

bool ScriptTextEdit::Implementation::isVimBlockCursorVisible() const
{
    return vimMode.usesBlockCursor() && q->hasFocus();
}

void ScriptTextEdit::Implementation::updateVimBlockCursor()
{
    //
    // Курсор-блок подсвечивает символ под курсором, а в конце строки он рисуется поверх текста
    //
    QList<PageTextEdit::ExtraSelection> selections;
    if (isVimBlockCursorVisible()) {
        QTextCursor cursor(q->document());
        cursor.setPosition(vimMode.blockCursorPosition());
        if (!cursor.atBlockEnd()) {
            cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);

            QTextCharFormat format;
            format.setBackground(vimModeColor());
            format.setForeground(vimModeTextColor());
            selections.append({ cursor, format });
        }
    }

    q->setExtraSelections(selections);
    q->viewport()->update();
}

QColor ScriptTextEdit::Implementation::vimModeColor() const
{
    switch (vimMode.mode()) {
    case VimTextEditController::Mode::Insert:
        return DesignSystem::color().primary();
    case VimTextEditController::Mode::Visual:
    case VimTextEditController::Mode::VisualLine:
        return DesignSystem::color().warning();
    default:
        return DesignSystem::color().accent();
    }
}

QColor ScriptTextEdit::Implementation::vimModeTextColor() const
{
    switch (vimMode.mode()) {
    case VimTextEditController::Mode::Insert:
        return DesignSystem::color().onPrimary();
    case VimTextEditController::Mode::Visual:
    case VimTextEditController::Mode::VisualLine:
        return DesignSystem::color().onWarning();
    default:
        return DesignSystem::color().onAccent();
    }
}


// ****


ScriptTextEdit::ScriptTextEdit(QWidget* _parent)
    : BaseTextEdit(_parent)
    , d(new Implementation(this))
{
    setVimModeEnabled(settingsValue(DataStorageLayer::kApplicationUseVimModeKey).toBool());
}

ScriptTextEdit::~ScriptTextEdit() = default;

bool ScriptTextEdit::showSuggestionsInEmptyBlocks() const
{
    return d->showSuggestionsInEmptyBlocks;
}

void ScriptTextEdit::setShowSuggestionsInEmptyBlocks(bool _show)
{
    d->showSuggestionsInEmptyBlocks = _show;
}

bool ScriptTextEdit::isVimModeEnabled() const
{
    return d->vimMode.isEnabled();
}

void ScriptTextEdit::setVimModeEnabled(bool _enabled)
{
    d->vimMode.setEnabled(_enabled);
    d->refreshVimModeUi();
}

void ScriptTextEdit::scrollVerticallyBy(int _delta)
{
    auto scrollBar = verticalScrollBar();
    if (scrollBar == nullptr || _delta == 0) {
        return;
    }

    scrollBar->setValue(scrollBar->value() + _delta);
}

void ScriptTextEdit::setTextCursorAndKeepScrollBars(const QTextCursor& _cursor)
{
    //
    // При позиционировании курсора в невидимый блок, Qt откидывает прокрутку в самый верх, поэтому
    // перед редактированием документа запомним прокрутку, а после завершения редактирования
    // восстановим значения полос прокрутки
    //
    const auto verticalScrollValue = verticalScroll();
    const auto horizontalScrollValue = horizontalScroll();

    //
    // Задаём курсор
    //
    setTextCursor(_cursor);

    //
    // Восстанавливаем значения полос прокрутки
    //
    setVerticalScroll(verticalScrollValue);
    setHorizontalScroll(horizontalScrollValue);
}

void ScriptTextEdit::insertMimeData(const QMimeData* _source)
{
    insertFromMimeData(_source);
}

bool ScriptTextEdit::isTrackChangesEnabled() const
{
    return false;
}

void ScriptTextEdit::markInsertedText(int _from, int _to)
{
    Q_UNUSED(_from)
    Q_UNUSED(_to)
}

void ScriptTextEdit::setCollaboratorsCursors(const QVector<Domain::CursorInfo>& _cursors)
{
    d->pendingCollaboratorsCursorInfo = _cursors;
    d->collaboratorCursorInfoUpdateDebouncer.orderWork();
}

void ScriptTextEdit::updateCollaboratorsCursors(int _position, int _charsRemoved, int _charsAdded)
{
    for (auto& collaboratorCursor : d->collaboratorsCursorInfo) {
        int collaboratorCursorPosition = collaboratorCursor.cursorData.toInt();
        if (collaboratorCursorPosition >= _position) {
            collaboratorCursorPosition += _charsAdded - _charsRemoved;
            collaboratorCursor.cursorData = QByteArray::number(collaboratorCursorPosition);
        }
    }

    d->collaboratorCursorInfoUpdateDebouncer.abortWork();
}

void ScriptTextEdit::paintCollaboratorsCursors(QPainter& _painter, const QUuid& _documentUuid,
                                               const QTextBlock& _topBlock,
                                               const QTextBlock& _bottomBlock) const
{
    if (!d->collaboratorsCursorInfo.isEmpty()) {
        for (const auto& cursorInfo : std::as_const(d->collaboratorsCursorInfo)) {
            //
            // Пропускаем курсоры из других документов
            //
            if (cursorInfo.documentUuid != _documentUuid) {
                continue;
            }

            //
            // Пропускаем курсоры, которые находятся за пределами экрана
            //
            const auto cursorPosition = cursorInfo.cursorData.toInt();
            if (_bottomBlock.isValid()
                && (cursorPosition < _topBlock.position()
                    || cursorPosition > (_bottomBlock.position() + _bottomBlock.length()))) {
                continue;
            }


            BusinessLayer::TextCursor cursor(document());
            cursor.setPosition(cursorPosition);
            if (!cursor.block().isVisible()) {
                continue;
            }

            const auto cursorR = cursorRect(cursor).adjusted(-DesignSystem::layout().px(), 0,
                                                             DesignSystem::layout().px(), 0);
            const auto backgroundColor = ColorHelper::forText(cursorInfo.name);

            //
            // ... рисуем его
            //
            _painter.fillRect(cursorR, backgroundColor);

            //
            // ... выводим имя соавтора, если курсор мыши около курсора соавтора
            //
            if (cursorR.adjusted(-DesignSystem::layout().px4(), 0, DesignSystem::layout().px4(), 0)
                    .contains(d->lastMousePos)) {
                _painter.setFont(DesignSystem::font().subtitle2());
                const QRect usernameRect(cursorR.left() - DesignSystem::layout().px4(),
                                         cursorR.top() - DesignSystem::layout().px24(),
                                         TextHelper::fineTextWidth(cursorInfo.name, _painter.font())
                                             + DesignSystem::layout().px12(),
                                         DesignSystem::layout().px24());
                _painter.setPen(Qt::NoPen);
                _painter.setBrush(backgroundColor);
                _painter.drawRoundedRect(usernameRect, DesignSystem::button().borderRadius(),
                                         DesignSystem::button().borderRadius());
                _painter.setPen(ColorHelper::contrasted(backgroundColor));
                _painter.drawText(usernameRect, Qt::AlignCenter, cursorInfo.name);
            }
        }
    }
}

void ScriptTextEdit::mouseMoveEvent(QMouseEvent* _event)
{
    d->lastMousePos = _event->pos();

    BaseTextEdit::mouseMoveEvent(_event);
}

void ScriptTextEdit::mousePressEvent(QMouseEvent* _event)
{
    d->vimMode.cancelPendingInput();

    BaseTextEdit::mousePressEvent(_event);
    d->refreshVimModeUi();
}

void ScriptTextEdit::resizeEvent(QResizeEvent* _event)
{
    BaseTextEdit::resizeEvent(_event);
    d->updateVimModeLabelGeometry();
}

void ScriptTextEdit::inputMethodEvent(QInputMethodEvent* _event)
{
    //
    // В командных режимах Vim'а текст из методов ввода не вставляется, а обрабатывается как
    // команды, например символы, набранные с мёртвыми клавишами
    //
    if (d->vimMode.handleInputMethodEvent(_event)) {
        d->refreshVimModeUi();
        return;
    }

    BaseTextEdit::inputMethodEvent(_event);
}

void ScriptTextEdit::paintOverText(QPainter* _painter)
{
    BaseTextEdit::paintOverText(_painter);

    if (!d->isVimBlockCursorVisible()) {
        return;
    }

    QTextCursor cursor(document());
    cursor.setPosition(d->vimMode.blockCursorPosition());
    if (!cursor.atBlockEnd()) {
        return;
    }

    const auto cursorR = cursorRect(cursor);
    const qreal cursorWidth
        = qMax<qreal>(cursorR.height() / 2.0, QFontMetricsF(currentFont()).horizontalAdvance("M"));
    QRectF blockCursorRect(cursorR.topLeft(), QSizeF(cursorWidth, cursorR.height()));
    _painter->fillRect(blockCursorRect, d->vimModeColor());
}

bool ScriptTextEdit::event(QEvent* _event)
{
    if (_event->type() == QEvent::ShortcutOverride
        && d->vimMode.handleShortcutOverride(static_cast<QKeyEvent*>(_event))) {
        _event->accept();
        return true;
    }

    if (_event->type() == QEvent::KeyPress && d->vimMode.isEnabled()) {
        const bool handled = d->vimMode.handleKeyPress(static_cast<QKeyEvent*>(_event));
        d->refreshVimModeUi();
        if (handled) {
            return true;
        }
    }

    if (static_cast<int>(_event->type())
        == static_cast<int>(EventType::TextEditingOptionsChangeEvent)) {
        const auto event = static_cast<TextEditingOptionsChangeEvent*>(_event);
        if (event->useVimMode.has_value()) {
            setVimModeEnabled(event->useVimMode.value());
        }
    }

    const bool result = BaseTextEdit::event(_event);
    if (static_cast<int>(_event->type()) == static_cast<int>(EventType::DesignSystemChangeEvent)) {
        d->vimModeLabelNeedUpdate = true;
        d->refreshVimModeUi();
    } else if (_event->type() == QEvent::FocusIn || _event->type() == QEvent::FocusOut) {
        //
        // ... при переходе в другой виджет набираемая команда отменяется, но не при открытии
        //     всплывающих окон и переключении на другое приложение
        //
        if (_event->type() == QEvent::FocusOut) {
            const auto reason = static_cast<QFocusEvent*>(_event)->reason();
            if (reason != Qt::PopupFocusReason && reason != Qt::ActiveWindowFocusReason
                && reason != Qt::MenuBarFocusReason) {
                d->vimMode.cancelPendingInput();
            }
        }
        d->refreshVimModeUi();
    }
    return result;
}

bool ScriptTextEdit::keyPressEventReimpl(QKeyEvent* _event)
{
    //
    // Стандартно обрабатываем в базовом классе
    //
    const bool isEventHandled = BaseTextEdit::keyPressEventReimpl(_event);
    //
    // ... если это было вырезание и курсор остался в пустом контейнере таблицы, то нужно его
    //     удалить, чтобы он не оставлял за собой артефакта в виде открывающего/закрывающего блока
    //
    if (_event == QKeySequence::Cut) {
        auto cursor = textCursor();
        if (cursor.block().text().isEmpty()
            && BusinessLayer::TextBlockStyle::forCursor(cursor)
                == BusinessLayer::TextParagraphType::PageSplitter) {
            if (!cursor.atStart()) {
                cursor.deletePreviousChar();
                cursor.movePosition(QTextCursor::PreviousCharacter);
            } else {
                cursor.deleteChar();
                cursor.movePosition(QTextCursor::NextCharacter);
            }
            setTextCursor(cursor);
        }
    }

    return isEventHandled;
}

bool ScriptTextEdit::updateEnteredText(const QKeyEvent* _event)
{
    //
    // Ничего не делаем, если текст пуст,
    // или нажат хотя бы один из модификаторов делающих нажатие шорткатом
    //
    if (_event->text().isEmpty() || _event->modifiers().testFlag(Qt::ControlModifier)
        || _event->modifiers().testFlag(Qt::AltModifier)
        || _event->modifiers().testFlag(Qt::MetaModifier)) {
        return false;
    }

    //
    // Получим значения
    //
    const auto eventText = _event->text();
    // ... курсора
    QTextCursor cursor = textCursor();
    // ... блок текста в котором находится курсор
    QTextBlock currentBlock = cursor.block();
    // ... текст блока
    QString currentBlockText = currentBlock.text();
    // ... текст до курсора
    QString cursorBackwardText = currentBlockText.left(cursor.positionInBlock());
    // ... текст после курсора
    QString cursorForwardText = currentBlockText.mid(cursor.positionInBlock());
    // ... стиль шрифта блока
    QTextCharFormat currentCharFormat = currentBlock.charFormat();

    //
    // Определяем необходимость установки верхнего регистра для первого символа блока
    //
    if (currentCharFormat.boolProperty(TextBlockStyle::PropertyIsFirstUppercase)
        && cursorBackwardText != " " && cursorBackwardText == eventText
        && eventText[0] != TextHelper::smartToUpper(eventText[0])) {
        //
        // Сформируем правильное представление строки
        //
        QString correctedText = eventText;
        correctedText[0] = TextHelper::smartToUpper(correctedText[0]);

        //
        // Стираем предыдущий введённый текст
        //
        cursor.beginEditBlock();
        for (int repeats = 0; repeats < eventText.length(); ++repeats) {
            cursor.deletePreviousChar();
        }

        //
        // Выводим необходимый
        //
        cursor.insertText(correctedText);
        setTextCursorAndKeepScrollBars(cursor);
        cursor.endEditBlock();

        return true;
    }

    //
    // Если перед нами конец предложения
    // и не сокращение
    // и после курсора нет текста (для ремарки допустима скобка)
    //
    static QRegularExpression endOfSentenceRx;
    endOfSentenceRx.setPattern(
        QString("([.]|[?]|[!]|[…]) %1$").arg(TextHelper::toRxEscaped(eventText)));
    if (cursorBackwardText.contains(endOfSentenceRx) && cursorForwardText.isEmpty()
        && eventText[0] != TextHelper::smartToUpper(eventText[0])) {
        //
        // Сделаем первую букву заглавной
        //
        QString correctedText = eventText;
        correctedText[0] = TextHelper::smartToUpper(correctedText[0]);

        //
        // Стираем предыдущий введённый текст
        //
        cursor.beginEditBlock();
        for (int repeats = 0; repeats < eventText.length(); ++repeats) {
            cursor.deletePreviousChar();
        }

        //
        // Выводим необходимый
        //
        cursor.insertText(correctedText);
        setTextCursorAndKeepScrollBars(cursor);
        cursor.endEditBlock();

        return true;
    }

    return BaseTextEdit::updateEnteredText(_event);
}

} // namespace Ui
