#include "comic_book_text_document.h"

#include "comic_book_text_block_data.h"
#include "comic_book_text_corrector.h"
#include "comic_book_text_cursor.h"

#include <business_layer/model/comic_book/text/comic_book_text_model.h>
#include <business_layer/model/comic_book/text/comic_book_text_model_folder_item.h>
#include <business_layer/model/comic_book/text/comic_book_text_model_page_item.h>
#include <business_layer/model/comic_book/text/comic_book_text_model_panel_item.h>
#include <business_layer/model/comic_book/text/comic_book_text_model_splitter_item.h>
#include <business_layer/model/comic_book/text/comic_book_text_model_text_item.h>
#include <business_layer/templates/comic_book_template.h>
#include <business_layer/templates/templates_facade.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <ui/widgets/text_edit/page/page_text_edit.h>
#include <utils/helpers/text_helper.h>
#include <utils/shugar.h>

#include <QDateTime>
#include <QPointer>
#include <QScopedValueRollback>
#include <QTextTable>

using BusinessLayer::ComicBookBlockStyle;
using BusinessLayer::ComicBookParagraphType;
using BusinessLayer::TemplatesFacade;


namespace BusinessLayer {

enum class DocumentState { Undefined, Loading, Changing, Correcting, Ready };


class ComicBookTextDocument::Implementation
{
public:
    explicit Implementation(ComicBookTextDocument* _document);

    /**
     * @brief Получить шаблон оформления текущего документа
     */
    const ComicBookTemplate& documentTemplate() const;

    /**
     * @brief Скорректировать позиции элементов на заданную дистанцию
     */
    void correctPositionsToItems(
        std::map<int, BusinessLayer::ComicBookTextModelItem*>::iterator _from, int _distance);
    void correctPositionsToItems(int _fromPosition, int _distance);

    /**
     * @brief Считать содержимое элмента модели с заданным индексом
     *        и вставить считанные данные в текущее положение курсора
     */
    void readModelItemContent(int _itemRow, const QModelIndex& _parent,
                              ComicBookTextCursor& _cursor, bool& _isFirstParagraph);

    /**
     * @brief Считать содержимое вложенных в заданный индекс элементов
     *        и вставить считанные данные в текущее положение курсора
     */
    void readModelItemsContent(const QModelIndex& _parent, ComicBookTextCursor& _cursor,
                               bool& _isFirstParagraph);

    /**
     * @brief Скорректировать документ, если это возможно
     */
    void tryToCorrectDocument();


    ComicBookTextDocument* q = nullptr;

    DocumentState state = DocumentState::Undefined;
    QString templateId;
    QPointer<BusinessLayer::ComicBookTextModel> model;
    bool canChangeModel = true;
    std::map<int, BusinessLayer::ComicBookTextModelItem*> positionsToItems;
    ComicBookTextCorrector corrector;
};

ComicBookTextDocument::Implementation::Implementation(ComicBookTextDocument* _document)
    : q(_document)
    , corrector(_document)
{
}

const ComicBookTemplate& ComicBookTextDocument::Implementation::documentTemplate() const
{
    return TemplatesFacade::comicBookTemplate(templateId);
}

void ComicBookTextDocument::Implementation::correctPositionsToItems(
    std::map<int, BusinessLayer::ComicBookTextModelItem*>::iterator _from, int _distance)
{
    if (_from == positionsToItems.end()) {
        return;
    }

    if (_distance > 0) {
        auto reversed = [](std::map<int, BusinessLayer::ComicBookTextModelItem*>::iterator iter) {
            return std::prev(std::make_reverse_iterator(iter));
        };
        for (auto iter = positionsToItems.rbegin(); iter != std::make_reverse_iterator(_from);
             ++iter) {
            auto itemToUpdate = positionsToItems.extract(iter->first);
            itemToUpdate.key() = itemToUpdate.key() + _distance;
            iter = reversed(positionsToItems.insert(std::move(itemToUpdate)).position);
        }
    } else if (_distance < 0) {
        for (auto iter = _from; iter != positionsToItems.end(); ++iter) {
            auto itemToUpdate = positionsToItems.extract(iter->first);
            itemToUpdate.key() = itemToUpdate.key() + _distance;
            iter = positionsToItems.insert(std::move(itemToUpdate)).position;
        }
    }
}

void ComicBookTextDocument::Implementation::correctPositionsToItems(int _fromPosition,
                                                                    int _distance)
{
    correctPositionsToItems(positionsToItems.lower_bound(_fromPosition), _distance);
}

void ComicBookTextDocument::Implementation::readModelItemContent(int _itemRow,
                                                                 const QModelIndex& _parent,
                                                                 ComicBookTextCursor& _cursor,
                                                                 bool& _isFirstParagraph)
{
    const auto itemIndex = model->index(_itemRow, 0, _parent);
    const auto item = model->itemForIndex(itemIndex);
    switch (item->type()) {
    case ComicBookTextModelItemType::Folder:
    case ComicBookTextModelItemType::Page:
    case ComicBookTextModelItemType::Panel: {
        break;
    }

    case ComicBookTextModelItemType::Splitter: {
        const auto splitterItem = static_cast<ComicBookTextModelSplitterItem*>(item);
        switch (splitterItem->splitterType()) {
        case ComicBookTextModelSplitterItemType::Start: {
            //
            // Если это не первый абзац, вставим блок для него
            //
            if (!_isFirstParagraph) {
                _cursor.insertBlock();
            }
            //
            // ... в противном же случае, новый блок нет необходимости вставлять
            //
            else {
                _isFirstParagraph = false;
            }

            //
            // Скорректируем позиции последующих элементов
            //
            correctPositionsToItems(_cursor.position(), 4);

            //
            // Запомним позицию разделителя
            //
            positionsToItems.emplace(_cursor.position(), splitterItem);

            //
            // Назначим блоку перед таблицей формат PageSplitter
            //
            auto insertPageSplitter = [&_cursor, this] {
                const auto style
                    = documentTemplate().paragraphStyle(ComicBookParagraphType::PageSplitter);
                _cursor.setBlockFormat(style.blockFormat());
                _cursor.setBlockCharFormat(style.charFormat());
                _cursor.setCharFormat(style.charFormat());
            };
            insertPageSplitter();
            //
            // ... и сохраним данные блока
            //
            auto blockData = new ComicBookTextBlockData(splitterItem);
            _cursor.block().setUserData(blockData);

            //
            // Вставляем таблицу
            //
            q->insertTable(_cursor);

            //
            // Назначим блоку после таблицы формат PageSplitter
            //
            insertPageSplitter();

            //
            // После вставки таблицы нужно завершить транзакцию изменения документа,
            // чтобы корректно считывались таблицы в положении курсора
            //
            _cursor.restartEditBlock();

            //
            // Помещаем курсор в первую ячейку для дальнейшего наполнения
            //
            _cursor.movePosition(QTextCursor::PreviousBlock, QTextCursor::MoveAnchor, 2);
            //
            // ... и помечаем, что вставлять новый блок нет необходимости
            //
            _isFirstParagraph = true;

            break;
        }

        case ComicBookTextModelSplitterItemType::End: {
            //
            // Блок завершения таблицы уже был вставлен, при вставке начала и самой таблицы,
            // поэтому тут лишь сохраняем его в карту позиций элементов
            //
            _cursor.movePosition(QTextCursor::NextBlock);
            positionsToItems.emplace(_cursor.position(), splitterItem);
            //
            // ... и сохраняем в блоке информацию об элементе
            //
            auto blockData = new ComicBookTextBlockData(splitterItem);
            _cursor.block().setUserData(blockData);

            break;
        }

        default:
            break;
        }

        break;
    }

    case ComicBookTextModelItemType::Text: {
        const auto textItem = static_cast<ComicBookTextModelTextItem*>(item);

        //
        // Если новый блок должен быть в другой колонке
        //
        if (_cursor.inTable() && _cursor.inFirstColumn()) {
            Q_ASSERT(textItem->isInFirstColumn().has_value());
            if (*textItem->isInFirstColumn() == false) {
                //
                // Переходим к следующей колонке
                //
                _cursor.movePosition(QTextCursor::NextBlock);
                //
                // ... и помечаем, что вставлять новый блок нет необходимости
                //
                _isFirstParagraph = true;
            }
        }

        //
        // Если вставляемый элемент должен быть в таблице, а курсор стоит на разделителе
        //
        if (ComicBookBlockStyle::forBlock(_cursor.block()) == ComicBookParagraphType::PageSplitter
            && textItem->isInFirstColumn().has_value()) {
            //
            // Зайдём внутрь таблицы
            //
            _cursor.movePosition(ComicBookTextCursor::NextBlock);
            //
            // ... и пометим, что вставлять новый блок нет нужны
            //
            _isFirstParagraph = true;
        }

        //
        // При корректировке положений блоков нужно учитывать перенос строки
        //
        int additionalDistance = 1;
        //
        // Если это не первый абзац, вставим блок для него
        //
        if (!_isFirstParagraph) {
            _cursor.insertBlock();
        }
        //
        // ... в противном же случае, новый блок нет необходимости вставлять
        //
        else {
            _isFirstParagraph = false;
            additionalDistance = 0;
        }

        //
        // Запомним позицию элемента
        //
        correctPositionsToItems(_cursor.position(), textItem->text().length() + additionalDistance);
        positionsToItems.emplace(_cursor.position(), textItem);

        //
        // Установим стиль блока
        //
        const auto currentStyle = documentTemplate().paragraphStyle(textItem->paragraphType());
        _cursor.setBlockFormat(currentStyle.blockFormat(_cursor.inTable()));
        _cursor.setBlockCharFormat(currentStyle.charFormat());
        _cursor.setCharFormat(currentStyle.charFormat());

        //
        // Для докараций, добавим дополнительные флаги
        //
        auto decorationFormat = _cursor.block().blockFormat();
        if (textItem->isCorrection()) {
            decorationFormat.setProperty(ComicBookBlockStyle::PropertyIsCorrection, true);
            decorationFormat.setProperty(PageTextEdit::PropertyDontShowCursor, true);
        }
        if (textItem->isCorrectionContinued()) {
            decorationFormat.setProperty(ComicBookBlockStyle::PropertyIsCorrectionContinued, true);
            decorationFormat.setTopMargin(0);
        }
        if (textItem->isBreakCorrectionStart()) {
            decorationFormat.setProperty(ComicBookBlockStyle::PropertyIsBreakCorrectionStart, true);
        }
        if (textItem->isBreakCorrectionEnd()) {
            decorationFormat.setProperty(ComicBookBlockStyle::PropertyIsBreakCorrectionEnd, true);
        }
        _cursor.setBlockFormat(decorationFormat);

        //
        // ... выравнивание
        //
        if (textItem->alignment().has_value()) {
            auto blockFormat = _cursor.blockFormat();
            blockFormat.setAlignment(textItem->alignment().value());
            _cursor.setBlockFormat(blockFormat);
        }

        //
        // Вставим текст абзаца
        //
        const auto textToInsert = TextHelper::fromHtmlEscaped(textItem->text());
        _cursor.insertText(textToInsert);

        //
        // Вставим данные блока
        //
        auto blockData = new ComicBookTextBlockData(textItem);
        _cursor.block().setUserData(blockData);

        //
        // Вставим форматирование
        //
        auto formatCursor = _cursor;
        for (const auto& format : textItem->formats()) {
            formatCursor.setPosition(formatCursor.block().position() + format.from);
            formatCursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor,
                                      format.length);
            formatCursor.mergeCharFormat(format.charFormat());
        }

        //
        // Вставим редакторские заметки
        //
        auto reviewCursor = _cursor;
        for (const auto& reviewMark : textItem->reviewMarks()) {
            reviewCursor.setPosition(reviewCursor.block().position() + reviewMark.from);
            reviewCursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor,
                                      reviewMark.length);
            reviewCursor.mergeCharFormat(reviewMark.charFormat());
        }

        break;
    }

