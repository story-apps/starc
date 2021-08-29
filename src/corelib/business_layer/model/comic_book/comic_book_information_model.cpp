#include "comic_book_information_model.h"

#include <business_layer/model/abstract_image_wrapper.h>
#include <domain/document_object.h>
#include <utils/helpers/image_helper.h>
#include <utils/helpers/text_helper.h>

#include <QDomDocument>


namespace BusinessLayer {

namespace {
const QString kDocumentKey = "document";
const QString kNameKey = "name";
const QString kTaglineKey = "tagline";
const QString kLoglineKey = "logline";
const QString kTitlePageVisibleKey = "title_page_visible";
const QString kSynopsisVisibleKey = "synopsis_visible";
const QString kComicBookTextVisibleKey = "comicBook_text_visible";
const QString kComicBookStatisticsVisibleKey = "comicBook_statistics_visible";
const QString kHeaderKey = "header";
const QString kPrintHeaderOnTitlePageKey = "print_header_on_title";
const QString kFooterKey = "footer";
const QString kPrintFooterOnTitlePageKey = "print_footer_on_title";
} // namespace

class ComicBookInformationModel::Implementation
{
public:
    QString name;
    QString tagline;
    QString logline;
    bool titlePageVisible = true;
    bool synopsisVisible = true;
    bool comicBookTextVisible = true;
    bool comicBookStatisticsVisible = true;
    QString header;
    bool printHeaderOnTitlePage = true;
    QString footer;
    bool printFooterOnTitlePage = true;
};


// ****


ComicBookInformationModel::ComicBookInformationModel(QObject* _parent)
    : AbstractModel({ kDocumentKey, kNameKey, kLoglineKey, kHeaderKey, kPrintHeaderOnTitlePageKey,
                      kFooterKey, kPrintFooterOnTitlePageKey },
                    _parent)
    , d(new Implementation)
{
    connect(this, &ComicBookInformationModel::nameChanged, this,
            &ComicBookInformationModel::updateDocumentContent);
    connect(this, &ComicBookInformationModel::taglineChanged, this,
            &ComicBookInformationModel::updateDocumentContent);
    connect(this, &ComicBookInformationModel::loglineChanged, this,
            &ComicBookInformationModel::updateDocumentContent);
    connect(this, &ComicBookInformationModel::titlePageVisibleChanged, this,
            &ComicBookInformationModel::updateDocumentContent);
    connect(this, &ComicBookInformationModel::synopsisVisibleChanged, this,
            &ComicBookInformationModel::updateDocumentContent);
    connect(this, &ComicBookInformationModel::comicBookTextVisibleChanged, this,
            &ComicBookInformationModel::updateDocumentContent);
    connect(this, &ComicBookInformationModel::comicBookStatisticsVisibleChanged, this,
            &ComicBookInformationModel::updateDocumentContent);
    connect(this, &ComicBookInformationModel::headerChanged, this,
            &ComicBookInformationModel::updateDocumentContent);
    connect(this, &ComicBookInformationModel::printHeaderOnTitlePageChanged, this,
            &ComicBookInformationModel::updateDocumentContent);
    connect(this, &ComicBookInformationModel::footerChanged, this,
            &ComicBookInformationModel::updateDocumentContent);
    connect(this, &ComicBookInformationModel::printFooterOnTitlePageChanged, this,
            &ComicBookInformationModel::updateDocumentContent);
}

ComicBookInformationModel::~ComicBookInformationModel() = default;

const QString& ComicBookInformationModel::name() const
{
    return d->name;
}

void ComicBookInformationModel::setName(const QString& _name)
{
    if (d->name == _name) {
        return;
    }

    d->name = _name;
    emit nameChanged(d->name);
    emit documentNameChanged(d->name);
}

void ComicBookInformationModel::setDocumentName(const QString& _name)
{
    setName(_name);
}

const QString& ComicBookInformationModel::tagline() const
{
    return d->tagline;
}

void ComicBookInformationModel::setTagline(const QString& _tagline)
{
    if (d->tagline == _tagline) {
        return;
    }

    d->tagline = _tagline;
    emit taglineChanged(d->tagline);
}

const QString& ComicBookInformationModel::logline() const
{
    return d->logline;
}

void ComicBookInformationModel::setLogline(const QString& _logline)
{
    if (d->logline == _logline) {
        return;
    }

    d->logline = _logline;
    emit loglineChanged(d->logline);
}

bool ComicBookInformationModel::titlePageVisible() const
{
    return d->titlePageVisible;
}

void ComicBookInformationModel::setTitlePageVisible(bool _visible)
{
    if (d->titlePageVisible == _visible) {
        return;
    }

    d->titlePageVisible = _visible;
    emit titlePageVisibleChanged(d->titlePageVisible);
}

bool ComicBookInformationModel::synopsisVisible() const
{
    return d->synopsisVisible;
}

void ComicBookInformationModel::setSynopsisVisible(bool _visible)
{
    if (d->synopsisVisible == _visible) {
        return;
    }

    d->synopsisVisible = _visible;
    emit synopsisVisibleChanged(d->synopsisVisible);
}

bool ComicBookInformationModel::comicBookTextVisible() const
{
    return d->comicBookTextVisible;
}

void ComicBookInformationModel::setComicBookTextVisible(bool _visible)
{
    if (d->comicBookTextVisible == _visible) {
        return;
    }

    d->comicBookTextVisible = _visible;
    emit comicBookTextVisibleChanged(d->comicBookTextVisible);
}

bool ComicBookInformationModel::comicBookStatisticsVisible() const
{
    return d->comicBookStatisticsVisible;
}

void ComicBookInformationModel::setComicBookStatisticsVisible(bool _visible)
{
    if (d->comicBookStatisticsVisible == _visible) {
        return;
    }

    d->comicBookStatisticsVisible = _visible;
    emit comicBookStatisticsVisibleChanged(d->comicBookStatisticsVisible);
}

const QString& ComicBookInformationModel::header() const
{
    return d->header;
}

void ComicBookInformationModel::setHeader(const QString& _header)
{
    if (d->header == _header) {
        return;
    }

    d->header = _header;
    emit headerChanged(d->header);
}

bool ComicBookInformationModel::printHeaderOnTitlePage() const
{
    return d->printHeaderOnTitlePage;
}

void ComicBookInformationModel::setPrintHeaderOnTitlePage(bool _print)
{
    if (d->printHeaderOnTitlePage == _print) {
        return;
    }

    d->printHeaderOnTitlePage = _print;
    emit printHeaderOnTitlePageChanged(d->printHeaderOnTitlePage);
}

const QString& ComicBookInformationModel::footer() const
{
    return d->footer;
}

void ComicBookInformationModel::setFooter(const QString& _footer)
{
    if (d->footer == _footer) {
        return;
    }

    d->footer = _footer;
    emit footerChanged(d->footer);
}

bool ComicBookInformationModel::printFooterOnTitlePage() const
{
    return d->printFooterOnTitlePage;
}

void ComicBookInformationModel::setPrintFooterOnTitlePage(bool _print)
{
    if (d->printFooterOnTitlePage == _print) {
        return;
    }

    d->printFooterOnTitlePage = _print;
    emit printFooterOnTitlePageChanged(d->printFooterOnTitlePage);
}

void ComicBookInformationModel::initDocument()
{
    if (document() == nullptr) {
        return;
    }

    QDomDocument domDocument;
    const bool isContentValid = domDocument.setContent(document()->content());
    if (!isContentValid) {
        return;
    }

    const auto documentNode = domDocument.firstChildElement(kDocumentKey);
    d->name = documentNode.firstChildElement(kNameKey).text();
    d->tagline = documentNode.firstChildElement(kTaglineKey).text();
    d->logline = documentNode.firstChildElement(kLoglineKey).text();
    d->titlePageVisible = documentNode.firstChildElement(kTitlePageVisibleKey).text() == "true";
    d->synopsisVisible = documentNode.firstChildElement(kSynopsisVisibleKey).text() == "true";
    d->comicBookTextVisible
        = documentNode.firstChildElement(kComicBookTextVisibleKey).text() == "true";
    d->comicBookStatisticsVisible
        = documentNode.firstChildElement(kComicBookStatisticsVisibleKey).text() == "true";
    d->header = documentNode.firstChildElement(kHeaderKey).text();
    d->printHeaderOnTitlePage
        = documentNode.firstChildElement(kPrintHeaderOnTitlePageKey).text() == "true";
    d->footer = documentNode.firstChildElement(kFooterKey).text();
    d->printFooterOnTitlePage
        = documentNode.firstChildElement(kPrintFooterOnTitlePageKey).text() == "true";
}

void ComicBookInformationModel::clearDocument()
{
    QSignalBlocker signalBlocker(this);

    setName({});
    setLogline({});
    setTitlePageVisible({});
    setSynopsisVisible({});
    setComicBookTextVisible({});
    setComicBookStatisticsVisible({});
    setHeader({});
    setPrintHeaderOnTitlePage({});
    setFooter({});
    setPrintFooterOnTitlePage({});
}

QByteArray ComicBookInformationModel::toXml() const
{
    if (document() == nullptr) {
        return {};
    }

    QByteArray xml = "<?xml version=\"1.0\"?>\n";
    xml += QString("<%1 mime-type=\"%2\" version=\"1.0\">\n")
               .arg(kDocumentKey, Domain::mimeTypeFor(document()->type()))
               .toUtf8();
    xml += QString("<%1><![CDATA[%2]]></%1>\n").arg(kNameKey, d->name).toUtf8();
    xml += QString("<%1><![CDATA[%2]]></%1>\n").arg(kTaglineKey, d->tagline).toUtf8();
    xml += QString("<%1><![CDATA[%2]]></%1>\n").arg(kLoglineKey, d->logline).toUtf8();
    xml += QString("<%1><![CDATA[%2]]></%1>\n")
               .arg(kTitlePageVisibleKey, d->titlePageVisible ? "true" : "false")
               .toUtf8();
    xml += QString("<%1><![CDATA[%2]]></%1>\n")
               .arg(kSynopsisVisibleKey, d->synopsisVisible ? "true" : "false")
               .toUtf8();
    xml += QString("<%1><![CDATA[%2]]></%1>\n")
               .arg(kComicBookTextVisibleKey, d->comicBookTextVisible ? "true" : "false")
               .toUtf8();
    xml += QString("<%1><![CDATA[%2]]></%1>\n")
               .arg(kComicBookStatisticsVisibleKey,
                    d->comicBookStatisticsVisible ? "true" : "false")
               .toUtf8();
    xml += QString("<%1><![CDATA[%2]]></%1>\n").arg(kHeaderKey, d->header).toUtf8();
    xml += QString("<%1><![CDATA[%2]]></%1>\n")
               .arg(kPrintHeaderOnTitlePageKey, d->printHeaderOnTitlePage ? "true" : "false")
               .toUtf8();
    xml += QString("<%1><![CDATA[%2]]></%1>\n").arg(kFooterKey, d->footer).toUtf8();
    xml += QString("<%1><![CDATA[%2]]></%1>\n")
               .arg(kPrintFooterOnTitlePageKey, d->printFooterOnTitlePage ? "true" : "false")
               .toUtf8();
    xml += QString("</%1>").arg(kDocumentKey).toUtf8();
    return xml;
}

} // namespace BusinessLayer
