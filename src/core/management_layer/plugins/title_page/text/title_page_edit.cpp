#include "title_page_edit.h"

#include "handlers/key_press_handler_facade.h"

#include <business_layer/document/simple_text/simple_text_document.h>
#include <business_layer/document/text/text_block_data.h>
#include <business_layer/document/text/text_cursor.h>
#include <business_layer/export/export_options.h>
#include <business_layer/export/simple_text/simple_text_markdown_exporter.h>
#include <business_layer/import/text/simple_text_markdown_importer.h>
#include <business_layer/model/audioplay/audioplay_information_model.h>
#include <business_layer/model/audioplay/audioplay_title_page_model.h>
#include <business_layer/model/characters/character_model.h>
#include <business_layer/model/comic_book/comic_book_information_model.h>
#include <business_layer/model/comic_book/comic_book_title_page_model.h>
#include <business_layer/model/novel/novel_information_model.h>
#include <business_layer/model/novel/novel_title_page_model.h>
#include <business_layer/model/screenplay/screenplay_information_model.h>
#include <business_layer/model/screenplay/screenplay_title_page_model.h>
#include <business_layer/model/simple_text/simple_text_model.h>
#include <business_layer/model/stageplay/stageplay_information_model.h>
#include <business_layer/model/stageplay/stageplay_title_page_model.h>
#include <business_layer/model/text/text_model_text_item.h>
#include <business_layer/templates/audioplay_template.h>
#include <business_layer/templates/comic_book_template.h>
#include <business_layer/templates/novel_template.h>
#include <business_layer/templates/screenplay_template.h>
#include <business_layer/templates/simple_text_template.h>
#include <business_layer/templates/stageplay_template.h>
#include <business_layer/templates/templates_facade.h>
#include <domain/document_object.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/context_menu/context_menu.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/model_helper.h>
#include <utils/helpers/text_helper.h>

#include <QAction>
#include <QCoreApplication>
#include <QDir>
#include <QLocale>
#include <QMimeData>
#include <QPainter>
#include <QPointer>
#include <QScrollBar>
#include <QTextTable>

using BusinessLayer::TemplatesFacade;
using BusinessLayer::TextBlockStyle;
using BusinessLayer::TextCursor;
using BusinessLayer::TextParagraphType;

namespace Ui {

namespace {
const QLatin1String kMarkdownMimeType("text/markdown");
}

class TitlePageEdit::Implementation
{
public:
    explicit Implementation(TitlePageEdit* _q);

    /**
     * @brief Текущий шаблон документа
     */
    const BusinessLayer::TextTemplate& textTemplate() const;

    /**
     * @brief Отменить/повторить последнее действие
     */
    void revertAction(bool previous);


    TitlePageEdit* q = nullptr;