    default: {
        Q_ASSERT(false);
        break;
    }
    }
}

void ComicBookTextDocument::Implementation::readModelItemsContent(const QModelIndex& _parent,
                                                                  ComicBookTextCursor& _cursor,
                                                                  bool& _isFirstParagraph)
{
    for (int itemRow = 0; itemRow < model->rowCount(_parent); ++itemRow) {
        readModelItemContent(itemRow, _parent, _cursor, _isFirstParagraph);

        //
        // Считываем информацию о детях
        //
        const auto itemIndex = model->index(itemRow, 0, _parent);
        readModelItemsContent(itemIndex, _cursor, _isFirstParagraph);
    }
}

void ComicBookTextDocument::Implementation::tryToCorrectDocument()
{
    if (state != DocumentState::Ready) {
        return;
    }

    QScopedValueRollback temporatryState(state, DocumentState::Correcting);
    corrector.makePlannedCorrection();
}


// ****


ComicBookTextDocument::ComicBookTextDocument(QObject* _parent)
    : QTextDocument(_parent)
    , d(new Implementation(this))
{
    connect(this, &ComicBookTextDocument::contentsChange, this,
            &ComicBookTextDocument::updateModelOnContentChange);
    connect(this, &ComicBookTextDocument::contentsChange, &d->corrector,
            &ComicBookTextCorrector::planCorrection);
    connect(this, &ComicBookTextDocument::contentsChanged, this,
            [this] { d->tryToCorrectDocument(); });
}

ComicBookTextDocument::~ComicBookTextDocument() = default;

void ComicBookTextDocument::setTemplateId(const QString& _templateId)
{
    if (d->templateId == _templateId) {
        return;
    }

    d->templateId = _templateId;
    d->corrector.setTemplateId(_templateId);
}

