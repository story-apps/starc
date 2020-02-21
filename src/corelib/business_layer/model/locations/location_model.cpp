#include "location_model.h"

#include <domain/document_object.h>

#include <QDomDocument>


namespace BusinessLayer
{

namespace {
    const QString kDocumentKey = QStringLiteral("document");
    const QString kNameKey = QStringLiteral("name");
}

class LocationModel::Implementation
{
public:
    QString name;
};


// ****


LocationModel::LocationModel(QObject* _parent)
    : AbstractModel({ kDocumentKey, kNameKey },
                    _parent),
      d(new Implementation)
{
    connect(this, &LocationModel::nameChanged, this, &LocationModel::updateDocumentContent);
}

LocationModel::~LocationModel() = default;

const QString& LocationModel::name() const
{
    return d->name;
}

void LocationModel::setName(const QString& _name)
{
    if (d->name == _name) {
        return;
    }

    d->name = _name;
    emit nameChanged(d->name);
}

void LocationModel::setDocumentName(const QString& _name)
{
    setName(_name);
}

void LocationModel::initDocument()
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

void LocationModel::clearDocument()
{
    QSignalBlocker signalBlocker(this);

    setName({});
}

QByteArray LocationModel::toXml() const
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
