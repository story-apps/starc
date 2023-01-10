#include "simple_text_edit.h"

#include "handlers/key_press_handler_facade.h"

#include <business_layer/document/simple_text/simple_text_document.h>
#include <business_layer/document/text/text_block_data.h>
#include <business_layer/document/text/text_cursor.h>
#include <business_layer/import/text/simple_text_markdown_importer.h>
#include <business_layer/model/simple_text/simple_text_model.h>
#include <business_layer/model/text/text_model_text_item.h>
#include <business_layer/templates/simple_text_template.h>
#include <business_layer/templates/templates_facade.h>
#include <domain/starcloud_api.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/context_menu/context_menu.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/text_helper.h>

#include <QAction>
#include <QCoreApplication>
#include <QLocale>
#include <QMimeData>
#include <QPainter>
#include <QPointer>
#include <QRegularExpression>
#include <QScrollBar>
#include <QTextTable>

using BusinessLayer::TemplatesFacade;
using BusinessLayer::TextBlockStyle;
using BusinessLayer::TextCursor;
using BusinessLayer::TextParagraphType;

namespace Ui {

class SimpleTextEdit::Implementation
{
public:
    explicit Implementation(SimpleTextEdit* _q);

    /**
     * @brief Текущий шаблон документа
     */
    const BusinessLayer::TextTemplate& textTemplate() const;

    /**
     * @brief Отменить/повторить последнее действие
     */
    void revertAction(bool previous);


    SimpleTextEdit* q = nullptr;

    QPointer<BusinessLayer::SimpleTextModel> model;
    BusinessLayer::SimpleTextDocument document;

    bool showSceneNumber = false;
    bool showSceneNumberOnLeft = false;
    bool showSceneNumberOnRight = false;
    bool showDialogueNumber = false;

