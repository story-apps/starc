#include "stageplay_fountain_importer.h"

#include <business_layer/import/import_options.h>
#include <business_layer/model/stageplay/text/stageplay_text_block_parser.h>
#include <business_layer/model/text/text_model_xml.h>
#include <business_layer/templates/stageplay_template.h>
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
    TextParagraphType::Dialogue,     TextParagraphType::Action,
    TextParagraphType::InlineNote,   TextParagraphType::UnformattedText,
};

/**
 * @brief Тип блока по умолчанию
 */
static const TextParagraphType kDefaultBlockType = TextParagraphType::UnformattedText;

} // namespace

class StageplayFountainImporter::Implementation
{
public:
    /**
     * @brief Закрыт ли разделитель
     */
    bool splitterIsOpen = false;
};


// ****


StageplayFountainImporter::StageplayFountainImporter()
    : AbstractStageplayImporter()
    , AbstractFountainImporter(kPossibleBlockTypes, kDefaultBlockType)
    , d(new Implementation)
{
}

StageplayFountainImporter::~StageplayFountainImporter() = default;

AbstractStageplayImporter::Stageplay StageplayFountainImporter::importStageplay(
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
    auto stageplay = stageplayText(fountainFile.readAll());
    if (stageplay.name.isEmpty()) {
        stageplay.name = QFileInfo(_options.filePath).completeBaseName();
    }

    return stageplay;
}

AbstractStageplayImporter::Stageplay StageplayFountainImporter::stageplayText(
    const QString& _stageplayText) const
{
    Stageplay result;
    result.text = documentText(_stageplayText);
    return result;
}

QString StageplayFountainImporter::characterName(const QString& _text) const
{
    return StageplayCharacterParser::name(_text);
}

QString StageplayFountainImporter::locationName(const QString& _text) const
{
    Q_UNUSED(_text)

    return "";
}

void StageplayFountainImporter::writeBlock(const QString& _paragraphText, TextParagraphType _type,
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
        if (TemplatesFacade::stageplayTemplate().placeDialoguesInTable() && !d->splitterIsOpen) {
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
        if (TemplatesFacade::stageplayTemplate().placeDialoguesInTable() && !d->splitterIsOpen) {
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

void StageplayFountainImporter::postProcessBlock(TextParagraphType _type,
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
