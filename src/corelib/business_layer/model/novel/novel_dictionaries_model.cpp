#include "novel_dictionaries_model.h"

#include <domain/document_object.h>
#include <utils/helpers/text_helper.h>

#include <QDomDocument>


namespace BusinessLayer {

namespace {

const QLatin1String kDocumentKey("document");
const QLatin1String kPageIntrosKey("scene_intros");
const QLatin1String kSceneTimesKey("scene_times");
const QLatin1String kCharacterExtensionsKey("character_extensions");
const QLatin1String kTransitionsKey("transitions");
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
    QVector<QString> sceneIntros;
    QVector<QString> sceneTimes;
    QVector<QString> characterExtensions;
    QVector<QString> transitions;

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
    : AbstractModel({ kDocumentKey, kPageIntrosKey, kSceneTimesKey, kStoryDaysKey,
                      kCharacterExtensionsKey, kTransitionsKey, kItemKey },
                    _parent)
    , d(new Implementation)
{
    connect(this, &NovelDictionariesModel::sceneIntrosChanged, this,
            &NovelDictionariesModel::updateDocumentContent);
    connect(this, &NovelDictionariesModel::sceneTimesChanged, this,
            &NovelDictionariesModel::updateDocumentContent);
    connect(this, &NovelDictionariesModel::storyDaysChanged, this,
            &NovelDictionariesModel::updateDocumentContent);
    connect(this, &NovelDictionariesModel::charactersExtensionsChanged, this,
            &NovelDictionariesModel::updateDocumentContent);
    connect(this, &NovelDictionariesModel::transitionsChanged, this,
            &NovelDictionariesModel::updateDocumentContent);
    connect(this, &NovelDictionariesModel::tagsChanged, this,
            &NovelDictionariesModel::updateDocumentContent);
}

NovelDictionariesModel::~NovelDictionariesModel() = default;

const QVector<QString>& NovelDictionariesModel::sceneIntros() const
{
    return d->sceneIntros;
}

void NovelDictionariesModel::addSceneIntro(const QString& _intro)
{
    const auto introCorrected = TextHelper::smartToUpper(_intro);
    if (d->sceneIntros.contains(introCorrected)) {
        return;
    }

    d->sceneIntros.append(introCorrected);
    emit sceneIntrosChanged();
}

void NovelDictionariesModel::setSceneIntro(int _index, const QString& _intro)
{
    const auto introCorrected = TextHelper::smartToUpper(_intro);
    if (d->sceneIntros.contains(introCorrected)) {
        return;
    }

    if (_index < 0 || d->sceneIntros.size() <= _index) {
        return;
    }

    d->sceneIntros[_index] = introCorrected;
    emit sceneIntrosChanged();
}

void NovelDictionariesModel::removeSceneIntro(int _index)
{
    if (_index < 0 || d->sceneIntros.size() <= _index) {
        return;
    }

    d->sceneIntros.removeAt(_index);
    emit sceneIntrosChanged();
}

const QVector<QString>& NovelDictionariesModel::sceneTimes() const
{
    return d->sceneTimes;
}

void NovelDictionariesModel::addSceneTime(const QString& _time)
{
    const auto timeCorrected = TextHelper::smartToUpper(_time);
    if (d->sceneTimes.contains(timeCorrected)) {
        return;
    }

    d->sceneTimes.append(timeCorrected);
    emit sceneTimesChanged();
}

void NovelDictionariesModel::setSceneTime(int _index, const QString& _time)
{
    const auto timeCorrected = TextHelper::smartToUpper(_time);
    if (d->sceneTimes.contains(timeCorrected)) {
        return;
    }

    if (_index < 0 || d->sceneTimes.size() <= _index) {
        return;
    }

    d->sceneTimes[_index] = timeCorrected;
    emit sceneTimesChanged();
}

void NovelDictionariesModel::removeSceneTime(int _index)
{
    if (_index < 0 || d->sceneTimes.size() <= _index) {
        return;
    }

    d->sceneTimes.removeAt(_index);
    emit sceneTimesChanged();
}

const QVector<QString>& NovelDictionariesModel::characterExtensions() const
{
    return d->characterExtensions;
}

void NovelDictionariesModel::addCharacterExtension(const QString& _extension)
{
    const auto extensionCorrected = TextHelper::smartToUpper(_extension);
    if (d->characterExtensions.contains(extensionCorrected)) {
        return;
    }

    d->characterExtensions.append(extensionCorrected);
    emit charactersExtensionsChanged();
}

void NovelDictionariesModel::setCharacterExtension(int _index, const QString& _extension)
{
    const auto extensionCorrected = TextHelper::smartToUpper(_extension);
    if (d->characterExtensions.contains(extensionCorrected)) {
        return;
    }

    if (_index < 0 || d->characterExtensions.size() <= _index) {
        return;
    }

    d->characterExtensions[_index] = extensionCorrected;
    emit charactersExtensionsChanged();
}

void NovelDictionariesModel::removeCharacterExtension(int _index)
{
    if (_index < 0 || d->characterExtensions.size() <= _index) {
        return;
    }

    d->characterExtensions.removeAt(_index);
    emit charactersExtensionsChanged();
}

const QVector<QString>& NovelDictionariesModel::transitions() const
{
    return d->transitions;
}

void NovelDictionariesModel::addTransition(const QString& _transition)
{
    const auto transitionsCorrected = TextHelper::smartToUpper(_transition);
    if (d->transitions.contains(transitionsCorrected)) {
        return;
    }

    d->transitions.append(transitionsCorrected);
    emit transitionsChanged();
}

void NovelDictionariesModel::setTransition(int _index, const QString& _transition)
{
    const auto transitionCorrected = TextHelper::smartToUpper(_transition);
    if (d->transitions.contains(transitionCorrected)) {
        return;
    }

    if (_index < 0 || d->transitions.size() <= _index) {
        return;
    }

    d->transitions[_index] = transitionCorrected;
    emit transitionsChanged();
}

void NovelDictionariesModel::removeTransition(int _index)
{
    if (_index < 0 || d->transitions.size() <= _index) {
        return;
    }

    d->transitions.removeAt(_index);
    emit transitionsChanged();
}

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
    const QVector<QString> defaultSceneIntros = {
        tr("INT."),
        tr("EXT."),
        tr("INT./EXT."),
        tr("EXT./INT."),
    };
    fillDictionary(kPageIntrosKey, defaultSceneIntros, d->sceneIntros);
    //
    const QVector<QString> defaultSceneTimes = {
        tr("DAY"),   tr("NIGHT"),         tr("MORNING"),    tr("AFTERNOON"),    tr("EVENING"),
        tr("LATER"), tr("MOMENTS LATER"), tr("CONTINUOUS"), tr("THE NEXT DAY"),
    };
    fillDictionary(kSceneTimesKey, defaultSceneTimes, d->sceneTimes);
    //
    const QVector<QString> defaultCharacterExtensions = {
        tr("V.O."), tr("O.S."), tr("O.C."), tr("SUBTITLE"), tr("CONT'D"),
    };
    fillDictionary(kCharacterExtensionsKey, defaultCharacterExtensions, d->characterExtensions);
    //
    const QVector<QString> defaultTransitions = {
        tr("CUT TO:"),       tr("FADE IN:"),     tr("FADE OUT"),
        tr("FADE TO:"),      tr("DISSOLVE TO:"), tr("BACK TO:"),
        tr("MATCH CUT TO:"), tr("JUMP CUT TO:"), tr("FADE TO BLACK"),
    };
    fillDictionary(kTransitionsKey, defaultTransitions, d->transitions);
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

    d->sceneIntros.clear();
    d->sceneTimes.clear();
    d->characterExtensions.clear();
    d->transitions.clear();
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
    auto writeDictionary = [&xml](const QString& _key, const QVector<QString>& _values) {
        xml += QString("<%1>\n").arg(_key).toUtf8();
        for (const auto& value : _values) {
            xml += QString("<%1><![CDATA[%2]]></%1>\n")
                       .arg(kItemKey, TextHelper::toHtmlEscaped(value))
                       .toUtf8();
        }
        xml += QString("</%1>\n").arg(_key).toUtf8();
    };
    writeDictionary(kPageIntrosKey, d->sceneIntros);
    writeDictionary(kSceneTimesKey, d->sceneTimes);
    writeDictionary(kCharacterExtensionsKey, d->characterExtensions);
    writeDictionary(kTransitionsKey, d->transitions);
    //
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
