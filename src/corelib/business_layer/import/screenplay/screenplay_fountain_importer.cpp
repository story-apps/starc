#include "screenplay_fountain_importer.h"

#include "screenplay_import_options.h"

#include <business_layer/model/screenplay/text/screenplay_text_block_parser.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_text_item.h>
#include <business_layer/model/text/text_model_xml.h>
#include <business_layer/templates/screenplay_template.h>
#include <domain/document_object.h>
#include <utils/helpers/text_helper.h>

#include <QCoreApplication>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStack>
#include <QXmlStreamWriter>

#include <set>

#define _ZERO_ 0

namespace BusinessLayer {

ScreenplayFountainImporter::ScreenplayFountainImporter()
    : AbstractScreenplayImporter()
    , AbstractFountainImporter(true)
{
}

ScreenplayFountainImporter::~ScreenplayFountainImporter() = default;

AbstractScreenplayImporter::Documents ScreenplayFountainImporter::importDocuments(
    const ScreenplayImportOptions& _options) const
{
    QVector<QPair<TextParagraphType, QString>> paragraphs = parapraphsForDocuments(_options);
    std::set<QString> characterNames;
    std::set<QString> locationNames;
    for (const auto &paragraph : paragraphs) {
        switch (paragraph.first) {
        case TextParagraphType::SceneHeading: {
            if (!_options.importLocations) {
                break;
            }

            const auto locationName = ScreenplaySceneHeadingParser::location(paragraph.second);
            if (locationName.isEmpty()) {
                break;
            }

            Document location = { Domain::DocumentObjectType::Location, locationName, {}, {} };
            locationNames.emplace(locationName);
            break;
        }

        case TextParagraphType::Character: {
            if (!_options.importCharacters) {
                break;
            }

            const auto characterName = ScreenplayCharacterParser::name(paragraph.second);
            if (characterName.isEmpty()) {
                break;
            }

            Document character = { Domain::DocumentObjectType::Character, characterName, {}, {} };
            characterNames.emplace(characterName);
            break;
        }

        default:{
            break;
        }
        }

    }

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

QVector<AbstractScreenplayImporter::Screenplay> ScreenplayFountainImporter::importScreenplays(
    const ScreenplayImportOptions& _options) const
{
    if (_options.importText == false) {
        return {};
    }

    //
    // Открываем файл
    //
    QFile fountainFile(_options.filePath);
    if (!fountainFile.open(QIODevice::ReadOnly)) {
        return {};
    }

    //
    // Импортируем
    //
    auto screenplay = importScreenplay(fountainFile.readAll());
    if (screenplay.name.isEmpty()) {
        screenplay.name = QFileInfo(_options.filePath).completeBaseName();
    }

    return { screenplay };
}

AbstractScreenplayImporter::Screenplay ScreenplayFountainImporter::importScreenplay(
    const QString& _screenplayText) const
{
    if (_screenplayText.simplified().isEmpty()) {
        return {};
    }

    Screenplay result;
    result.text = documentText(_screenplayText);
    return result;
}

void ScreenplayFountainImporter::preprocessBlock(QVector<QString>& _paragraphs, QXmlStreamWriter& _writer) const
{
    //
    // TODO: этот метод сначала сделал в AbstractFountainImporter, потом перенес сюда -> нужно переделать все обращения в внутренней реализации
    //
//    const int paragraphsCount = _paragraphs.size();
//    QStack<QString> dirs;
//    auto prevBlockType = TextParagraphType::Undefined;
//    TextParagraphType blockType = TextParagraphType::Undefined;
//    for (int i = 0; i != paragraphsCount; ++i) {
//        QString paragraphText = _paragraphs[i];

//        if (d->isNotation || d->isCommenting) {
//            //
//            // Если мы комментируем или делаем заметку, то продолжим это
//            //
//            processBlock(paragraphText, prevBlockType, _writer);
//            continue;
//        }

//        if (paragraphText.isEmpty()) {
//            continue;
//        }

//        blockType = TextParagraphType::Action;

//        //
//        // Собираем типы выделения текста и очищаем текст от форматных символов
//        //
//        collectSelectionTypes(paragraphText);

//        switch (paragraphText[0].toLatin1()) {
//        case '.': {
//            blockType = TextParagraphType::SceneHeading;
//            //
//            // TODO: номера сцен игнорируем, поскольку в фонтане они являются строками
//            //
//            int sharpPos = paragraphText.size();
//            if (paragraphText.endsWith("#")) {
//                sharpPos = paragraphText.lastIndexOf('#', paragraphText.size() - 2);
//            }
//            if (sharpPos == -1) {
//                sharpPos = paragraphText.size();
//            }
//            paragraphText.truncate(sharpPos);
//            paragraphText.remove(0, 1);
//            movePreviousTypes(0, 1);
//            break;
//        }

//        case '!': {
//            blockType = TextParagraphType::Action;
//            paragraphText.remove(0, 1);
//            movePreviousTypes(0, 1);
//            break;
//        }

//        case '@': {
//            blockType = TextParagraphType::Character;
//            paragraphText.remove(0, 1);
//            movePreviousTypes(0, 1);
//            break;
//        }

//        case '>': {
//            if (paragraphText.endsWith("<")) {
//                blockType = TextParagraphType::Action;
//                paragraphText.chop(1);
//                paragraphText.remove(0, 1);
//                movePreviousTypes(0, 1);
//            } else {
//                blockType = TextParagraphType::Transition;
//                paragraphText.remove(0, 1);
//                movePreviousTypes(0, 1);
//            }
//            break;
//        }

//        case '=': {
//            bool isPageBreak = false;
//            if (paragraphText.startsWith("===")) {
//                isPageBreak = true;
//                for (int j = 3; j != paragraphText.size(); ++j) {
//                    if (paragraphText[j] != '=') {
//                        isPageBreak = false;
//                        break;
//                    }
//                }

//                //
//                // Если состоит из трех или более '=', то это PageBreak
//                // TODO: У нас такого сейчас нет
//                //
//                continue;
//            }
//            if (!isPageBreak) {
//                blockType = TextParagraphType::BeatHeading;
//                paragraphText.remove(0, 1);
//                movePreviousTypes(0, 1);
//            }
//            break;
//        }

//        case '~': {
//            //
//            // Лирика
//            //
//            blockType = TextParagraphType::Lyrics;
//            paragraphText.remove(0, 1);
//            movePreviousTypes(0, 1);
//            break;
//        }

//        case '#': {
//            //
//            // Директории
//            //
//            int sharpCount = 0;
//            while (paragraphText[sharpCount] == '#') {
//                ++sharpCount;
//            }

//            if (sharpCount <= dirs.size()) {
//                //
//                // Закроем нужное число раз уже открытые
//                //
//                unsigned toClose = dirs.size() - sharpCount + 1;
//                for (unsigned i = 0; i != toClose; ++i) {
//                    processBlock({}, TextParagraphType::SequenceFooter, _writer);
//                    dirs.pop();
//                }
//            }
//            //
//            // И откроем новую
//            //
//            QString text = paragraphText.mid(sharpCount);
//            processBlock(text, TextParagraphType::SequenceHeading, _writer);
//            dirs.push(text);
//            prevBlockType = TextParagraphType::SequenceHeading;

//            //
//            // Поскольку директории добавляются прямо здесь без обработки, то в конец цикла идти не
//            // надо
//            //
//            continue;
//        }

//        default: {
//            bool startsWithHeading = false;
//            for (const QString& sceneHeading : sceneHeadingsDictionary()) {
//                if (paragraphText.startsWith(sceneHeading)) {
//                    startsWithHeading = true;
//                    break;
//                }
//            }

//            if (startsWithHeading && i + 1 < paragraphsCount && _paragraphs[i + 1].isEmpty()) {
//                //
//                // Если начинается с одного из времен действия, а после обязательно пустая строка
//                // Значит это заголовок сцены
//                //
//                blockType = TextParagraphType::SceneHeading;

//                //
//                // TODO: номера сцен игнорируем, поскольку в фонтане они являются строками
//                //
//                int sharpPos = paragraphText.size();
//                if (paragraphText.endsWith("#")) {
//                    sharpPos = paragraphText.lastIndexOf('#', paragraphText.size() - 2);
//                }
//                if (sharpPos == -1) {
//                    sharpPos = paragraphText.size();
//                }
//                paragraphText = paragraphText.left(sharpPos);
//            } else if (paragraphText.startsWith("[[") && paragraphText.endsWith("]]")) {
//                //
//                // Редакторская заметка
//                //
//                paragraphText.chop(2);
//                paragraphText.remove(0, 2);
//                movePreviousTypes(0, 2);
//                d->notes.append(std::make_tuple(paragraphText, d->noteStartPos, d->noteLen));
//                d->noteStartPos += d->noteLen;
//                d->noteLen = 0;
//                continue;
//            } else if (paragraphText.startsWith("/*")) {
//                //
//                // Начинается комментарий
//                //
//            } else if (paragraphText == TextHelper::smartToUpper(paragraphText) && i != 0
//                       && _paragraphs[i - 1].isEmpty() && i + 1 < paragraphsCount
//                       && _paragraphs[i + 1].isEmpty() && paragraphText.endsWith("TO:")) {
//                //
//                // Если состоит только из заглавных букв, предыдущая и следующая строки пустые
//                // и заканчивается "TO:", то это переход
//                //
//                blockType = TextParagraphType::Transition;
//            } else if (paragraphText.startsWith("(") && paragraphText.endsWith(")")
//                       && (prevBlockType == TextParagraphType::Character
//                           || prevBlockType == TextParagraphType::Dialogue)) {
//                //
//                // Если текущий блок обернут в (), то это ремарка
//                //
//                blockType = TextParagraphType::Parenthetical;
//                paragraphText.chop(1);
//                paragraphText.remove(0, 1);
//                movePreviousTypes(0, 1);
//            } else if (paragraphText == TextHelper::smartToUpper(paragraphText) && i != 0
//                       && _paragraphs[i - 1].isEmpty() && i + 1 < paragraphsCount
//                       && !_paragraphs[i + 1].isEmpty()) {
//                //
//                // Если состоит из только из заглавных букв, впереди не пустая строка, а перед
//                // пустая Значит это имя персонажа (для реплики)
//                //
//                blockType = TextParagraphType::Character;
//                if (paragraphText.endsWith("^")) {
//                    //
//                    // Двойной диалог, который мы пока что не умеем обрабатывать
//                    //
//                    paragraphText.chop(1);
//                }
//            } else if (prevBlockType == TextParagraphType::Character
//                       || prevBlockType == TextParagraphType::Parenthetical
//                       || (prevBlockType == TextParagraphType::Dialogue && i > 0
//                           && !_paragraphs[i - 1].isEmpty())) {
//                //
//                // Если предыдущий блок - имя персонажа или ремарка, то сейчас диалог
//                // Или предыдущая строка является диалогом
//                //
//                blockType = TextParagraphType::Dialogue;
//            } else {
//                //
//                // Во всех остальных случаях - Action
//                //
//                blockType = TextParagraphType::Action;
//            }
//        }
//        }
//        //
//        // Отправим блок на обработку
//        //
//        processBlock(paragraphText, blockType, _writer);
//        prevBlockType = blockType;
//    }
//    //
//    // Добавим комментарии к последнему блоку
//    //
//    d->appendComments(_writer);

//    //
//    // Закроем последний блок
//    //
//    _writer.writeEndElement();

//    //
//    // Закроем директории нужное число раз
//    //
//    while (!dirs.empty()) {
//        processBlock({}, TextParagraphType::SequenceFooter, _writer);
//        dirs.pop();
//    }
}


} // namespace BusinessLayer