void ComicBookTextDocument::setModel(BusinessLayer::ComicBookTextModel* _model,
                                     bool _canChangeModel)
{
    d->state = DocumentState::Loading;

    if (d->model) {
        d->model->disconnect(this);
    }

    d->model = _model;
    d->canChangeModel = _canChangeModel;
    d->positionsToItems.clear();

    //
    // Сбрасываем корректор
    //
    d->corrector.clear();

    //
    // Аккуратно очищаем текст, чтобы не сломать форматирование самого документа
    //
    ComicBookTextCursor cursor(this);
    cursor.beginEditBlock();
    cursor.select(QTextCursor::Document);
    cursor.deleteChar();
    cursor.block().setUserData(nullptr);
    cursor.endEditBlock();

    if (d->model == nullptr) {
        d->state = DocumentState::Ready;
        return;
    }

    //
    // Обновим шрифт документа, в моменте когда текста нет
    //
    const auto templateDefaultFont
        = d->documentTemplate().paragraphStyle(ComicBookParagraphType::Description).font();
    if (defaultFont() != templateDefaultFont) {
        setDefaultFont(templateDefaultFont);
    }

    //
    // Начинаем операцию вставки
    //
    cursor.beginEditBlock();

    //
    // Последовательно формируем текст документа
    //
    bool isFirstParagraph = true;
    d->readModelItemsContent({}, cursor, isFirstParagraph);

    //
    // Завершаем операцию
    //
    cursor.endEditBlock();

    //
    // Настроим соединения
    //
    connect(d->model, &ComicBookTextModel::modelReset, this, [this] {
        QSignalBlocker signalBlocker(this);
        setModel(d->model);
    });
    connect(
        d->model, &ComicBookTextModel::dataChanged, this,
        [this](const QModelIndex& _topLeft, const QModelIndex& _bottomRight) {
            if (d->state != DocumentState::Ready) {
                return;
            }

            Q_ASSERT(_topLeft == _bottomRight);

            QScopedValueRollback temporatryState(d->state, DocumentState::Changing);

            const auto position = itemStartPosition(_topLeft);
            if (position < 0) {
                return;
            }

            const auto item = d->model->itemForIndex(_topLeft);
            if (item->type() != ComicBookTextModelItemType::Text) {
                return;
            }

            const auto textItem = static_cast<ComicBookTextModelTextItem*>(item);

            ComicBookTextCursor cursor(this);
            cursor.setPosition(position);
            cursor.beginEditBlock();

            //
            // Если элемент расположен там, где и должен быть, обновим его
            //
            if (textItem->isInFirstColumn().has_value() == cursor.inTable()) {
                //
                // ... тип параграфа
                //
                if (ComicBookBlockStyle::forBlock(cursor.block()) != textItem->paragraphType()) {
                    applyParagraphType(textItem->paragraphType(), cursor);
                }
                //
                // ... выравнивание
                //
                if (textItem->alignment().has_value()
                    && cursor.blockFormat().alignment() != textItem->alignment()) {
                    //
                    // ... если пользователь задал выравнивание
                    //
                    auto blockFormat = cursor.blockFormat();
                    blockFormat.setAlignment(textItem->alignment().value());
                    cursor.setBlockFormat(blockFormat);
                } else if (!textItem->alignment().has_value()
                           && cursor.blockFormat().alignment()
                               != d->documentTemplate()
                                      .paragraphStyle(textItem->paragraphType())
                                      .align()) {
                    //
                    // ... если выравнивание должно быть, как в стиле
                    //
                    auto blockFormat = cursor.blockFormat();
                    blockFormat.setAlignment(
                        d->documentTemplate().paragraphStyle(textItem->paragraphType()).align());
                    cursor.setBlockFormat(blockFormat);
                }
                //
                // ... текст
                //
                if (cursor.block().text() != textItem->text()) {
                    //
                    // Корректируем позиции всех элментов идущих за обновляемым
                    //
                    const auto distanse
                        = textItem->text().length() - cursor.block().text().length();
                    d->correctPositionsToItems(position + 1, distanse);

                    //
                    // TODO: Сделать более умный алгоритм, который заменяет только изменённые части
                    // текста
                    //
                    cursor.movePosition(QTextCursor::StartOfBlock);
                    cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
                    cursor.insertText(textItem->text());
                }
                //
                // ... редакторские заметки и форматирование
                //
                {
                    //
                    // TODO: придумать, как не перезаписывать форматирование каждый раз
                    //

                    //
                    // Сбросим текущее форматирование
                    //
                    cursor.movePosition(QTextCursor::StartOfBlock);
                    cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
                    const auto blockType = ComicBookBlockStyle::forBlock(cursor.block());
                    const auto blockStyle = d->documentTemplate().paragraphStyle(blockType);
                    cursor.setBlockCharFormat(blockStyle.charFormat());
                    cursor.setCharFormat(blockStyle.charFormat());

                    //
                    // Применяем форматирование из редакторских заметок элемента
                    //
                    for (const auto& reviewMark : textItem->reviewMarks()) {
                        cursor.movePosition(QTextCursor::StartOfBlock);
                        cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::MoveAnchor,
                                            reviewMark.from);
                        cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor,
                                            reviewMark.length);
                        cursor.mergeCharFormat(reviewMark.charFormat());
                    }
                    for (const auto& format : textItem->formats()) {
                        cursor.movePosition(QTextCursor::StartOfBlock);
                        cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::MoveAnchor,
                                            format.from);
                        cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor,
                                            format.length);
                        cursor.mergeCharFormat(format.charFormat());
                    }
                }
                //
                // ... разрыв
                //
                if (cursor.blockFormat().boolProperty(
                        ComicBookBlockStyle::PropertyIsBreakCorrectionStart)
                    && !textItem->isBreakCorrectionStart()) {
                    auto clearFormat = cursor.blockFormat();
                    clearFormat.clearProperty(ComicBookBlockStyle::PropertyIsBreakCorrectionStart);
                    cursor.setBlockFormat(clearFormat);
                    //
                    // ... в обратную сторону не делаем, т.к. это реализует сам корректор текста
                    //
                }
            }
            //
            // А если нужно скорректировать позицию элемента
            //
            else {
                //
                // ... то удалим его старую версию
                //
                cursor.movePosition(ComicBookTextCursor::EndOfBlock,
                                    ComicBookTextCursor::KeepAnchor);
                const auto positionsCorrectionDelta = cursor.selectedText().length();
                if (cursor.hasSelection()) {
                    cursor.deleteChar();
                }
                //
                // ... удаляем блок в карте
                //
                d->positionsToItems.erase(cursor.position());
                //
                // ... и корректируем позиции элементов
                //
                d->correctPositionsToItems(cursor.position(), -1 * positionsCorrectionDelta);
                //
                // ... удалим сам блок и при этом нужно сохранить данные блока и его формат
                //
                auto block = cursor.block().previous();
                ComicBookTextBlockData* blockData = nullptr;
                if (block.userData() != nullptr) {
                    blockData = new ComicBookTextBlockData(
                        static_cast<ComicBookTextBlockData*>(block.userData()));
                }
                const auto blockFormat = cursor.block().previous().blockFormat();
                cursor.deletePreviousChar();
                cursor.block().setUserData(blockData);
                cursor.setBlockFormat(blockFormat);
                d->correctPositionsToItems(cursor.position(), -1);

                //
                // ... и вставим новую
                //
                if (*textItem->isInFirstColumn()) {
                    while (!cursor.inTable() || !cursor.inFirstColumn()) {
                        cursor.movePosition(ComicBookTextCursor::PreviousBlock);
                        cursor.movePosition(ComicBookTextCursor::EndOfBlock);
                    }
                } else {
                    while (!cursor.inTable()) {
                        cursor.movePosition(ComicBookTextCursor::PreviousBlock);
                        cursor.movePosition(ComicBookTextCursor::EndOfBlock);
                    }
                }
                bool isFirstParagraph = ComicBookBlockStyle::forBlock(cursor.block())
                    == ComicBookParagraphType::Undefined;
                d->readModelItemContent(_topLeft.row(), _topLeft.parent(), cursor,
                                        isFirstParagraph);
            }


            cursor.endEditBlock();
        });
    connect(d->model, &ComicBookTextModel::rowsInserted, this,
            [this](const QModelIndex& _parent, int _from, int _to) {
                if (d->state != DocumentState::Ready) {
                    return;
                }

                QScopedValueRollback temporatryState(d->state, DocumentState::Changing);

                //
                // Игнорируем добавление пустых сцен и папок
                //
                const auto item = d->model->itemForIndex(d->model->index(_from, 0, _parent));
                if ((item->type() == ComicBookTextModelItemType::Folder
                     || item->type() == ComicBookTextModelItemType::Page
                     || item->type() == ComicBookTextModelItemType::Panel)
                    && !item->hasChildren()) {
                    return;
                }

                //
                // Определим позицию курсора откуда нужно начинать вставку
                //
                QModelIndex cursorItemIndex;
                if (_from > 0) {
                    cursorItemIndex = d->model->index(_from - 1, 0, _parent);
                } else {
                    cursorItemIndex = d->model->index(_parent.row() - 1, 0, _parent.parent());
                }
                //
                bool isFirstParagraph = !cursorItemIndex.isValid();
                const int cursorPosition = isFirstParagraph ? 0 : itemEndPosition(cursorItemIndex);
                if (cursorPosition < 0) {
                    return;
                }

                //
                // Собственно вставляем контент
                //
                ComicBookTextCursor cursor(this);
                cursor.beginEditBlock();

                cursor.setPosition(cursorPosition);
                if (isFirstParagraph) {
                    //
                    // Если первый параграф, то нужно перенести блок со своими данными дальше
                    //
                    ComicBookTextBlockData* blockData = nullptr;
                    auto block = cursor.block();
                    if (block.userData() != nullptr) {
                        blockData = new ComicBookTextBlockData(
                            static_cast<ComicBookTextBlockData*>(block.userData()));
                        block.setUserData(nullptr);
                    }
                    cursor.insertBlock();
                    cursor.block().setUserData(blockData);
                    //
                    // И вернуться назад, для вставки данныхшт
                    //
                    cursor.movePosition(QTextCursor::PreviousBlock);
                    //
                    // Корректируем позиции всех блоков на один символ
                    //
                    d->correctPositionsToItems(0, 1);
                } else {
                    cursor.movePosition(QTextCursor::EndOfBlock);
                }

                for (int itemRow = _from; itemRow <= _to; ++itemRow) {
                    d->readModelItemContent(itemRow, _parent, cursor, isFirstParagraph);

                    //
                    // Считываем информацию о детях
                    //
                    const auto itemIndex = d->model->index(itemRow, 0, _parent);
                    d->readModelItemsContent(itemIndex, cursor, isFirstParagraph);
                }

                cursor.endEditBlock();
            });
    connect(d->model, &ComicBookTextModel::rowsAboutToBeRemoved, this,
            [this](const QModelIndex& _parent, int _from, int _to) {
                if (d->state != DocumentState::Ready) {
                    return;
                }

                QScopedValueRollback temporatryState(d->state, DocumentState::Changing);

                const QModelIndex fromIndex = d->model->index(_from, 0, _parent);
                if (!fromIndex.isValid()) {
                    return;
                }
                auto fromPosition = itemStartPosition(fromIndex);
                if (fromPosition < 0) {
                    return;
                }

                //
                // Если происходит удаление разделителя таблицы, то удаляем таблицу,
                // а все блоки переносим за таблицу
                //
                if (auto item = d->model->itemForIndex(fromIndex);
                    item->type() == ComicBookTextModelItemType::Splitter) {
                    auto splitterItem = static_cast<ComicBookTextModelSplitterItem*>(item);

                    //
                    // ... убираем таблицу на удалении стартового элемента
                    //
                    if (splitterItem->splitterType() == ComicBookTextModelSplitterItemType::Start) {
                        //
                        // Заходим в таблицу
                        //
                        ComicBookTextCursor cursor(this);
                        cursor.setPosition(fromPosition);
                        cursor.movePosition(ComicBookTextCursor::NextBlock);
                        //
                        // Берём второй курсор, куда будем переносить блоки
                        //
                        int row = _from + 1;
                        auto insertCursor = cursor;
                        while (ComicBookBlockStyle::forBlock(insertCursor.block())
                               != ComicBookParagraphType::PageSplitter) {
                            insertCursor.movePosition(ComicBookTextCursor::NextBlock);
                        }
                        bool isFirstParagraph = false;

                        //
                        // Идём по таблице
                        //
                        while (ComicBookBlockStyle::forBlock(cursor.block())
                               != ComicBookParagraphType::PageSplitter) {
                            //
                            // ... проставим элементу блока флаг, что он теперь вне таблицы
                            //
                            auto item = d->positionsToItems[cursor.position()];
                            if (item->type() == ComicBookTextModelItemType::Text) {
                                auto textItem = static_cast<ComicBookTextModelTextItem*>(item);
                                textItem->setInFirstColumn({});
                            }
                            //
                            // ... удаляем блок в таблице
                            //
                            cursor.movePosition(ComicBookTextCursor::EndOfBlock,
                                                ComicBookTextCursor::KeepAnchor);
                            const auto positionsCorrectionDelta = cursor.selectedText().length();
                            if (cursor.hasSelection()) {
                                cursor.deleteChar();
                            }
                            //
                            // ... удаляем блок в карте
                            //
                            d->positionsToItems.erase(cursor.position());
                            //
                            // ... и корректируем позиции элементов
                            //
                            d->correctPositionsToItems(cursor.position(),
                                                       -1 * positionsCorrectionDelta);

                            //
                            // ... если всё ещё в одной колонке, то удаляем текущий пустой блок
                            //
                            const auto currentBlockInFirstColumn = cursor.inFirstColumn();
                            if (cursor.movePosition(ComicBookTextCursor::NextBlock,
                                                    ComicBookTextCursor::KeepAnchor)) {
                                if (currentBlockInFirstColumn == cursor.inFirstColumn()) {
                                    cursor.deleteChar();
                                    d->correctPositionsToItems(cursor.position(), -1);
                                }
                                //
                                // ... если перешли в другую колонку, то сбросим выделение
                                //
                                else {
                                    cursor.clearSelection();
                                }
                            }
                            //
                            // ... если дошли до конца таблицы, то переходим в следующий блок
                            //
                            else {
                                cursor.movePosition(ComicBookTextCursor::NextBlock);
                            }

                            //
                            // ... вставляем этот же блок после таблицы
                            //
                            d->readModelItemContent(row++, fromIndex.parent(), insertCursor,
                                                    isFirstParagraph);
                        }

                        //
                        // Удаляем саму таблицу
                        //
                        cursor.movePosition(QTextCursor::NextBlock);
                        cursor.movePosition(QTextCursor::PreviousBlock, QTextCursor::KeepAnchor);
                        cursor.movePosition(QTextCursor::PreviousBlock, QTextCursor::KeepAnchor);
                        cursor.deletePreviousChar();

                        //
                        // Корректируем карту позиций блоков
                        //
                        auto fromIter = d->positionsToItems.lower_bound(fromPosition);
                        auto endIter = std::next(fromIter, 2);
                        d->positionsToItems.erase(fromIter, endIter);

                        //
                        // Корректируем позиции элементов
                        //
                        d->correctPositionsToItems(cursor.position(), -4);
                    }

                    return;
                }

                const QModelIndex toIndex = d->model->index(_to, 0, _parent);
                if (!toIndex.isValid()) {
                    return;
                }
                const auto toPosition = itemEndPosition(toIndex);
                if (toPosition < 0) {
                    return;
                }

                ComicBookTextCursor cursor(this);
                cursor.setPosition(fromPosition);
                cursor.setPosition(toPosition, QTextCursor::KeepAnchor);
                cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
                if (!cursor.hasSelection() && !cursor.block().text().isEmpty()) {
                    return;
                }

                //
                // Корректируем карту позиций элементов
                //
                auto fromIter = d->positionsToItems.lower_bound(cursor.selectionInterval().from);
                auto endIter = d->positionsToItems.lower_bound(cursor.selectionInterval().to);
                //
                // ... если удаление заканчивается на пустом абзаце, берём следующий за ним элемент
                //
                if (cursor.block().text().isEmpty()) {
                    endIter = std::next(endIter);
                }
                //
                // ... определим дистанцию занимаемую удаляемыми элементами
                //
                const auto distance = endIter->first - fromIter->first;
                //
                // ... удаляем сами элементы из карты
                //
                d->positionsToItems.erase(fromIter, endIter);
                //
                // ... корректируем позиции остальных элементов
                //
                d->correctPositionsToItems(cursor.selectionInterval().to, -1 * distance);

                cursor.beginEditBlock();
                if (cursor.hasSelection()) {
                    cursor.deleteChar();
                }

                //
                // Если это самый первый блок, то нужно удалить на один символ больше, чтобы удалить
                // сам блок
                //
                if (fromPosition == 0 && toPosition != characterCount()) {
                    cursor.deleteChar();
                }
                //
                // Если это не самый первый блок, то нужно взять на один символ назад, чтобы удалить
                // сам блок
                //
                else if (fromPosition > 0) {
                    //
                    // ... и при этом нужно сохранить данные блока и его формат
                    //
                    ComicBookTextBlockData* blockData = nullptr;
                    auto block = cursor.block().previous();
                    if (block.userData() != nullptr) {
                        blockData = new ComicBookTextBlockData(
                            static_cast<ComicBookTextBlockData*>(block.userData()));
                    }
                    const auto blockFormat = cursor.block().previous().blockFormat();
                    cursor.deletePreviousChar();
                    cursor.block().setUserData(blockData);
                    cursor.setBlockFormat(blockFormat);
                }
                cursor.endEditBlock();
            });
    //
    // Группируем массовые изменения, чтобы не мелькать пользователю перед глазами
    //
    connect(d->model, &ComicBookTextModel::rowsAboutToBeChanged, this,
            [this] { ComicBookTextCursor(this).beginEditBlock(); });
    connect(d->model, &ComicBookTextModel::rowsChanged, this, [this] {
        //
        // Завершаем групповое изменение, но при этом обходим стороной корректировки документа,
        // т.к. всё это происходило в модели и документ уже находится в синхронизированном с
        // моделью состоянии
        //
        d->state = DocumentState::Changing;
        ComicBookTextCursor(this).endEditBlock();
        d->state = DocumentState::Ready;

        //
        // После того, как все изменения были выполнены, пробуем скорректировать документ
        //
        d->tryToCorrectDocument();
    });

    d->state = DocumentState::Ready;

    d->tryToCorrectDocument();
}

