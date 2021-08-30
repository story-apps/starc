#include "comic_book_text_block_parser.h"

#include <business_layer/templates/screenplay_template.h>
#include <business_layer/templates/templates_facade.h>
#include <utils/helpers/text_helper.h>

#include <QRegularExpression>
#include <QString>
#include <QStringList>


namespace BusinessLayer {

ComicBookPanelParser::Section ComicBookPanelParser::section(const QString& _text)
{
    ComicBookPanelParser::Section section = SectionUndefined;

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

QString ComicBookPanelParser::sceneIntro(const QString& _text)
{
    QString placeName;

    if (_text.split(". ").count() > 0) {
        placeName = _text.split(". ").value(0);
    }

    return TextHelper::smartToUpper(placeName).simplified();
}

QString ComicBookPanelParser::location(const QString& _text, bool _force)
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

QString ComicBookPanelParser::storyDay(const QString& _text)
{
    QString scenarioDayName;

    if (_text.split(", ").count() == 2) {
        scenarioDayName = _text.split(", ").last();
    }

    return TextHelper::smartToUpper(scenarioDayName).simplified();
}

QString ComicBookPanelParser::sceneTime(const QString& _text)
{
    QString timeName;

    if (_text.split(" - ").count() >= 2) {
        timeName = _text.split(" - ").last().split(",").first();
        timeName = timeName.simplified();
    }

    return TextHelper::smartToUpper(timeName).simplified();
}

// ****

ComicBookCharacterParser::Section ComicBookCharacterParser::section(const QString& _text)
{
    ComicBookCharacterParser::Section section = SectionUndefined;

    if (_text.split("(").count() == 2) {
        section = SectionState;
    } else {
        section = SectionName;
    }

    return section;
}

QString ComicBookCharacterParser::name(const QString& _text)
{
    //
    // В блоке персонажа так же могут быть указания, что он говорит за кадром и т.п.
    // эти указания даются в скобках
    //

    QString name = _text;
    return TextHelper::smartToUpper(name.remove(QRegularExpression("[(](.*)")).simplified());
}

QString ComicBookCharacterParser::extension(const QString& _text)
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

} // namespace BusinessLayer