    QVector<Domain::CursorInfo> collaboratorsCursorInfo;
};

SimpleTextEdit::Implementation::Implementation(SimpleTextEdit* _q)
    : q(_q)
{
}

const BusinessLayer::TextTemplate& SimpleTextEdit::Implementation::textTemplate() const
{
    return TemplatesFacade::textTemplate(model);
}

void SimpleTextEdit::Implementation::revertAction(bool previous)
{
    if (model == nullptr) {
        return;
    }

    const auto lastCursorPosition = q->textCursor().position();
    //
    if (previous) {
        model->undo();
    } else {
        model->redo();
    }
    //
    if (document.characterCount() > lastCursorPosition) {
        auto cursor = q->textCursor();
        cursor.setPosition(lastCursorPosition);
        q->setTextCursorAndKeepScrollBars(cursor);
        q->ensureCursorVisible();

        //
        // При отмене/повторе последнего действия позиция курсора могла и не поменяться,
        // но тип параграфа сменился, поэтому перестраховываемся и говорим будто бы
        // сменилась позиция курсора, чтобы обновить состояние панелей
        //
        emit q->cursorPositionChanged();
    }
}


// ****


SimpleTextEdit::SimpleTextEdit(QWidget* _parent)
    : ScriptTextEdit(_parent)
    , d(new Implementation(this))
{
    setContextMenuPolicy(Qt::CustomContextMenu);
    setFrameShape(QFrame::NoFrame);
    setShowPageNumbers(true);

    setDocument(&d->document);
    setCapitalizeWords(false);
}

SimpleTextEdit::~SimpleTextEdit() = default;

void SimpleTextEdit::initWithModel(BusinessLayer::SimpleTextModel* _model)
{
    if (d->model) {
        d->model->disconnect(this);
    }

    d->model = _model;

    if (usePageMode()) {
        const auto currentTemplate = TemplatesFacade::textTemplate(_model);
        setPageFormat(currentTemplate.pageSizeId());
        setPageMarginsMm(currentTemplate.pageMargins());
        setPageNumbersAlignment(currentTemplate.pageNumbersAlignment());
    }

    //
    // Документ нужно формировать только после того, как редактор настроен, чтобы избежать лишний
    // изменений
    //
    d->document.setModel(d->model);

    if (d->model) {
        connect(d->model, &BusinessLayer::SimpleTextModel::dataChanged, this,
                qOverload<>(&SimpleTextEdit::update));
    }
}

void SimpleTextEdit::reinit()
{
    //
    // Перенастроим всё, что зависит от шаблона
    //
    initWithModel(d->model);
}

const BusinessLayer::TextTemplate& SimpleTextEdit::textTemplate() const
{
    return d->textTemplate();
}

void SimpleTextEdit::undo()
{
    d->revertAction(true);
}

void SimpleTextEdit::redo()
{
    d->revertAction(false);
}

void SimpleTextEdit::addParagraph(BusinessLayer::TextParagraphType _type)
{
    d->document.addParagraph(_type, textCursor());

    emit paragraphTypeChanged();
}

void SimpleTextEdit::setCurrentParagraphType(BusinessLayer::TextParagraphType _type)
{
    if (currentParagraphType() == _type) {
        return;
    }

    d->document.setParagraphType(_type, textCursor());

    emit paragraphTypeChanged();
}

BusinessLayer::TextParagraphType SimpleTextEdit::currentParagraphType() const
{
    return TextBlockStyle::forBlock(textCursor().block());
}

QModelIndex SimpleTextEdit::currentModelIndex() const
{
    if (d->model == nullptr) {
        return {};
    }

    auto userData = textCursor().block().userData();
    if (userData == nullptr) {
        return {};
    }

    auto screenplayBlockData = static_cast<BusinessLayer::TextBlockData*>(userData);
    return d->model->indexForItem(screenplayBlockData->item());
}

void SimpleTextEdit::setCurrentModelIndex(const QModelIndex& _index)
{
    if (!_index.isValid() || _index.model() != d->model) {
        return;
    }

    QTextCursor textCursor(document());
    textCursor.setPosition(d->document.itemStartPosition(_index));
    ensureCursorVisible(textCursor);
}

int SimpleTextEdit::positionForModelIndex(const QModelIndex& _index)
{
    return d->document.itemStartPosition(_index);
}

void SimpleTextEdit::addReviewMark(const QColor& _textColor, const QColor& _backgroundColor,
                                   const QString& _comment)
{
    QTextCursor cursor(textCursor());
    if (!cursor.hasSelection()) {
        return;
    }

    d->document.addReviewMark(_textColor, _backgroundColor, _comment, cursor);
}

void SimpleTextEdit::setCursors(const QVector<Domain::CursorInfo>& _cursors)
{
    d->collaboratorsCursorInfo = _cursors;

    update();
}

void SimpleTextEdit::keyPressEvent(QKeyEvent* _event)
{
    if (isReadOnly()) {
        ScriptTextEdit::keyPressEvent(_event);
        return;
    }

    //
    // Подготовим событие к обработке
    //
    _event->setAccepted(false);

    //
    // Получим обработчик
    //
    auto handler = KeyProcessingLayer::KeyPressHandlerFacade::instance(this);

    //
    // Подготовка к обработке
    //
    handler->prepare(_event);

    //
    // Предварительная обработка
    //
    handler->prepareForHandle(_event);

    //
    // Отправить событие в базовый класс
    //
    if (handler->needSendEventToBaseClass()) {
        if (keyPressEventReimpl(_event)) {
            _event->accept();
        } else {
            ScriptTextEdit::keyPressEvent(_event);
            _event->ignore();
        }

        updateEnteredText(_event->text());
    }

    //
    // Обработка
    //
    if (!_event->isAccepted()) {
        handler->handle(_event);
    }

    //
    // Событие дошло по назначению
    //
    _event->accept();

    //
    // Убедимся, что курсор виден
    //
    if (handler->needEnsureCursorVisible()) {
        ensureCursorVisible();
    }

    //
    // Подготовим следующий блок к обработке
    //
    if (handler->needPrehandle()) {
        handler->prehandle();
    }
}

bool SimpleTextEdit::keyPressEventReimpl(QKeyEvent* _event)
{
    bool isEventHandled = true;

    //
    // Переопределяем
    //
    // ... отмена последнего действия
    //
    if (_event == QKeySequence::Undo) {
        undo();
    }
    //
    // ... отмена последнего действия
    //
    else if (_event == QKeySequence::Redo) {
        redo();
    }
    //
    // ... вырезать текст
    //
    else if (_event == QKeySequence::Cut) {
        copy();
        QTextCursor cursor = textCursor();
        cursor.removeSelectedText();
        d->model->saveChanges();
    }
    //
    // ... вставить текст
    //
    else if (_event == QKeySequence::Paste) {
        paste();
        d->model->saveChanges();
    }
    //
    // Обрабатываем в базовом классе
    //
    else {
        isEventHandled = ScriptTextEdit::keyPressEventReimpl(_event);
    }

    return isEventHandled;
}

void SimpleTextEdit::paintEvent(QPaintEvent* _event)
{
    ScriptTextEdit::paintEvent(_event);

    //
    // Определить область прорисовки по краям от текста
    //
    const bool isLeftToRight = QLocale().textDirection() == Qt::LeftToRight;
    const qreal pageLeft = 0;
    const qreal pageRight
        = viewport()->width() - verticalScrollBar()->width() - DesignSystem::layout().px8();
    const qreal spaceBetweenSceneNumberAndText = DesignSystem::layout().px(10);
    const qreal textLeft = pageLeft - (isLeftToRight ? 0 : horizontalScrollBar()->maximum())
        + document()->rootFrame()->frameFormat().leftMargin() - spaceBetweenSceneNumberAndText;
    const qreal textRight = pageRight + (isLeftToRight ? horizontalScrollBar()->maximum() : 0)
        - document()->rootFrame()->frameFormat().rightMargin() + spaceBetweenSceneNumberAndText;
    const qreal leftDelta = (isLeftToRight ? -1 : 1) * horizontalScrollBar()->value();
    qreal verticalMargin = 0;


    //
    // Определим начальный блок на экране
    //
    QTextBlock topBlock = document()->lastBlock();
    {
        const auto topCursor = cursorForPositionReimpl(viewport()->mapFromParent(QPoint(0, 0)));
        if (topBlock.blockNumber() > topCursor.block().blockNumber()) {
            topBlock = topCursor.block();
        }
    }
    //
    // ... идём до начала сцены
    //
    while (TextBlockStyle::forBlock(topBlock) != TextParagraphType::ChapterHeading1
           && TextBlockStyle::forBlock(topBlock) != TextParagraphType::ChapterHeading2
           && TextBlockStyle::forBlock(topBlock) != TextParagraphType::ChapterHeading3
           && TextBlockStyle::forBlock(topBlock) != TextParagraphType::ChapterHeading4
           && TextBlockStyle::forBlock(topBlock) != TextParagraphType::ChapterHeading5
           && TextBlockStyle::forBlock(topBlock) != TextParagraphType::ChapterHeading6
           && topBlock != document()->firstBlock()) {
        topBlock = topBlock.previous();
    }

    //
    // Определим последний блок на экране
    //
    QTextBlock bottomBlock = document()->firstBlock();
    {
        const auto bottomCursor
            = cursorForPositionReimpl(viewport()->mapFromParent(QPoint(0, viewport()->height())));
        if (bottomBlock.blockNumber() < bottomCursor.block().blockNumber()) {
            bottomBlock = bottomCursor.block();
        }
    }
    if (bottomBlock == document()->firstBlock()) {
        bottomBlock = document()->lastBlock();
    }
    bottomBlock = bottomBlock.next();

    //
    // Прорисовка дополнительных элементов редактора
    //
    QPainter painter(viewport());
    {
        //
        // Декорации текста
        //
        {
            clipPageDecorationRegions(&painter);

            //
            // Проходим блоки на экране и декорируем их
            //
            QTextBlock block = topBlock;
            const QRectF viewportGeometry = viewport()->geometry();
            int previousSceneBlockBottom = 0;
            int lastSceneBlockBottom = 0;
            QVector<QColor> lastSceneColors;
            bool isLastBlockSceneHeadingWithNumberAtRight = false;
            //
            auto setPainterPen = [&painter, &block, this](const QColor& _color) {
                painter.setPen(ColorHelper::transparent(
                    _color,
                    1.0
                        - (isFocusCurrentParagraph() && block != textCursor().block()
                               ? Ui::DesignSystem::inactiveTextOpacity()
                               : 0.0)));
            };

            QTextCursor cursor(document());
            while (block.isValid() && block != bottomBlock) {
                //
                // Стиль текущего блока
                //
                const auto blockType = TextBlockStyle::forBlock(block);

                cursor.setPosition(block.position());
                const QRect cursorR = cursorRect(cursor);
                cursor.movePosition(QTextCursor::EndOfBlock);
                const QRect cursorREnd = cursorRect(cursor);
                //
                verticalMargin = cursorR.height() / 2;

                //
                // Определим цвет главы
                //

                switch (blockType) {
                case TextParagraphType::ChapterHeading1:
                case TextParagraphType::ChapterHeading2:
                case TextParagraphType::ChapterHeading3:
                case TextParagraphType::ChapterHeading4:
                case TextParagraphType::ChapterHeading5:
                case TextParagraphType::ChapterHeading6: {
                    previousSceneBlockBottom = lastSceneBlockBottom;
                    lastSceneBlockBottom = cursorR.top();
                    lastSceneColors = d->document.itemColors(block);
                    break;
                }
                default: {
                    break;
                }
                }

                //
                // Нарисуем цвета сцены
                //
                if (!lastSceneColors.isEmpty()) {
                    const QPointF topLeft(isLeftToRight
                                              ? pageRight + leftDelta - DesignSystem::layout().px4()
                                              : pageLeft + leftDelta,
                                          lastSceneBlockBottom - verticalMargin);
                    const QPointF bottomRight(isLeftToRight ? pageRight + leftDelta
                                                            : pageLeft + leftDelta
                                                      + DesignSystem::layout().px4(),
                                              cursorREnd.bottom() + verticalMargin);
                    QRectF rect(topLeft, bottomRight);
                    for (const auto& color : std::as_const(lastSceneColors)) {
                        if (!color.isValid()) {
                            continue;
                        }

                        auto colorRect = rect;
                        if (color != lastSceneColors.constLast()) {
                            colorRect.setTop(previousSceneBlockBottom);
                        }

                        painter.setPen(Qt::NoPen);
                        painter.setBrush(color);
                        painter.drawRect(colorRect);
                        rect.moveLeft(rect.left() - DesignSystem::layout().px12());
                    }
                }

                //
                // Курсор на экране
                //
                // ... ниже верхней границы
                if ((cursorR.top() > 0 || cursorR.bottom() > 0)
                    // ... и выше нижней
                    && cursorR.top() < viewportGeometry.bottom()) {

                    //
                    // Прорисовка закладок
                    //
                    const auto bookmark = d->document.bookmark(block);
                    if (bookmark.isValid()) {
                        setPainterPen(bookmark.color);
                        painter.setFont(DesignSystem::font().iconsForEditors());

                        //
                        // Определим область для отрисовки
                        //
                        QPointF topLeft(isLeftToRight
                                            ? (pageLeft + leftDelta
                                               + Ui::DesignSystem::card().shadowMargins().left())
                                            : (textRight + leftDelta),
                                        cursorR.top());
                        QPointF bottomRight(
                            isLeftToRight ? textLeft + leftDelta
                                          : (pageRight + leftDelta
                                             - Ui::DesignSystem::card().shadowMargins().right()),
                            cursorR.bottom());
                        QRectF rect(topLeft, bottomRight);
                        const auto yDelta = Ui::DesignSystem::layout().px(32) - rect.height() / 2.0;
                        //
                        // корректируем размер области, чтобы получить квадрат для отрисовки иконки
                        // закладки
                        //
                        if (yDelta > 0) {
                            rect.adjust(0, -yDelta, 0, yDelta);
                        }
                        if (isLeftToRight) {
                            rect.setWidth(rect.height());
                        } else {
                            rect.setLeft(rect.right() - rect.height());
                        }
                        painter.fillRect(
                            rect,
                            ColorHelper::transparent(bookmark.color,
                                                     Ui::DesignSystem::elevationEndOpacity()));
                        painter.drawText(rect, Qt::AlignCenter, u8"\U000F00C0");
                    }

                    //
                    // Прорисовка декораций пустой строки
                    //
                    if (block.text().simplified().isEmpty()) {
                        //
                        // Настроим цвет
                        //
                        setPainterPen(ColorHelper::transparent(
                            palette().text().color(), Ui::DesignSystem::inactiveItemOpacity()));

                        //
                        // Рисуем индикатор пустой строки
                        //
                        painter.setFont(block.charFormat().font());
                        const QString emptyLineMark = "» ";
                        //
                        // Определим область для отрисовки и выведем символ в редактор
                        //
                        const QPointF topLeft(isLeftToRight ? pageLeft + leftDelta
                                                            : textRight + leftDelta,
                                              cursorR.top());
                        const QPointF bottomRight(isLeftToRight ? textLeft + leftDelta
                                                                : pageRight + leftDelta,
                                                  cursorR.bottom() + 2);
                        const QRectF rect(topLeft, bottomRight);
                        painter.drawText(rect, Qt::AlignRight | Qt::AlignTop, emptyLineMark);
                    }
                }

                block = block.next();
            }
        }
    }

    //
    // Курсоры соавторов
    //
    if (!d->collaboratorsCursorInfo.isEmpty()) {
        for (const auto& cursorInfo : std::as_const(d->collaboratorsCursorInfo)) {
            //
            // Пропускаем курсоры, которые находятся за пределами экрана
            //
            const auto cursorPosition = cursorInfo.cursorData.toInt();
            if (bottomBlock.isValid()
                && (cursorPosition < topBlock.position()
                    || cursorPosition > (bottomBlock.position() + bottomBlock.length()))) {
                continue;
            }


            TextCursor cursor(document());
            cursor.setPosition(cursorPosition);
            const auto cursorR = cursorRect(cursor).adjusted(0, 0, 1, 0);

            const auto backgroundColor = ColorHelper::forText(cursorInfo.name);

            //
            // ... рисуем его
            //
            painter.fillRect(cursorR, backgroundColor);

            //
            // ... выводим имя соавтора
            //
            painter.setFont(DesignSystem::font().subtitle2());
            const QRect usernameRect(cursorR.left() - Ui::DesignSystem::layout().px4(),
                                     cursorR.top() - Ui::DesignSystem::layout().px24(),
                                     TextHelper::fineTextWidth(cursorInfo.name, painter.font())
                                         + Ui::DesignSystem::layout().px12(),
                                     Ui::DesignSystem::layout().px24());
            painter.setPen(Qt::NoPen);
            painter.setBrush(backgroundColor);
            painter.drawRoundedRect(usernameRect, Ui::DesignSystem::button().borderRadius(),
                                    Ui::DesignSystem::button().borderRadius());
            painter.setPen(ColorHelper::contrasted(backgroundColor));
            painter.drawText(usernameRect, Qt::AlignCenter, cursorInfo.name);
        }
    }
}

ContextMenu* SimpleTextEdit::createContextMenu(const QPoint& _position, QWidget* _parent)
{
    //
    // Сначала нужно создать контекстное меню в базовом классе, т.к. в этот момент может
    // измениться курсор, который установлен в текстовом редакторе, и использовать его
    //
    auto menu = ScriptTextEdit::createContextMenu(_position, _parent);
    if (isReadOnly() || (!textCursor().hasSelection() && isMispelledWordUnderCursor(_position))) {
        return menu;
    }

    const BusinessLayer::TextCursor cursor = textCursor();

    //
    // Работа с закладками
    //
    auto bookmarkAction = new QAction(this);
    bookmarkAction->setText(tr("Bookmark"));
    bookmarkAction->setIconText(u8"\U000F00C3");
    if (!d->document.bookmark(cursor.block()).isValid()) {
        auto createBookmark = new QAction(bookmarkAction);
        createBookmark->setText(tr("Add"));
        createBookmark->setIconText(u8"\U000F00C4");
        connect(createBookmark, &QAction::triggered, this, &SimpleTextEdit::addBookmarkRequested);
    } else {
        auto editBookmark = new QAction(bookmarkAction);
        editBookmark->setText(tr("Edit"));
        editBookmark->setIconText(u8"\U000F03EB");
        connect(editBookmark, &QAction::triggered, this, &SimpleTextEdit::editBookmarkRequested);
        //
        auto removeBookmark = new QAction(bookmarkAction);
        removeBookmark->setText(tr("Remove"));
        removeBookmark->setIconText(u8"\U000F01B4");
        connect(removeBookmark, &QAction::triggered, this,
                &SimpleTextEdit::removeBookmarkRequested);
    }
    //
    auto showBookmarks = new QAction(bookmarkAction);
    showBookmarks->setText(tr("Show/hide list"));
    showBookmarks->setIconText(u8"\U000F0E16");
    connect(showBookmarks, &QAction::triggered, this, &SimpleTextEdit::showBookmarksRequested);

    auto actions = menu->actions().toVector();
    actions.first()->setSeparator(true);
    actions.prepend(bookmarkAction);
    menu->setActions(actions);

    return menu;
}

bool SimpleTextEdit::canInsertFromMimeData(const QMimeData* _source) const
{
    return _source->formats().contains(d->model->mimeTypes().first()) || _source->hasText();
}

QMimeData* SimpleTextEdit::createMimeDataFromSelection() const
{
    if (!textCursor().hasSelection()) {
        return {};
    }

    QMimeData* mimeData = new QMimeData;
    TextCursor cursor = textCursor();
    const auto selection = cursor.selectionInterval();

    //
    // Сформируем в текстовом виде, для вставки наружу
    // TODO: экспорт в фонтан
    //
    {
        QString text;
        auto cursor = textCursor();
        cursor.setPosition(selection.from);
        do {
            cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
            if (cursor.position() > selection.to) {
                cursor.setPosition(selection.to, QTextCursor::KeepAnchor);
            }
            if (!text.isEmpty()) {
                text.append("\r\n\r\n");
            }
            const auto selectedText = cursor.selectedText().replace(QChar::LineSeparator, "\r\n");
            text.append(cursor.blockCharFormat().fontCapitalization() == QFont::AllUppercase
                            ? TextHelper::smartToUpper(selectedText)
                            : selectedText);
        } while (cursor.position() < textCursor().selectionEnd() && !cursor.atEnd()
                 && cursor.movePosition(QTextCursor::NextBlock));

        mimeData->setData("text/plain", text.toUtf8());
    }

    //
    // Поместим в буфер данные о тексте в специальном формате
    //
    {
        mimeData->setData(d->model->mimeTypes().first(),
                          d->document.mimeFromSelection(selection.from, selection.to).toUtf8());
    }

    return mimeData;
}

void SimpleTextEdit::insertFromMimeData(const QMimeData* _source)
{
    if (isReadOnly()) {
        return;
    }

    //
    // Удаляем выделенный текст
    //
    TextCursor cursor = textCursor();
    if (cursor.hasSelection()) {
        cursor.removeSelectedText();
    }

    //
    // Если в моменте входа мы в состоянии редактирования (такое возможно в момент дропа),
    // то запомним это состояние, чтобы восстановить после дропа, а для вставки важно,
    // чтобы режим редактирования был выключен, т.к. данные будут загружаться через модель
    //
    const bool wasInEditBlock = cursor.isInEditBlock();
    if (wasInEditBlock) {
        cursor.endEditBlock();
    }

    //
    // Вставляем сценарий из майм-данных
    //
    QString textToInsert;

    //
    // Если вставляются данные в сценарном формате, то вставляем как положено
    //
    const int invalidPosition = -1;
    int removeCharacterAtPosition = invalidPosition;
    if (_source->formats().contains(d->model->mimeTypes().constFirst())) {
        textToInsert = _source->data(d->model->mimeTypes().constFirst());
    }
    //
    // Если простой текст
    //
    else if (_source->hasText()) {
        const auto text = _source->text();

        //
        // ... если в тексте всего одна строка и вставка происходит в пустой абзац, то вставим в
        // него пробел, чтобы его стиль не изменился, а сам текст будем вставлять в начало абзаца
        //
        if (!text.contains('\n') && cursor.block().text().isEmpty()) {
            removeCharacterAtPosition = cursor.position();
            cursor.insertText(" ");
            setTextCursor(cursor);
        }

        //
        // ... если строк несколько, то вставляем его, импортировав с фонтана
        // NOTE: Перед текстом нужно обязательно добавить перенос строки, чтобы он
        //       не воспринимался как титульная страница
        //
        BusinessLayer::SimpleTextMarkdownImporter markdownImporter;
        textToInsert = markdownImporter.importDocument(text).text;
    }

    //
    // Собственно вставка данных
    //
    auto cursorPosition = d->document.insertFromMime(textCursor().position(), textToInsert);

    //
    // Удалим лишний пробел, который вставляли
    //
    if (removeCharacterAtPosition != invalidPosition) {
        cursor.setPosition(removeCharacterAtPosition);
        cursor.deleteChar();
        if (removeCharacterAtPosition < cursorPosition) {
            --cursorPosition;
        }
    }

    //
    // Восстанавливаем режим редактирования, если нужно
    //
    if (wasInEditBlock) {
        cursor.beginEditBlock();
    }

    //
    // Позиционируем курсор
    //
    if (cursorPosition >= 0) {
        cursor.setPosition(cursorPosition);
        setTextCursor(cursor);
    }
}

void SimpleTextEdit::dropEvent(QDropEvent* _event)
{
    //
    // Если в момент вставки было выделение
    //
    if (textCursor().hasSelection()) {
        QTextCursor cursor = textCursor();
        //
        // ... и это перемещение содержимого внутри редактора
        //
        if (_event->source() == this) {
            //
            // ... то удалим выделенный текст
            //
            cursor.removeSelectedText();
        }
        //
        // ... а если контент заносят снаружи
        //
        else {
            //
            // ... то очистим выделение, чтобы оставить контент
            //
            cursor.clearSelection();
        }
    }

    PageTextEdit::dropEvent(_event);
}

} // namespace Ui
