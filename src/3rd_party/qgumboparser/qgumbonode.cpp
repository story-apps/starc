#include <cstring>
#include <sstream>
#include <QString>
#include <QDebug>
#include <QStringList>
#include "qgumbonode.h"
#include "qgumboattribute.h"

namespace {

const char* const ID_ATTRIBUTE 		= "id";
const char* const CLASS_ATTRIBUTE 	= "class";

template<typename TFunctor>
bool iterateTree(GumboNode* node, TFunctor& functor)
{
    if (!node || node->type != GUMBO_NODE_ELEMENT)
        return false;

    if (functor(node))
        return true;

    for (uint i = 0; i < node->v.element.children.length; ++i) {
        if (iterateTree(static_cast<GumboNode*>(node->v.element.children.data[i]), functor))
            return true;
    }

    return false;
}

template<typename TFunctor>
bool iterateChildren(GumboNode* node, TFunctor& functor)
{
    if (node->type != GUMBO_NODE_ELEMENT)
        return false;

    GumboVector& vec = node->v.element.children;

    for (uint i = 0, e = vec.length; i < e; ++i) {
        GumboNode* node = static_cast<GumboNode*>(vec.data[i]);
        if (functor(node))
            return true;
    }

    return false;
}

} /* namespace */

QGumboNode::QGumboNode()
{
}

QGumboNode::QGumboNode(GumboNode* node) :
    ptr_(node)
{
    if (!ptr_)
        throw std::runtime_error("can't create Node from nullptr");
}

QGumboNodes QGumboNode::getElementById(const QString& nodeId) const
{
    Q_ASSERT(ptr_);

    if (nodeId.isEmpty())
        throw std::invalid_argument("id can't be empty string");

    QGumboNodes nodes;

    auto functor = [&nodes, &nodeId] (GumboNode* node) {
        GumboAttribute* attr = gumbo_get_attribute(&node->v.element.attributes, ID_ATTRIBUTE);
        if (attr) {
            const QString value = QString::fromUtf8(attr->value);
            if (value.compare(nodeId, Qt::CaseInsensitive) == 0) {
                nodes.emplace_back(QGumboNode(node));
                return true;
            }
        }
        return false;
    };

    iterateTree(ptr_, functor);

    return nodes;
}

QGumboNodes QGumboNode::getElementsByTagName(HtmlTag tag) const
{
    Q_ASSERT(ptr_);

    GumboTag tag_ = static_cast<GumboTag>(tag);
    QGumboNodes nodes;

    auto functor = [&nodes, tag_](GumboNode* node) {
        if (node->v.element.tag == tag_) {
            nodes.emplace_back(QGumboNode(node));
        }
        return false;
    };

    iterateTree(ptr_, functor);

    return nodes;
}

QGumboNodes QGumboNode::getElementsByClassName(const QString& name) const
{
    Q_ASSERT(ptr_);

    if (name.isEmpty())
        throw std::invalid_argument("class name can't be empty string");

    QGumboNodes nodes;

    auto functor = [&nodes, &name] (GumboNode* node) {
        GumboAttribute* attr = gumbo_get_attribute(&node->v.element.attributes, CLASS_ATTRIBUTE);
        if (attr) {
            const QString value = QString::fromUtf8(attr->value);
            const auto parts =
                    value.split(QChar(' '), Qt::SkipEmptyParts, Qt::CaseInsensitive);

            for (const auto& part: parts) {
                if (part.compare(name, Qt::CaseInsensitive) == 0) {
                    nodes.emplace_back(QGumboNode(node));
                    break;
                }
            }
        }
        return false;
    };

    iterateTree(ptr_, functor);

    return nodes;
}

QGumboNodes QGumboNode::childNodes() const
{
    Q_ASSERT(ptr_);

    QGumboNodes nodes;

    auto functor = [&nodes] (GumboNode* node) {
        nodes.emplace_back(QGumboNode(node));
        return false;
    };

    iterateChildren(ptr_, functor);

    return nodes;
}

QGumboNodes QGumboNode::children() const
{
    Q_ASSERT(ptr_);

    QGumboNodes nodes;

    auto functor = [&nodes] (GumboNode* node) {
        if (node->type == GUMBO_NODE_ELEMENT) {
            nodes.emplace_back(QGumboNode(node));
        }
        return false;
    };

    iterateChildren(ptr_, functor);

    return nodes;
}

int QGumboNode::childElementCount() const
{
    Q_ASSERT(ptr_);

    int count = 0;

    auto functor = [&count] (GumboNode* node) {
        if (node->type == GUMBO_NODE_ELEMENT)
            ++count;
        return false;
    };

    iterateChildren(ptr_, functor);

    return count;
}

