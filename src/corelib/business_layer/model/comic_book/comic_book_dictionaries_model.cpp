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
    QVector<QString> singlePageIntros;
    QVector<QString> multiplePageIntros;
    QVector<QString> panelIntros;
    QVector<QString> commonCharacters;
    QVector<QString> characterExtensions;
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

const QVector<QString>& ComicBookDictionariesModel::singlePageIntros() const
{
    return d->singlePageIntros;
}

const QVector<QString>& ComicBookDictionariesModel::multiplePageIntros() const
{
    return d->multiplePageIntros;
}

const QVector<QString>& ComicBookDictionariesModel::panelIntros() const
{
    return d->panelIntros;
}

const QVector<QString>& ComicBookDictionariesModel::commonCharacters() const
{
    return d->commonCharacters;
}

const QVector<QString>& ComicBookDictionariesModel::characterExtensions() const
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

    QDomDocument domDocument;
    domDocument.setContent(document()->content());
    const auto documentNode = domDocument.firstChildElement(kDocumentKey);
    auto fillDictionary = [documentNode](const QString& _key, const QVector<QString>& _defaultItems,
                                         QVector<QString>& _dictionary) {
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
    const QVector<QString> defaultSinglePageIntros = { "page", tr("page") };
    fillDictionary({}, defaultSinglePageIntros, d->singlePageIntros);
    const QVector<QString> defaultMultiplePageIntros = { "pages", tr("pages") };
    fillDictionary({}, defaultMultiplePageIntros, d->multiplePageIntros);
    //
    const QVector<QString> defaultPanelIntros = { "panel", tr("panel"), "panels", tr("panels") };
    fillDictionary({}, defaultPanelIntros, d->panelIntros);
    //
    const QVector<QString> defaultCommonCharacters = { tr("CAPTION"), tr("NARRATION"), tr("SFX") };
    fillDictionary({}, defaultCommonCharacters, d->commonCharacters);
    //
    const QVector<QString> defaultCharacterExtensions
        = { tr("OFF"), tr("WHISPER"), tr("BURST"), tr("WEAK"), tr("SINGING") };
    fillDictionary(kCharacterExtensionsKey, defaultCharacterExtensions, d->characterExtensions);
}

void ComicBookDictionariesModel::clearDocument()
{
    QSignalBlocker signalBlocker(this);

    d->singlePageIntros.clear();
    d->panelIntros.clear();
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
    auto writeDictionary = [&xml](const QString& _key, const QVector<QString>& _values) {
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
