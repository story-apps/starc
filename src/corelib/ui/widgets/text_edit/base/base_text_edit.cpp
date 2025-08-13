#include "base_text_edit.h"

#include <include/custom_events.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/context_menu/context_menu.h>
#include <utils/helpers/text_helper.h>

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QKeyEvent>
#include <QLocale>
#include <QRegularExpression>
#include <QStyleHints>
#include <QTextBlock>

namespace {

/**
 * @brief Расположить курсор так, чтобы извлечь форматирование для последующего инвертирования
 */
QTextCursor cursorForFormatInversion(const QTextCursor& _cursor)
{
    if (!_cursor.hasSelection()) {
        return _cursor;
    }

    auto cursor = _cursor;
    //
    // Максимум, потому что позиция в начале выделения имеет формат предыдущего символа
    //
    cursor.setPosition(std::max(_cursor.selectionStart(), _cursor.selectionEnd()));
    //
    // Если конец выделения попал в блок, в котором нет форматов, идём назад, в поисках блока,
    // обладающего форматированием, и тогда мы сможем из него извлечь необходимую информацию
    // для последующего инвертирования
    //
    const auto startPosition = std::min(_cursor.selectionStart(), _cursor.selectionEnd());
    while (cursor.block().textFormats().isEmpty() && cursor.position() > startPosition) {
        cursor.movePosition(QTextCursor::PreviousCharacter);
    }
    return cursor;
}

/**
 * @brief Оканчивается ли строка сокращением
 */
bool stringEndsWithAbbrev(const QString& _text)
{
    Q_UNUSED(_text);

    //
    // TODO: Добавить словарь сокращений. Возможно его можно вытащить из ханспела
    //

    return false;
}

/**
 * @brief Функции для получения корректных кавычек в зависимости от локали приложения
 */
/** @{ */
QString localeDoubleQuote(bool _open)
{
    switch (QLocale().language()) {
    default: {
        return "\"";
    }

    case QLocale::Russian:
    case QLocale::Spanish:
    case QLocale::French: {
        if (_open) {
            return "«";
        } else {
            return "»";
        }
    }

    case QLocale::English: {
        if (_open) {
            return "“";
        } else {
            return "”";
        }
    }
    }
}
QString localOpenDoubleQuote()
{
    return localeDoubleQuote(true);
}
QString localCloseDoubleQuote()
{
    return localeDoubleQuote(false);
}
//
QString localeSingleQuote(bool _open)
{
    switch (QLocale().language()) {
    default: {
        return "\'";
    }

    case QLocale::English: {
        if (_open) {
            return "‘";
        } else {
            return "’";
        }
    }
    }
}
QString localOpenSingleQuote()
{
    return localeSingleQuote(true);
}
QString localCloseSingleQuote()
{
    return localeSingleQuote(false);
}
/** @} */

/**
 * @brief Можно ли показать курсор в заданной позиции
 */
bool canShowCursor(const QTextCursor& _cursor)
{
    return _cursor.block().isVisible()
        && !_cursor.blockFormat().boolProperty(PageTextEdit::PropertyDontShowCursor);
}

} // namespace


class BaseTextEdit::Implementation
{
public:
    Implementation(BaseTextEdit* _q);

    /**
     * @brief Перенастроить редактор в соответствии с актуальной дизайн системой
     */
    void reconfigure();

    /**
     * @brief Выделить блок при тройном клике
     */
    bool selectBlockOnTripleClick(QMouseEvent* _event);

    /**
     * @brief Обновить форматирование в блоке
     */
    void updateSelectionFormatting(
        BaseTextEdit* _textEdit,
        std::function<QTextCharFormat(const QTextCharFormat&)> _updateFormat);

    /**
     * @brief Всвтаить текст из буфера обмена, как простой
     */
    void pasteAsPlainTextFromClipboard();


    BaseTextEdit* q = nullptr;

    bool capitalizeWords = true;
    bool correctDoubleCapitals = true;
    bool capitalizeSingleILetter = true;
    bool replaceThreeDots = false;
    bool smartQuotes = false;
    bool replaceTwoDashes = false;
    bool avoidMultipleSpaces = false;
    bool pasteAsPlainTextAvailable = true;
    bool formattingAvailable = true;

    /**
     * @brief Количеств
     */
    int mouseClicks = 0;

    /**
     * @brief Время последнего клика мышки, мс
     */
    qint64 lastMouseClickTime = 0;

#ifdef Q_OS_MAC
    /**
     * @brief Время нажатия последнего пробела
     */
    qint64 lastSpaceWasPressed = 0;
#endif
};

BaseTextEdit::Implementation::Implementation(BaseTextEdit* _q)
    : q(_q)
{
}

void BaseTextEdit::Implementation::reconfigure()
{
    q->setCursorWidth(Ui::DesignSystem::layout().px2());
}