QString QGumboNode::innerText() const
{
    Q_ASSERT(ptr_);

    QString text;

    auto functor = [&text] (GumboNode* node) {
        if (node->type == GUMBO_NODE_TEXT) {
            text += QString::fromUtf8(node->v.text.text);
        } else if (node->type == GUMBO_NODE_WHITESPACE) {
            text += " ";
        }
        return false;
    };

    iterateChildren(ptr_, functor);

    return text;
}

QString QGumboNode::outerHtml() const
{
    Q_ASSERT(ptr_);

    QString text;
    switch (ptr_->type) {
    case GUMBO_NODE_DOCUMENT: {
        throw std::runtime_error("invalid node type");
    }
    case GUMBO_NODE_ELEMENT: {
        const auto& elem = ptr_->v.element;
        const auto& tag = elem.original_tag;
        if (tag.data && tag.length) {
            int lenght = elem.end_pos.offset - elem.start_pos.offset + elem.original_end_tag.length;
            Q_ASSERT(lenght > 0);
            text = QString::fromUtf8(tag.data, lenght);
        }
        break;
    }
    default: {
        const auto& str = ptr_->v.text.original_text;
        text = QString::fromUtf8(str.data, str.length);
    }}
    return text;
}

HtmlTag QGumboNode::tag() const
{
    if (isElement())
        return HtmlTag(ptr_->v.element.tag);

    return HtmlTag::UNKNOWN;
}

QString QGumboNode::tagName() const
{
    Q_ASSERT(ptr_);
    GumboTag tag = ptr_->v.element.tag;
    return QString::fromUtf8(gumbo_normalized_tagname(tag));
}

QString QGumboNode::nodeName() const
{
    return tagName();
}

QString QGumboNode::id() const
{
    GumboAttribute* attr = gumbo_get_attribute(&ptr_->v.element.attributes, ID_ATTRIBUTE);
    if (attr)
        return QString::fromUtf8(attr->value);

    return QString();
}

QStringList QGumboNode::classList() const
{
    GumboAttribute* attr = gumbo_get_attribute(&ptr_->v.element.attributes, CLASS_ATTRIBUTE);
    if (attr) {
        QString values = QString::fromUtf8(attr->value);
        return values.split(" ", Qt::SkipEmptyParts, Qt::CaseInsensitive);
    }

    return QStringList();
}

bool QGumboNode::isElement() const
{
    return ptr_->type == GUMBO_NODE_ELEMENT;
}

bool QGumboNode::hasAttribute(const QString& name) const
{
    if (name.isEmpty())
        throw std::invalid_argument("attribute can't be empty string");

    GumboAttribute* attr = gumbo_get_attribute(&ptr_->v.element.attributes,
                                               name.toUtf8().constData());
    return attr != nullptr;
}

QString QGumboNode::getAttribute(const QString& attrName) const
{
    if (attrName.isEmpty())
        throw std::invalid_argument("attribute name can't be empty string");

    GumboAttribute* attr = gumbo_get_attribute(&ptr_->v.element.attributes,
                                               attrName.toUtf8().constData());
    if (attr)
        return QString::fromUtf8(attr->value);

    return QString();
}

int QGumboNode::childStartPosition(const QGumboNode& _child) const
{
    int position = _child.rawStartPosition() - rawStartPosition() - ptr_->v.element.original_tag.length;
    for (const auto& child : children()) {
        if (child.ptr_ == _child.ptr_) {
            break;
        }

        position -= child.outerHtml().length() - child.innerText().length();
    }
    return position;
}

int QGumboNode::rawStartPosition() const
{
    return ptr_->v.element.start_pos.offset;
}

QGumboAttributes QGumboNode::allAttributes() const
{
    Q_ASSERT(ptr_);

    QGumboAttributes attrs;

    for (uint i = 0, len = ptr_->v.element.attributes.length; i < len; ++i) {
        GumboAttribute* attr =
                static_cast<GumboAttribute*>(ptr_->v.element.attributes.data[i]);
        attrs.emplace_back(QGumboAttribute(attr->name, attr->value));
    }
    return attrs;
}


void QGumboNode::forEach(std::function<void(const QGumboNode&)> func) const
{
    Q_ASSERT(ptr_);

    auto functor = [&func](GumboNode* node) {
        func(QGumboNode(node));
        return false;
    };

    iterateTree(ptr_, functor);
}

QGumboNode::operator bool() const
{
    return ptr_;
}
