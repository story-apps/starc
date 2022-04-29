#include "screenplay_dictionaries_model.h"

#include <domain/document_object.h>
#include <utils/helpers/text_helper.h>

#include <QDomDocument>


namespace BusinessLayer {

namespace {
const QString kDocumentKey = "document";
const QString kPageIntrosKey = "scene_intros";
const QString kSceneTimesKey = "scene_times";
const QString kStoryDaysKey = "story_days";
const QString kCharacterExtensionsKey = "character_extensions";
const QString kPanelIntrosKey = "transitions";
const QString kItemKey = "v";
} // namespace

class ScreenplayDictionariesModel::Implementation
{
public:
    QVector<QString> sceneIntros;
    QVector<QString> sceneTimes;
    QVector<QString> storyDays;
    QVector<QString> characterExtensions;
    QVector<QString> transitions;
};


// ****


ScreenplayDictionariesModel::ScreenplayDictionariesModel(QObject* _parent)
    : AbstractModel({ kDocumentKey, kPageIntrosKey, kSceneTimesKey, kStoryDaysKey,
                      kCharacterExtensionsKey, kPanelIntrosKey, kItemKey },
                    _parent)
    , d(new Implementation)
{
    connect(this, &ScreenplayDictionariesModel::sceneIntrosChanged, this,
            &ScreenplayDictionariesModel::updateDocumentContent);
    connect(this, &ScreenplayDictionariesModel::sceneTimesChanged, this,
            &ScreenplayDictionariesModel::updateDocumentContent);
    connect(this, &ScreenplayDictionariesModel::storyDaysChanged, this,
            &ScreenplayDictionariesModel::updateDocumentContent);
    connect(this, &ScreenplayDictionariesModel::charactersExtensionsChanged, this,
            &ScreenplayDictionariesModel::updateDocumentContent);
    connect(this, &ScreenplayDictionariesModel::transitionsChanged, this,
            &ScreenplayDictionariesModel::updateDocumentContent);
}

ScreenplayDictionariesModel::~ScreenplayDictionariesModel() = default;

const QVector<QString>& ScreenplayDictionariesModel::sceneIntros() const
{
    return d->sceneIntros;
}

void ScreenplayDictionariesModel::addSceneIntro(const QString& _intro)
{
    const auto introCorrected = TextHelper::smartToUpper(_intro);
    if (d->sceneIntros.contains(introCorrected)) {
        return;
    }

    d->sceneIntros.append(introCorrected);
    emit sceneIntrosChanged();
}

void ScreenplayDictionariesModel::setSceneIntro(int _index, const QString& _intro)
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

void ScreenplayDictionariesModel::removeSceneIntro(int _index)
{
    if (_index < 0 || d->sceneIntros.size() <= _index) {
        return;
    }

    d->sceneIntros.removeAt(_index);
    emit sceneIntrosChanged();
}

const QVector<QString>& ScreenplayDictionariesModel::sceneTimes() const
{
    return d->sceneTimes;
}

void ScreenplayDictionariesModel::addSceneTime(const QString& _time)
{
    const auto timeCorrected = TextHelper::smartToUpper(_time);
    if (d->sceneTimes.contains(timeCorrected)) {
        return;
    }

    d->sceneTimes.append(timeCorrected);
    emit sceneTimesChanged();
}

void ScreenplayDictionariesModel::setSceneTime(int _index, const QString& _time)
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

void ScreenplayDictionariesModel::removeSceneTime(int _index)
{
    if (_index < 0 || d->sceneTimes.size() <= _index) {
        return;
    }

    d->sceneTimes.removeAt(_index);
    emit sceneTimesChanged();
}

const QVector<QString>& ScreenplayDictionariesModel::storyDays() const
{
    return d->storyDays;
}

void ScreenplayDictionariesModel::addStoryDay(const QString& _day)
{
    const auto dayCorrected = TextHelper::smartToUpper(_day);
    if (d->storyDays.contains(dayCorrected)) {
        return;
    }

    d->storyDays.append(dayCorrected);
    emit storyDaysChanged();
}

void ScreenplayDictionariesModel::setStoryDay(int _index, const QString& _day)
{
    const auto dayCorrected = TextHelper::smartToUpper(_day);
    if (d->storyDays.contains(dayCorrected)) {
        return;
    }

    if (_index < 0 || d->storyDays.size() <= _index) {
        return;
    }

    d->storyDays[_index] = dayCorrected;
    emit storyDaysChanged();
}

void ScreenplayDictionariesModel::removeStoryDay(int _index)
{
    if (_index < 0 || d->storyDays.size() <= _index) {
        return;
    }

    d->storyDays.removeAt(_index);
    emit storyDaysChanged();
}

const QVector<QString>& ScreenplayDictionariesModel::characterExtensions() const
{
    return d->characterExtensions;
}

void ScreenplayDictionariesModel::addCharacterExtension(const QString& _extension)
{
    const auto extensionCorrected = TextHelper::smartToUpper(_extension);
    if (d->characterExtensions.contains(extensionCorrected)) {
        return;
    }

    d->characterExtensions.append(extensionCorrected);
    emit charactersExtensionsChanged();
}

void ScreenplayDictionariesModel::setCharacterExtension(int _index, const QString& _extension)
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

void ScreenplayDictionariesModel::removeCharacterExtension(int _index)
{
    if (_index < 0 || d->characterExtensions.size() <= _index) {
        return;
    }

    d->characterExtensions.removeAt(_index);
    emit charactersExtensionsChanged();
}

const QVector<QString>& ScreenplayDictionariesModel::transitions() const
{
    return d->transitions;
}

void ScreenplayDictionariesModel::addTransition(const QString& _transition)
{
    const auto transitionsCorrected = TextHelper::smartToUpper(_transition);
    if (d->transitions.contains(transitionsCorrected)) {
        return;
    }

    d->transitions.append(transitionsCorrected);
    emit transitionsChanged();
}

void ScreenplayDictionariesModel::setTransition(int _index, const QString& _transition)
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

void ScreenplayDictionariesModel::removeTransition(int _index)
{
    if (_index < 0 || d->transitions.size() <= _index) {
        return;
    }

    d->transitions.removeAt(_index);
    emit transitionsChanged();
}

void ScreenplayDictionariesModel::initDocument()
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
    fillDictionary(kStoryDaysKey, {}, d->storyDays);
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
    fillDictionary(kPanelIntrosKey, defaultTransitions, d->transitions);
}

void ScreenplayDictionariesModel::clearDocument()
{
    QSignalBlocker signalBlocker(this);

    d->sceneIntros.clear();
    d->sceneTimes.clear();
    d->characterExtensions.clear();
    d->transitions.clear();
}

QByteArray ScreenplayDictionariesModel::toXml() const
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
    writeDictionary(kStoryDaysKey, d->storyDays);
    writeDictionary(kCharacterExtensionsKey, d->characterExtensions);
    writeDictionary(kPanelIntrosKey, d->transitions);
    xml += QString("</%1>").arg(kDocumentKey).toUtf8();
    return xml;
}

} // namespace BusinessLayer