bool BaseTextEdit::Implementation::selectBlockOnTripleClick(QMouseEvent* _event)
{
    if (_event->button() != Qt::LeftButton) {
        return false;
    }

    const qint64 curentMouseClickTime = QDateTime::currentMSecsSinceEpoch();
    const qint64 timeDelta = curentMouseClickTime - lastMouseClickTime;
    if (timeDelta <= (QApplication::styleHints()->mouseDoubleClickInterval())) {
        ++mouseClicks;
    } else {
        mouseClicks = 1;
    }
    lastMouseClickTime = curentMouseClickTime;

    //
    // Тройной клик обрабатываем самостоятельно
    //
    if (mouseClicks > 2) {
        mouseClicks = 0;
        QTextCursor cursor = q->textCursor();
        cursor.movePosition(QTextCursor::StartOfBlock);
        cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        q->setTextCursor(cursor);
        _event->accept();
        return true;
    }

    return false;
}

void BaseTextEdit::Implementation::updateSelectionFormatting(
    BaseTextEdit* _textEdit, std::function<QTextCharFormat(const QTextCharFormat&)> _updateFormat)
{
    auto cursor = _textEdit->textCursor();

    //
    // Для кейса с изменением формата без выделенного текста нужно использовать метод самого
    // редактора текста, видимо он как-то форсит это изменение, а если пробовать изменять чисто в
    // курсоре, то тогда изменение не будет применяться
    //
    if (!cursor.hasSelection()) {
        const auto newFormat = _updateFormat(cursor.charFormat());
        cursor.mergeCharFormat(newFormat);
        _textEdit->mergeCurrentCharFormat(newFormat);
        return;
    }

    TextHelper::updateSelectionFormatting(cursor, _updateFormat);
}

void BaseTextEdit::Implementation::pasteAsPlainTextFromClipboard()
{
    const auto keepLineBreaks = true;
    const auto textToInsert
        = TextHelper::simplified(QGuiApplication::clipboard()->text(), keepLineBreaks);
    q->insertPlainText(textToInsert);
}


// ****


BaseTextEdit::BaseTextEdit(QWidget* _parent)
    : CompleterTextEdit(_parent)
    , d(new Implementation(this))
{
    d->reconfigure();
}

BaseTextEdit::~BaseTextEdit() = default;

void BaseTextEdit::setCapitalizeWords(bool _capitalize)
{
    d->capitalizeWords = _capitalize;
}

void BaseTextEdit::setCorrectDoubleCapitals(bool _correct)
{
    d->correctDoubleCapitals = _correct;
}

void BaseTextEdit::setCapitalizeSingleILetter(bool _capitalize)
{
    d->capitalizeSingleILetter = _capitalize;
}

void BaseTextEdit::setReplaceThreeDots(bool _replace)
{
    d->replaceThreeDots = _replace;
}

void BaseTextEdit::setUseSmartQuotes(bool _use)
{
    d->smartQuotes = _use;
}

void BaseTextEdit::setReplaceTwoDashes(bool _replace)
{
    d->replaceTwoDashes = _replace;
}

void BaseTextEdit::setAvoidMultipleSpaces(bool _avoid)
{
    d->avoidMultipleSpaces = _avoid;
}

void BaseTextEdit::setPasteAsPlainTextAvailable(bool _available)
{
    d->pasteAsPlainTextAvailable = _available;
}

void BaseTextEdit::setFormattigAvailable(bool _available)
{
    d->formattingAvailable = _available;
}

void BaseTextEdit::setTextBold(bool _bold)
{
    auto buildFormat = [_bold](const QTextCharFormat& _format) {
        auto format = _format;
        format.setFontWeight(_bold ? QFont::Bold : QFont::Normal);
        return format;
    };
    d->updateSelectionFormatting(this, buildFormat);
}

void BaseTextEdit::setTextItalic(bool _italic)
{
    auto buildFormat = [_italic](const QTextCharFormat& _format) {
        auto format = _format;
        format.setFontItalic(_italic);
        return format;
    };
    d->updateSelectionFormatting(this, buildFormat);
}

void BaseTextEdit::setTextUnderline(bool _underline)
{
    auto buildFormat = [_underline](const QTextCharFormat& _format) {
        auto format = _format;
        format.setFontUnderline(_underline);
        return format;
    };
    d->updateSelectionFormatting(this, buildFormat);
}

void BaseTextEdit::setTextStrikethrough(bool _strikethrough)
{
    auto buildFormat = [_strikethrough](const QTextCharFormat& _format) {
        auto format = _format;
        format.setFontStrikeOut(_strikethrough);
        return format;
    };
    d->updateSelectionFormatting(this, buildFormat);
}

void BaseTextEdit::invertTextBold()
{
    setTextBold(!cursorForFormatInversion(textCursor()).charFormat().font().bold());
}

void BaseTextEdit::invertTextItalic()
{
    setTextItalic(!cursorForFormatInversion(textCursor()).charFormat().font().italic());
}

void BaseTextEdit::invertTextUnderline()
{
    setTextUnderline(!cursorForFormatInversion(textCursor()).charFormat().font().underline());
}

void BaseTextEdit::invertTextStrikethrough()
{
    setTextStrikethrough(!cursorForFormatInversion(textCursor()).charFormat().font().strikeOut());
}