int ComicBookTextDocument::itemPosition(const QModelIndex& _index, bool _fromStart)
{
    auto item = d->model->itemForIndex(_index);
    if (item == nullptr) {
        return -1;
    }

    while (item->childCount() > 0) {
        item = item->childAt(_fromStart ? 0 : item->childCount() - 1);
    }
    for (const auto& [key, value] : d->positionsToItems) {
        if (value == item) {
            return key;
        }
    }

    return -1;
}

int ComicBookTextDocument::itemStartPosition(const QModelIndex& _index)
{
    return itemPosition(_index, true);
}

int ComicBookTextDocument::itemEndPosition(const QModelIndex& _index)
{
    return itemPosition(_index, false);
}

QString ComicBookTextDocument::pageNumber(const QTextBlock& _forBlock) const
{
    if (_forBlock.userData() == nullptr) {
        return {};
    }

    const auto blockData = static_cast<ComicBookTextBlockData*>(_forBlock.userData());
    if (blockData == nullptr) {
        return {};
    }

    const auto itemParent = blockData->item()->parent();
    if (itemParent == nullptr || itemParent->type() != ComicBookTextModelItemType::Page) {
        return {};
    }

    const auto panelItem = static_cast<const ComicBookTextModelPageItem*>(itemParent);
    return panelItem->number().value;
}

QString ComicBookTextDocument::panelNumber(const QTextBlock& _forBlock) const
{
    if (_forBlock.userData() == nullptr) {
        return {};
    }

    const auto blockData = static_cast<ComicBookTextBlockData*>(_forBlock.userData());
    if (blockData == nullptr) {
        return {};
    }

    const auto itemParent = blockData->item()->parent();
    if (itemParent == nullptr || itemParent->type() != ComicBookTextModelItemType::Panel) {
        return {};
    }

    const auto panelItem = static_cast<const ComicBookTextModelPanelItem*>(itemParent);
    return panelItem->number().value;
}

QString ComicBookTextDocument::dialogueNumber(const QTextBlock& _forBlock) const
{
    if (_forBlock.userData() == nullptr) {
        return {};
    }

    const auto blockData = static_cast<ComicBookTextBlockData*>(_forBlock.userData());
    if (blockData == nullptr) {
        return {};
    }

    const auto item = blockData->item();
    if (item == nullptr || item->type() != ComicBookTextModelItemType::Text) {
        return {};
    }

    const auto textItem = static_cast<const ComicBookTextModelTextItem*>(item);
    return textItem->number().value_or(ComicBookTextModelTextItem::Number()).value;
}

QColor ComicBookTextDocument::itemColor(const QTextBlock& _forBlock) const
{
    if (_forBlock.userData() == nullptr) {
        return {};
    }

    const auto blockData = static_cast<ComicBookTextBlockData*>(_forBlock.userData());
    if (blockData == nullptr) {
        return {};
    }

    const auto itemParent = blockData->item()->parent();
    if (itemParent == nullptr) {
        return {};
    }
    QColor color;
    if (itemParent->type() == ComicBookTextModelItemType::Folder) {
        const auto folderItem = static_cast<const ComicBookTextModelFolderItem*>(itemParent);
        color = folderItem->color();
    } else if (itemParent->type() == ComicBookTextModelItemType::Page) {
        const auto pageItem = static_cast<const ComicBookTextModelPageItem*>(itemParent);
        color = pageItem->color();
    } else if (itemParent->type() == ComicBookTextModelItemType::Panel) {
        const auto panelItem = static_cast<const ComicBookTextModelPanelItem*>(itemParent);
        color = panelItem->color();
    }

    return color;
}

