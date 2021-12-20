#include "screenplay_text_block_parser.h"

#include <business_layer/templates/screenplay_template.h>
#include <business_layer/templates/templates_facade.h>
#include <utils/helpers/text_helper.h>

#include <QRegularExpression>
#include <QString>
#include <QStringList>


namespace BusinessLayer {

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
    return TextHelper::smartToUpper(name.remove(QRegularExpression("[(](.*)")).simplified());
}

QString ScreenplayCharacterParser::extension(const QString& _text)
{
    //
    // В блоке персонажа так же могут быть указания, что он говорит за кадром и т.п.
    // эти указания даются в скобках, они нам как раз и нужны
    //

    const QRegularExpression rx_state("[(](.*)");
    QRegularExpressionMatch match = rx_state.match(_text);
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

    if (_text.split(", ").count() == 2) {
        section = SectionStoryDay;
    } else if (_text.split(" - ").count() >= 2) {
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
    QString placeName;

    if (_text.split(". ").count() > 0) {
        placeName = _text.split(". ").value(0);
    }

    return TextHelper::smartToUpper(placeName).simplified();
}

QString ScreenplaySceneHeadingParser::location(const QString& _text, bool _force)
{
    QString locationName;

    if (_text.split(". ").count() > 1) {
        locationName = _text.mid(_text.indexOf(". ") + 2);
        if (!_force) {
            const QString suffix = locationName.split(" - ").last();
            locationName = locationName.remove(" - " + suffix);
            locationName = locationName.simplified();
        }
    }

    return TextHelper::smartToUpper(locationName).simplified();
}

QString ScreenplaySceneHeadingParser::storyDay(const QString& _text)
{
    QString scenarioDayName;

    if (_text.split(", ").count() == 2) {
        scenarioDayName = _text.split(", ").last();
    }

    return TextHelper::smartToUpper(scenarioDayName).simplified();
}

QString ScreenplaySceneHeadingParser::sceneTime(const QString& _text)
{
    QString timeName;

    if (_text.split(" - ").count() >= 2) {
        timeName = _text.split(" - ").last().split(",").first();
        timeName = timeName.simplified();
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
        charactersList[index] = TextHelper::smartToUpper(charactersList[index].simplified());
    }

    return charactersList;
}

} // namespace BusinessLayer
