#ifndef QGUMBODOCUMENT_H
#define QGUMBODOCUMENT_H

#include <QByteArray>
#include "gumbo-parser/src/gumbo.h"

class QString;
class QGumboNode;

class QGumboDocument
{
public:
    static QGumboDocument parse(const QString& htmlText);
    static QGumboDocument parse(const char* utf8data);
    static QGumboDocument parse(QByteArray utf8data);

public:
    ~QGumboDocument();
    QGumboDocument(QGumboDocument&&);

    QGumboNode rootNode() const;

private:
    QGumboDocument(QByteArray);

    QGumboDocument(const QGumboDocument&) = delete;
    QGumboDocument& operator=(const QGumboDocument&) = delete;

    GumboOutput *gumboOutput_ = nullptr;
    const GumboOptions *options_ = nullptr;
    QByteArray sourceData_;
};

#endif // QGUMBODOCUMENT_H
