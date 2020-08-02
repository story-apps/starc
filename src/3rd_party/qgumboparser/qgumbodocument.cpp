#include <QByteArray>
#include <QString>
#include <stdexcept>
#include "qgumbodocument.h"
#include "qgumbonode.h"

QGumboDocument QGumboDocument::parse(const char *utf8data)
{
    if (!utf8data)
        throw std::invalid_argument("arg should point to utf8 encoded string and can't be nullptr");
    return QGumboDocument(QByteArray(utf8data));
}

QGumboDocument QGumboDocument::parse(const QString& html)
{
    QByteArray array = html.toUtf8();
    return QGumboDocument(array);
}

QGumboDocument QGumboDocument::parse(QByteArray data)
{
    return QGumboDocument(data);
}

QGumboDocument::QGumboDocument(QByteArray arr) :
    options_(&kGumboDefaultOptions),
    sourceData_(arr)
{
    gumboOutput_ = gumbo_parse_with_options(options_,
                                            sourceData_.constData(),
                                            sourceData_.length());
    if (!gumboOutput_)
        throw std::runtime_error("the data can't be parsed");
}

QGumboDocument::~QGumboDocument()
{
    if (gumboOutput_)
        gumbo_destroy_output(options_, gumboOutput_);
    if (options_ != &kGumboDefaultOptions)
        delete options_;
}

QGumboDocument::QGumboDocument(QGumboDocument &&source) :
    gumboOutput_(source.gumboOutput_),
    options_(source.options_),
    sourceData_(source.sourceData_)
{
    source.gumboOutput_ = nullptr;
    source.options_ = nullptr;
}

QGumboNode QGumboDocument::rootNode() const
{
    return QGumboNode(gumboOutput_->root);
}
