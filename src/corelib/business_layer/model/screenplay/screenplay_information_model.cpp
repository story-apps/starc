#include "screenplay_information_model.h"

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
    const QString kHeaderKey = QStringLiteral("header");
    const QString kPrintHeaderOnTitlePageKey = QStringLiteral("print_header_on_title");
    const QString kFooterKey = QStringLiteral("footer");
    const QString kPrintFooterOnTitlePageKey = QStringLiteral("print_footer_on_title");
    const QString kScenesNumbersPrefixKey = QStringLiteral("scenes_numbers_prefix");
    const QString kScenesNumberingStartAtKey = QStringLiteral("scenes_numbering_start_at");

}

class ScreenplayInformationModel::Implementation
{
public:
    QString name;
    QString logline;
    QString header;
    bool printHeaderOnTitlePage = true;
    QString footer;
    bool printFooterOnTitlePage = true;
    QString scenesNumbersPrefix;
    int scenesNumberingStartAt = 1;
};


// ****


ScreenplayInformationModel::ScreenplayInformationModel(QObject* _parent)
    : AbstractModel({ kDocumentKey, kNameKey, kLoglineKey, kHeaderKey, kPrintHeaderOnTitlePageKey,
                      kFooterKey, kPrintFooterOnTitlePageKey, kScenesNumbersPrefixKey,
                      kScenesNumberingStartAtKey },
                    _parent),
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

const QString& ScreenplayInformationModel::logline() const
{
    return d->logline;
}

void ScreenplayInformationModel::setLogline(const QString& _logline)
{
    if (d->logline == _logline) {
            return;
    }

    d->logline = _logline;
    emit loglineChanged(d->logline);
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

bool ScreenplayInformationModel::printHeaderOnTitlePage() const
{
    return d->printHeaderOnTitlePage;
}

void ScreenplayInformationModel::setPrintHeaderOnTitlePage(bool _print)
{
    if (d->printHeaderOnTitlePage == _print) {
        return;
    }

    d->printHeaderOnTitlePage = _print;
    emit printHeaderOnTitlePageChanged(d->printHeaderOnTitlePage);
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

bool ScreenplayInformationModel::printFooterOnTitlePage() const
{
    return d->printFooterOnTitlePage;
}

void ScreenplayInformationModel::setPrintFooterOnTitlePage(bool _print)
{
    if (d->printFooterOnTitlePage == _print) {
        return;
    }

    d->printFooterOnTitlePage = _print;
    emit printFooterOnTitlePageChanged(d->printFooterOnTitlePage);
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
    const auto documentNode = domDocument.firstChildElement(kDocumentKey);
    d->name = documentNode.firstChildElement(kNameKey).text();
    d->logline = documentNode.firstChildElement(kLoglineKey).text();
    d->header = documentNode.firstChildElement(kHeaderKey).text();
    d->printHeaderOnTitlePage = documentNode.firstChildElement(kPrintHeaderOnTitlePageKey).text() == "true";
    d->footer = documentNode.firstChildElement(kFooterKey).text();
    d->printFooterOnTitlePage = documentNode.firstChildElement(kPrintFooterOnTitlePageKey).text() == "true";
    d->scenesNumbersPrefix = documentNode.firstChildElement(kScenesNumbersPrefixKey).text();
    const auto scenesNumberingStartAtNode = documentNode.firstChildElement(kScenesNumberingStartAtKey);
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
    xml += QString("<%1 mime-type=\"%2\" version=\"1.0\">\n").arg(kDocumentKey, Domain::mimeTypeFor(document()->type()));
    xml += QString("<%1><![CDATA[%2]]></%1>\n").arg(kNameKey, d->name);
    xml += QString("<%1><![CDATA[%2]]></%1>\n").arg(kLoglineKey, d->logline);
    xml += QString("<%1><![CDATA[%2]]></%1>\n").arg(kHeaderKey, d->header);
    xml += QString("<%1><![CDATA[%2]]></%1>\n").arg(kPrintHeaderOnTitlePageKey, d->printHeaderOnTitlePage ? "true" : "false");
    xml += QString("<%1><![CDATA[%2]]></%1>\n").arg(kFooterKey, d->footer);
    xml += QString("<%1><![CDATA[%2]]></%1>\n").arg(kPrintFooterOnTitlePageKey, d->printFooterOnTitlePage ? "true" : "false");
    xml += QString("<%1><![CDATA[%2]]></%1>\n").arg(kScenesNumbersPrefixKey, d->scenesNumbersPrefix);
    xml += QString("<%1><![CDATA[%2]]></%1>\n").arg(kScenesNumberingStartAtKey, QString::number(d->scenesNumberingStartAt));
    xml += QString("</%1>").arg(kDocumentKey);
    return xml;
}

} // namespace BusinessLayer
