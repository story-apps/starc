#include "screenplay_document_importer.h"

#include "screenlay_import_options.h"

#include <business_layer/model/screenplay/text/screenplay_text_block_parser.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_xml.h>
#include <business_layer/templates/screenplay_template.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <domain/document_object.h>
#include <utils/helpers/text_helper.h>

#include <QDateTime>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QXmlStreamWriter>

#include <format_helpers.h>
#include <format_manager.h>
#include <format_reader.h>
#include <set>


namespace BusinessLayer {

namespace {

/**
 * @brief Регулярное выражение для определения блока "Время и место" по наличию слов места
 */
const QRegularExpression kPlaceContainsChecker("(^|[^\\S])(INT|EXT|ИНТ|НАТ|ПАВ|ЭКСТ)([.]|[ - ])");
/**
 * @brief Регулярное выражение для определения блока "Титр" по наличию ключевых слов
 */
const QRegularExpression kTitleChecker("(^|[^\\S])(TITLE|ТИТР)([:] )");

/**
 * @brief Регулярное выражение для определения блока "Время и место" по началу с номера
 */
const QRegularExpression kStartFromNumberChecker(
    "^([\\d]{1,}[\\d\\S]{0,})([.]|[-])(([\\d\\S]{1,})([.]|)|) ");

/**
 * @brief Допущение для блоков, которые по идее вообще не должны иметь отступа в пикселях (16 мм)
 */
const int kLeftMarginDelta = 60;

/**
 * @brief Некоторые программы выравнивают текст при помощи пробелов
 */
const QString kOldSchoolCenteringPrefix = "                    ";

/**
 * @brief Определить тип блока в текущей позиции курсора
 *		  с указанием предыдущего типа и количества предшествующих пустых строк
 */
ScreenplayParagraphType typeForTextCursor(const QTextCursor& _cursor,
                                          ScreenplayParagraphType _lastBlockType,
                                          int _prevEmptyLines, int _minLeftMargin)
{
    //
    // Определим текст блока
    //
    const QString blockText = _cursor.block().text();
    const QString blockTextUppercase = TextHelper::smartToUpper(blockText);

    //
    // Для всех нераспознаных блоков ставим тип "Описание действия"
    //
    ScreenplayParagraphType blockType = ScreenplayParagraphType::Action;

    //
    // Определим некоторые характеристики исследуемого текста
    //
    // ... стили блока
    const QTextBlockFormat blockFormat = _cursor.blockFormat();
    const QTextCharFormat charFormat = _cursor.charFormat();
    // ... текст в верхнем регистре (FIXME: такие строки, как "Я.")
    bool textIsUppercase = charFormat.fontCapitalization() == QFont::AllUppercase
        || blockText == TextHelper::smartToUpper(blockText);
    // ... блоки находящиеся в центре
    bool isCentered = !blockFormat.alignment().testFlag(Qt::AlignRight)
        && (((blockFormat.leftMargin() + blockFormat.indent()) > kLeftMarginDelta + _minLeftMargin)
            || (blockFormat.alignment() == Qt::AlignCenter)
            || blockText.startsWith(kOldSchoolCenteringPrefix));

    //
    // Собственно определение типа
    //
    {
        //
        // Блоки текста посередине
        //
        if (isCentered) {
            //
            // Персонаж
            // 1. В верхнем регистре
            //
            if (textIsUppercase && _lastBlockType != ScreenplayParagraphType::Character) {
                blockType = ScreenplayParagraphType::Character;
            }
            //
            // Ремарка
            // 1. начинается со скобки
            //
            else if (blockText.startsWith("(")) {
                blockType = ScreenplayParagraphType::Parenthetical;
            }
            //
            // Реплика
            // 1. всё что осталось
            //
            else {
                blockType = ScreenplayParagraphType::Dialogue;
            }

        }
        //
        // Не посередине
        //
        else {
            //
            // Блоки текста в верхнем регистре
            //
            if (textIsUppercase) {
                //
                // Время и место
                // 1. текст в верхнем регистре
                // 2. содержит ключевые сокращения места действия или начинается с номера сцены
                //
                if (blockTextUppercase.contains(kPlaceContainsChecker)
                    || blockTextUppercase.contains(kStartFromNumberChecker)) {
                    blockType = ScreenplayParagraphType::SceneHeading;
                }
                //
                // Участника сцены
                // 1. в верхнем регистре
                // 2. идут сразу же после времени и места
                // 3. не имеют сверху отступа
                //
                else if (_lastBlockType == ScreenplayParagraphType::SceneHeading
                         && _prevEmptyLines == 0 && blockFormat.topMargin() == 0) {
                    blockType = ScreenplayParagraphType::SceneCharacters;
                }
                //
                // Примечание
                // 1. всё что осталось и не имеет отступов
                // 2. выровнено по левому краю
                //
                else if (blockFormat.alignment().testFlag(Qt::AlignLeft) && !isCentered) {
                    blockType = ScreenplayParagraphType::UnformattedText;
                }
                //
                // Переход
                // 1. всё что осталось и выровнено по правому краю
                //
                else if (blockFormat.alignment().testFlag(Qt::AlignRight)) {
                    blockType = ScreenplayParagraphType::Transition;
                }
            }
        }

        //
        // Отдельные проверки для блоков, которые могут быть в разных регистрах и в разных местах
        // страницы
        //
        // Титр (формируем, как описание действия)
        // 1. начинается со слова ТИТР:
        //
        if (blockTextUppercase.contains(kTitleChecker)) {
            blockType = ScreenplayParagraphType::Action;
        }
    }

    return blockType;
}

/**
 * @brief Шум, который может встречаться в тексте
 */
const QString NOISE("([.]|[,]|[:]|[ ]|[-]){1,}");

/**
 * @brief Регулярное выражение для удаления мусора в начале текста
 */
const QRegularExpression NOISE_AT_START("^" + NOISE);

/**
 * @brief Регулярное выражение для удаления мусора в конце текста
 */
const QRegularExpression NOISE_AT_END(NOISE + "$");

/**
 * @brief Очистка блоков от мусора и их корректировки
 */
static QString clearBlockText(ScreenplayParagraphType _blockType, const QString& _blockText)
{
    QString result = _blockText;

    //
    // Удаляем длинные тире
    //
    result = result.replace("–", "-");

    //
    // Для блока времени и места:
    // * всевозможные "инт - " меняем на "инт. "
    // * убираем точки в конце названия локации
    //
    if (_blockType == ScreenplayParagraphType::SceneHeading) {
        const QString location = ScreenplaySceneHeadingParser::location(_blockText);
        QString clearLocation = location.simplified();
        clearLocation.remove(NOISE_AT_START);
        clearLocation.remove(NOISE_AT_END);
        if (location != clearLocation) {
            result = result.replace(location, clearLocation);
        }
    }
    //
    // Для персонажей
    // * убираем точки в конце
    //
    else if (_blockType == ScreenplayParagraphType::Character) {
        const QString name = ScreenplayCharacterParser::name(_blockText);
        QString clearName = name.simplified();
        clearName.remove(NOISE_AT_END);
        if (name != clearName) {
            result = result.replace(name, clearName);
        }
    }
    //
    // Ремарка
    // * убираем скобки
    //
    else if (_blockType == ScreenplayParagraphType::Parenthetical) {
        QString clearParenthetical = _blockText.simplified();
        if (!clearParenthetical.isEmpty() && clearParenthetical.front() == '(') {
            clearParenthetical.remove(0, 1);
        }
        if (!clearParenthetical.isEmpty() && clearParenthetical.back() == ')') {
            clearParenthetical.chop(1);
        }
        result = clearParenthetical;
    }

    return result;
}

} // namespace

ScreenplayAbstractImporter::Documents ScreenplayDocumentImporter::importDocuments(
    const ScreenplayImportOptions& _options) const
{
    //
    // Открываем файл
    //
    QFile documentFile(_options.filePath);
    if (!documentFile.open(QIODevice::ReadOnly)) {
        return {};
    }

    //
    // Преобразовать заданный документ в QTextDocument
    //
    QTextDocument documentForImport;
    {
        QScopedPointer<FormatReader> reader(FormatManager::createReader(&documentFile));
        reader->read(&documentFile, &documentForImport);
    }

    //
    // Найти минимальный отступ слева для всех блоков
    // ЗАЧЕМ: во многих программах (Final Draft, Screeviner) сделано так, что поля
    //		  задаются за счёт оступов. Получается что и заглавие сцены и описание действия
    //		  имеют отступы. Так вот это и будет минимальным отступом, который не будем считать
    //
    int minLeftMargin = 1000;
    {
        QTextCursor cursor(&documentForImport);
        while (!cursor.atEnd()) {
            if (minLeftMargin > cursor.blockFormat().leftMargin()) {
                minLeftMargin = std::max(0.0, cursor.blockFormat().leftMargin());
            }

            cursor.movePosition(QTextCursor::NextBlock);
            cursor.movePosition(QTextCursor::EndOfBlock);
        }
    }

    QTextCursor cursor(&documentForImport);

    //
    // Для каждого блока текста определяем тип
    //
    // ... последний стиль блока
    auto lastBlockType = ScreenplayParagraphType::Undefined;
    // ... количество пустых строк
    int emptyLines = 0;
    std::set<QString> characterNames;
    std::set<QString> locationNames;
    do {
        cursor.movePosition(QTextCursor::EndOfBlock);

        //
        // Если в блоке есть текст
        //
        if (!cursor.block().text().simplified().isEmpty()) {
            //
            // ... определяем тип
            //
            const auto blockType
                = typeForTextCursor(cursor, lastBlockType, emptyLines, minLeftMargin);
            QString paragraphText = cursor.block().text().simplified();

            //
            // Если текущий тип "Время и место", то удалим номер сцены
            //
            if (blockType == ScreenplayParagraphType::SceneHeading) {
                paragraphText = TextHelper::smartToUpper(paragraphText);
                const auto match = kStartFromNumberChecker.match(paragraphText);
                if (match.hasMatch()) {
                    paragraphText = paragraphText.mid(match.capturedEnd());
                }
            }

            //
            // Выполняем корректировки
            //
            paragraphText = clearBlockText(blockType, paragraphText);

            switch (blockType) {
            case ScreenplayParagraphType::SceneHeading: {
                if (!_options.importLocations) {
                    break;
                }

                const auto locationName = ScreenplaySceneHeadingParser::location(paragraphText);
                if (locationName.isEmpty()) {
                    break;
                }

                locationNames.emplace(locationName);
                break;
            }

            case ScreenplayParagraphType::Character: {
                if (!_options.importCharacters) {
                    break;
                }

                const auto characterName = ScreenplayCharacterParser::name(paragraphText);
                if (characterName.isEmpty()) {
                    break;
                }

                characterNames.emplace(characterName);
                break;
            }

            default:
                break;
            }

            //
            // Запомним последний стиль блока и обнулим счётчик пустых строк
            //
            lastBlockType = blockType;
            emptyLines = 0;
        }
        //
        // Если в блоке нет текста, то увеличиваем счётчик пустых строк
        //
        else {
            ++emptyLines;
        }

        cursor.movePosition(QTextCursor::NextCharacter);
    } while (!cursor.atEnd());

    Documents documents;
    for (const auto& characterName : characterNames) {
        documents.characters.append({ characterName, {} });
    }
    for (const auto& locationName : locationNames) {
        documents.locations.append({ locationName, {} });
    }
    return documents;
}

QVector<ScreenplayAbstractImporter::Screenplay> ScreenplayDocumentImporter::importScreenplays(
    const ScreenplayImportOptions& _options) const
{
    if (_options.importScreenplay == false) {
        return {};
    }

    Screenplay result;
    result.name = QFileInfo(_options.filePath).completeBaseName();

    //
    // Открываем файл
    //
    QFile documentFile(_options.filePath);
    if (!documentFile.open(QIODevice::ReadOnly)) {
        return { result };
    }

    //
    // Преобразовать заданный документ в QTextDocument
    //
    QTextDocument documentForImport;
    {
        QScopedPointer<FormatReader> reader(FormatManager::createReader(&documentFile));
        reader->read(&documentFile, &documentForImport);
    }

    //
    // Найти минимальный отступ слева для всех блоков
    // ЗАЧЕМ: во многих программах (Final Draft, Screeviner) сделано так, что поля
    //		  задаются за счёт оступов. Получается что и заглавие сцены и описание действия
    //		  имеют отступы. Так вот это и будет минимальным отступом, который не будем считать
    //
    int minLeftMargin = 1000;
    {
        QTextCursor cursor(&documentForImport);
        while (!cursor.atEnd()) {
            if (minLeftMargin > cursor.blockFormat().leftMargin()) {
                minLeftMargin = cursor.blockFormat().leftMargin();
            }

            cursor.movePosition(QTextCursor::NextBlock);
            cursor.movePosition(QTextCursor::EndOfBlock);
        }
    }

    //
    // Преобразовать его в xml-строку
    //
    QTextCursor cursor(&documentForImport);

    QXmlStreamWriter writer(&result.text);
    writer.writeStartDocument();
    writer.writeStartElement(xml::kDocumentTag);
    writer.writeAttribute(xml::kMimeTypeAttribute,
                          Domain::mimeTypeFor(Domain::DocumentObjectType::ScreenplayText));
    writer.writeAttribute(xml::kVersionAttribute, "1.0");

    //
    // Для каждого блока текста определяем тип
    //
    // ... последний стиль блока
    auto lastBlockType = ScreenplayParagraphType::Undefined;
    // ... количество пустых строк
    int emptyLines = 0;
    bool alreadyInScene = false;
    do {
        cursor.movePosition(QTextCursor::EndOfBlock);

        //
        // Если в блоке есть текст
        //
        if (!cursor.block().text().simplified().isEmpty()) {
            //
            // ... определяем тип
            //
            const auto blockType
                = typeForTextCursor(cursor, lastBlockType, emptyLines, minLeftMargin);
            //
            // Если текущий тип "Время и место", то удалим номер сцены
            //
            if (blockType == ScreenplayParagraphType::SceneHeading && !_options.keepSceneNumbers) {
                const auto match
                    = kStartFromNumberChecker.match(cursor.block().text().simplified());
                if (match.hasMatch()) {
                    cursor.movePosition(QTextCursor::StartOfBlock);
                    cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor,
                                        match.capturedEnd());
                    if (cursor.hasSelection()) {
                        cursor.deleteChar();
                    }
                    cursor.movePosition(QTextCursor::EndOfBlock);
                }
            }

            //
            // Выполняем корректировки
            //
            const auto paragraphText
                = clearBlockText(blockType, cursor.block().text().simplified());

            //
            // Формируем блок сценария
            //
            if (blockType == ScreenplayParagraphType::SceneHeading) {
                if (alreadyInScene) {
                    writer.writeEndElement(); // контент предыдущей сцены
                    writer.writeEndElement(); // предыдущая сцена
                }
                alreadyInScene = true;

                writer.writeStartElement(xml::kSceneTag);
                writer.writeAttribute(xml::kUuidAttribute, QUuid::createUuid().toString());
                writer.writeStartElement(xml::kContentTag);
            }
            writer.writeStartElement(toString(blockType));
            writer.writeStartElement(xml::kValueTag);
            writer.writeCDATA(TextHelper::toHtmlEscaped(paragraphText));
            writer.writeEndElement(); // value
            //
            // Пишем редакторские комментарии
            //
            {
                const QTextBlock currentBlock = cursor.block();
                if (!currentBlock.textFormats().isEmpty()) {
                    writer.writeStartElement(xml::kReviewMarksTag);
                    for (const auto& range : currentBlock.textFormats()) {
                        if (range.format.boolProperty(Docx::IsForeground)
                            || range.format.boolProperty(Docx::IsBackground)
                            || range.format.boolProperty(Docx::IsHighlight)
                            || range.format.boolProperty(Docx::IsComment)) {
                            writer.writeStartElement(xml::kReviewMarkTag);
                            writer.writeAttribute(xml::kFromAttribute,
                                                  QString::number(range.start));
                            writer.writeAttribute(xml::kLengthAttribute,
                                                  QString::number(range.length));
                            if (range.format.hasProperty(QTextFormat::ForegroundBrush)) {
                                writer.writeAttribute(xml::kColorAttribute,
                                                      range.format.foreground().color().name());
                            }
                            if (range.format.hasProperty(QTextFormat::BackgroundBrush)) {
                                writer.writeAttribute(xml::kBackgroundColorAttribute,
                                                      range.format.background().color().name());
                            }
                            //
                            // ... комментарии
                            //
                            QStringList authors
                                = range.format.property(Docx::CommentsAuthors).toStringList();
                            if (authors.isEmpty()) {
                                authors.append(DataStorageLayer::StorageFacade::settingsStorage()
                                                   ->accountName());
                            }
                            QStringList dates
                                = range.format.property(Docx::CommentsDates).toStringList();
                            if (dates.isEmpty()) {
                                dates.append(QDateTime::currentDateTime().toString(Qt::ISODate));
                            }
                            QStringList comments
                                = range.format.property(Docx::Comments).toStringList();
                            if (comments.isEmpty()) {
                                comments.append(QString());
                            }
                            for (int commentIndex = 0; commentIndex < comments.size();
                                 ++commentIndex) {
                                writer.writeStartElement(xml::kCommentTag);
                                writer.writeAttribute(xml::kAuthorAttribute,
                                                      authors.at(commentIndex));
                                writer.writeAttribute(xml::kDateAttribute, dates.at(commentIndex));
                                writer.writeCDATA(
                                    TextHelper::toHtmlEscaped(comments.at(commentIndex)));
                                writer.writeEndElement(); // comment
                            }
                            //
                            writer.writeEndElement(); // review mark
                        }
                    }
                    writer.writeEndElement(); // review marks
                }
            }

            //
            // Пишем форматирование
            //
            {
                const QTextBlock currentBlock = cursor.block();
                if (!currentBlock.textFormats().isEmpty()) {
                    writer.writeStartElement(xml::kFormatsTag);
                    for (const auto& range : currentBlock.textFormats()) {
                        if (range.format.fontWeight() != QFont::Normal || range.format.fontItalic()
                            || range.format.fontUnderline()) {
                            writer.writeEmptyElement(xml::kFormatTag);
                            writer.writeAttribute(xml::kFromAttribute,
                                                  QString::number(range.start));
                            writer.writeAttribute(xml::kLengthAttribute,
                                                  QString::number(range.length));
                            if (range.format.fontWeight() != QFont::Normal) {
                                writer.writeAttribute(xml::kBoldAttribute, "true");
                            }
                            if (range.format.boolProperty(QTextFormat::FontItalic)) {
                                writer.writeAttribute(xml::kItalicAttribute, "true");
                            }
                            if (range.format.boolProperty(QTextFormat::TextUnderlineStyle)) {
                                writer.writeAttribute(xml::kUnderlineAttribute, "true");
                            }
                            if (range.format.boolProperty(QTextFormat::FontStrikeOut)) {
                                writer.writeAttribute(xml::kStrikethroughAttribute, "true");
                            }
                        }
                    }
                    writer.writeEndElement();
                }
            }
            writer.writeEndElement(); // block type

            //
            // Запомним последний стиль блока и обнулим счётчик пустых строк
            //
            lastBlockType = blockType;
            emptyLines = 0;
        }
        //
        // Если в блоке нет текста, то увеличиваем счётчик пустых строк
        //
        else {
            ++emptyLines;
        }

        cursor.movePosition(QTextCursor::NextCharacter);
    } while (!cursor.atEnd());

    writer.writeEndDocument();

    return { result };
}

} // namespace BusinessLayer
