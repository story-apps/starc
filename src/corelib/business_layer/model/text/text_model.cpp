#include "text_model.h"

#include <business_layer/model/abstract_image_wrapper.h>

#include <domain/document_object.h>

#include <utils/helpers/image_helper.h>
#include <utils/helpers/text_helper.h>

#include <QDomDocument>
#include <QPixmap>


namespace BusinessLayer
{

class TextModel::Implementation
{
public:
    QString name;
    QString text;
};


// ****


TextModel::TextModel(QObject* _parent)
    : AbstractModel({}, _parent),
      d(new Implementation)
{
    connect(this, &TextModel::nameChanged, this, &TextModel::updateDocumentContent);
    connect(this, &TextModel::textChanged, this, &TextModel::updateDocumentContent);
}

TextModel::~TextModel() = default;

const QString& TextModel::name() const
{
    return d->name;
}

void TextModel::setName(const QString& _name)
{
    if (d->name == _name) {
        return;
    }

    d->name = _name;
    emit nameChanged(d->name);
    emit documentNameChanged(d->name);
}

const QString& TextModel::text() const
{
    return d->text;
}

void TextModel::setText(const QString& _text)
{
    if (d->text == _text) {
        return;
    }

    d->text = _text;
    emit textChanged(d->text);
}

void TextModel::initDocument()
{
    if (document() == nullptr) {
        return;
    }

    QDomDocument domDocument;
    domDocument.setContent(document()->content());
    auto documentNode = domDocument.firstChildElement("document");
    auto nameNode = documentNode.firstChildElement();
    d->name = TextHelper::fromHtmlEscaped(nameNode.text());
    auto textNode = nameNode.nextSiblingElement();
    d->text = TextHelper::fromHtmlEscaped(textNode.text());
}

void TextModel::clearDocument()
{
    setName({});
    setText({});
}

QByteArray TextModel::toXml() const
{
    if (document() == nullptr) {
        return {};
    }

    QByteArray xml = "<?xml version=\"1.0\"?>\n";
    xml += "<document mime-type=\"" + Domain::mimeTypeFor(document()->type()) + "\" version=\"1.0\">\n";
    xml += "<name><![CDATA[" + TextHelper::toHtmlEscaped(d->name) + "]]></name>\n";
    xml += "<text><![CDATA[" + TextHelper::toHtmlEscaped(d->text) + "]]></text>\n";
    xml += "</document>";
    return xml;
}

} // namespace BusinessLayer

