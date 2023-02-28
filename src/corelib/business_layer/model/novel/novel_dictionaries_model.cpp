#include "novel_dictionaries_model.h"

#include <domain/document_object.h>
#include <utils/helpers/text_helper.h>

#include <QDomDocument>


namespace BusinessLayer {

namespace {

const QLatin1String kDocumentKey("document");
const QLatin1String kStoryDaysKey("story_days");
const QLatin1String kTagsKey("tags");
const QLatin1String kItemKey("v");
const QLatin1String kItemColorAttribute("color");
const QLatin1String kItemCountAttribute("count");

struct ColorsComparator {
    bool operator()(const QPair<QString, QColor>& _lhs, const QPair<QString, QColor>& _rhs) const
    {
        return _lhs.first < _rhs.first && _lhs.second.name() < _rhs.second.name();
    }
};

} // namespace

class NovelDictionariesModel::Implementation
{
public:
    /**
     * @brief Дни истории <название, кол-во>
     */
    std::map<QString, int> storyDays;

    /**
     * @brief Тэги карточек <тэг, кол-вл>
     */
    std::map<QPair<QString, QColor>, int, ColorsComparator> tags;
};


// ****


NovelDictionariesModel::NovelDictionariesModel(QObject* _parent)
    : AbstractModel({ kDocumentKey, kStoryDaysKey, kItemKey }, _parent)
    , d(new Implementation)
{
    connect(this, &NovelDictionariesModel::storyDaysChanged, this,
            &NovelDictionariesModel::updateDocumentContent);
    connect(this, &NovelDictionariesModel::tagsChanged, this,
            &NovelDictionariesModel::updateDocumentContent);
}

NovelDictionariesModel::~NovelDictionariesModel() = default;

QVector<QString> NovelDictionariesModel::storyDays() const
{
    QVector<QString> storyDays;
    for (const auto& [storyDay, count] : d->storyDays) {
        storyDays.append(storyDay);
    }
    return storyDays;
}

void NovelDictionariesModel::addStoryDay(const QString& _day)
{
    if (_day.isEmpty()) {
        return;
    }

    auto iter = d->storyDays.find(_day);
    if (iter == d->storyDays.end()) {
        d->storyDays.emplace(_day, 1);
    } else {
        ++d->storyDays[iter->first];
    }
    emit storyDaysChanged();
}

void NovelDictionariesModel::removeStoryDay(const QString& _day)
{
    auto iter = d->storyDays.find(_day);
    if (iter == d->storyDays.end()) {
        return;
    }

    if (iter->second == 1) {
        d->storyDays.erase(iter);
    } else {
        --d->storyDays[iter->first];
    }
    emit storyDaysChanged();
}

QVector<QPair<QString, QColor>> NovelDictionariesModel::tags() const
{
    QVector<QPair<QString, QColor>> tags;
    for (const auto& [tag, count] : d->tags) {
        tags.append(tag);
    }
    return tags;
}

void NovelDictionariesModel::addTags(const QVector<QPair<QString, QColor>>& _tags)
{
    for (const auto& tag : _tags) {
        if (tag.first.isEmpty()) {
            continue;
        }

        auto iter = d->tags.find(tag);
        if (iter == d->tags.end()) {
            d->tags.emplace(tag, 1);
        } else {
            ++d->tags[iter->first];
        }
    }
    emit tagsChanged();
}

void NovelDictionariesModel::removeTags(const QVector<QPair<QString, QColor>>& _tags)
{
    for (const auto& tag : _tags) {
        auto iter = d->tags.find(tag);
        if (iter == d->tags.end()) {
            continue;
        }

        if (iter->second == 1) {
            d->tags.erase(iter);
        } else {
            --d->tags[iter->first];
        }
    }
    emit tagsChanged();
}

void NovelDictionariesModel::initDocument()
{
    if (document() == nullptr) {
        return;
    }

    QDomDocument domDocument;
    domDocument.setContent(document()->content());
    const auto documentNode = domDocument.firstChildElement(kDocumentKey);
    auto fillDictionary = [documentNode](const QString& _key, const QVector<QString>& _defaultItems,
                                         QVector<QString>& _dictionary) {
        const auto dictionaryNode = documentNode.firstChildElement(_key);
        auto itemNode = dictionaryNode.firstChildElement();
        while (!itemNode.isNull()) {
            _dictionary.append(TextHelper::fromHtmlEscaped(itemNode.text()));
            itemNode = itemNode.nextSiblingElement();
        }

        if (_dictionary.isEmpty()) {
            _dictionary.append(_defaultItems);
        }
    };
    //
    {
        const auto storyDaysNode = documentNode.firstChildElement(kStoryDaysKey);
        auto storyDayNode = storyDaysNode.firstChildElement();
        while (!storyDayNode.isNull()) {
            const auto storyDay = TextHelper::fromHtmlEscaped(storyDayNode.text());
            const auto count = storyDayNode.hasAttribute(kItemCountAttribute)
                ? storyDayNode.attributeNode(kItemCountAttribute).value().toInt()
                : 1;
            d->storyDays.emplace(storyDay, count);
            storyDayNode = storyDayNode.nextSiblingElement();
        }
    }
    //
    {
        const auto tagsNode = documentNode.firstChildElement(kTagsKey);
        auto tagNode = tagsNode.firstChildElement();
        while (!tagNode.isNull()) {
            const auto tag = qMakePair(TextHelper::fromHtmlEscaped(tagNode.text()),
                                       tagNode.attributeNode(kItemColorAttribute).value());
            const auto count = tagNode.hasAttribute(kItemCountAttribute)
                ? tagNode.attributeNode(kItemCountAttribute).value().toInt()
                : 1;
            d->tags.emplace(tag, count);
            tagNode = tagNode.nextSiblingElement();
        }
    }
}

void NovelDictionariesModel::clearDocument()
{
    QSignalBlocker signalBlocker(this);

    d->storyDays.clear();
    d->tags.clear();
}

QByteArray NovelDictionariesModel::toXml() const
{
    if (document() == nullptr) {
        return {};
    }

    QByteArray xml = "<?xml version=\"1.0\"?>\n";
    xml += QString("<%1 mime-type=\"%2\" version=\"1.0\">\n")
               .arg(kDocumentKey, Domain::mimeTypeFor(document()->type()))
               .toUtf8();
    xml += QString("<%1>\n").arg(kStoryDaysKey).toUtf8();
    for (const auto& [storyDay, count] : std::as_const(d->storyDays)) {
        xml += QString("<%1 %2=\"%3\"><![CDATA[%4]]></%1>\n")
                   .arg(kItemKey, kItemCountAttribute, QString::number(count),
                        TextHelper::toHtmlEscaped(storyDay))
                   .toUtf8();
    }
    xml += QString("</%1>\n").arg(kStoryDaysKey).toUtf8();
    //
    xml += QString("<%1>\n").arg(kTagsKey).toUtf8();
    for (const auto& [tag, count] : std::as_const(d->tags)) {
        xml += QString("<%1 %2=\"%3\" %4=\"%5\"><![CDATA[%6]]></%1>\n")
                   .arg(kItemKey, kItemColorAttribute, tag.second.name(), kItemCountAttribute,
                        QString::number(count), TextHelper::toHtmlEscaped(tag.first))
                   .toUtf8();
    }
    xml += QString("</%1>\n").arg(kTagsKey).toUtf8();

    xml += QString("</%1>").arg(kDocumentKey).toUtf8();
    return xml;
}

} // namespace BusinessLayer
