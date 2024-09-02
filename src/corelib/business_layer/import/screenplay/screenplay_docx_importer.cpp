#include "screenplay_docx_importer.h"

#include "screenplay_import_options.h"

#include <business_layer/model/screenplay/text/screenplay_text_block_parser.h>
#include <business_layer/model/text/text_model_xml.h>
#include <business_layer/templates/screenplay_template.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <utils/helpers/text_helper.h>

#include <QFileInfo>
#include <QRegularExpression>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>

#include <format_helpers.h>
#include <format_manager.h>
#include <format_reader.h>
#include <set>


namespace BusinessLayer {

namespace {

/**
 * @brief Регулярное выражение для определения блока "Время и место" по началу с номера
 */
const QRegularExpression kStartFromNumberChecker(
    "^([\\d]{1,}[\\d\\S]{0,})([.]|[-])(([\\d\\S]{1,})([.]|)|) ");

} // namespace

AbstractScreenplayImporter::Documents ScreenplayDocxImporter::importDocuments(
    const ImportOptions& _options) const
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
    auto lastBlockType = TextParagraphType::Undefined;
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
            if (blockType == TextParagraphType::SceneHeading) {
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
            case TextParagraphType::SceneHeading: {
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

            case TextParagraphType::Character: {
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
        documents.characters.append(
            { Domain::DocumentObjectType::Character, characterName, {}, {} });
    }
    for (const auto& locationName : locationNames) {
        documents.locations.append({ Domain::DocumentObjectType::Location, locationName, {}, {} });
    }
    return documents;
}

QVector<AbstractScreenplayImporter::Screenplay> ScreenplayDocxImporter::importScreenplays(
    const ScreenplayImportOptions& _options) const
{
    Screenplay screenplay;
    screenplay.name = QFileInfo(_options.filePath).completeBaseName();

    if (_options.importText == false) {
        return { screenplay };
    }

    //
    // Открываем файл
    //
    QFile documentFile(_options.filePath);
    if (!documentFile.open(QIODevice::ReadOnly)) {
        return { screenplay };
    }

    //
    // Преобразовать заданный документ в QTextDocument
    //
    QTextDocument documentForImport;
    {
        QScopedPointer<FormatReader> reader(FormatManager::createReader(&documentFile));
        reader->read(&documentFile, &documentForImport);
    }

    screenplay.text = parseDocument(_options, documentForImport);

    return { screenplay };
}

QString ScreenplayDocxImporter::extractSceneNumber(const ImportOptions& _options,
                                                   QTextCursor& _cursor) const
{
    QString sceneNumber;
    const auto match = kStartFromNumberChecker.match(_cursor.block().text().simplified());
    if (match.hasMatch()) {
        _cursor.movePosition(QTextCursor::StartOfBlock);
        _cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor,
                             match.capturedEnd());
        if (_cursor.hasSelection()) {
            const auto& options = static_cast<const ScreenplayImportOptions&>(_options);
            if (options.keepSceneNumbers) {
                sceneNumber = _cursor.selectedText().trimmed();
                if (sceneNumber.endsWith('.')) {
                    sceneNumber.chop(1);
                }
            }
            _cursor.deleteChar();
        }
        _cursor.movePosition(QTextCursor::EndOfBlock);
    }
    return sceneNumber;
}

void ScreenplayDocxImporter::writeReviewMarks(QXmlStreamWriter& _writer, QTextCursor& _cursor) const
{
    const QTextBlock currentBlock = _cursor.block();
    if (!currentBlock.textFormats().isEmpty()) {
        _writer.writeStartElement(xml::kReviewMarksTag);
        for (const auto& range : currentBlock.textFormats()) {
            if (range.format.boolProperty(Docx::IsForeground)
                || range.format.boolProperty(Docx::IsBackground)
                || range.format.boolProperty(Docx::IsHighlight)
                || range.format.boolProperty(Docx::IsComment)) {
                _writer.writeStartElement(xml::kReviewMarkTag);
                _writer.writeAttribute(xml::kFromAttribute, QString::number(range.start));
                _writer.writeAttribute(xml::kLengthAttribute, QString::number(range.length));
                if (range.format.hasProperty(QTextFormat::ForegroundBrush)) {
                    _writer.writeAttribute(xml::kColorAttribute,
                                           range.format.foreground().color().name());
                }
                if (range.format.hasProperty(QTextFormat::BackgroundBrush)) {
                    _writer.writeAttribute(xml::kBackgroundColorAttribute,
                                           range.format.background().color().name());
                }
                //
                // ... комментарии
                //
                QStringList authors = range.format.property(Docx::CommentsAuthors).toStringList();
                if (authors.isEmpty()) {
                    authors.append(
                        DataStorageLayer::StorageFacade::settingsStorage()->accountName());
                }
                QStringList dates = range.format.property(Docx::CommentsDates).toStringList();
                if (dates.isEmpty()) {
                    dates.append(QDateTime::currentDateTime().toString(Qt::ISODate));
                }
                QStringList comments = range.format.property(Docx::Comments).toStringList();
                if (comments.isEmpty()) {
                    comments.append(QString());
                }
                for (int commentIndex = 0; commentIndex < comments.size(); ++commentIndex) {
                    _writer.writeStartElement(xml::kCommentTag);
                    _writer.writeAttribute(xml::kAuthorAttribute, authors.at(commentIndex));
                    _writer.writeAttribute(xml::kDateAttribute, dates.at(commentIndex));
                    _writer.writeCDATA(TextHelper::toHtmlEscaped(comments.at(commentIndex)));
                    _writer.writeEndElement(); // comment
                }
                //
                _writer.writeEndElement(); // review mark
            }
        }
        _writer.writeEndElement(); // review marks
    }
}

} // namespace BusinessLayer