void BaseTextEdit::setTextFont(const QFont& _font)
{
    auto buildFormat = [_font](const QTextCharFormat& _format) {
        auto format = _format;
        format.setFont(_font);
        return format;
    };
    d->updateSelectionFormatting(this, buildFormat);
}

void BaseTextEdit::setTextAlignment(Qt::Alignment _alignment)
{
    auto cursor = textCursor();
    const int startPosition = std::min(cursor.selectionStart(), cursor.selectionEnd());
    const int lastPosition = std::max(cursor.selectionStart(), cursor.selectionEnd());
    cursor.setPosition(startPosition);
    do {
        auto blockFormat = cursor.blockFormat();
        if (blockFormat.alignment() != _alignment) {
            blockFormat.setAlignment(_alignment);
            cursor.setBlockFormat(blockFormat);
        }

        cursor.movePosition(QTextCursor::EndOfBlock);
        cursor.movePosition(QTextCursor::NextBlock);
    } while (!cursor.atEnd() && cursor.position() < lastPosition);

    cursor.setPosition(lastPosition);
    setTextCursor(cursor);
}

void BaseTextEdit::changeTextCase(bool _upper)
{
    //
    // Нужно ли убирать выделение после операции
    //
    bool clearSelection = false;
    //
    // Если выделения нет, работаем со словом под курсором
    //
    QTextCursor cursor = textCursor();
    const int sourcePosition = cursor.position();
    if (!cursor.hasSelection()) {
        cursor.select(QTextCursor::WordUnderCursor);
        clearSelection = true;
    }

    if (QString selectedText = cursor.selectedText(); !selectedText.isEmpty()) {
        cursor.beginEditBlock();
        const QChar firstChar = selectedText.at(0);
        const bool firstToUpper = TextHelper::smartToUpper(firstChar) != firstChar;
        const bool textInUpper = (selectedText.length() > 1)
            && (TextHelper::smartToUpper(selectedText) == selectedText);
        const int fromPosition = qMin(cursor.selectionStart(), cursor.selectionEnd());
        const int toPosition = qMax(cursor.selectionStart(), cursor.selectionEnd());
        for (int position = fromPosition; position < toPosition; ++position) {
            cursor.setPosition(position);
            cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
            selectedText = cursor.selectedText();
            if (_upper) {
                //
                // Поднимаем для всего текста, или только для первого символа
                //
                if (!firstToUpper || (firstToUpper && position == fromPosition)) {
                    cursor.insertText(TextHelper::smartToUpper(selectedText));
                }
            } else {
                //
                // Опускаем для всего текста, или для всех символов, кроме первого
                //
                if (!textInUpper || (textInUpper && position != fromPosition)) {
                    cursor.insertText(TextHelper::smartToLower(selectedText));
                }
            }
        }
        cursor.endEditBlock();

        if (clearSelection) {
            cursor.setPosition(sourcePosition);
        } else {
            cursor.setPosition(fromPosition);
            cursor.setPosition(toPosition, QTextCursor::KeepAnchor);
        }
        setTextCursor(cursor);
    }
}

