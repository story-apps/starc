#include "screenplay_text_block_parser.h"

#include <utils/helpers/text_helper.h>

#include <QRegularExpression>
#include <QString>
#include <QStringList>


namespace BusinessLayer {

namespace {
static const QRegularExpression s_rxState("[(](.*)");
}

ScreenplayCharacterParser::Section ScreenplayCharacterParser::section(const QString& _text)
{
    ScreenplayCharacterParser::Section section = SectionUndefined;

    if (_text.split("(").count() == 2) {
        section = SectionExtension;
    } else {
        section = SectionName;
    }

    return section;
}

QString ScreenplayCharacterParser::name(const QString& _text)
{
    //
    // В блоке персонажа так же могут быть указания, что он говорит за кадром и т.п.
    // эти указания даются в скобках
    //

    QString name = _text;
    return TextHelper::smartToUpper(name.remove(s_rxState).simplified());
}

QString ScreenplayCharacterParser::extension(const QString& _text)
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

ScreenplaySceneHeadingParser::Section ScreenplaySceneHeadingParser::section(const QString& _text)
{
    ScreenplaySceneHeadingParser::Section section = SectionUndefined;

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

QString ScreenplaySceneHeadingParser::sceneIntro(const QString& _text)
{
    if (!_text.contains(". ")) {
        return TextHelper::smartToUpper(_text);
    }

    const auto placeName = _text.split(". ").constFirst();
    return TextHelper::smartToUpper(placeName).simplified() + ".";
}

QString ScreenplaySceneHeadingParser::location(const QString& _text, bool _force)
{
    QString locationName;

    if (_text.split(". ").count() > 1) {
        locationName = _text.mid(_text.indexOf(". ") + 2);
        if (!_force) {
            if (auto locationParts = locationName.split(" -- "); locationParts.size() > 1) {
                locationName = locationName.remove(" -- " + locationParts.constLast());
            } else {
                const QString suffix = locationName.split(" - ").constLast();
                locationName = locationName.remove(" - " + suffix);
            }
            locationName = locationName.simplified();
        }
    }

    return TextHelper::smartToUpper(locationName).simplified();
}

QString ScreenplaySceneHeadingParser::sceneTime(const QString& _text)
{
    QString timeName;

    if (_text.split(" -- ").count() >= 2) {
        timeName = _text.split(" -- ").last().simplified();
    } else if (_text.split(" - ").count() >= 2) {
        timeName = _text.split(" - ").last().simplified();
    }

    return TextHelper::smartToUpper(timeName).simplified();
}

// ****

QStringList ScreenplaySceneCharactersParser::characters(const QString& _text)
{
    const auto characters = _text.simplified();
    auto charactersList = characters.split(",", Qt::SkipEmptyParts);

    //
    // Убираем символы пробелов
    //
    for (int index = 0; index < charactersList.size(); ++index) {
        charactersList[index] = ScreenplayCharacterParser::name(charactersList[index].simplified());
    }

    return charactersList;
}

} // namespace BusinessLayer
