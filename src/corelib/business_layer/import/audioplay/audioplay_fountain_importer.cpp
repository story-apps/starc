#include "audioplay_fountain_importer.h"

#include "audioplay_import_options.h"

#include <business_layer/model/audioplay/text/audioplay_text_model_text_item.h>
#include <business_layer/model/text/text_model_xml.h>
#include <business_layer/templates/audioplay_template.h>
#include <domain/document_object.h>
#include <utils/helpers/text_helper.h>

#include <QCoreApplication>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QStack>
#include <QXmlStreamWriter>

#include <set>

namespace BusinessLayer {

class AudioplayFountainImporter::Implementation
{
public:
    /**
     * @brief Зашли ли мы уже в сцену
     */
    bool alreadyInScene = false;

    /**
     * @brief Закрыт ли разделитель
     */
    bool splitterIsOpen = false;
};

AudioplayFountainImporter::AudioplayFountainImporter()
    : AbstractAudioplayImporter()
    , AbstractFountainImporter()
    , d(new Implementation)
{
}

AudioplayFountainImporter::~AudioplayFountainImporter() = default;

AbstractAudioplayImporter::Documents AudioplayFountainImporter::importDocuments(
    const AudioplayImportOptions& _options) const
{
    QVector<QPair<TextParagraphType, QString>> paragraphs = parapraphsForDocuments(_options);
    std::set<QString> characterNames;
    for (const auto& paragraph : paragraphs) {
        switch (paragraph.first) {
        case TextParagraphType::Character: {
            if (!_options.importCharacters) {
                break;
            }

            const auto characterName = paragraph.second;
            if (characterName.isEmpty()) {
                break;
            }

            characterNames.emplace(characterName);
            break;
        }

        default: {
            break;
        }
        }
    }

    Documents documents;
    for (const auto& characterName : characterNames) {
        documents.characters.append({ characterName, {} });
    }
    return documents;
}

QVector<AbstractAudioplayImporter::Audioplay> AudioplayFountainImporter::importAudioplays(
    const AudioplayImportOptions& _options) const
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
    auto audioplay = importAudioplay(fountainFile.readAll());
    if (audioplay.name.isEmpty()) {
        audioplay.name = QFileInfo(_options.filePath).completeBaseName();
    }

    return { audioplay };
}

AbstractAudioplayImporter::Audioplay AudioplayFountainImporter::importAudioplay(
    const QString& _audioplayText) const
{
    if (_audioplayText.simplified().isEmpty()) {
        return {};
    }

    Audioplay result;
    result.text = documentText(_audioplayText);
    return result;
}

QStack<QString> AudioplayFountainImporter::processBlocks(QVector<QString>& _paragraphs,
                                                         QXmlStreamWriter& _writer) const
{
    const int paragraphsCount = _paragraphs.size();
    QStack<QString> dirs;
    auto prevBlockType = TextParagraphType::Undefined;
    TextParagraphType blockType = TextParagraphType::Undefined;
    for (int i = 0; i != paragraphsCount; ++i) {
        QString paragraphText = _paragraphs[i];

        if (isNotation() || isCommenting()) {
            //
            // Если мы комментируем или делаем заметку, то продолжим это
            //
            processNotes(paragraphText, prevBlockType, _writer);
            continue;
        }

        if (paragraphText.isEmpty()) {
            continue;
        }

        blockType = TextParagraphType::UnformattedText;

        //
        // Собираем типы выделения текста и очищаем текст от форматных символов
        //
        collectSelectionTypes(paragraphText);

        switch (paragraphText[0].toLatin1()) {
        case '.': {
            blockType = TextParagraphType::SceneHeading;
            //
            // TODO: номера сцен игнорируем, поскольку в фонтане они являются строками
            //
            int sharpPos = paragraphText.size();
            if (paragraphText.endsWith("#")) {
                sharpPos = paragraphText.lastIndexOf('#', paragraphText.size() - 2);
            }
            if (sharpPos == -1) {
                sharpPos = paragraphText.size();
            }
            paragraphText.truncate(sharpPos);
            paragraphText.remove(0, 1);
            movePreviousTypes(0, 1);
            break;
        }

        case '@': {
            blockType = TextParagraphType::Character;
            paragraphText.remove(0, 1);
            movePreviousTypes(0, 1);
            break;
        }

        case '=': {
            //
            // Биты пропускаем (если один '='), а PageBreak (три и более '=') у нас сейчас нет
            //
            continue;
        }

        default: {
            bool startsWithHeading = false;
            for (const QString& sceneHeading : sceneHeadingsDictionary()) {
                if (paragraphText.startsWith(sceneHeading)) {
                    startsWithHeading = true;
                    break;
                }
            }

            if (startsWithHeading && i + 1 < paragraphsCount && _paragraphs[i + 1].isEmpty()) {
                //
                // Если начинается с одного из времен действия, а после обязательно пустая строка
                // Значит это заголовок сцены
                //
                blockType = TextParagraphType::SceneHeading;

                //
                // TODO: номера сцен игнорируем, поскольку в фонтане они являются строками
                //
                int sharpPos = paragraphText.size();
                if (paragraphText.endsWith("#")) {
                    sharpPos = paragraphText.lastIndexOf('#', paragraphText.size() - 2);
                }
                if (sharpPos == -1) {
                    sharpPos = paragraphText.size();
                }
                paragraphText = paragraphText.left(sharpPos);
            } else if (paragraphText.startsWith("[[") && paragraphText.endsWith("]]")) {
                //
                // Редакторская заметка
                //
                paragraphText.chop(2);
                paragraphText.remove(0, 2);
                movePreviousTypes(0, 2);
                appendNotes(paragraphText);
                continue;
            } else if (paragraphText.startsWith("/*")) {
                //
                // Начинается комментарий
                //
            } else if (paragraphText.startsWith("(") && paragraphText.endsWith(")")
                       && (prevBlockType == TextParagraphType::Character
                           || prevBlockType == TextParagraphType::Dialogue)) {
                //
                // Если текущий блок обернут в (), то это ремарка
                //
                blockType = TextParagraphType::Parenthetical;
                paragraphText.chop(1);
                paragraphText.remove(0, 1);
                movePreviousTypes(0, 1);
            } else if (paragraphText == TextHelper::smartToUpper(paragraphText) && i != 0
                       && _paragraphs[i - 1].isEmpty() && i + 1 < paragraphsCount
                       && !_paragraphs[i + 1].isEmpty()) {
                //
                // Если состоит из только из заглавных букв, впереди не пустая строка, а перед
                // пустая Значит это имя персонажа (для реплики)
                //
                blockType = TextParagraphType::Character;
                if (paragraphText.endsWith("^")) {
                    //
                    // Двойной диалог, который мы пока что не умеем обрабатывать
                    //
                    paragraphText.chop(1);
                }
            } else if (prevBlockType == TextParagraphType::Character
                       || prevBlockType == TextParagraphType::Parenthetical
                       || (prevBlockType == TextParagraphType::Dialogue && i > 0
                           && !_paragraphs[i - 1].isEmpty())) {
                //
                // Если предыдущий блок - имя персонажа или ремарка, то сейчас диалог
                // Или предыдущая строка является диалогом
                //
                blockType = TextParagraphType::Dialogue;
            } else {
                //
                // Во всех остальных случаях - простой текст
                //
                blockType = TextParagraphType::UnformattedText;
            }
        }
        }
        //
        // Отправим блок на обработку комментариев и редакторских заметок
        //
        processNotes(paragraphText, blockType, _writer);
        prevBlockType = blockType;
    }
    return dirs;
}