ContextMenu* BaseTextEdit::createContextMenu(const QPoint& _position, QWidget* _parent)
{
    auto menu = CompleterTextEdit::createContextMenu(_position, _parent);
    if (isReadOnly() || hasSpellingMenu(_position)) {
        return menu;
    }

    //
    // Базовые опции контекстного меню
    //
    auto actions = menu->actions().toVector();

    if (d->formattingAvailable) {
        auto formattingAction = new QAction;
        formattingAction->setSeparator(true);
        formattingAction->setText(tr("Formatting"));
        formattingAction->setIconText(u8"\U000F0284");
        {
            auto boldAction = new QAction(formattingAction);
            boldAction->setText(tr("Bold"));
            boldAction->setWhatsThis(
                QKeySequence(QKeySequence::Bold).toString(QKeySequence::NativeText));
            connect(boldAction, &QAction::triggered, this, &BaseTextEdit::invertTextBold);
            //
            auto italicAction = new QAction(formattingAction);
            italicAction->setText(tr("Italic"));
            italicAction->setWhatsThis(
                QKeySequence(QKeySequence::Italic).toString(QKeySequence::NativeText));
            connect(italicAction, &QAction::triggered, this, &BaseTextEdit::invertTextItalic);
            //
            auto underlineAction = new QAction(formattingAction);
            underlineAction->setText(tr("Underline"));
            underlineAction->setWhatsThis(
                QKeySequence(QKeySequence::Underline).toString(QKeySequence::NativeText));
            connect(underlineAction, &QAction::triggered, this, &BaseTextEdit::invertTextUnderline);
            //
            auto strikethroughAction = new QAction(formattingAction);
            strikethroughAction->setText(tr("Strikethrough"));
            strikethroughAction->setWhatsThis(
                QKeySequence("Shift+Ctrl+X").toString(QKeySequence::NativeText));
            connect(strikethroughAction, &QAction::triggered, this,
                    &BaseTextEdit::invertTextStrikethrough);

            auto alignLeftAction = new QAction(formattingAction);
            alignLeftAction->setSeparator(true);
            alignLeftAction->setText(tr("Align left"));
            alignLeftAction->setWhatsThis(
                QKeySequence("Ctrl+L").toString(QKeySequence::NativeText));
            connect(alignLeftAction, &QAction::triggered, this,
                    [this] { setTextAlignment(Qt::AlignLeft); });
            //
            auto alignCenterAction = new QAction(formattingAction);
            alignCenterAction->setText(tr("Align center"));
            alignCenterAction->setWhatsThis(
                QKeySequence("Ctrl+E").toString(QKeySequence::NativeText));
            connect(alignCenterAction, &QAction::triggered, this,
                    [this] { setTextAlignment(Qt::AlignHCenter); });
            //
            auto alignRightAction = new QAction(formattingAction);
            alignRightAction->setText(tr("Align right"));
            alignRightAction->setWhatsThis(
                QKeySequence("Ctrl+R").toString(QKeySequence::NativeText));
            connect(alignRightAction, &QAction::triggered, this,
                    [this] { setTextAlignment(Qt::AlignRight); });
            //
            auto alignJustifyAction = new QAction(formattingAction);
            alignJustifyAction->setText(tr("Align justify"));
            alignJustifyAction->setWhatsThis(
                QKeySequence("Ctrl+J").toString(QKeySequence::NativeText));
            connect(alignJustifyAction, &QAction::triggered, this,
                    [this] { setTextAlignment(Qt::AlignJustify); });

            auto uppercaseAction = new QAction(formattingAction);
            uppercaseAction->setSeparator(true);
            uppercaseAction->setText(tr("Make uppercase"));
            uppercaseAction->setWhatsThis(
                QKeySequence("Ctrl+Shift+Up").toString(QKeySequence::NativeText));
            connect(uppercaseAction, &QAction::triggered, this, [this] { changeTextCase(true); });
            //
            auto lowercaseAction = new QAction(formattingAction);
            lowercaseAction->setText(tr("Make lowercase"));
            lowercaseAction->setWhatsThis(
                QKeySequence("Ctrl+Shift+Down").toString(QKeySequence::NativeText));
            connect(lowercaseAction, &QAction::triggered, this, [this] { changeTextCase(false); });
        }

        //
        // Показываем меню форматирования после базовых действий, и перед "выделить всё"
        //
        actions.insert(actions.size() - 1, formattingAction);
    }

    //
    // Добавляем возможность вставить текст из буфера обмена, как просто текст
    //
    if (d->pasteAsPlainTextAvailable && !QGuiApplication::clipboard()->text().isEmpty()) {
        auto pasteAsPlainTextAction = new QAction(menu);
        pasteAsPlainTextAction->setIconText(u8"\U000f68c0");
        pasteAsPlainTextAction->setText(tr("Paste without formatting"));
        pasteAsPlainTextAction->setWhatsThis(
            QKeySequence("Ctrl+Shift+V").toString(QKeySequence::NativeText));
        connect(pasteAsPlainTextAction, &QAction::triggered, this,
                [this] { d->pasteAsPlainTextFromClipboard(); });
        //
        // ... вставляем его после стандартной вставки
        //
        for (const auto action : actions) {
            if (action->whatsThis()
                == QKeySequence(QKeySequence::Paste).toString(QKeySequence::NativeText)) {
                actions.insert(actions.indexOf(action) + 1, pasteAsPlainTextAction);
                break;
            }
        }
    }

    menu->setActions(actions);
    return menu;
}

bool BaseTextEdit::event(QEvent* _event)
{
    switch (static_cast<int>(_event->type())) {
    case static_cast<QEvent::Type>(EventType::DesignSystemChangeEvent): {
        d->reconfigure();
        updateGeometry();
        update();
        return true;
    }


    case static_cast<int>(EventType::TextEditingOptionsChangeEvent): {
        const auto event = static_cast<TextEditingOptionsChangeEvent*>(_event);
        if (event->spelling.has_value()) {
            SpellCheckTextEdit::event(_event);
        }
        if (event->correctDoubleCapitals.has_value()) {
            setCorrectDoubleCapitals(event->correctDoubleCapitals.value());
        }
        if (event->capitalizeSingleILetter.has_value()) {
            setCapitalizeSingleILetter(event->capitalizeSingleILetter.value());
        }
        if (event->replaceThreeDots.has_value()) {
            setReplaceThreeDots(event->replaceThreeDots.value());
        }
        if (event->useSmartQuotes.has_value()) {
            setUseSmartQuotes(event->useSmartQuotes.value());
        }
        if (event->replaceTwoDashes.has_value()) {
            setReplaceTwoDashes(event->replaceTwoDashes.value());
        }
        if (event->avoidMultipleSpaces.has_value()) {
            setAvoidMultipleSpaces(event->avoidMultipleSpaces.value());
        }
        return false;
    }

    default: {
        return CompleterTextEdit::event(_event);
    }
    }
}

void BaseTextEdit::mousePressEvent(QMouseEvent* _event)
{
    const auto isHandled = d->selectBlockOnTripleClick(_event);
    if (isHandled) {
        return;
    }

    CompleterTextEdit::mousePressEvent(_event);
}