    QPointer<BusinessLayer::TitlePageModel> model;
    BusinessLayer::SimpleTextDocument document;
};

TitlePageEdit::Implementation::Implementation(TitlePageEdit* _q)
    : q(_q)
{
}

const BusinessLayer::TextTemplate& TitlePageEdit::Implementation::textTemplate() const
{
    return TemplatesFacade::textTemplate(model);
}

void TitlePageEdit::Implementation::revertAction(bool previous)
{
    if (model == nullptr) {
        return;
    }

    auto finalCursorPosition = q->textCursor().position();

    BusinessLayer::ChangeCursor changeCursor;
    if (previous) {
        changeCursor = model->undo();
    } else {
        changeCursor = model->redo();
    }
    //
    if (changeCursor.item != nullptr) {
        const auto item = static_cast<BusinessLayer::TextModelItem*>(changeCursor.item);
        const auto itemIndex = model->indexForItem(item);
        finalCursorPosition = document.itemEndPosition(itemIndex);
        if (changeCursor.position >= 0) {
            finalCursorPosition += changeCursor.position;
        } else if (item->type() == BusinessLayer::TextModelItemType::Text) {
            const auto textItem = static_cast<BusinessLayer::TextModelTextItem*>(item);
            finalCursorPosition += textItem->text().length();
        }
    }

    auto cursor = q->textCursor();
    cursor.setPosition(finalCursorPosition);
    q->setTextCursorAndKeepScrollBars(cursor);
    q->ensureCursorVisible();

    //
    // При отмене/повторе последнего действия позиция курсора могла и не поменяться,
    // но тип параграфа сменился, поэтому перестраховываемся и говорим будто бы
    // сменилась позиция курсора, чтобы обновить состояние панелей
    //
    emit q->cursorPositionChanged();
}


// ****


TitlePageEdit::TitlePageEdit(QWidget* _parent)
    : ScriptTextEdit(_parent)
    , d(new Implementation(this))
{
    setContextMenuPolicy(Qt::CustomContextMenu);
    setFrameShape(QFrame::NoFrame);
    setShowPageNumbers(false);

    setDocument(&d->document);
    setCapitalizeWords(false);
}

TitlePageEdit::~TitlePageEdit() = default;

void TitlePageEdit::initWithModel(BusinessLayer::TitlePageModel* _model)
{
    if (auto titlePageModel = qobject_cast<BusinessLayer::ScreenplayTitlePageModel*>(d->model)) {
        titlePageModel->informationModel()->disconnect(this);
    } else if (auto titlePageModel
               = qobject_cast<BusinessLayer::ComicBookTitlePageModel*>(d->model)) {
        titlePageModel->informationModel()->disconnect(this);
    } else if (auto titlePageModel
               = qobject_cast<BusinessLayer::AudioplayTitlePageModel*>(d->model)) {
        titlePageModel->informationModel()->disconnect(this);
    } else if (auto titlePageModel
               = qobject_cast<BusinessLayer::StageplayTitlePageModel*>(d->model)) {
        titlePageModel->informationModel()->disconnect(this);
    } else if (auto titlePageModel = qobject_cast<BusinessLayer::NovelTitlePageModel*>(d->model)) {
        titlePageModel->informationModel()->disconnect(this);
    }

    d->model = _model;

    //
    // Сбрасываем модель, чтобы не вылезали изменения документа при изменении параметров страницы
    //
    d->document.setModel(nullptr);

    //
    // Сценарий
    //
    if (auto titlePageModel = qobject_cast<BusinessLayer::ScreenplayTitlePageModel*>(d->model)) {
        auto updateHeader = [this, titlePageModel] {
            setHeader(titlePageModel->informationModel()->printHeaderOnTitlePage()
                          ? titlePageModel->informationModel()->header()
                          : QString());
        };
        updateHeader();
        connect(titlePageModel->informationModel(),
                &BusinessLayer::ScreenplayInformationModel::printHeaderOnTitlePageChanged, this,
                updateHeader);
        connect(titlePageModel->informationModel(),
                &BusinessLayer::ScreenplayInformationModel::headerChanged, this, updateHeader);

        auto updateFooter = [this, titlePageModel] {
            setFooter(titlePageModel->informationModel()->printFooterOnTitlePage()
                          ? titlePageModel->informationModel()->footer()
                          : QString());
        };
        updateFooter();
        connect(titlePageModel->informationModel(),
                &BusinessLayer::ScreenplayInformationModel::printFooterOnTitlePageChanged, this,
                updateFooter);
        connect(titlePageModel->informationModel(),
                &BusinessLayer::ScreenplayInformationModel::footerChanged, this, updateFooter);

        connect(titlePageModel->informationModel(),
                &BusinessLayer::ScreenplayInformationModel::templateIdChanged, this,
                &TitlePageEdit::reinit);
    }
    //
    // Комикс
    //
    else if (auto titlePageModel
             = qobject_cast<BusinessLayer::ComicBookTitlePageModel*>(d->model)) {
        auto updateHeader = [this, titlePageModel] {
            setHeader(titlePageModel->informationModel()->printHeaderOnTitlePage()
                          ? titlePageModel->informationModel()->header()
                          : QString());
        };
        updateHeader();
        connect(titlePageModel->informationModel(),
                &BusinessLayer::ComicBookInformationModel::printHeaderOnTitlePageChanged, this,
                updateHeader);
        connect(titlePageModel->informationModel(),
                &BusinessLayer::ComicBookInformationModel::headerChanged, this, updateHeader);

        auto updateFooter = [this, titlePageModel] {
            setFooter(titlePageModel->informationModel()->printFooterOnTitlePage()
                          ? titlePageModel->informationModel()->footer()
                          : QString());
        };
        updateFooter();
        connect(titlePageModel->informationModel(),
                &BusinessLayer::ComicBookInformationModel::printFooterOnTitlePageChanged, this,
                updateFooter);
        connect(titlePageModel->informationModel(),
                &BusinessLayer::ComicBookInformationModel::footerChanged, this, updateFooter);

        connect(titlePageModel->informationModel(),
                &BusinessLayer::ComicBookInformationModel::templateIdChanged, this,
                &TitlePageEdit::reinit);
    }
    //
    // Аудиопостановка
    //
    else if (auto titlePageModel
             = qobject_cast<BusinessLayer::AudioplayTitlePageModel*>(d->model)) {
        auto updateHeader = [this, titlePageModel] {
            setHeader(titlePageModel->informationModel()->printHeaderOnTitlePage()
                          ? titlePageModel->informationModel()->header()
                          : QString());
        };
        updateHeader();
        connect(titlePageModel->informationModel(),
                &BusinessLayer::AudioplayInformationModel::printHeaderOnTitlePageChanged, this,
                updateHeader);
        connect(titlePageModel->informationModel(),
                &BusinessLayer::AudioplayInformationModel::headerChanged, this, updateHeader);

        auto updateFooter = [this, titlePageModel] {
            setFooter(titlePageModel->informationModel()->printFooterOnTitlePage()
                          ? titlePageModel->informationModel()->footer()
                          : QString());
        };
        updateFooter();
        connect(titlePageModel->informationModel(),
                &BusinessLayer::AudioplayInformationModel::printFooterOnTitlePageChanged, this,
                updateFooter);
        connect(titlePageModel->informationModel(),
                &BusinessLayer::AudioplayInformationModel::footerChanged, this, updateFooter);

        connect(titlePageModel->informationModel(),
                &BusinessLayer::AudioplayInformationModel::templateIdChanged, this,
                &TitlePageEdit::reinit);
    }
    //
    // Пьеса
    //
    else if (auto titlePageModel
             = qobject_cast<BusinessLayer::StageplayTitlePageModel*>(d->model)) {
        auto updateHeader = [this, titlePageModel] {
            setHeader(titlePageModel->informationModel()->printHeaderOnTitlePage()
                          ? titlePageModel->informationModel()->header()
                          : QString());
        };
        updateHeader();
        connect(titlePageModel->informationModel(),
                &BusinessLayer::StageplayInformationModel::printHeaderOnTitlePageChanged, this,
                updateHeader);
        connect(titlePageModel->informationModel(),
                &BusinessLayer::StageplayInformationModel::headerChanged, this, updateHeader);

        auto updateFooter = [this, titlePageModel] {
            setFooter(titlePageModel->informationModel()->printFooterOnTitlePage()
                          ? titlePageModel->informationModel()->footer()
                          : QString());
        };
        updateFooter();
        connect(titlePageModel->informationModel(),
                &BusinessLayer::StageplayInformationModel::printFooterOnTitlePageChanged, this,
                updateFooter);
        connect(titlePageModel->informationModel(),
                &BusinessLayer::StageplayInformationModel::footerChanged, this, updateFooter);

        connect(titlePageModel->informationModel(),
                &BusinessLayer::StageplayInformationModel::templateIdChanged, this,
                &TitlePageEdit::reinit);
    }
    //
    // Роман
    //
    else if (auto titlePageModel = qobject_cast<BusinessLayer::NovelTitlePageModel*>(d->model)) {
        auto updateHeader = [this, titlePageModel] {
            setHeader(titlePageModel->informationModel()->printHeaderOnTitlePage()
                          ? titlePageModel->informationModel()->header()
                          : QString());
        };
        updateHeader();
        connect(titlePageModel->informationModel(),
                &BusinessLayer::NovelInformationModel::printHeaderOnTitlePageChanged, this,
                updateHeader);
        connect(titlePageModel->informationModel(),
                &BusinessLayer::NovelInformationModel::headerChanged, this, updateHeader);

        auto updateFooter = [this, titlePageModel] {
            setFooter(titlePageModel->informationModel()->printFooterOnTitlePage()
                          ? titlePageModel->informationModel()->footer()
                          : QString());
        };
        updateFooter();
        connect(titlePageModel->informationModel(),
                &BusinessLayer::NovelInformationModel::printFooterOnTitlePageChanged, this,
                updateFooter);
        connect(titlePageModel->informationModel(),
                &BusinessLayer::NovelInformationModel::footerChanged, this, updateFooter);

        connect(titlePageModel->informationModel(),
                &BusinessLayer::NovelInformationModel::templateIdChanged, this,
                &TitlePageEdit::reinit);
    }

    //
    // Если в модели всего один пустой элемент, значит нужно заменить титульной страницей из шаблона
    //
    ModelHelper::initTitlePageModel(d->model);

    //
    // Настроим редактор в соответствии с параметрами шаблона
    //
    const auto& currentTemplate = textTemplate();
    setPageFormat(currentTemplate.pageSizeId());
    setPageNumbersAlignment(currentTemplate.pageNumbersAlignment());

    //
    // Для титульной страницы уравновесим поля в сторону большего,
    // на случай, если кто-то ещё их печатает и сшивает
    //
    auto pageMargins = currentTemplate.pageMargins();
    if (pageMargins.left() < pageMargins.right()) {
        pageMargins.setLeft(pageMargins.right());
    } else if (pageMargins.right() < pageMargins.left()) {
        pageMargins.setRight(pageMargins.left());
    }
    setPageMarginsMm(pageMargins);

    //
    // Документ нужно формировать только после того, как редактор настроен, чтобы избежать лишний
    // изменений
    //
    d->document.setModel(d->model);

    emit cursorPositionChanged();
}

void TitlePageEdit::reinit()
{
    //
    // Перенастроим всё, что зависит от шаблона
    //
    initWithModel(d->model);
}

const BusinessLayer::TextTemplate& TitlePageEdit::textTemplate() const
{
    return d->textTemplate();
}

void TitlePageEdit::undo()
{
    d->revertAction(true);
}

void TitlePageEdit::redo()
{
    d->revertAction(false);
}

void TitlePageEdit::addCastList()
{
    //
    // Пока что нам нужны в персонажей только для аудиопьес и пьес
    //
    if (d->model.isNull() || d->model->document() == nullptr
        || (d->model->document()->type() != Domain::DocumentObjectType::AudioplayTitlePage
            && d->model->document()->type() != Domain::DocumentObjectType::StageplayTitlePage)) {
        return;
    }

    //
    // Запрашиваем обновление списка персонажей сценария
    //
    emit d->model->charactersUpdateRequested();

    const auto characters = d->model->characters();
    if (characters.isEmpty()) {
        return;
    }

    //
    // Выводим список персонажей в конец документа
    //
    auto cursor = textCursor();
    //
    // ... добавляем заголовок (посередине и полужирным)
    //
    moveCursor(QTextCursor::End);
    addParagraph(BusinessLayer::TextParagraphType::Text);
    addParagraph(BusinessLayer::TextParagraphType::ChapterHeading6);
    cursor.movePosition(QTextCursor::End);
    auto blockFormat = cursor.blockFormat();
    blockFormat.setAlignment(Qt::AlignHCenter);
    cursor.setBlockFormat(blockFormat);
    auto charFormat = cursor.blockCharFormat();
    charFormat.setFontWeight(QFont::Bold);
    cursor.mergeBlockCharFormat(charFormat);
    cursor.insertText(tr("Cast list"));
    const auto titleCursorPosition = cursor.position();
    moveCursor(QTextCursor::End);
    addParagraph(BusinessLayer::TextParagraphType::Text);
    for (const auto& character : characters) {
        //
        // ... в зависимости от типа контента генерируем список действующих лиц
        //
        moveCursor(QTextCursor::End);
        addParagraph(BusinessLayer::TextParagraphType::Text);
        cursor.movePosition(QTextCursor::End);
        cursor.insertText(character.first);
        //
        // ... для аудиопьес подготавливаем место для указания исполнителя роли
        //
        if (d->model->document()->type() == Domain::DocumentObjectType::AudioplayTitlePage) {
            const auto tbdTerm = " " + tr("TBD");
            cursor.insertText(" " + QString().fill('.', 70) + tbdTerm);
            cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::MoveAnchor,
                                tbdTerm.length());
            while (cursor.block().layout()->lineCount() > 1) {
                cursor.deletePreviousChar();
            }
            //
            // ... а так же делаем пустую строку, чтобы список действующих лиц не конфликтовал
            //     с основным текстом, т.к. аудиопостановки имеют полуторный интервал обычно
            //
            moveCursor(QTextCursor::End);
            addParagraph(BusinessLayer::TextParagraphType::Text);
        }
        //
        // ... а для пьес добавляем описание персонажа, если оно доступно
        //
        else if (d->model->document()->type() == Domain::DocumentObjectType::StageplayTitlePage) {
            if (!character.second.isEmpty()) {
                cursor.insertText(", " + character.second);
            }
        }
    }