void AudioplayFountainImporter::appendBlock(const QString& _paragraphText, TextParagraphType _type,
                                            QXmlStreamWriter& _writer,
                                            bool _shouldClosePrevBlock) const
{
    if (_shouldClosePrevBlock) {
        _writer.writeEndElement();
        //
        // Нужно закрыть разделитель, если он октрыт и если текущий блок - не диалог
        //
        if (d->splitterIsOpen && _type != TextParagraphType::Dialogue) {
            _writer.writeEmptyElement(xml::kSplitterTag);
            _writer.writeAttribute(xml::kTypeAttribute, "end");
            d->splitterIsOpen = false;
        }
    }

    QString paragraphText = _paragraphText;
    while (!paragraphText.isEmpty() && paragraphText.startsWith(" ")) {
        paragraphText = paragraphText.mid(1);
    }

    auto writeValue = [&_writer, &paragraphText]() {
        _writer.writeStartElement(xml::kValueTag);
        _writer.writeCDATA(TextHelper::toHtmlEscaped(paragraphText));
        _writer.writeEndElement(); // value
    };

    //
    // Формируем блок аудиопьесы
    //
    switch (_type) {
    case TextParagraphType::SceneHeading: {
        if (d->alreadyInScene) {
            _writer.writeEndElement(); // контент предыдущей сцены
            _writer.writeEndElement(); // предыдущая сцена
        }

        d->alreadyInScene = true; // вошли в новую сцену

        _writer.writeStartElement(toString(TextGroupType::Scene));
        _writer.writeAttribute(xml::kUuidAttribute, QUuid::createUuid().toString());
        _writer.writeStartElement(xml::kContentTag);

        _writer.writeStartElement(toString(_type));
        writeValue();
        break;
    }

    case TextParagraphType::Character: {
        //
        // Если разделитель ещё не открыт, то откроем
        //
        if (!d->splitterIsOpen) {
            _writer.writeEmptyElement(xml::kSplitterTag);
            _writer.writeAttribute(xml::kTypeAttribute, "start");
            d->splitterIsOpen = true;
        }

        _writer.writeStartElement(toString(_type));
        _writer.writeEmptyElement(xml::kParametersTag);
        _writer.writeAttribute(xml::kInFirstColumnAttribute, "true");
        writeValue();
        break;
    }

    case TextParagraphType::Dialogue: {
        //
        // Если разделитель ещё не открыт, то откроем
        //
        if (!d->splitterIsOpen) {
            _writer.writeEmptyElement(xml::kSplitterTag);
            _writer.writeAttribute(xml::kTypeAttribute, "start");
            d->splitterIsOpen = true;
        }

        _writer.writeStartElement(toString(_type));
        _writer.writeEmptyElement(xml::kParametersTag);
        _writer.writeAttribute(xml::kInFirstColumnAttribute, "false");
        writeValue();

        break;
    }

    default: {
        _writer.writeStartElement(toString(_type));
        writeValue();
        break;
    }
    }

    //
    // Пишем форматирование, если оно есть
    //
    writeSelectionTypes(_writer);

    //
    // Запоминаем текст блока
    //
    setCurrentBlockTextLast();

    //
    // Не закрываем блок, чтобы можно было добавить редакторских заметок
    //
}

} // namespace BusinessLayer