QString ComicBookTextDocument::mimeFromSelection(int _fromPosition, int _toPosition) const
{
    const auto fromBlock = findBlock(_fromPosition);
    if (fromBlock.userData() == nullptr) {
        return {};
    }
    auto fromBlockData = static_cast<ComicBookTextBlockData*>(fromBlock.userData());
    if (fromBlockData == nullptr) {
        return {};
    }
    const auto fromItemIndex = d->model->indexForItem(fromBlockData->item());
    const auto fromPositionInBlock = _fromPosition - fromBlock.position();

    const auto toBlock = findBlock(_toPosition);
    if (toBlock.userData() == nullptr) {
        return {};
    }
    auto toBlockData = static_cast<ComicBookTextBlockData*>(toBlock.userData());
    if (toBlockData == nullptr) {
        return {};
    }
    const auto toItemIndex = d->model->indexForItem(toBlockData->item());
    const auto toPositionInBlock = _toPosition - toBlock.position();

    const bool clearUuid = true;
    return d->model->mimeFromSelection(fromItemIndex, fromPositionInBlock, toItemIndex,
                                       toPositionInBlock, clearUuid);
}

void ComicBookTextDocument::insertFromMime(int _position, const QString& _mimeData)
{
    const auto block = findBlock(_position);
    if (block.userData() == nullptr) {
        return;
    }

    auto blockData = static_cast<ComicBookTextBlockData*>(block.userData());
    if (blockData == nullptr) {
        return;
    }

    const auto itemIndex = d->model->indexForItem(blockData->item());
    const auto positionInBlock = _position - block.position();
    d->model->insertFromMime(itemIndex, positionInBlock, _mimeData);
}

void ComicBookTextDocument::addParagraph(BusinessLayer::ComicBookParagraphType _type,
                                         ComicBookTextCursor _cursor)
{
    _cursor.beginEditBlock();

    //
    // Если параграф целиком переносится (энтер нажат перед всем текстом блока),
    // необходимо перенести данные блока с текущего на следующий
    //
    if (_cursor.block().text().left(_cursor.positionInBlock()).isEmpty()
        && !_cursor.block().text().isEmpty()) {
        ComicBookTextBlockData* blockData = nullptr;
        auto block = _cursor.block();
        if (block.userData() != nullptr) {
            blockData = new ComicBookTextBlockData(
                static_cast<ComicBookTextBlockData*>(block.userData()));
            block.setUserData(nullptr);
        }

        //
        // Вставим блок
        //
        _cursor.insertBlock();

        //
        // Перенесём данные блока
        //
        _cursor.block().setUserData(blockData);

        //
        // Перейдём к предыдущему абзацу
        //
        _cursor.movePosition(QTextCursor::PreviousBlock);
        //
        // ...и применим стиль к нему
        //
        applyParagraphType(_type, _cursor);
    }
    //
    // Вставляем новый блок после текущего
    //
    else {
        //
        // Вставим блок
        //
        _cursor.insertBlock();

        //
        // Применим стиль к новому блоку
        //
        applyParagraphType(_type, _cursor);
    }

    _cursor.endEditBlock();
}

void ComicBookTextDocument::setParagraphType(BusinessLayer::ComicBookParagraphType _type,
                                             const ComicBookTextCursor& _cursor)
{
    const auto currentParagraphType = ComicBookBlockStyle::forBlock(_cursor.block());
    if (currentParagraphType == _type) {
        return;
    }

    //
    // Нельзя сменить стиль конечных элементов папок
    //
    if (currentParagraphType == ComicBookParagraphType::FolderFooter) {
        return;
    }

    auto cursor = _cursor;
    cursor.beginEditBlock();

    //
    // Первым делом очищаем пользовательские данные
    //
    cursor.block().setUserData(nullptr);

    //
    // Обработаем предшествующий установленный стиль
    //
    cleanParagraphType(_cursor);

    //
    // Применим новый стиль к блоку
    //
    applyParagraphType(_type, _cursor);

    cursor.endEditBlock();
}

void ComicBookTextDocument::cleanParagraphType(const ComicBookTextCursor& _cursor)
{
    const auto oldBlockType = ComicBookBlockStyle::forBlock(_cursor.block());
    if (oldBlockType != ComicBookParagraphType::FolderHeader) {
        return;
    }

    auto cursor = _cursor;

    //
    // Удалить завершающий блок папки
    //
    cursor.movePosition(QTextCursor::NextBlock);

    // ... открытые группы на пути поиска необходимого для обновления блока
    int openedGroups = 0;
    bool isFooterUpdated = false;
    do {
        const auto currentType = ComicBookBlockStyle::forBlock(cursor.block());
        if (currentType == ComicBookParagraphType::FolderFooter) {
            if (openedGroups == 0) {
                cursor.movePosition(QTextCursor::PreviousBlock);
                cursor.movePosition(QTextCursor::EndOfBlock);
                cursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor);
                cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
                cursor.deleteChar();

                isFooterUpdated = true;
            } else {
                --openedGroups;
            }
        } else if (currentType == oldBlockType) {
            //
            // Встретилась новая папка
            //
            ++openedGroups;
        }

        if (cursor.atEnd()) {
            break;
        }

        cursor.movePosition(QTextCursor::EndOfBlock);
        cursor.movePosition(QTextCursor::NextBlock);
    } while (!isFooterUpdated);
}

void ComicBookTextDocument::applyParagraphType(BusinessLayer::ComicBookParagraphType _type,
                                               const ComicBookTextCursor& _cursor)
{
    auto cursor = _cursor;
    cursor.beginEditBlock();

    const auto newBlockStyle = d->documentTemplate().paragraphStyle(_type);

    //
    // Обновим стили
    //
    cursor.setBlockCharFormat(newBlockStyle.charFormat());
    cursor.setBlockFormat(newBlockStyle.blockFormat(cursor.inTable()));

    //
    // Применим стиль текста ко всему блоку, выделив его,
    // т.к. в блоке могут находиться фрагменты в другом стиле
    // + сохраняем форматирование выделений
    //
    {
        cursor.movePosition(QTextCursor::StartOfBlock);

        //
        // Если в блоке есть выделения, обновляем цвет только тех частей, которые не входят в
        // выделения
        //
        QTextBlock currentBlock = cursor.block();
        if (!currentBlock.textFormats().isEmpty()) {
            const auto formats = currentBlock.textFormats();
            for (const auto& range : formats) {
                if (range.format.boolProperty(ComicBookBlockStyle::PropertyIsReviewMark)) {
                    continue;
                }
                cursor.setPosition(currentBlock.position() + range.start);
                cursor.setPosition(cursor.position() + range.length, QTextCursor::KeepAnchor);
                cursor.mergeCharFormat(newBlockStyle.charFormat());
            }
        }
        //
        // Если выделений нет, обновляем блок целиком
        //
        else {
            cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
            cursor.mergeCharFormat(newBlockStyle.charFormat());
        }

        cursor.clearSelection();
    }

    //
    // Для заголовка папки нужно создать завершение
    //
    if (_type == ComicBookParagraphType::FolderHeader) {
        const auto footerStyle
            = d->documentTemplate().paragraphStyle(ComicBookParagraphType::FolderFooter);

        //
        // Вставляем закрывающий блок
        //
        cursor.insertBlock();
        cursor.setBlockCharFormat(footerStyle.charFormat());
        cursor.setBlockFormat(footerStyle.blockFormat(cursor.inTable()));
    }

    cursor.endEditBlock();
}

void ComicBookTextDocument::splitParagraph(const ComicBookTextCursor& _cursor)
{
    //
    // Получим курсор для блока, который хочет разделить пользователь
    //
    ComicBookTextCursor cursor = _cursor;
    cursor.beginEditBlock();

    //
    // Сохраним текущий формат блока
    //
    const auto lastBlockType
        = ComicBookBlockStyle::forBlock(findBlock(cursor.selectionInterval().from));
    //
    // Вырезаем выделение, захватывая блоки целиком
    //
    if (cursor.hasSelection()) {
        const auto selection = cursor.selectionInterval();
        cursor.setPosition(selection.from);
        cursor.movePosition(QTextCursor::StartOfBlock);
        cursor.setPosition(selection.to, QTextCursor::KeepAnchor);
        cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    }
    //
    // ... либо только текущий блок
    //
    else {
        cursor.movePosition(QTextCursor::StartOfBlock);
        cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    }
    //
    // ... проставляем вырезаемым элементам флаг, что они будут теперь внутри таблицы
    //
    {
        auto block = findBlock(cursor.selectionStart());
        while (block.isValid() && block.position() < cursor.selectionEnd()) {
            auto item = d->positionsToItems[block.position()];
            if (item->type() == ComicBookTextModelItemType::Text) {
                auto textItem = static_cast<ComicBookTextModelTextItem*>(item);
                textItem->setInFirstColumn(true);
            }

            block = block.next();
        }
    }
    //
    // ... собственно вырезаем
    //
    const QString mime
        = mimeFromSelection(cursor.selectionInterval().from, cursor.selectionInterval().to);
    cursor.removeSelectedText();

    //
    // Назначим блоку перед таблицей формат PageSplitter
    //
    setParagraphType(ComicBookParagraphType::PageSplitter, cursor);

    //
    // Вставляем таблицу
    //
    insertTable(cursor);
    cursor.movePosition(QTextCursor::PreviousBlock, QTextCursor::MoveAnchor, 2);
    cursor.restartEditBlock();

    //
    // Применяем сохранённый формат блока каждой из колонок
    //
    setParagraphType(lastBlockType, cursor);
    cursor.movePosition(QTextCursor::NextBlock);
    setParagraphType(lastBlockType, cursor);
    cursor.movePosition(QTextCursor::NextBlock);

    //
    // Назначим блоку после таблицы формат PageSplitter
    //
    setParagraphType(ComicBookParagraphType::PageSplitter, cursor);

    //
    // Вставляем параграф после таблицы - это обязательное условие, чтобы после таблицы всегда
    // оставался один параграф, чтобы пользователь всегда мог выйти из таблицы
    //
    if (cursor.atEnd()) {
        addParagraph(ComicBookParagraphType::Description, cursor);
    }

    //
    // Завершаем редактирование
    //
    cursor.endEditBlock();
    //
    // ... и только после этого вставляем текст в первую колонку
    //
    cursor.movePosition(QTextCursor::PreviousBlock, QTextCursor::MoveAnchor, 3);
    insertFromMime(cursor.position(), mime);
}

