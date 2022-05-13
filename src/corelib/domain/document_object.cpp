#include "document_object.h"


namespace Domain {

namespace {

const QHash<DocumentObjectType, QByteArray> kDocumentObjectTypeToMimeType = {
    { DocumentObjectType::Undefined, "application/x-starc/document/undefined" },
    { DocumentObjectType::Structure, "application/x-starc/document/structure" },
    { DocumentObjectType::Project, "application/x-starc/document/project" },
    { DocumentObjectType::RecycleBin, "application/x-starc/document/recycle-bin" },
    { DocumentObjectType::Screenplay, "application/x-starc/document/screenplay" },
    { DocumentObjectType::ScreenplayTitlePage,
      "application/x-starc/document/screenplay/title-page" },
    { DocumentObjectType::ScreenplaySynopsis, "application/x-starc/document/screenplay/synopsis" },
    { DocumentObjectType::ScreenplayTreatment,
      "application/x-starc/document/screenplay/treatment" },
    { DocumentObjectType::ScreenplayText, "application/x-starc/document/screenplay/text" },
    { DocumentObjectType::ScreenplayDictionaries,
      "application/x-starc/document/screenplay/dictionaries" },
    { DocumentObjectType::ScreenplayStatistics,
      "application/x-starc/document/screenplay/statistics" },
    { DocumentObjectType::ComicBook, "application/x-starc/document/comicbook" },
    { DocumentObjectType::ComicBookTitlePage, "application/x-starc/document/comicbook/title-page" },
    { DocumentObjectType::ComicBookSynopsis, "application/x-starc/document/comicbook/synopsis" },
    { DocumentObjectType::ComicBookText, "application/x-starc/document/comicbook/text" },
    { DocumentObjectType::ComicBookDictionaries,
      "application/x-starc/document/comicbook/dictionaries" },
    { DocumentObjectType::ComicBookStatistics,
      "application/x-starc/document/comicbook/statistics" },
    { DocumentObjectType::Audioplay, "application/x-starc/document/audioplay" },
    { DocumentObjectType::AudioplayTitlePage, "application/x-starc/document/audioplay/title-page" },
    { DocumentObjectType::AudioplaySynopsis, "application/x-starc/document/audioplay/synopsis" },
    { DocumentObjectType::AudioplayText, "application/x-starc/document/audioplay/text" },
    { DocumentObjectType::AudioplayDictionaries,
      "application/x-starc/document/audioplay/dictionaries" },
    { DocumentObjectType::AudioplayStatistics,
      "application/x-starc/document/audioplay/statistics" },
    { DocumentObjectType::Stageplay, "application/x-starc/document/stageplay" },
    { DocumentObjectType::StageplayTitlePage, "application/x-starc/document/stageplay/title-page" },
    { DocumentObjectType::StageplaySynopsis, "application/x-starc/document/stageplay/synopsis" },
    { DocumentObjectType::StageplayText, "application/x-starc/document/stageplay/text" },
    { DocumentObjectType::StageplayDictionaries,
      "application/x-starc/document/stageplay/dictionaries" },
    { DocumentObjectType::StageplayStatistics,
      "application/x-starc/document/stageplay/statistics" },
    { DocumentObjectType::Characters, "application/x-starc/document/characters" },
    { DocumentObjectType::Character, "application/x-starc/document/character" },
    { DocumentObjectType::Locations, "application/x-starc/document/locations" },
    { DocumentObjectType::Location, "application/x-starc/document/location" },
    { DocumentObjectType::Folder, "application/x-starc/document/folder" },
    { DocumentObjectType::SimpleText, "application/x-starc/document/text" }
};
const QHash<DocumentObjectType, QString> kDocumentObjectTypeToIcon
    = { { DocumentObjectType::Undefined, u8"\U000f078b" },
        { DocumentObjectType::Structure, u8"\U000f078b" },
        { DocumentObjectType::Project, u8"\U000f0ab7" },
        { DocumentObjectType::RecycleBin, u8"\U000f01b4" },
        { DocumentObjectType::Screenplay, u8"\U000F0FCE" },
        { DocumentObjectType::ScreenplayTitlePage, u8"\U000f00be" },
        { DocumentObjectType::ScreenplaySynopsis, u8"\U000f021a" },
        { DocumentObjectType::ScreenplayTreatment, u8"\U000f021a" },
        { DocumentObjectType::ScreenplayText, u8"\U000f021a" },
        { DocumentObjectType::ScreenplayStatistics, u8"\U000f0127" },
        { DocumentObjectType::ComicBook, u8"\U000F056E" },
        { DocumentObjectType::ComicBookTitlePage, u8"\U000f00be" },
        { DocumentObjectType::ComicBookSynopsis, u8"\U000f021a" },
        { DocumentObjectType::ComicBookText, u8"\U000f021a" },
        { DocumentObjectType::ComicBookStatistics, u8"\U000f0127" },
        { DocumentObjectType::Audioplay, u8"\U000F02CB" },
        { DocumentObjectType::AudioplayTitlePage, u8"\U000f00be" },
        { DocumentObjectType::AudioplaySynopsis, u8"\U000f021a" },
        { DocumentObjectType::AudioplayText, u8"\U000f021a" },
        { DocumentObjectType::AudioplayStatistics, u8"\U000f0127" },
        { DocumentObjectType::Stageplay, u8"\U000F0D02" },
        { DocumentObjectType::StageplayTitlePage, u8"\U000f00be" },
        { DocumentObjectType::StageplaySynopsis, u8"\U000f021a" },
        { DocumentObjectType::StageplayText, u8"\U000f021a" },
        { DocumentObjectType::StageplayStatistics, u8"\U000f0127" },
        { DocumentObjectType::Characters, u8"\U000f0849" },
        { DocumentObjectType::Character, u8"\U000f0004" },
        { DocumentObjectType::Locations, u8"\U000f0d15" },
        { DocumentObjectType::Location, u8"\U000f02dc" },
        { DocumentObjectType::Folder, u8"\U000F024B" },
        { DocumentObjectType::SimpleText, u8"\U000F021A" } };
} // namespace

QByteArray mimeTypeFor(DocumentObjectType _type)
{
    return kDocumentObjectTypeToMimeType.value(_type);
}

DocumentObjectType typeFor(const QByteArray& _mime)
{
    return static_cast<DocumentObjectType>(kDocumentObjectTypeToMimeType.key(_mime));
}

QString iconForType(DocumentObjectType _type)
{
    return kDocumentObjectTypeToIcon.value(_type);
}

const QUuid& DocumentObject::uuid() const
{
    return m_uuid;
}

void DocumentObject::setUuid(const QUuid& _uuid)
{
    if (m_uuid == _uuid) {
        return;
    }

    m_uuid = _uuid;
    markChangesNotStored();
}

DocumentObjectType DocumentObject::type() const
{
    return m_type;
}

void DocumentObject::setType(DocumentObjectType _type)
{
    if (m_type == _type) {
        return;
    }

    m_type = _type;
    markChangesNotStored();
}

const QByteArray& DocumentObject::content() const
{
    return m_content;
}

void DocumentObject::setContent(const QByteArray& _content)
{
    //
    // NOTE: Тут специально нет проверки, т.к. данные могут быть очень большими
    //
    const int maximumComparableSize = 3000;
    if (m_content.size() <= maximumComparableSize && _content.size() <= maximumComparableSize
        && m_content == _content) {
        return;
    }

    m_content = _content;
    markChangesNotStored();
}

DocumentObject::DocumentObject(const Identifier& _id, const QUuid& _uuid, DocumentObjectType _type,
                               const QByteArray& _content)
    : DomainObject(_id)
    , m_uuid(_uuid)
    , m_type(_type)
    , m_content(_content)
{
}

} // namespace Domain
