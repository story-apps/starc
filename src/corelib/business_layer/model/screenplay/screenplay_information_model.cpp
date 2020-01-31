#include "screenplay_information_model.h"

#include <business_layer/model/abstract_image_wrapper.h>

#include <domain/document_object.h>

#include <utils/helpers/image_helper.h>
#include <utils/helpers/text_helper.h>

#include <QDomDocument>
#include <QPixmap>


namespace BusinessLayer
{

class ScreenplayInformationModel::Implementation
{
public:
    QString name;
    QString header;
    QString footer;
    QString scenesNumbersPrefix;
    int scenesNumberingStartAt = 1;
};


// ****


ScreenplayInformationModel::ScreenplayInformationModel(QObject* _parent)
    : AbstractModel({}, _parent),
      d(new Implementation)
{
    connect(this, &ScreenplayInformationModel::nameChanged, this, &ScreenplayInformationModel::updateDocumentContent);
    connect(this, &ScreenplayInformationModel::headerChanged, this, &ScreenplayInformationModel::updateDocumentContent);
    connect(this, &ScreenplayInformationModel::footerChanged, this, &ScreenplayInformationModel::updateDocumentContent);
    connect(this, &ScreenplayInformationModel::scenesNumbersPrefixChanged, this, &ScreenplayInformationModel::updateDocumentContent);
    connect(this, &ScreenplayInformationModel::scenesNumberingStartAtChanged, this, &ScreenplayInformationModel::updateDocumentContent);
}

ScreenplayInformationModel::~ScreenplayInformationModel() = default;

const QString& ScreenplayInformationModel::name() const
{
    return d->name;
}

void ScreenplayInformationModel::setName(const QString& _name)
{
    if (d->name == _name) {
        return;
    }

    d->name = _name;
    emit nameChanged(d->name);
    emit documentNameChanged(d->name);
}

void ScreenplayInformationModel::setDocumentName(const QString& _name)
{
    setName(_name);
}

const QString& ScreenplayInformationModel::header() const
{
    return d->header;
}

void ScreenplayInformationModel::setHeader(const QString& _header)
{
    if (d->header == _header) {
        return;
    }

    d->header = _header;
    emit headerChanged(d->header);
}

const QString& ScreenplayInformationModel::footer() const
{
    return d->footer;
}

void ScreenplayInformationModel::setFooter(const QString& _footer)
{
    if (d->footer == _footer) {
        return;
    }

    d->footer = _footer;
    emit footerChanged(d->footer);
}

const QString& ScreenplayInformationModel::scenesNumbersPrefix() const
{
    return d->scenesNumbersPrefix;
}

void ScreenplayInformationModel::setScenesNumbersPrefix(const QString& _prefix)
{
    if (d->scenesNumbersPrefix == _prefix) {
        return;
    }

    d->scenesNumbersPrefix = _prefix;
    emit scenesNumbersPrefixChanged(d->scenesNumbersPrefix);
}

int ScreenplayInformationModel::scenesNumberingStartAt() const
{
    return d->scenesNumberingStartAt;
}

void ScreenplayInformationModel::setScenesNumberingStartAt(int _startNumber)
{
    if (d->scenesNumberingStartAt == _startNumber) {
        return;
    }

    d->scenesNumberingStartAt = _startNumber;
    emit scenesNumberingStartAtChanged(d->scenesNumberingStartAt);
}

void ScreenplayInformationModel::initDocument()
{
    if (document() == nullptr) {
        return;
    }

    QDomDocument domDocument;
    domDocument.setContent(document()->content());
    auto documentNode = domDocument.firstChildElement("document");
    auto nameNode = documentNode.firstChildElement();
    d->name = TextHelper::fromHtmlEscaped(nameNode.text());
    auto headerNode = nameNode.nextSiblingElement();
    d->header = TextHelper::fromHtmlEscaped(headerNode.text());
    auto footerNode = headerNode.nextSiblingElement();
    d->footer = TextHelper::fromHtmlEscaped(footerNode.text());
    auto scenesNumbersPrefixNode = footerNode.nextSiblingElement();
    d->scenesNumbersPrefix = TextHelper::fromHtmlEscaped(scenesNumbersPrefixNode.text());
    auto scenesNumberingStartAtNode = scenesNumbersPrefixNode.nextSiblingElement();
    if (!scenesNumberingStartAtNode.isNull()) {
        d->scenesNumberingStartAt = scenesNumberingStartAtNode.text().toInt();
    }
}

void ScreenplayInformationModel::clearDocument()
{
    QSignalBlocker signalBlocker(this);

    setName({});
    setHeader({});
}

QByteArray ScreenplayInformationModel::toXml() const
{
    if (document() == nullptr) {
        return {};
    }

    QByteArray xml = "<?xml version=\"1.0\"?>\n";
    xml += "<document mime-type=\"" + Domain::mimeTypeFor(document()->type()) + "\" version=\"1.0\">\n";
    xml += "<name><![CDATA[" + TextHelper::toHtmlEscaped(d->name) + "]]></name>\n";
    xml += "<header><![CDATA[" + TextHelper::toHtmlEscaped(d->header) + "]]></header>\n";
    xml += "<footer><![CDATA[" + TextHelper::toHtmlEscaped(d->footer) + "]]></footer>\n";
    xml += "<scenes_numbers_prefix><![CDATA[" + TextHelper::toHtmlEscaped(d->scenesNumbersPrefix) + "]]></scenes_numbers_prefix>\n";
    xml += "<scenes_numbering_start_at><![CDATA[" + QString::number(d->scenesNumberingStartAt) + "]]></scenes_numbering_start_at>\n";
    xml += "</document>";
    return xml;
}

} // namespace BusinessLayer