void ComicBookTextDocument::mergeParagraph(const ComicBookTextCursor& _cursor)
{
    //
    // Получим курсор для блока, из которого пользователь хочет убрать разделение
    //
    ComicBookTextCursor cursor = _cursor;
    cursor.movePosition(QTextCursor::StartOfBlock);
    cursor.beginEditBlock();

    //
    // Идём до начала таблицы
    //
    while (ComicBookBlockStyle::forBlock(cursor.block()) != ComicBookParagraphType::PageSplitter) {
        cursor.movePosition(QTextCursor::PreviousBlock);
    }

    //
    // Проставить элементам блоков в таблице флаг, что они будут теперь вне таблицы
    //
    {
        auto updateCursor = cursor;
        updateCursor.movePosition(QTextCursor::NextBlock);
        while (!updateCursor.atEnd() && updateCursor.inTable()) {
            auto item = d->positionsToItems[updateCursor.position()];
            if (item->type() == ComicBookTextModelItemType::Text) {
                auto textItem = static_cast<ComicBookTextModelTextItem*>(item);
                textItem->setInFirstColumn({});
            }

            updateCursor.movePosition(QTextCursor::EndOfBlock);
            updateCursor.movePosition(QTextCursor::NextBlock);
        }
    }

    //
    // Выделяем и сохраняем текст из первой ячейки
    //
    cursor.movePosition(QTextCursor::NextBlock);
    QTextTable* table = cursor.currentTable();
    while (table->cellAt(cursor).column() == 0) {
        cursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor);
    }
    cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor);
    const QString firstColumnData = cursor.selectedText().isEmpty()
        ? QString()
        : mimeFromSelection(cursor.selectionInterval().from, cursor.selectionInterval().to);
    cursor.removeSelectedText();

    //
    // Выделяем и сохраняем текст из второй ячейки
    //
    cursor.movePosition(QTextCursor::NextBlock);
    while (cursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor))
        ;
    cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    const QString secondColumnData = cursor.selectedText().isEmpty()
        ? QString()
        : mimeFromSelection(cursor.selectionInterval().from, cursor.selectionInterval().to);
    cursor.removeSelectedText();

    //
    // Удаляем таблицу
    //
    // NOTE: Делается это только таким костылём, как удалить таблицу по-человечески я не нашёл...
    //
    cursor.movePosition(QTextCursor::NextBlock);
    cursor.movePosition(QTextCursor::NextBlock);
    cursor.movePosition(QTextCursor::PreviousBlock, QTextCursor::KeepAnchor);
    cursor.movePosition(QTextCursor::PreviousBlock, QTextCursor::KeepAnchor);
    cursor.deletePreviousChar();
    cursor.endEditBlock();

    //
    // Вставляем текст из удалённых ячеек
    //
    cursor.movePosition(QTextCursor::StartOfBlock);
    cursor.movePosition(QTextCursor::PreviousCharacter);
    const int insertPosition = cursor.position();
    if (!secondColumnData.isEmpty()) {
        cursor.insertBlock({});
        insertFromMime(cursor.position(), secondColumnData);
        cursor.setPosition(insertPosition);
    }
    if (!firstColumnData.isEmpty()) {
        cursor.insertBlock({});
        insertFromMime(cursor.position(), firstColumnData);
    }
}

void ComicBookTextDocument::setCorrectionOptions(bool _needToCorrectCharactersNames,
                                                 bool _needToCorrectPageBreaks)
{
    d->corrector.setNeedToCorrectCharactersNames(_needToCorrectCharactersNames);
    d->corrector.setNeedToCorrectPageBreaks(_needToCorrectPageBreaks);
}

void ComicBookTextDocument::addReviewMark(const QColor& _textColor, const QColor& _backgroundColor,
                                          const QString& _comment,
                                          const ComicBookTextCursor& _cursor)
{
    ComicBookTextModelTextItem::ReviewMark reviewMark;
    if (_textColor.isValid()) {
        reviewMark.textColor = _textColor;
    }
    if (_backgroundColor.isValid()) {
        reviewMark.backgroundColor = _backgroundColor;
    }
    reviewMark.comments.append({ DataStorageLayer::StorageFacade::settingsStorage()->userName(),
                                 QDateTime::currentDateTime().toString(Qt::ISODate), _comment });

    auto cursor = _cursor;
    cursor.mergeCharFormat(reviewMark.charFormat());
}

