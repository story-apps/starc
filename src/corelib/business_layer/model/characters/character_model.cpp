#include "character_model.h"

#include <domain/document_object.h>

#include <QDomDocument>


namespace BusinessLayer
{

namespace {
    const QString kDocumentKey = QStringLiteral("document");
    const QString kNameKey = QStringLiteral("name");
}

class CharacterModel::Implementation
{
public:
    QString name;
};


// ****


CharacterModel::CharacterModel(QObject* _parent)
    : AbstractModel({ kDocumentKey, kNameKey },
                    _parent),
      d(new Implementation)
{
    connect(this, &CharacterModel::nameChanged, this, &CharacterModel::updateDocumentContent);
}

CharacterModel::~CharacterModel() = default;

const QString& CharacterModel::name() const
{
    return d->name;
}

void CharacterModel::setName(const QString& _name)
{
    if (d->name == _name) {
        return;
    }

    d->name = _name;
    emit nameChanged(d->name);
}

void CharacterModel::setDocumentName(const QString& _name)
{
    setName(_name);
}

void CharacterModel::initDocument()
{
    if (document() == nullptr) {
        return;
    }

    QDomDocument domDocument;
    domDocument.setContent(document()->content());
    const auto documentNode = domDocument.firstChildElement(kDocumentKey);
    const auto nameNode = documentNode.firstChildElement(kNameKey);
    d->name = nameNode.text();
}

void CharacterModel::clearDocument()
{
    QSignalBlocker signalBlocker(this);

    setName({});
}

QByteArray CharacterModel::toXml() const
{
    if (document() == nullptr) {
        return {};
    }

    QByteArray xml = "<?xml version=\"1.0\"?>\n";
    xml += QString("<%1 mime-type=\"%2\" version=\"1.0\">\n").arg(kDocumentKey, Domain::mimeTypeFor(document()->type()));
    xml += QString("<%1><![CDATA[%2]]></%1>\n").arg(kNameKey, d->name);
    xml += QString("</%1>").arg(kDocumentKey);
    return xml;
}

} // namespace BusinessLayer