    cursor.setPosition(titleCursorPosition);
    ensureCursorVisible(cursor);
}

void TitlePageEdit::restoreFromTemplate()
{
    ModelHelper::resetTitlePageModel(d->model);
}

void TitlePageEdit::addParagraph(BusinessLayer::TextParagraphType _type)
{
    d->document.addParagraph(_type, textCursor());

    emit paragraphTypeChanged();
}

QModelIndex TitlePageEdit::currentModelIndex() const
{
    if (d->model == nullptr || d->document.isEditTransactionActive()) {
        return {};
    }

    auto userData = textCursor().block().userData();
    if (userData == nullptr) {
        return {};
    }

    auto screenplayBlockData = static_cast<BusinessLayer::TextBlockData*>(userData);
    return d->model->indexForItem(screenplayBlockData->item());
}

void TitlePageEdit::setCurrentModelIndex(const QModelIndex& _index)
{
    if (!_index.isValid() || _index.model() != d->model) {
        return;
    }

    QTextCursor textCursor(document());
    textCursor.setPosition(d->document.itemStartPosition(_index));
    ensureCursorVisible(textCursor);
}

int TitlePageEdit::positionForModelIndex(const QModelIndex& _index)
{
    return d->document.itemStartPosition(_index);
}

void TitlePageEdit::addReviewMark(const QColor& _textColor, const QColor& _backgroundColor,
                                  const QString& _comment, bool _isRevision, bool _isAddition,
                                  bool _isRemoval)
{
    QTextCursor cursor(textCursor());
    if (!cursor.hasSelection()) {
        return;
    }

    d->document.addReviewMark(_textColor, _backgroundColor, _comment, _isRevision, _isAddition,
                              _isRemoval, cursor);
}

