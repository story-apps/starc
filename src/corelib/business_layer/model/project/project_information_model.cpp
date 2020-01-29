#include "project_information_model.h"

#include <business_layer/model/abstract_image_wrapper.h>

#include <domain/document_object.h>

#include <utils/helpers/image_helper.h>
#include <utils/helpers/text_helper.h>

#include <QDomDocument>
#include <QPixmap>


namespace BusinessLayer
{

class ProjectInformationModel::Implementation
{
public:
    QString name;
    QString logline;
    struct {
        QUuid uuid;
        QPixmap image;
    } cover;
};


// ****


ProjectInformationModel::ProjectInformationModel(QObject* _parent)
    : AbstractModel({}, _parent),
      d(new Implementation)
{
    connect(this, &ProjectInformationModel::nameChanged, this, &ProjectInformationModel::updateDocumentContent);
    connect(this, &ProjectInformationModel::loglineChanged, this, &ProjectInformationModel::updateDocumentContent);
    connect(this, &ProjectInformationModel::coverChanged, this, &ProjectInformationModel::updateDocumentContent);
}

ProjectInformationModel::~ProjectInformationModel() = default;

const QString& ProjectInformationModel::name() const
{
    return d->name;
}

void ProjectInformationModel::setName(const QString& _name)
{
    if (d->name == _name) {
        return;
    }

    d->name = _name;
    emit nameChanged(d->name);
}

const QString& ProjectInformationModel::logline() const
{
    return d->logline;
}

void ProjectInformationModel::setLogline(const QString& _logline)
{
    if (d->logline == _logline) {
        return;
    }

    d->logline = _logline;
    emit loglineChanged(d->logline);
}

const QPixmap& ProjectInformationModel::cover() const
{
    return d->cover.image;
}

void ProjectInformationModel::setCover(const QPixmap& _cover)
{
    d->cover.image = _cover;
    d->cover.uuid = imageWrapper()->save(d->cover.image);
    emit coverChanged(d->cover.image);
}

void ProjectInformationModel::initDocument()
{
    if (document() == nullptr) {
        return;
    }

    QDomDocument domDocument;
    domDocument.setContent(document()->content());
    auto documentNode = domDocument.firstChildElement("document");
    auto nameNode = documentNode.firstChildElement();
    d->name = TextHelper::fromHtmlEscaped(nameNode.text());
    auto loglineNode = nameNode.nextSiblingElement();
    d->logline = TextHelper::fromHtmlEscaped(loglineNode.text());
    auto coverNode = loglineNode.nextSiblingElement();
    d->cover.uuid = coverNode.text();
    d->cover.image = imageWrapper()->load(d->cover.uuid);
}

void ProjectInformationModel::clearDocument()
{
    QSignalBlocker signalBlocker(this);

    setName({});
    setLogline({});
    setCover({});
}

QByteArray ProjectInformationModel::toXml() const
{
    if (document() == nullptr) {
        return {};
    }

    QByteArray xml = "<?xml version=\"1.0\"?>\n";
    xml += "<document mime-type=\"" + Domain::mimeTypeFor(document()->type()) + "\" version=\"1.0\">\n";
    xml += "<name><![CDATA[" + TextHelper::toHtmlEscaped(d->name) + "]]></name>\n";
    xml += "<logline><![CDATA[" + TextHelper::toHtmlEscaped(d->logline) + "]]></logline>\n";
    xml += "<cover><![CDATA[" + d->cover.uuid.toString() + "]]></cover>\n";
    xml += "</document>";
    return xml;
}

} // namespace BusinessLayer