void BaseTextEdit::mouseDoubleClickEvent(QMouseEvent* _event)
{
    const auto isHandled = d->selectBlockOnTripleClick(_event);
    if (isHandled) {
        return;
    }

    CompleterTextEdit::mouseDoubleClickEvent(_event);
}

bool BaseTextEdit::keyPressEventReimpl(QKeyEvent* _event)
{
    bool isEventHandled = true;

    //
    // Переопределяем
    //

    //
    // Для форматирования нужно брать значение в стиле у последнего символа из выделения,
    // т.к. charFormat() отдаёт значение стиля предыдущего перед курсором символа
    //
    // ... сделать текст полужирным
    //
    if (_event == QKeySequence::Bold) {
        invertTextBold();
    }
    //
    // ... сделать текст курсивом
    //
    else if (_event == QKeySequence::Italic) {
        invertTextItalic();
    }
    //
    // ... сделать текст подчёркнутым
    //
    else if (_event == QKeySequence::Underline) {
        invertTextUnderline();
    }
    //
    // ... сделать текст перечёркнутым
    //
    else if (_event->key() == Qt::Key_X && _event->modifiers().testFlag(Qt::ShiftModifier)
             && _event->modifiers().testFlag(Qt::ControlModifier)) {
        invertTextStrikethrough();
    }
    //
    // Выравнивание
    //
    else if (_event->modifiers().testFlag(Qt::ControlModifier) && _event->key() == Qt::Key_L) {
        setTextAlignment(Qt::AlignLeft);
    } else if (_event->modifiers().testFlag(Qt::ControlModifier) && _event->key() == Qt::Key_E) {
        setTextAlignment(Qt::AlignHCenter);
    } else if (_event->modifiers().testFlag(Qt::ControlModifier) && _event->key() == Qt::Key_R) {
        setTextAlignment(Qt::AlignRight);
    } else if (_event->modifiers().testFlag(Qt::ControlModifier) && _event->key() == Qt::Key_J) {
        setTextAlignment(Qt::AlignJustify);
    }
    //
    // Поднятие/опускание регистра букв
    // Работает в три шага:
    // 1. ВСЕ ЗАГЛАВНЫЕ
    // 2. Первая заглавная
    // 3. все строчные
    //
    else if (_event->modifiers().testFlag(Qt::ControlModifier)
             && _event->modifiers().testFlag(Qt::ShiftModifier)
             && (_event->key() == Qt::Key_Up || _event->key() == Qt::Key_Down)) {
        changeTextCase(_event->key() == Qt::Key_Up);
    }
    //
    // Сдвигаем на одну строку
    // TODO: в macOS на эту комбинацию отрабатывает перемещение курсора в начало/конец документа
    //
    else if (_event->modifiers().testFlag(Qt::ControlModifier)
             && (_event->key() == Qt::Key_Up || _event->key() == Qt::Key_Down)) {
        setVerticalScroll(verticalScroll()
                          + verticalScrollSingleStep() * (_event->key() == Qt::Key_Up ? -1 : 1));
    }
    // ... перевод курсора к следующему символу
    //
    else if (_event == QKeySequence::MoveToNextChar) {
        const auto moveAnchor = _event->modifiers().testFlag(Qt::ShiftModifier)
            ? QTextCursor::KeepAnchor
            : QTextCursor::MoveAnchor;
        auto cursor = textCursor();
        if (textCursor().block().textDirection() == Qt::LeftToRight) {
            do {
                cursor.movePosition(QTextCursor::NextCharacter, moveAnchor);
            } while (!cursor.atEnd() && !canShowCursor(cursor));
        } else {
            do {
                cursor.movePosition(QTextCursor::PreviousCharacter, moveAnchor);
            } while (!cursor.atStart() && !canShowCursor(cursor));
        }
        setTextCursor(cursor);
    }
    //
    // ... перевод курсора к предыдущему символу
    //
    else if (_event == QKeySequence::MoveToPreviousChar) {
        const auto moveAnchor = _event->modifiers().testFlag(Qt::ShiftModifier)
            ? QTextCursor::KeepAnchor
            : QTextCursor::MoveAnchor;
        auto cursor = textCursor();
        if (textCursor().block().textDirection() == Qt::LeftToRight) {
            do {
                cursor.movePosition(QTextCursor::PreviousCharacter, moveAnchor);
            } while (!cursor.atStart() && !canShowCursor(cursor));
        } else {
            do {
                cursor.movePosition(QTextCursor::NextCharacter, moveAnchor);
            } while (!cursor.atEnd() && !canShowCursor(cursor));
        }
        setTextCursor(cursor);
    }
    //
    // ... перевод курсора к концу строки
    //
    else if (_event == QKeySequence::MoveToEndOfLine || _event == QKeySequence::SelectEndOfLine) {
        QTextCursor cursor = textCursor();
        const int startY = cursorRect(cursor).y();
        const QTextCursor::MoveMode keepAncor = _event->modifiers().testFlag(Qt::ShiftModifier)
            ? QTextCursor::KeepAnchor
            : QTextCursor::MoveAnchor;
        while (!cursor.atBlockEnd()) {
            cursor.movePosition(QTextCursor::NextCharacter, keepAncor);
            if (cursorRect(cursor).y() > startY) {
                cursor.movePosition(QTextCursor::PreviousCharacter, keepAncor);
                setTextCursor(cursor);
                break;
            }
        }
        setTextCursor(cursor);
    }
    //
    // ... перевод курсора к началу строки
    //
    else if (_event == QKeySequence::MoveToStartOfLine
             || _event == QKeySequence::SelectStartOfLine) {
        QTextCursor cursor = textCursor();
        const int startY = cursorRect(cursor).y();
        const QTextCursor::MoveMode keepAncor = _event->modifiers().testFlag(Qt::ShiftModifier)
            ? QTextCursor::KeepAnchor
            : QTextCursor::MoveAnchor;
        while (!cursor.atBlockStart()) {
            cursor.movePosition(QTextCursor::PreviousCharacter, keepAncor);
            if (cursorRect(cursor).y() < startY) {
                cursor.movePosition(QTextCursor::NextCharacter, keepAncor);
                setTextCursor(cursor);
                break;
            }
        }
        setTextCursor(cursor);
    }
    //
    // ... перевод к концу документа
    //
    else if (_event == QKeySequence::MoveToEndOfDocument) {
        auto cursor = textCursor();
        cursor.movePosition(QTextCursor::End);
        while (!cursor.atStart() && !cursor.block().isVisible()) {
            cursor.movePosition(QTextCursor::PreviousBlock);
            cursor.movePosition(QTextCursor::EndOfBlock);
        }
        setTextCursor(cursor);
    }
    //
    // ... выделить все
    //
    else if (_event == QKeySequence::SelectAll) {
        auto cursor = textCursor();
        cursor.select(QTextCursor::Document);
        setTextCursor(cursor);
    }
    //
    // ... вставим перенос строки внутри абзаца
    //
    else if ((_event->key() == Qt::Key_Enter || _event->key() == Qt::Key_Return)
             && _event->modifiers().testFlag(Qt::ShiftModifier)) {
        textCursor().insertText(QChar(QChar::LineSeparator));
    }
    //
    // ... копировать весь абзац, если текст не выделен
    //
    else if (auto cursor = textCursor(); _event == QKeySequence::Copy && !cursor.hasSelection()) {
        const int position = cursor.position();
        cursor.select(QTextCursor::BlockUnderCursor);
        setTextCursor(cursor);
        copy();
        cursor.setPosition(position);
        setTextCursor(cursor);
    }
    //
    // ... вырезать текст
    //
    else if (_event == QKeySequence::Cut) {
        QTextCursor cursor = textCursor();
        //
        // ... если текст не выделен, вырезать весь абзац
        //
        if (!cursor.hasSelection()) {
            cursor.select(QTextCursor::BlockUnderCursor);
        }
        setTextCursor(cursor);
        cut();
    }
    //
    // ... вставить текст как простой
    //
    else if (d->pasteAsPlainTextAvailable && _event->key() == Qt::Key_V
             && _event->modifiers().testFlag(Qt::ShiftModifier)
             && _event->modifiers().testFlag(Qt::ControlModifier)) {
        d->pasteAsPlainTextFromClipboard();
    }
#ifdef Q_OS_MAC
    //
    // Control + Option + . для вставки точки независимо от раскладки
    //
    else if (_event->modifiers().testFlag(Qt::MetaModifier)
             && _event->modifiers().testFlag(Qt::AltModifier)
             && (_event->key() == Qt::Key_Period || _event->key() == 1070)) {
        insertPlainText(".");
    }
    //
    // Control + Option + , для вставки запятой независимо от раскладки
    //
    else if (_event->modifiers().testFlag(Qt::MetaModifier)
             && _event->modifiers().testFlag(Qt::AltModifier)
             && (_event->key() == Qt::Key_Comma || _event->key() == 1041)) {
        insertPlainText(",");
    }
    //
    // Home & End перекидываем в начало и конец строки
    //
    else if (_event->key() == Qt::Key_Home) {
        auto cursor = textCursor();
        cursor.movePosition(QTextCursor::StartOfLine);
        setTextCursor(cursor);
    } else if (_event->key() == Qt::Key_End) {
        auto cursor = textCursor();
        cursor.movePosition(QTextCursor::EndOfLine);
        setTextCursor(cursor);
    }
    //
    // Двойной пробел добавляет точку
    //
    else if (_event->key() == Qt::Key_Space) {
        const auto spacePressedAt = QDateTime::currentMSecsSinceEpoch();
        //
        // Выделяем два последних символа и проверяем, что первый из них буква или число,
        // второй пробел, а ещё нужно, чтобы предыдущее нажатие пробела было совсем недавно
        //
        auto cursor = textCursor();
        cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor, 2);
        if (cursor.hasSelection() && cursor.selectedText().size() == 2
            && cursor.selectedText().at(0).isLetterOrNumber()
            && cursor.selectedText().at(1).isSpace()
            && spacePressedAt - d->lastSpaceWasPressed
                <= QApplication::styleHints()->mouseDoubleClickInterval()) {
            d->lastSpaceWasPressed = 0;
            cursor.clearSelection();
            cursor.movePosition(QTextCursor::NextCharacter);
            cursor.insertText(".");
        }
        //
        // В противном случае лишь запоминаем время, когда был нажат пробел в очередной раз
        //
        else {
            d->lastSpaceWasPressed = spacePressedAt;
            isEventHandled = false;
        }
    }
