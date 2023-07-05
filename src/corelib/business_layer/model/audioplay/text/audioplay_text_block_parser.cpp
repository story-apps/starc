#include "audioplay_text_block_parser.h"

#include <utils/helpers/text_helper.h>

#include <QRegularExpression>
#include <QString>
#include <QStringList>


namespace BusinessLayer {

namespace {
static const QRegularExpression s_rxState("[(](.*)");
}

QString AudioplayCharacterParser::name(const QString& _text)
{
    QString name = _text.trimmed();
    if (name.endsWith(':')) {
        name.chop(1);
    }
    return TextHelper::smartToUpper(name);
}

QString AudioplayCharacterParser::extension(const QString& _text)
{
    //
    // В блоке персонажа так же могут быть указания, что он говорит за кадром и т.п.
    // эти указания даются в скобках, они нам как раз и нужны
    //

    QRegularExpressionMatch match = s_rxState.match(_text);
    QString state;
    if (match.hasMatch()) {
        state = match.captured(0);
        state = state.remove("(").remove(")");
    }
    return TextHelper::smartToUpper(state).simplified();
}

// ****

AudioplaySceneHeadingParser::Section AudioplaySceneHeadingParser::section(const QString& _text)
{
    AudioplaySceneHeadingParser::Section section = SectionUndefined;

    if (_text.split(" -- ").count() >= 2 || _text.split(" - ").count() >= 2) {
        section = SectionSceneTime;
    } else {
        const int splitDotCount = _text.split(". ").count();
        if (splitDotCount == 1) {
            section = SectionSceneIntro;
        } else {
            section = SectionLocation;
        }
    }

    return section;
}

QString AudioplaySceneHeadingParser::sceneIntro(const QString& _text)
{
    if (!_text.contains(". ")) {
        return TextHelper::smartToUpper(_text);
    }

    const auto placeName = _text.split(". ").constFirst();
    return TextHelper::smartToUpper(placeName).simplified() + ".";
}

QString AudioplaySceneHeadingParser::location(const QString& _text, bool _force)
{
    QString locationName;

    if (_text.split(". ").count() > 1) {
        locationName = _text.mid(_text.indexOf(". ") + 2);
    } else {
        locationName = _text;
    }

    if (!_force) {
        if (auto locationParts = locationName.split(" -- "); locationParts.size() > 1) {
            locationName = locationName.remove(" -- " + locationParts.constLast());
        } else {
            const QString suffix = locationName.split(" - ").constLast();
            locationName = locationName.remove(" - " + suffix);
        }
        locationName = locationName.simplified();
    }

    return TextHelper::smartToUpper(locationName).simplified();
}

QString AudioplaySceneHeadingParser::sceneTime(const QString& _text)
{
    QString timeName;

    if (_text.split(" -- ").count() >= 2) {
        timeName = _text.split(" -- ").last().simplified();
    } else if (_text.split(" - ").count() >= 2) {
        timeName = _text.split(" - ").last().simplified();
    }

    return TextHelper::smartToUpper(timeName).simplified();
}

} // namespace BusinessLayer
