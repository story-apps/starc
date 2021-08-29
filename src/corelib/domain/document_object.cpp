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
    { DocumentObjectType::Characters, "application/x-starc/document/characters" },
    { DocumentObjectType::Character, "application/x-starc/document/character" },
    { DocumentObjectType::Locations, "application/x-starc/document/locations" },
    { DocumentObjectType::Location, "application/x-starc/document/location" },
    { DocumentObjectType::Folder, "application/x-starc/document/folder" },
    { DocumentObjectType::Text, "application/x-starc/document/text" }
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
        { DocumentObjectType::Characters, u8"\U000f0849" },
        { DocumentObjectType::Character, u8"\U000f0004" },
        { DocumentObjectType::Locations, u8"\U000f0d15" },
        { DocumentObjectType::Location, u8"\U000f02dc" },
        { DocumentObjectType::Folder, u8"\U000F024B" },
        { DocumentObjectType::Text, u8"\U000F021A" } };
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
