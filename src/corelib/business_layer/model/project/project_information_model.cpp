#include "project_information_model.h"

#include <business_layer/model/abstract_image_wrapper.h>

#include <domain/document_object.h>

#include <utils/helpers/image_helper.h>
#include <utils/helpers/text_helper.h>

#include <QDomDocument>
#include <QPixmap>


namespace BusinessLayer
{

namespace {
    const QString kDocumentKey = QStringLiteral("document");
    const QString kNameKey = QStringLiteral("name");
    const QString kLoglineKey = QStringLiteral("logline");
    const QString kCoverKey = QStringLiteral("cover");
}

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
    : AbstractModel({ kDocumentKey, kNameKey, kLoglineKey, kCoverKey },
                    _parent),
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
    emit documentNameChanged(d->name);
}

void ProjectInformationModel::setDocumentName(const QString& _name)
{
    setName(_name);
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
    const auto documentNode = domDocument.firstChildElement(kDocumentKey);
    const auto nameNode = documentNode.firstChildElement(kNameKey);
    d->name = nameNode.text();
    const auto loglineNode = documentNode.firstChildElement(kLoglineKey);
    d->logline = loglineNode.text();
    const auto coverNode = documentNode.firstChildElement(kCoverKey);
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
    xml += QString("<%1 mime-type=\"%2\" version=\"1.0\">\n").arg(kDocumentKey, Domain::mimeTypeFor(document()->type()));
    xml += QString("<%1><![CDATA[%2]]></%1>\n").arg(kNameKey, d->name);
    xml += QString("<%1><![CDATA[%2]]></%1>\n").arg(kLoglineKey, d->logline);
    xml += QString("<%1><![CDATA[%2]]></%1>\n").arg(kCoverKey, d->cover.uuid.toString());
    xml += QString("</%1>").arg(kDocumentKey);
    return xml;
}

} // namespace BusinessLayer