#endif
    //
    // Оставляем необработанным
    //
    else {
        isEventHandled = false;
    }

    return isEventHandled;
}

bool BaseTextEdit::updateEnteredText(const QKeyEvent* _event)
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

    //
    // Определяем необходимость установки верхнего регистра для первого символа блока
    //
    if (d->capitalizeWords && cursorBackwardText != " " && cursorBackwardText == eventText
        && eventText[0] != TextHelper::smartToUpper(eventText[0])) {
        //
        // Сформируем правильное представление строки
        //
        QString correctedText = eventText;
        correctedText[0] = TextHelper::smartToUpper(correctedText[0]);

        //
        // Стираем предыдущий введённый текст
        //
        for (int repeats = 0; repeats < eventText.length(); ++repeats) {
            cursor.deletePreviousChar();
        }

        //
        // Выводим необходимый
        //
        cursor.insertText(correctedText);
        setTextCursor(cursor);

        return true;
    }

    //
    // Если перед нами конец предложения
    // и не сокращение
    // и после курсора нет текста (для ремарки допустима скобка)
    //
    const QString endOfSentancePattern
        = QString("([.]|[?]|[!]|[…]) %1$").arg(TextHelper::toRxEscaped(eventText));
    if (d->capitalizeWords && cursorBackwardText.contains(QRegularExpression(endOfSentancePattern))
        && !stringEndsWithAbbrev(cursorBackwardText) && cursorForwardText.isEmpty()
        && eventText[0] != TextHelper::smartToUpper(eventText[0])) {
        //
        // Сделаем первую букву заглавной
        //
        QString correctedText = eventText;
        correctedText[0] = TextHelper::smartToUpper(correctedText[0]);

        //
        // Стираем предыдущий введённый текст
        //
        for (int repeats = 0; repeats < eventText.length(); ++repeats) {
            cursor.deletePreviousChar();
        }

        //
        // Выводим необходимый
        //
        cursor.insertText(correctedText);
        setTextCursor(cursor);

        return true;
    }

    //
    // Исправляем проблему ДВойных ЗАглавных
    //
    if (d->correctDoubleCapitals) {
        const auto isPreviousCharacterUpper = cursorBackwardText.length() > 3
            && cursorBackwardText[cursorBackwardText.length() - 4].isLetter()
            && cursorBackwardText[cursorBackwardText.length() - 4]
                == TextHelper::smartToUpper(cursorBackwardText[cursorBackwardText.length() - 4]);
        const auto right3Characters = cursorBackwardText.right(3).simplified();

        //
        // Если две из трёх последних букв находятся в верхнем регистре, то это наш случай
        //
        if (!right3Characters.contains(" ") && right3Characters.length() == 3
            && right3Characters != TextHelper::smartToUpper(right3Characters)
            && right3Characters.left(2) == TextHelper::smartToUpper(right3Characters.left(2))
            && right3Characters[0].isLetter() && right3Characters[1].isLetter()
            && eventText != TextHelper::smartToUpper(eventText) && !isPreviousCharacterUpper) {
            //
            // Сделаем предпоследнюю букву строчной
            //
            QString correctedText = right3Characters.mid(1);
            correctedText[0] = TextHelper::smartToLower(correctedText[0]);

            //
            // Стираем предыдущий введённый текст
            //
            for (int repeats = 0; repeats < correctedText.length(); ++repeats) {
                cursor.deletePreviousChar();
            }

            //
            // Выводим необходимый
            //
            cursor.insertText(correctedText);
            setTextCursor(cursor);

            return true;
        }
    }

    //
    // Заменяем три точки символом многоточия
    //
    if (d->replaceThreeDots && eventText == "." && cursorBackwardText.endsWith("...")) {
        //
        // Три последних символа
        //
        cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor, 3);
        cursor.insertText("…");

        return true;
    }

    //
    // Корректируем кавычки
    //
    if (d->smartQuotes) {
        //
        // ... двойные кавычки
        //
        if (eventText == "\"") {
            //
            // Выделим введённый символ
            //
            cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor, 1);
            //
            // Определим предшествующий текст
            //
            QTextCursor cursorCopy = cursor;
            cursorCopy.setPosition(cursor.selectionStart());
            cursorCopy.movePosition(QTextCursor::StartOfBlock, QTextCursor::KeepAnchor);

            if (cursorCopy.selectedText().isEmpty()
                || QStringList({ " ", "(" }).contains(cursorCopy.selectedText().right(1))) {
                cursor.insertText(localOpenDoubleQuote());
            } else {
                cursor.insertText(localCloseDoubleQuote());
            }

            return true;
        }
        //
        // ... одинарные кавычки
        //
        else if (eventText == "\'") {
            //
            // Выделим введённый символ
            //
            cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor, 1);
            //
            // Определим предшествующий текст
            //
            QTextCursor cursorCopy = cursor;
            cursorCopy.setPosition(cursor.selectionStart());
            cursorCopy.movePosition(QTextCursor::StartOfBlock, QTextCursor::KeepAnchor);

            if (cursorCopy.selectedText().isEmpty()
                || QStringList({ " ", "(" }).contains(cursorCopy.selectedText().right(1))) {
                cursor.insertText(localOpenSingleQuote());
            } else {
                cursor.insertText(localCloseSingleQuote());
            }

            return true;
        }
    }

    //
    // Заменяем два тире символом длинного тире
    //
    if (d->replaceTwoDashes && eventText == "-" && cursorBackwardText.endsWith("--")) {
        //
        // Два последних символа
        //
        cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor, 2);
        cursor.insertText("—");

        return true;
    }

    //
    // Заменяем i на I для английского
    //
    if (d->capitalizeSingleILetter && !eventText.isEmpty()
        && (eventText.front().isPunct() || eventText.front().isSpace())
        && cursorBackwardText.left(cursorBackwardText.length() - eventText.length())
               .endsWith(" i")) {
        //
        // Несколько последних символа
        //
        cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor,
                            2 + eventText.length());
        cursor.insertText(QString(" I%1").arg(eventText));

        return true;
    }

    //
    // Если запрещено вводить несколько пробелов подряд
    //
    if (d->avoidMultipleSpaces) {
        //
        // ... и была попытка ввести несколько пробелов подряд, или пробел в начале строки,
        //     удаляем этот лишний пробел
        //
        if (cursorBackwardText == " " || cursorBackwardText.endsWith("  ")) {
            cursor.deletePreviousChar();
        }
        //
        // ... в кейсе когда пробел ставится после слова, но перед двумя пробелами, как бы смещаем
        //     курсор внутрь этих двух пробелов
        //
        else if (cursorBackwardText.endsWith(" ") && cursorForwardText.startsWith("  ")) {
            cursor.deleteChar();
        }

        return true;
    }

    return false;
}