void ComicBookTextDocument::updateModelOnContentChange(int _position, int _charsRemoved,
                                                       int _charsAdded)
{
    if (d->model == nullptr) {
        return;
    }

    if (!d->canChangeModel) {
        return;
    }

    //
    // Обновляем формат первого блока, если там нужен был разрыв страницы, чтобы первый блок
    // документа не перескакивал на вторую страницу
    //
    if (_position == 0
        && begin().blockFormat().pageBreakPolicy() == QTextFormat::PageBreak_AlwaysBefore) {
        ComicBookTextCursor cursor(this);
        cursor.movePosition(ComicBookTextCursor::Start);
        auto blockFormat = cursor.blockFormat();
        blockFormat.setPageBreakPolicy(QTextFormat::PageBreak_Auto);
        cursor.setBlockFormat(blockFormat);
    }

    if (d->state != DocumentState::Ready && d->state != DocumentState::Correcting) {
        return;
    }

    QScopedValueRollback temporatryState(d->state, DocumentState::Changing);

    using namespace BusinessLayer;

    //
    // Удаляем из модели элементы удалённых блоков и корректируем позиции блоков идущих после правки
    //
    {
        //
        // Собираем элементы которые потенциально могут быть удалены
        //
        std::map<ComicBookTextModelItem*, int> itemsToDelete;
        if (_charsRemoved > 0) {
            auto itemsToDeleteIter = d->positionsToItems.lower_bound(_position);
            while (itemsToDeleteIter != d->positionsToItems.end()
                   && itemsToDeleteIter->first <= _position + _charsRemoved) {
                itemsToDelete.emplace(itemsToDeleteIter->second, itemsToDeleteIter->first);
                ++itemsToDeleteIter;
            }
        }

        //
        // Проходим по изменённым блокам и фильтруем элементы, которые не были удалены
        //
        auto block = findBlock(_position);
        while (block.isValid()
               && block.position() <= _position + std::max(_charsRemoved, _charsAdded)) {
            if (block.userData() != nullptr) {
                const auto blockData = static_cast<ComicBookTextBlockData*>(block.userData());
                itemsToDelete.erase(blockData->item());
            }
            block = block.next();
        }

        //
        // Удаляем блоки, которые действительно были удалены из текста
        //
        auto removeIter = d->positionsToItems.lower_bound(_position);
        while (removeIter != d->positionsToItems.end()
               && removeIter->first <= _position + _charsRemoved) {
            //
            // Если элемент действительно удалён - удаляем его из модели
            //
            if (itemsToDelete.find(removeIter->second) != itemsToDelete.end()) {
                auto item = removeIter->second;

                //
                // Если удаляется сцена или папка, нужно удалить соответствующий элемент
                // и перенести элементы к предыдущему группирующему элементу
                //
                bool needToDeleteParent = false;
                if (item->type() == ComicBookTextModelItemType::Text) {
                    const auto textItem = static_cast<ComicBookTextModelTextItem*>(item);
                    //
                    // ... т.к. при удалении папки удаляются и заголовок и конец, но удаляются они
                    //     последовательно сверху вниз, то удалять непосредственно папку будем,
                    //     когда дойдём до обработки именно конца папки
                    //
                    needToDeleteParent
                        = textItem->paragraphType() == ComicBookParagraphType::FolderFooter
                        || textItem->paragraphType() == ComicBookParagraphType::Page
                        || textItem->paragraphType() == ComicBookParagraphType::Panel;
                }

                //
                // Запомним родителя и удаляем сам элемент
                //
                auto itemParent = item->parent();
                d->model->removeItem(item);

                //
                // Если необходимо удаляем родительский элемент
                //
                if (needToDeleteParent && itemParent != nullptr) {
                    //
                    // Определим предыдущий
                    //
                    ComicBookTextModelItem* previousItem = nullptr;
                    const int itemRow = itemParent->hasParent()
                        ? itemParent->parent()->rowOfChild(itemParent)
                        : 0;
                    if (itemRow > 0) {
                        const int previousItemRow = itemRow - 1;
                        previousItem = itemParent->parent()->childAt(previousItemRow);
                    }

                    //
                    // Переносим дочерние элементы на уровень родительского элемента
                    //
                    ComicBookTextModelItem* lastMovedItem = nullptr;
                    while (itemParent->childCount() > 0) {
                        auto childItem = itemParent->childAt(0);
                        d->model->takeItem(childItem, itemParent);

                        //
                        // Папки и сцены переносим на один уровень с текущим элементом
                        //
                        if (childItem->type() == ComicBookTextModelItemType::Folder
                            || childItem->type() == ComicBookTextModelItemType::Page
                            || childItem->type() == ComicBookTextModelItemType::Panel) {
                            if (lastMovedItem == nullptr
                                || lastMovedItem->parent() != itemParent->parent()) {
                                d->model->insertItem(childItem, itemParent);
                            } else {
                                d->model->insertItem(childItem, lastMovedItem);
                            }
                            previousItem = childItem;
                        }
                        //
                        // Все остальные элементы
                        //
                        else {
                            if (lastMovedItem == nullptr) {
                                //
                                // Если перед удаляемым была сцена или папка, то в её конец
                                //
                                if (previousItem != nullptr
                                    && (previousItem->type() == ComicBookTextModelItemType::Folder
                                        || previousItem->type() == ComicBookTextModelItemType::Page
                                        || previousItem->type()
                                            == ComicBookTextModelItemType::Panel)) {
                                    d->model->appendItem(childItem, previousItem);
                                }
                                //
                                // Если перед удаляемым внутри родителя нет ни одного элемента, то
                                // вставляем в начало к деду
                                //
                                else if (previousItem == nullptr
                                         && itemParent->parent() != nullptr) {
                                    d->model->prependItem(childItem, itemParent->parent());
                                }
                                //
                                // Во всех остальных случаях просто кладём на один уровень с
                                // предыдущим элементом
                                //
                                else {
                                    d->model->insertItem(childItem, previousItem);
                                }
                            } else {
                                d->model->insertItem(childItem, lastMovedItem);
                            }
                        }

                        lastMovedItem = childItem;
                    }

                    //
                    // Удаляем родителя удалённого элемента
                    //
                    d->model->removeItem(itemParent);

                    //
                    // Если после удаляемого элемента есть текстовые элементы, пробуем их встроить в
                    // предыдущую сцену
                    //
                    if (previousItem != nullptr
                        && (previousItem->type() == ComicBookTextModelItemType::Page
                            || previousItem->type() == ComicBookTextModelItemType::Panel)) {
                        const auto previousItemRow
                            = previousItem->parent()->rowOfChild(previousItem);
                        if (previousItemRow >= 0
                            && previousItemRow < previousItem->parent()->childCount() - 1) {
                            const int nextItemRow = previousItemRow + 1;
                            auto nextItem = previousItem->parent()->childAt(nextItemRow);
                            while (
                                nextItem != nullptr
                                && (nextItem->type() == ComicBookTextModelItemType::Text
                                    || nextItem->type() == ComicBookTextModelItemType::Splitter)) {
                                d->model->takeItem(nextItem, nextItem->parent());
                                d->model->appendItem(nextItem, previousItem);
                                nextItem = previousItem->parent()->childAt(nextItemRow);
                            }
                        }
                    }
                }
            }

            //
            // Убираем информацию о позиции блока, т.к. она могла измениться и будет обновлена далее
            //
            removeIter = d->positionsToItems.erase(removeIter);
        }

        //
        // Корректируем позиции элементов идущих за удаляемым блоком
        //

        auto itemToUpdateIter = removeIter;

        //
        // Формируем мапу элементов со скорректированными позициями
        //
        std::map<int, ComicBookTextModelItem*> correctedItems;
        for (auto itemIter = itemToUpdateIter; itemIter != d->positionsToItems.end(); ++itemIter) {
            correctedItems.emplace(itemIter->first - _charsRemoved + _charsAdded, itemIter->second);
        }

        //
        // Удаляем элементы со старыми позициями
        //
        d->positionsToItems.erase(itemToUpdateIter, d->positionsToItems.end());

        //
        // И записываем на их место новые элементы
        //
        d->positionsToItems.merge(correctedItems);
    }

    //
    // Идём с позиции начала, до конца добавления
    //
    auto block = findBlock(_position);
    //
    // ... определим элемент модели для предыдущего блока
    //
    auto previousItem = [block]() -> ComicBookTextModelItem* {
        if (!block.isValid()) {
            return nullptr;
        }

        auto previousBlock = block.previous();
        if (!previousBlock.isValid() || previousBlock.userData() == nullptr) {
            return nullptr;
        }

        auto blockData = static_cast<ComicBookTextBlockData*>(previousBlock.userData());
        return blockData->item();
    }();

    //
    // Информация о таблице, в которой находится блок
    //
    struct TableInfo {
        bool inTable = false;
        bool inFirstColumn = false;
    } tableInfo;
    auto updateTableInfo = [this, &tableInfo](const QTextBlock& _block) {
        ComicBookTextCursor cursor(this);
        cursor.setPosition(_block.position());
        tableInfo.inTable = cursor.inTable();
        tableInfo.inFirstColumn = cursor.inFirstColumn();
    };
    updateTableInfo(block);

    while (block.isValid() && block.position() <= _position + _charsAdded) {
        const auto paragraphType = ComicBookBlockStyle::forBlock(block);

        //
        // Новый блок
        //
        if (block.userData() == nullptr) {
            //
            // Разделитель
            //
            if (paragraphType == ComicBookParagraphType::PageSplitter) {
                ComicBookTextCursor cursor(this);
                cursor.setPosition(block.position());
                cursor.movePosition(QTextCursor::NextBlock);
                //
                // Сформируем элемент в зависимости от типа разделителя
                //
                ComicBookTextModelSplitterItem* splitterItem = nullptr;
                if (cursor.inTable() && !tableInfo.inTable) {
                    tableInfo.inTable = true;
                    tableInfo.inFirstColumn = true;
                    splitterItem = new ComicBookTextModelSplitterItem(
                        ComicBookTextModelSplitterItemType::Start);
                } else {
                    tableInfo = {};
                    splitterItem = new ComicBookTextModelSplitterItem(
                        ComicBookTextModelSplitterItemType::End);
                }
                if (previousItem == nullptr) {
                    d->model->prependItem(splitterItem);
                } else {
                    d->model->insertItem(splitterItem, previousItem);
                }

                //
                // Запомним информацию о разделителе в блоке
                //
                auto blockData = new ComicBookTextBlockData(splitterItem);
                block.setUserData(blockData);
                previousItem = splitterItem;

                //
                // Запомним новый блок, или обновим старый
                //
                d->positionsToItems.insert_or_assign(block.position(), previousItem);

                block = block.next();
                continue;
            } else {
                updateTableInfo(block);
            }

            //
            // Создаём группирующий элемент, если создаётся непосредственно сцена или папка
            //
            ComicBookTextModelItem* parentItem = nullptr;
            switch (paragraphType) {
            case ComicBookParagraphType::FolderHeader: {
                parentItem = new ComicBookTextModelFolderItem;
                break;
            }

            case ComicBookParagraphType::Page: {
                parentItem = new ComicBookTextModelPageItem;
                break;
            }

            case ComicBookParagraphType::Panel: {
                parentItem = new ComicBookTextModelPanelItem;
                break;
            }

            default:
                break;
            }

            //
            // Создаём сам текстовый элемент
            //
            auto textItem = new ComicBookTextModelTextItem;
            textItem->setCorrection(
                block.blockFormat().boolProperty(ComicBookBlockStyle::PropertyIsCorrection));
            textItem->setCorrectionContinued(block.blockFormat().boolProperty(
                ComicBookBlockStyle::PropertyIsCorrectionContinued));
            textItem->setBreakCorrectionStart(block.blockFormat().boolProperty(
                ComicBookBlockStyle::PropertyIsBreakCorrectionStart));
            textItem->setBreakCorrectionEnd(block.blockFormat().boolProperty(
                ComicBookBlockStyle::PropertyIsBreakCorrectionEnd));
            if (tableInfo.inTable) {
                textItem->setInFirstColumn(tableInfo.inFirstColumn);
            } else {
                textItem->setInFirstColumn({});
            }
            textItem->setParagraphType(paragraphType);
            if (d->documentTemplate().paragraphStyle(paragraphType).align()
                != block.blockFormat().alignment()) {
                textItem->setAlignment(block.blockFormat().alignment());
            } else {
                textItem->clearAlignment();
            }
            textItem->setText(block.text());
            textItem->setFormats(block.textFormats());
            textItem->setReviewMarks(block.textFormats());

            //
            // Является ли предыдущий элемент футером папки
            //
            const bool previousItemIsFolderFooter = [previousItem] {
                if (!previousItem || previousItem->type() != ComicBookTextModelItemType::Text) {
                    return false;
                }

                auto textItem = static_cast<ComicBookTextModelTextItem*>(previousItem);
                return textItem->paragraphType() == ComicBookParagraphType::FolderFooter;
            }();

            //
            // Добавляем элементы в модель
            //
            // ... в случае, когда вставляем внутрь созданной папки, или сцены
            //
            if (parentItem != nullptr) {
                //
                // Если перед вставляемым элементом что-то уже есть
                //
                if (previousItem != nullptr) {
                    auto previousTextItemParent = previousItem->parent();
                    Q_ASSERT(previousTextItemParent);

                    //
                    // Если элемент вставляется после другого элемента того же уровня, или после
                    // окончания папки, то вставляем его на том же уровне, что и предыдущий
                    //
                    if (previousTextItemParent->type() == parentItem->type()
                        || previousItemIsFolderFooter) {
                        d->model->insertItem(parentItem, previousTextItemParent);
                    }
                    //
                    //
                    //
                    else if (parentItem->type() == ComicBookTextModelItemType::Page
                             && previousTextItemParent->type()
                                 == ComicBookTextModelItemType::Panel) {
                        d->model->insertItem(parentItem, previousTextItemParent->parent());
                    }
                    //
                    // В противном случае вставляем внутрь
                    //
                    else {
                        d->model->insertItem(parentItem, previousItem);
                    }
                }
                //
                // Если перед вставляемым ничего нет, просто вставим в самое начало
                //
                else {
                    d->model->prependItem(parentItem);
                }

                //
                // Вставляем сам текстовый элемент в родителя
                //
                d->model->appendItem(textItem, parentItem);

                //
                // Если вставляется сцена, то все текстовые элементы идущие после неё нужно
                // положить к ней внутрь
                //
                if (parentItem->type() == ComicBookTextModelItemType::Page
                    || parentItem->type() == ComicBookTextModelItemType::Panel) {
                    //
                    // Определим родителя из которого нужно извлекать те самые текстовые элементы
                    //
                    auto grandParentItem = [previousItem, previousItemIsFolderFooter, parentItem] {
                        //
                        // Если есть предыдущий текстовый элемент
                        //
                        if (previousItem != nullptr) {
                            //
                            // Если это конец папки, то берём родителя папки
                            //
                            if (previousItemIsFolderFooter) {
                                return previousItem->parent()->parent();
                            }
                            //
                            // В противном случае, берём родителя предыдущего текстового элемента
                            //
                            else {
                                return previousItem->parent();
                            }
                        }

                        //
                        // Если перед сценой ничего нет, то берём родителя самой сцены
                        //
                        return parentItem->parent();
                    }();
                    Q_ASSERT(grandParentItem);

                    //
                    // Определим индекс, начиная с которого нужно извлекать текстовые элементы
                    //
                    const int itemIndex = [previousItem, previousItemIsFolderFooter, parentItem,
                                           grandParentItem] {
                        if (previousItem != nullptr) {
                            if (previousItemIsFolderFooter) {
                                return grandParentItem->rowOfChild(previousItem->parent()) + 2;
                            } else if (grandParentItem->type() == ComicBookTextModelItemType::Page
                                       || grandParentItem->type()
                                           == ComicBookTextModelItemType::Panel) {
                                return grandParentItem->rowOfChild(previousItem) + 1;
                            }
                        }

                        return grandParentItem->rowOfChild(parentItem) + 1;
                    }();

                    //
                    // Собственно переносим элементы
                    //
                    while (grandParentItem->childCount() > itemIndex) {
                        auto grandParentChildItem = grandParentItem->childAt(itemIndex);
                        if (grandParentChildItem->type() != ComicBookTextModelItemType::Text) {
                            break;
                        }

                        auto grandParentChildTextItem
                            = static_cast<ComicBookTextModelTextItem*>(grandParentChildItem);
                        if (grandParentChildTextItem->paragraphType()
                            == ComicBookParagraphType::FolderFooter) {
                            break;
                        }

                        d->model->takeItem(grandParentChildItem, grandParentItem);
                        d->model->appendItem(grandParentChildItem, parentItem);
                    }
                }
                //
                // А для папки, если она вставляется после сцены, то нужно перенести все текстовые
                // элементы, которые идут после вставленной папки на уровень самой папки
                //
                else if (previousItem != nullptr
                         && (previousItem->parent()->type() == ComicBookTextModelItemType::Page
                             || previousItem->parent()->type()
                                 == ComicBookTextModelItemType::Panel)) {
                    auto grandParentItem = previousItem->parent();
                    const int lastItemIndex = grandParentItem->rowOfChild(previousItem) + 1;
                    //
                    // Собственно переносим элементы
                    //
                    while (grandParentItem->childCount() > lastItemIndex) {
                        auto grandParentChildItem
                            = grandParentItem->childAt(grandParentItem->childCount() - 1);
                        if (grandParentChildItem->type() != ComicBookTextModelItemType::Text) {
                            break;
                        }

                        auto grandParentChildTextItem
                            = static_cast<ComicBookTextModelTextItem*>(grandParentChildItem);
                        if (grandParentChildTextItem->paragraphType()
                            == ComicBookParagraphType::FolderFooter) {
                            break;
                        }

                        d->model->takeItem(grandParentChildItem, grandParentItem);
                        d->model->insertItem(grandParentChildItem, parentItem);
                    }
                }
            }
            //
            // ... в случае, когда добавился просто текст
            //
            else {
                //
                // ... в самое начало документа
                //
                if (previousItem == nullptr) {
                    d->model->prependItem(textItem);
                }
                //
                // ... после предыдущего элемента
                //
                else {
                    //
                    // ... если блок вставляется после конца папки, то нужно вынести на уровень с
                    // папкой
                    //
                    if (previousItemIsFolderFooter) {
                        d->model->insertItem(textItem, previousItem->parent());
                    }
                    //
                    // ... в противном случае ставим на уровне с предыдущим элементом
                    //
                    else {
                        d->model->insertItem(textItem, previousItem);
                    }
                }
            }

            auto blockData = new ComicBookTextBlockData(textItem);
            block.setUserData(blockData);

            previousItem = textItem;
        }
        //
        // Старый блок
        //
        else {
            updateTableInfo(block);
            auto blockData = static_cast<ComicBookTextBlockData*>(block.userData());
            auto item = blockData->item();

            if (item->type() == ComicBookTextModelItemType::Text) {
                auto textItem = static_cast<ComicBookTextModelTextItem*>(item);
                textItem->setCorrection(
                    block.blockFormat().boolProperty(ComicBookBlockStyle::PropertyIsCorrection));
                textItem->setCorrectionContinued(block.blockFormat().boolProperty(
                    ComicBookBlockStyle::PropertyIsCorrectionContinued));
                textItem->setBreakCorrectionStart(block.blockFormat().boolProperty(
                    ComicBookBlockStyle::PropertyIsBreakCorrectionStart));
                textItem->setBreakCorrectionEnd(block.blockFormat().boolProperty(
                    ComicBookBlockStyle::PropertyIsBreakCorrectionEnd));
                if (tableInfo.inTable) {
                    textItem->setInFirstColumn(tableInfo.inFirstColumn);
                } else {
                    textItem->setInFirstColumn({});
                }
                textItem->setParagraphType(paragraphType);
                if (d->documentTemplate().paragraphStyle(paragraphType).align()
                    != block.blockFormat().alignment()) {
                    textItem->setAlignment(block.blockFormat().alignment());
                } else {
                    textItem->clearAlignment();
                }
                textItem->setText(block.text());
                textItem->setFormats(block.textFormats());
                textItem->setReviewMarks(block.textFormats());
            }

            d->model->updateItem(item);
            previousItem = item;
        }

        //
        // Запомним новый блок, или обновим старый
        //
        d->positionsToItems.insert_or_assign(block.position(), previousItem);

        //
        // Переходим к обработке следующего блока
        //
        block = block.next();
    }
}

