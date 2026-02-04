#include "comic_book_dictionaries_model.h"

#include <domain/document_object.h>
#include <utils/helpers/text_helper.h>

#include <QDomDocument>


namespace BusinessLayer {

namespace {
const QString kDocumentKey = "document";
const QString kCharacterExtensionsKey = "character_extensions";
const QString kItemKey = "v";
} // namespace

class ComicBookDictionariesModel::Implementation
{
public:
    QStringList singlePageIntros;
    QStringList multiplePageIntros;
    QStringList singlePanelIntros;
    QStringList multiplePanelIntros;
    QStringList commonCharacters;
    QStringList characterExtensions;
};


// ****


ComicBookDictionariesModel::ComicBookDictionariesModel(QObject* _parent)
    : AbstractModel(
        {
            kDocumentKey,
            kCharacterExtensionsKey,
            kItemKey,
        },
        _parent)
    , d(new Implementation)
{
    connect(this, &ComicBookDictionariesModel::charactersExtensionsChanged, this,
            &ComicBookDictionariesModel::updateDocumentContent);
}

const QStringList& ComicBookDictionariesModel::singlePageIntros() const
{
    return d->singlePageIntros;
}

const QStringList& ComicBookDictionariesModel::multiplePageIntros() const
{
    return d->multiplePageIntros;
}

const QStringList& ComicBookDictionariesModel::singlePanelIntros() const
{
    return d->singlePanelIntros;
}

const QStringList& ComicBookDictionariesModel::multiplePanelIntros() const
{
    return d->multiplePanelIntros;
}

const QStringList& ComicBookDictionariesModel::commonCharacters() const
{
    return d->commonCharacters;
}

const QStringList& ComicBookDictionariesModel::characterExtensions() const
{
    return d->characterExtensions;
}

void ComicBookDictionariesModel::addCharacterExtension(const QString& _extension)
{
    const auto extensionCorrected = TextHelper::smartToUpper(_extension);
    if (d->characterExtensions.contains(extensionCorrected)) {
        return;
    }

    d->characterExtensions.append(extensionCorrected);
    emit charactersExtensionsChanged();
}

ComicBookDictionariesModel::~ComicBookDictionariesModel() = default;

void ComicBookDictionariesModel::initDocument()
{
    if (document() == nullptr) {
        return;
    }

    const auto shouldBeInitialized = document()->content().isEmpty();

    QDomDocument domDocument;
    domDocument.setContent(document()->content());
    const auto documentNode = domDocument.firstChildElement(kDocumentKey);
    auto fillDictionary = [documentNode](const QString& _key, const QStringList& _defaultItems,
                                         QStringList& _dictionary) {
        if (!_key.isEmpty()) {
            const auto dictionaryNode = documentNode.firstChildElement(_key);
            auto itemNode = dictionaryNode.firstChildElement();
            while (!itemNode.isNull()) {
                _dictionary.append(TextHelper::fromHtmlEscaped(itemNode.text()));
                itemNode = itemNode.nextSiblingElement();
            }
        }

        if (_dictionary.isEmpty()) {
            _dictionary.append(_defaultItems);
        }
    };
    //: Comic book single page intro
    const QStringList defaultSinglePageIntros = { tr("Page") };
    fillDictionary({}, defaultSinglePageIntros, d->singlePageIntros);
    //: Comic book double pages intro
    const QStringList defaultMultiplePageIntros = { tr("Pages") };
    fillDictionary({}, defaultMultiplePageIntros, d->multiplePageIntros);
    //
    //: Comic book panel intro
    const QStringList defaultSinglePanelIntros = { tr("Panel"), tr("Background panel") };
    fillDictionary({}, defaultSinglePanelIntros, d->singlePanelIntros);
    //: Comic book multiple panels intro
    const QStringList defaultMultiplePanelIntros = { tr("Panels") };
    fillDictionary({}, defaultMultiplePanelIntros, d->multiplePanelIntros);
    //
    const QStringList defaultCommonCharacters = { tr("CAPTION"), tr("NARRATION"), tr("SFX") };
    fillDictionary({}, defaultCommonCharacters, d->commonCharacters);
    //
    const QStringList defaultCharacterExtensions
        = { tr("OFF"), tr("WHISPER"), tr("BURST"), tr("WEAK"), tr("SINGING") };
    fillDictionary(kCharacterExtensionsKey, defaultCharacterExtensions, d->characterExtensions);

    //
    // Т.к. документ справочников не создаётся в рамках структуры проекта, то нужно вручную
    // применить болванку контента для документа, чтобы далее он мог быть корректно сохранён
    //
    if (shouldBeInitialized) {
        reassignContent();
    }
}

void ComicBookDictionariesModel::clearDocument()
{
    QSignalBlocker signalBlocker(this);

    d->singlePageIntros.clear();
    d->multiplePageIntros.clear();
    d->singlePanelIntros.clear();
    d->multiplePanelIntros.clear();
    d->characterExtensions.clear();
}

QByteArray ComicBookDictionariesModel::toXml() const
{
    if (document() == nullptr) {
        return {};
    }

    QByteArray xml = "<?xml version=\"1.0\"?>\n";
    xml += QString("<%1 mime-type=\"%2\" version=\"1.0\">\n")
               .arg(kDocumentKey, Domain::mimeTypeFor(document()->type()))
               .toUtf8();
    auto writeDictionary = [&xml](const QString& _key, const QStringList& _values) {
        xml += QString("<%1>\n").arg(_key).toUtf8();
        for (const auto& value : _values) {
            xml += QString("<%1><![CDATA[%2]]></%1>\n")
                       .arg(kItemKey, TextHelper::toHtmlEscaped(value))
                       .toUtf8();
        }
        xml += QString("</%1>\n").arg(_key).toUtf8();
    };
    writeDictionary(kCharacterExtensionsKey, d->characterExtensions);
    xml += QString("</%1>").arg(kDocumentKey).toUtf8();
    return xml;
}

} // namespace BusinessLayer