void BaseTextEdit::doSetTextCursor(const QTextCursor& _cursor)
{
    QTextCursor cursor = _cursor;
    const auto sourceSelectionStart = _cursor.selectionStart() != _cursor.position()
        ? _cursor.selectionStart()
        : _cursor.selectionEnd();
    const auto sourceSelectionEnd = _cursor.selectionStart() != _cursor.position()
        ? _cursor.selectionEnd()
        : _cursor.selectionStart();
    const bool isSelectionForward = sourceSelectionStart < sourceSelectionEnd;
    cursor.setPosition(sourceSelectionStart);
    while ((isSelectionForward ? !cursor.atEnd() : !cursor.atStart()) && !canShowCursor(cursor)) {
        const auto isCursorMoved = cursor.movePosition(
            isSelectionForward ? QTextCursor::NextBlock : QTextCursor::PreviousBlock);
        if (isCursorMoved) {
            cursor.movePosition(isSelectionForward ? QTextCursor::StartOfBlock
                                                   : QTextCursor::EndOfBlock);
        } else {
            cursor.movePosition(isSelectionForward ? QTextCursor::EndOfBlock
                                                   : QTextCursor::StartOfBlock);
        }
    }
    const int selectionsStart = cursor.position();

    //
    // Если естьв ыделение, определим и завершающую позицию курсора
    //
    if (_cursor.hasSelection()) {
        cursor.setPosition(sourceSelectionEnd);
        while ((isSelectionForward ? !cursor.atStart() : !cursor.atEnd())
               && !canShowCursor(cursor)) {
            const auto isCursorMoved = cursor.movePosition(
                isSelectionForward ? QTextCursor::PreviousBlock : QTextCursor::NextBlock);
            if (isCursorMoved) {
                cursor.movePosition(isSelectionForward ? QTextCursor::EndOfBlock
                                                       : QTextCursor::StartOfBlock);
            } else {
                cursor.movePosition(isSelectionForward ? QTextCursor::StartOfBlock
                                                       : QTextCursor::EndOfBlock);
            }
        }
        const int selectionEnd = cursor.position();
        cursor.setPosition(selectionsStart);
        cursor.setPosition(selectionEnd, QTextCursor::KeepAnchor);
    }
    //
    // А если выделения нет, то проверим кейс, когда пользователь нажимает влево находять в описании
    // самого первого бита истории
    //
    else {
        if (!cursor.block().isVisible()) {
            while (!cursor.atEnd() && !canShowCursor(cursor)) {
                const auto isCursorMoved = cursor.movePosition(QTextCursor::NextBlock);
                if (isCursorMoved) {
                    cursor.movePosition(QTextCursor::StartOfBlock);
                } else {
                    cursor.movePosition(QTextCursor::EndOfBlock);
                }
            }
        }
    }

    CompleterTextEdit::doSetTextCursor(cursor);
}
