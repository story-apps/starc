#ifndef QGUMBONODE_H
#define QGUMBONODE_H

#include <vector>
#include <functional>
#include "gumbo-parser/src/gumbo.h"
#include "HtmlTag.h"

class QString;
class QGumboNode;
class QGumboAttribute;
class QGumboDocument;
class QStringList;

typedef std::vector<QGumboNode> 		QGumboNodes;
typedef std::vector<QGumboAttribute> 	QGumboAttributes;

class QGumboNode
{
public:
    QGumboNode(const QGumboNode&) = default;
#ifndef Q_OS_WIN
    QGumboNode(QGumboNode&&) noexcept = default;
#endif
    QGumboNode& operator=(const QGumboNode&) = default;

    HtmlTag tag() const;
    QString tagName() const;
    QString nodeName() const;

    QString id() const;
    QStringList classList() const;

    QGumboNodes getElementById(const QString&) const;
    QGumboNodes getElementsByTagName(HtmlTag) const;
    QGumboNodes getElementsByClassName(const QString&) const;
    QGumboNodes childNodes() const;
    QGumboNodes children() const;

    int childElementCount() const;

    bool isElement() const;
    bool hasAttribute(const QString&) const;

    QString innerText() const;
    QString outerHtml() const;
    QString getAttribute(const QString&) const;

    int childStartPosition(const QGumboNode&) const;
    int rawStartPosition() const;

    QGumboAttributes allAttributes() const;

    void forEach(std::function<void(const QGumboNode&)>) const;

    explicit operator bool() const;

private:
    QGumboNode();
    QGumboNode(GumboNode* node);

    friend class QGumboDocument;
private:
    GumboNode* ptr_;
};

#endif // QGUMBONODE_H
