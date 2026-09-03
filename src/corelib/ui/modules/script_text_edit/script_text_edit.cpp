#include "script_text_edit.h"

#include <business_layer/document/text/text_cursor.h>
#include <business_layer/templates/text_template.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <domain/starcloud_api.h>
#include <include/custom_events.h>
#include <ui/design_system/design_system.h>
#include <ui/modules/script_text_edit/vim_text_edit_controller.h>
#include <ui/widgets/text_edit/page/qt6/page_text_edit_scroll_bar.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/text_helper.h>
#include <utils/tools/debouncer.h>

#include <QFontMetricsF>
#include <QKeyEvent>
#include <QLabel>
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
    bool updateVimModeLabel();
    bool updateVimStatusLabel();
    void updateVimModeLabelGeometry();
    void updateVimBlockCursor();
    void configureVimLabel(QLabel* _label, const QColor& _backgroundColor,
                           const QColor& _textColor);
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
    QLabel* vimModeLabel = nullptr;
    QLabel* vimStatusLabel = nullptr;

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
    vimModeLabel = new QLabel(q->viewport());
    vimModeLabel->hide();
    vimModeLabel->setAttribute(Qt::WA_TransparentForMouseEvents);

    vimStatusLabel = new QLabel(q->viewport());
    vimStatusLabel->hide();
    vimStatusLabel->setAttribute(Qt::WA_TransparentForMouseEvents);

    QObject::connect(q, &PageTextEdit::cursorPositionChanged, q, [this] { refreshVimModeUi(); });
    QObject::connect(q, &PageTextEdit::selectionChanged, q, [this] { refreshVimModeUi(); });

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

    bool needUpdateGeometry = updateVimModeLabel();
    needUpdateGeometry = updateVimStatusLabel() || needUpdateGeometry;
    vimModeLabelNeedUpdate = false;
    if (needUpdateGeometry) {
        updateVimModeLabelGeometry();
    }

    updateVimBlockCursor();
}

bool ScriptTextEdit::Implementation::updateVimModeLabel()
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

    return needReconfigure;
}

bool ScriptTextEdit::Implementation::updateVimStatusLabel()
{
    const auto statusText = vimMode.statusText();
    if (statusText.isEmpty()) {
        const bool wasVisible = !vimStatusLabel->isHidden();
        vimStatusLabel->hide();
        return wasVisible;
    }

    const bool needReconfigure = vimModeLabelNeedUpdate || vimStatusLabel->text() != statusText;
    if (needReconfigure) {
        vimStatusLabel->setText(statusText);
        configureVimLabel(vimStatusLabel, DesignSystem::color().background(),
                          DesignSystem::color().onBackground());
    }

    vimStatusLabel->show();
    vimStatusLabel->raise();

    return needReconfigure;
}

void ScriptTextEdit::Implementation::configureVimLabel(QLabel* _label,
                                                       const QColor& _backgroundColor,
                                                       const QColor& _textColor)
{
    const int borderRadius = static_cast<int>(DesignSystem::layout().px4());
    const int verticalPadding = static_cast<int>(DesignSystem::layout().px4());
    const int horizontalPadding = static_cast<int>(DesignSystem::layout().px8());

    _label->setFont(DesignSystem::font().overline());
    _label->setStyleSheet(QString("QLabel { background-color: %1; color: %2; border-radius: %3px; "
                                  "padding: %4px %5px; }")
                              .arg(_backgroundColor.name(), _textColor.name())
                              .arg(borderRadius)
                              .arg(verticalPadding)
                              .arg(horizontalPadding));
    _label->adjustSize();
}

void ScriptTextEdit::Implementation::updateVimModeLabelGeometry()
{
    if (vimModeLabel == nullptr || vimStatusLabel == nullptr) {
        return;
    }

    const int margin = static_cast<int>(DesignSystem::layout().px8());
    const int viewportHeight = q->viewport()->height();
    const QSize labelSize = vimModeLabel->sizeHint();
    vimModeLabel->setGeometry(margin, viewportHeight - labelSize.height() - margin,
                              labelSize.width(), labelSize.height());

    //
    // ... строка ввода команды располагается справа от индикатора режима
    //
    const QSize statusSize = vimStatusLabel->sizeHint();
    const int statusLeft = margin + labelSize.width() + margin;
    const int statusWidth
        = qMin(statusSize.width(), qMax(0, q->viewport()->width() - statusLeft - margin));
    vimStatusLabel->setGeometry(statusLeft, viewportHeight - statusSize.height() - margin,
                                statusWidth, statusSize.height());
}

void ScriptTextEdit::Implementation::updateVimBlockCursor()
{
    QList<PageTextEdit::ExtraSelection> selections;
    if (!vimMode.usesBlockCursor()) {
        q->setExtraSelections(selections);
        q->viewport()->update();
        return;
    }

    QTextCursor cursor(q->document());
    cursor.setPosition(vimMode.blockCursorPosition());
    cursor.clearSelection();
    if (cursor.block().length() > 1) {
        if (cursor.atBlockEnd()) {
            cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor);
        } else {
            cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
        }

        QTextCharFormat format;
        format.setBackground(vimModeColor());
        format.setForeground(vimModeTextColor());
        selections.append({ cursor, format });
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

void ScriptTextEdit::resizeEvent(QResizeEvent* _event)
{
    BaseTextEdit::resizeEvent(_event);
    d->updateVimModeLabelGeometry();
}

void ScriptTextEdit::inputMethodEvent(QInputMethodEvent* _event)
{
    //
    // В командных режимах Vim'а ввод текста запрещён, в том числе и через методы ввода
    //
    if (d->vimMode.usesBlockCursor()) {
        _event->accept();
        return;
    }

    BaseTextEdit::inputMethodEvent(_event);
}

void ScriptTextEdit::paintOverText(QPainter* _painter)
{
    BaseTextEdit::paintOverText(_painter);

    if (!d->vimMode.usesBlockCursor()) {
        return;
    }

    QTextCursor cursor(document());
    cursor.setPosition(d->vimMode.blockCursorPosition());
    if (cursor.block().length() > 1) {
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