void ComicBookTextDocument::insertTable(const ComicBookTextCursor& _cursor)
{
    const auto scriptTemplate = d->documentTemplate();
    const auto pageSplitterWidth = scriptTemplate.pageSplitterWidth();
    const int qtTableBorderWidth = 2; // эта однопиксельная рамка никак не убирается...
    const qreal tableWidth = pageSize().width() - rootFrame()->frameFormat().leftMargin()
        - rootFrame()->frameFormat().rightMargin() - qtTableBorderWidth + pageSplitterWidth;
    const qreal leftColumnWidth = tableWidth * scriptTemplate.leftHalfOfPageWidthPercents() / 100.0;
    const qreal rightColumnWidth = tableWidth - leftColumnWidth;
    QTextTableFormat format;
    format.setBorder(0);
    format.setBorderStyle(QTextFrameFormat::BorderStyle_None);
    format.setWidth(QTextLength{ QTextLength::FixedLength, tableWidth });
    format.setColumnWidthConstraints({ QTextLength{ QTextLength::FixedLength, leftColumnWidth },
                                       QTextLength{ QTextLength::FixedLength, rightColumnWidth } });
    format.setLeftMargin(-1 * pageSplitterWidth);

    auto cursor = _cursor;
    cursor.insertTable(1, 2, format);
}

} // namespace BusinessLayer
