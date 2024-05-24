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

AudioplayFountainImporter::AudioplayFountainImporter()
    : AbstractAudioplayImporter()
    , AbstractFountainImporter(false)
{
}

AudioplayFountainImporter::~AudioplayFountainImporter() = default;

AbstractAudioplayImporter::Documents AudioplayFountainImporter::importDocuments(
    const AudioplayImportOptions& _options) const
{
    QVector<QPair<TextParagraphType, QString>> paragraphs = parapraphsForDocuments(_options);
    std::set<QString> characterNames;
    for (const auto &paragraph : paragraphs) {
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

        default:{
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

void AudioplayFountainImporter::preprocessBlock(QVector<QString>& _paragraphs, QXmlStreamWriter& _writer) const
{
    // TODO: обработка блоков аудиопьессы
}

} // namespace BusinessLayer