void TitlePageEdit::keyPressEvent(QKeyEvent* _event)
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

        updateEnteredText(_event);
    }

    //
    // Обработка
    //
    if (!_event->isAccepted()) {
        handler->handle(_event);
        updateTypewriterScroll();
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

bool TitlePageEdit::keyPressEventReimpl(QKeyEvent* _event)
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
        if (_event == QKeySequence::Cut) {
            d->model->saveChanges();
        }
    }

    return isEventHandled;
}

bool TitlePageEdit::canInsertFromMimeData(const QMimeData* _source) const
{
    return _source->formats().contains(d->model->mimeTypes().first()) || _source->hasText();
}

QMimeData* TitlePageEdit::createMimeDataFromSelection() const
{
    if (!textCursor().hasSelection()) {
        return {};
    }

    QMimeData* mimeData = new QMimeData;
    TextCursor cursor = textCursor();
    const auto selection = cursor.selectionInterval();

    //
    // Сформируем в текстовом виде, для вставки наружу
    //
    {
        QByteArray text;
        auto cursor = textCursor();
        cursor.setPosition(selection.from);
        do {
            cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
            if (cursor.position() > selection.to) {
                cursor.setPosition(selection.to, QTextCursor::KeepAnchor);
            }
            if (!text.isEmpty()) {
                text.append("\r\n");
            }
            text.append(cursor.blockCharFormat().fontCapitalization() == QFont::AllUppercase
                            ? TextHelper::smartToUpper(cursor.selectedText()).toUtf8()
                            : cursor.selectedText().toUtf8());
        } while (cursor.position() < textCursor().selectionEnd() && !cursor.atEnd()
                 && cursor.movePosition(QTextCursor::NextBlock));

        mimeData->setData("text/plain", text);
    }

    //
    // Добавим markdown
    //
    {
        //
        // Подготавливаем опции для экспорта в markdown
        //
        BusinessLayer::ExportOptions options;
        options.filePath = QDir::temp().absoluteFilePath("clipboard.md");
        options.includeTitlePage = false;
        options.includeReviewMarks = false;
        //
        // ... сохраняем в формате markdown
        //
        BusinessLayer::SimpleTextMarkdownExporter().exportTo(d->model, selection.from, selection.to,
                                                             options);
        //
        // ... читаем сохранённый экспорт из файла
        //
        QFile file(options.filePath);
        QByteArray text;
        if (file.open(QIODevice::ReadOnly)) {
            text = file.readAll();
            file.close();
        }

        if (!text.isEmpty()) {
            mimeData->setData(kMarkdownMimeType, text);
        }
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

void TitlePageEdit::insertFromMimeData(const QMimeData* _source)
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
    // Если вставляются данные в сценарном формате
    //
    if (_source->formats().contains(d->model->mimeTypes().constFirst())) {
        textToInsert = _source->data(d->model->mimeTypes().constFirst());
    }
    //
    // Если простой текст, то вставляем его, импортировав из markdown
    //
    else if (_source->hasFormat(kMarkdownMimeType) || _source->hasText()) {
        const auto text = _source->hasFormat(kMarkdownMimeType) ? _source->data(kMarkdownMimeType)
                                                                : _source->text();

        BusinessLayer::SimpleTextMarkdownImporter markdownImporter;
        textToInsert = markdownImporter.importSimpleText(text).text;
    }

    //
    // Собственно вставка данных
    //
    auto cursorPosition = d->document.insertFromMime(textCursor().position(), textToInsert);

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

void TitlePageEdit::dropEvent(QDropEvent* _event)
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
