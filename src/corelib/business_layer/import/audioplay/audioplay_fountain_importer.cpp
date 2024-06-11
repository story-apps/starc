#include "audioplay_fountain_importer.h"

#include <business_layer/import/import_options.h>
#include <business_layer/model/audioplay/text/audioplay_text_block_parser.h>
#include <business_layer/model/text/text_model_xml.h>
#include <business_layer/templates/audioplay_template.h>
#include <business_layer/templates/templates_facade.h>
#include <utils/helpers/text_helper.h>

#include <QFileInfo>
#include <QSet>


namespace BusinessLayer {

namespace {

/**
 * @brief Типы блоков, которые могут быть в документе
 */
static const QSet<TextParagraphType> kPossibleBlockTypes = {
    TextParagraphType::SceneHeading, TextParagraphType::Character,
    TextParagraphType::Dialogue,     TextParagraphType::Sound,
    TextParagraphType::Music,        TextParagraphType::Cue,
    TextParagraphType::InlineNote,   TextParagraphType::UnformattedText,
};

/**
 * @brief Тип блока по умолчанию
 */
static const TextParagraphType kDefaultBlockType = TextParagraphType::UnformattedText;

} // namespace

class AudioplayFountainImporter::Implementation
{
public:
    /**
     * @brief Закрыт ли разделитель
     */
    bool splitterIsOpen = false;
};


// ****


AudioplayFountainImporter::AudioplayFountainImporter()
    : AbstractAudioplayImporter()
    , AbstractFountainImporter(kPossibleBlockTypes, kDefaultBlockType)
    , d(new Implementation)
{
}

AudioplayFountainImporter::~AudioplayFountainImporter() = default;

AbstractAudioplayImporter::Audioplay AudioplayFountainImporter::importAudioplay(
    const ImportOptions& _options) const
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
    auto audioplay = audioplayText(fountainFile.readAll());
    if (audioplay.name.isEmpty()) {
        audioplay.name = QFileInfo(_options.filePath).completeBaseName();
    }

    return audioplay;
}

AbstractAudioplayImporter::Audioplay AudioplayFountainImporter::audioplayText(
    const QString& _audioplayText) const
{
    Audioplay result;
    result.text = documentText(_audioplayText);
    return result;
}

QString AudioplayFountainImporter::characterName(const QString& _text) const
{
    return AudioplayCharacterParser::name(_text);
}

QString AudioplayFountainImporter::locationName(const QString& _text) const
{
    Q_UNUSED(_text)

    return "";
}

TextParagraphType AudioplayFountainImporter::blockType(QString& _paragraphText) const
{
    TextParagraphType blockType = TextParagraphType::Undefined;
    if (_paragraphText.startsWith("Music: ")) {
        _paragraphText.remove("Music: ");
        movePreviousTypes(0, QString("Music: ").size());
        blockType = TextParagraphType::Music;
    } else if (_paragraphText.startsWith("Sound: ")) {
        _paragraphText.remove("Sound: ");
        movePreviousTypes(0, QString("Sound: ").size());
        blockType = TextParagraphType::Sound;
    } else {
        blockType = AbstractFountainImporter::blockType(_paragraphText);
    }
    return blockType;
}

void AudioplayFountainImporter::writeBlock(const QString& _paragraphText, TextParagraphType _type,
                                           QXmlStreamWriter& _writer) const
{

    auto writeValue = [&_writer](const QString& _text) {
        _writer.writeStartElement(xml::kValueTag);
        _writer.writeCDATA(TextHelper::toHtmlEscaped(_text));
        _writer.writeEndElement(); // value
    };

    switch (_type) {
    case TextParagraphType::Character: {
        //
        // Если диалоги располагаются в таблице и разделитель ещё не открыт, то откроем
        //
        if (TemplatesFacade::audioplayTemplate().placeDialoguesInTable() && !d->splitterIsOpen) {
            _writer.writeEmptyElement(xml::kSplitterTag);
            _writer.writeAttribute(xml::kTypeAttribute, "start");
            d->splitterIsOpen = true;
        }

        _writer.writeStartElement(toString(_type));
        _writer.writeEmptyElement(xml::kParametersTag);
        _writer.writeAttribute(xml::kInFirstColumnAttribute, "true");
        writeValue(_paragraphText);
        break;
    }

    case TextParagraphType::Dialogue: {
        //
        // Если диалоги располагаются в таблице и разделитель ещё не открыт, то откроем
        //
        if (TemplatesFacade::audioplayTemplate().placeDialoguesInTable() && !d->splitterIsOpen) {
            _writer.writeEmptyElement(xml::kSplitterTag);
            _writer.writeAttribute(xml::kTypeAttribute, "start");
            d->splitterIsOpen = true;

            //
            // ... и запишем пустое имя персонажа в первую колонку
            //
            _writer.writeStartElement(toString(TextParagraphType::Character));
            _writer.writeEmptyElement(xml::kParametersTag);
            _writer.writeAttribute(xml::kInFirstColumnAttribute, "true");
            writeValue("");
            _writer.writeEndElement(); // сharacter
        }

        _writer.writeStartElement(toString(_type));
        _writer.writeEmptyElement(xml::kParametersTag);
        _writer.writeAttribute(xml::kInFirstColumnAttribute, "false");
        writeValue(_paragraphText);
        break;
    }

    default: {
        AbstractFountainImporter::writeBlock(_paragraphText, _type, _writer);
        break;
    }
    }
}

void AudioplayFountainImporter::postProcessBlock(TextParagraphType _type,
                                                 QXmlStreamWriter& _writer) const
{
    //
    // Нужно закрыть разделитель, если он октрыт и если текущий блок - не диалог
    //
    if (d->splitterIsOpen && _type != TextParagraphType::Dialogue) {
        _writer.writeEmptyElement(xml::kSplitterTag);
        _writer.writeAttribute(xml::kTypeAttribute, "end");
        d->splitterIsOpen = false;
    }
}

} // namespace BusinessLayer
