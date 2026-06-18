#include "screenplay_information_model.h"

#include <business_layer/chronometry/chronometer.h>
#include <business_layer/model/abstract_image_wrapper.h>
#include <business_layer/model/abstract_model_xml.h>
#include <business_layer/templates/screenplay_template.h>
#include <business_layer/templates/templates_facade.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <domain/document_object.h>
#include <utils/diff_match_patch/diff_match_patch_controller.h>
#include <utils/helpers/image_helper.h>
#include <utils/helpers/text_helper.h>
#include <utils/logging.h>

#include <QDomDocument>


namespace BusinessLayer {

namespace {
const QLatin1String kDocumentKey("document");
const QLatin1String kNameKey("name");
const QLatin1String kTaglineKey("tagline");
const QLatin1String kLoglineKey("logline");
const QLatin1String kTitlePageVisibleKey("title_page_visible");
const QLatin1String kSynopsisVisibleKey("synopsis_visible");
const QLatin1String kTreatmentVisibleKey("treatment_visible");
const QLatin1String kScreenplayTextVisibleKey("screenplay_text_visible");
const QLatin1String kScreenplayStatisticsVisibleKey("screenplay_statistics_visible");
const QLatin1String kHeaderKey("header");
const QLatin1String kPrintHeaderOnTitlePageKey("print_header_on_title");
const QLatin1String kFooterKey("footer");
const QLatin1String kPrintFooterOnTitlePageKey("print_footer_on_title");
const QLatin1String kScenesNumbersTemplateKey("scenes_numbers_template");
const QLatin1String kScenesNumberingStartAtKey("scenes_numbering_start_at");
const QLatin1String kIsScenesNumberingLockedKey("is_scenes_numbering_locked");
const QLatin1String kCharactersOrderKey("characters_order");
const QLatin1String kLocationsOrderKey("locations_order");
const QLatin1String kStoryLinesKey("story_lines");
const QLatin1String kStoryLineKey("story_line");
} // namespace

class ScreenplayInformationModel::Implementation
{
public:
    QString name;
    QString tagline;
    QString logline;
    bool titlePageVisible = true;
    bool synopsisVisible = true;
    bool treatmentVisible = true;
    bool screenplayTextVisible = true;
    bool screenplayStatisticsVisible = true;

    QString header;
    bool printHeaderOnTitlePage = false;
    QString footer;
    bool printFooterOnTitlePage = false;
    QString scenesNumbersTemplate = "#.";
    int scenesNumberingStartAt = 1;
    bool isScenesNumbersLocked = false;


    QVector<QString> charactersOrder;
    QVector<QString> locationsOrder;
    QVector<QString> storyLines;
};


// ****


ScreenplayInformationModel::ScreenplayInformationModel(QObject* _parent)
    : AbstractModel(
          {
              kDocumentKey,
              kNameKey,
              kTaglineKey,
              kLoglineKey,
              kTitlePageVisibleKey,
              kSynopsisVisibleKey,
              kTreatmentVisibleKey,
              kScreenplayTextVisibleKey,
              kScreenplayStatisticsVisibleKey,
              kHeaderKey,
              kPrintHeaderOnTitlePageKey,
              kFooterKey,
              kPrintFooterOnTitlePageKey,
              kScenesNumbersTemplateKey,
              kScenesNumberingStartAtKey,
              kIsScenesNumberingLockedKey,
              kCharactersOrderKey,
              kLocationsOrderKey,
              kStoryLinesKey,
              kStoryLineKey,
          },
          _parent)
    , d(new Implementation)
{
    connect(this, &ScreenplayInformationModel::nameChanged, this,
            &ScreenplayInformationModel::updateDocumentContent);
    connect(this, &ScreenplayInformationModel::taglineChanged, this,
            &ScreenplayInformationModel::updateDocumentContent);
    connect(this, &ScreenplayInformationModel::loglineChanged, this,
            &ScreenplayInformationModel::updateDocumentContent);
    connect(this, &ScreenplayInformationModel::titlePageVisibleChanged, this,
            &ScreenplayInformationModel::updateDocumentContent);
    connect(this, &ScreenplayInformationModel::synopsisVisibleChanged, this,
            &ScreenplayInformationModel::updateDocumentContent);
    connect(this, &ScreenplayInformationModel::treatmentVisibleChanged, this,
            &ScreenplayInformationModel::updateDocumentContent);
    connect(this, &ScreenplayInformationModel::screenplayTextVisibleChanged, this,
            &ScreenplayInformationModel::updateDocumentContent);
    connect(this, &ScreenplayInformationModel::screenplayStatisticsVisibleChanged, this,
            &ScreenplayInformationModel::updateDocumentContent);
    connect(this, &ScreenplayInformationModel::headerChanged, this,
            &ScreenplayInformationModel::updateDocumentContent);
    connect(this, &ScreenplayInformationModel::printHeaderOnTitlePageChanged, this,
            &ScreenplayInformationModel::updateDocumentContent);
    connect(this, &ScreenplayInformationModel::footerChanged, this,
            &ScreenplayInformationModel::updateDocumentContent);
    connect(this, &ScreenplayInformationModel::printFooterOnTitlePageChanged, this,
            &ScreenplayInformationModel::updateDocumentContent);
    connect(this, &ScreenplayInformationModel::scenesNumbersTemplateChanged, this,
            &ScreenplayInformationModel::updateDocumentContent);
    connect(this, &ScreenplayInformationModel::scenesNumberingStartAtChanged, this,
            &ScreenplayInformationModel::updateDocumentContent);
    connect(this, &ScreenplayInformationModel::isSceneNumbersLockedChanged, this,
            &ScreenplayInformationModel::updateDocumentContent);
    connect(this, &ScreenplayInformationModel::charactersOrderChanged, this,
            &ScreenplayInformationModel::updateDocumentContent);
    connect(this, &ScreenplayInformationModel::locationsOrderChanged, this,
            &ScreenplayInformationModel::updateDocumentContent);
    connect(this, &ScreenplayInformationModel::storyLinesChanged, this,
            &ScreenplayInformationModel::updateDocumentContent);
}

ScreenplayInformationModel::~ScreenplayInformationModel() = default;

const QString& ScreenplayInformationModel::name() const
{
    return d->name;
}

void ScreenplayInformationModel::setName(const QString& _name)
{
    if (d->name == _name) {
        return;
    }

    d->name = _name;
    emit nameChanged(d->name);
    emit documentNameChanged(d->name);
}

QString ScreenplayInformationModel::documentName() const
{
    return name();
}

void ScreenplayInformationModel::setDocumentName(const QString& _name)
{
    setName(_name);
}

const QString& ScreenplayInformationModel::tagline() const
{
    return d->tagline;
}

void ScreenplayInformationModel::setTagline(const QString& _tagline)
{
    if (d->tagline == _tagline) {
        return;
    }

    d->tagline = _tagline;
    emit taglineChanged(d->tagline);
}

const QString& ScreenplayInformationModel::logline() const
{
    return d->logline;
}

void ScreenplayInformationModel::setLogline(const QString& _logline)
{
    if (d->logline == _logline) {
        return;
    }

    d->logline = _logline;
    emit loglineChanged(d->logline);
}

bool ScreenplayInformationModel::titlePageVisible() const
{
    return d->titlePageVisible;
}

void ScreenplayInformationModel::setTitlePageVisible(bool _visible)
{
    if (d->titlePageVisible == _visible) {
        return;
    }

    d->titlePageVisible = _visible;
    emit titlePageVisibleChanged(d->titlePageVisible);
}

bool ScreenplayInformationModel::synopsisVisible() const
{
    return d->synopsisVisible;
}

void ScreenplayInformationModel::setSynopsisVisible(bool _visible)
{
    if (d->synopsisVisible == _visible) {
        return;
    }

    d->synopsisVisible = _visible;
    emit synopsisVisibleChanged(d->synopsisVisible);
}

bool ScreenplayInformationModel::treatmentVisible() const
{
    return d->treatmentVisible;
}

void ScreenplayInformationModel::setTreatmentVisible(bool _visible)
{
    if (d->treatmentVisible == _visible) {
        return;
    }

    d->treatmentVisible = _visible;
    emit treatmentVisibleChanged(d->treatmentVisible);
}

bool ScreenplayInformationModel::screenplayTextVisible() const
{
    return d->screenplayTextVisible;
}

void ScreenplayInformationModel::setScreenplayTextVisible(bool _visible)
{
    if (d->screenplayTextVisible == _visible) {
        return;
    }

    d->screenplayTextVisible = _visible;
    emit screenplayTextVisibleChanged(d->screenplayTextVisible);
}

bool ScreenplayInformationModel::screenplayStatisticsVisible() const
{
    return d->screenplayStatisticsVisible;
}

void ScreenplayInformationModel::setScreenplayStatisticsVisible(bool _visible)
{
    if (d->screenplayStatisticsVisible == _visible) {
        return;
    }

    d->screenplayStatisticsVisible = _visible;
    emit screenplayStatisticsVisibleChanged(d->screenplayStatisticsVisible);
}

const QString& ScreenplayInformationModel::header() const
{
    return d->header;
}

void ScreenplayInformationModel::setHeader(const QString& _header)
{
    if (d->header == _header) {
        return;
    }

    d->header = _header;
    emit headerChanged(d->header);
}

bool ScreenplayInformationModel::printHeaderOnTitlePage() const
{
    return d->printHeaderOnTitlePage;
}

void ScreenplayInformationModel::setPrintHeaderOnTitlePage(bool _print)
{
    if (d->printHeaderOnTitlePage == _print) {
        return;
    }

    d->printHeaderOnTitlePage = _print;
    emit printHeaderOnTitlePageChanged(d->printHeaderOnTitlePage);
}

const QString& ScreenplayInformationModel::footer() const
{
    return d->footer;
}

void ScreenplayInformationModel::setFooter(const QString& _footer)
{
    if (d->footer == _footer) {
        return;
    }

    d->footer = _footer;
    emit footerChanged(d->footer);
}

bool ScreenplayInformationModel::printFooterOnTitlePage() const
{
    return d->printFooterOnTitlePage;
}

void ScreenplayInformationModel::setPrintFooterOnTitlePage(bool _print)
{
    if (d->printFooterOnTitlePage == _print) {
        return;
    }

    d->printFooterOnTitlePage = _print;
    emit printFooterOnTitlePageChanged(d->printFooterOnTitlePage);
}

const QString& ScreenplayInformationModel::scenesNumbersTemplate() const
{
    return d->scenesNumbersTemplate;
}

void ScreenplayInformationModel::setScenesNumbersTemplate(const QString& _template)
{
    if (d->scenesNumbersTemplate == _template) {
        return;
    }

    d->scenesNumbersTemplate = _template;
    emit scenesNumbersTemplateChanged(d->scenesNumbersTemplate);
}

int ScreenplayInformationModel::scenesNumberingStartAt() const
{
    return d->scenesNumberingStartAt;
}

void ScreenplayInformationModel::setScenesNumberingStartAt(int _startNumber)
{
    if (d->scenesNumberingStartAt == _startNumber) {
        return;
    }

    d->scenesNumberingStartAt = _startNumber;
    emit scenesNumberingStartAtChanged(d->scenesNumberingStartAt);
}

bool ScreenplayInformationModel::isSceneNumbersLocked() const
{
    return d->isScenesNumbersLocked;
}

void ScreenplayInformationModel::setScenesNumbersLocked(bool _locked)
{
    //
    // Тут поверх блокировки может прийти ещё одна блокировка, поэтому дополнительной проверки нет
    //

    d->isScenesNumbersLocked = _locked;
    emit isSceneNumbersLockedChanged(_locked);
}

QString ScreenplayInformationModel::templateId() const
{
    return TemplatesFacade::screenplayTemplate().id();
}

bool ScreenplayInformationModel::showSceneNumbers() const
{
    return settingsValue(DataStorageLayer::kComponentsScreenplayEditorShowSceneNumbersKey).toBool();
}

bool ScreenplayInformationModel::showSceneNumbersOnLeft() const
{
    return settingsValue(DataStorageLayer::kComponentsScreenplayEditorShowSceneNumbersOnLeftKey)
        .toBool();
}

bool ScreenplayInformationModel::showSceneNumbersOnRight() const
{
    return settingsValue(DataStorageLayer::kComponentsScreenplayEditorShowSceneNumbersOnRightKey)
        .toBool();
}

bool ScreenplayInformationModel::showDialoguesNumbers() const
{
    return settingsValue(DataStorageLayer::kComponentsScreenplayEditorShowDialogueNumbersKey)
        .toBool();
}

ChronometerOptions ScreenplayInformationModel::chronometerOptions() const
{
    using namespace DataStorageLayer;
    ChronometerOptions options;
    options.type
        = static_cast<ChronometerType>(settingsValue(kComponentsScreenplayDurationTypeKey).toInt());
    switch (options.type) {
    case ChronometerType::Page: {
        options.page.seconds
            = settingsValue(kComponentsScreenplayDurationByPageDurationKey).toInt();
        break;
    }

    case ChronometerType::Characters: {
        options.characters.characters
            = settingsValue(kComponentsScreenplayDurationByCharactersCharactersKey).toInt();
        options.characters.considerSpaces
            = settingsValue(kComponentsScreenplayDurationByCharactersIncludeSpacesKey).toBool();
        options.characters.seconds
            = settingsValue(kComponentsScreenplayDurationByCharactersDurationKey).toInt();
        break;
    }

    case ChronometerType::Sophocles: {
        options.sophocles.secsPerAction
            = settingsValue(
                  kComponentsScreenplayDurationConfigurableSecondsPerParagraphForActionKey)
                  .toDouble();
        options.sophocles.secsPerEvery50Action
            = settingsValue(kComponentsScreenplayDurationConfigurableSecondsPerEvery50ForActionKey)
                  .toDouble();
        options.sophocles.secsPerDialogue
            = settingsValue(
                  kComponentsScreenplayDurationConfigurableSecondsPerParagraphForDialogueKey)
                  .toDouble();
        options.sophocles.secsPerEvery50Dialogue
            = settingsValue(
                  kComponentsScreenplayDurationConfigurableSecondsPerEvery50ForDialogueKey)
                  .toDouble();
        options.sophocles.secsPerSceneHeading
            = settingsValue(
                  kComponentsScreenplayDurationConfigurableSecondsPerParagraphForSceneHeadingKey)
                  .toDouble();
        options.sophocles.secsPerEvery50SceneHeading
            = settingsValue(
                  kComponentsScreenplayDurationConfigurableSecondsPerEvery50ForSceneHeadingKey)
                  .toDouble();
        break;
    }
    default: {
        Q_ASSERT(false);
        break;
    }
    }

    return options;
}

QVector<QString> ScreenplayInformationModel::charactersOrder() const
{
    return d->charactersOrder;
}

void ScreenplayInformationModel::setCharactersOrder(const QVector<QString>& _characters)
{
    if (d->charactersOrder == _characters) {
        return;
    }

    d->charactersOrder = _characters;
    emit charactersOrderChanged(d->charactersOrder);
}

QVector<QString> ScreenplayInformationModel::locationsOrder() const
{
    return d->locationsOrder;
}

void ScreenplayInformationModel::setLocationsOrder(const QVector<QString>& _locations)
{
    if (d->locationsOrder == _locations) {
        return;
    }

    d->locationsOrder = _locations;
    emit locationsOrderChanged(d->locationsOrder);
}

QVector<QString> ScreenplayInformationModel::storyLines() const
{
    return d->storyLines;
}

void ScreenplayInformationModel::setStoryLines(const QVector<QString>& _storyLines)
{
    if (d->storyLines == _storyLines) {
        return;
    }

    d->storyLines = _storyLines;
    emit storyLinesChanged(d->storyLines);
}

void ScreenplayInformationModel::initDocument()
{
    if (document() == nullptr) {
        return;
    }

    QDomDocument domDocument;
    const auto isContentValid = domDocument.setContent(document()->content());
    if (!isContentValid) {
        return;
    }

    const auto documentNode = domDocument.firstChildElement(kDocumentKey);
    d->name = documentNode.firstChildElement(kNameKey).text();
    d->tagline = documentNode.firstChildElement(kTaglineKey).text();
    d->logline = documentNode.firstChildElement(kLoglineKey).text();
    d->titlePageVisible = documentNode.firstChildElement(kTitlePageVisibleKey).text() == "true";
    d->synopsisVisible = documentNode.firstChildElement(kSynopsisVisibleKey).text() == "true";
    d->treatmentVisible = documentNode.firstChildElement(kTreatmentVisibleKey).text() == "true";
    d->screenplayTextVisible
        = documentNode.firstChildElement(kScreenplayTextVisibleKey).text() == "true";
    d->screenplayStatisticsVisible
        = documentNode.firstChildElement(kScreenplayStatisticsVisibleKey).text() == "true";
    d->header = documentNode.firstChildElement(kHeaderKey).text();
    d->printHeaderOnTitlePage
        = documentNode.firstChildElement(kPrintHeaderOnTitlePageKey).text() == "true";
    d->footer = documentNode.firstChildElement(kFooterKey).text();
    d->printFooterOnTitlePage
        = documentNode.firstChildElement(kPrintFooterOnTitlePageKey).text() == "true";
    d->scenesNumbersTemplate = documentNode.firstChildElement(kScenesNumbersTemplateKey).text();
    const auto scenesNumberingStartAtNode
        = documentNode.firstChildElement(kScenesNumberingStartAtKey);
    if (!scenesNumberingStartAtNode.isNull()) {
        d->scenesNumberingStartAt = scenesNumberingStartAtNode.text().toInt();
    }
    d->isScenesNumbersLocked
        = documentNode.firstChildElement(kIsScenesNumberingLockedKey).text() == "true";
    d->charactersOrder
        = documentNode.firstChildElement(kCharactersOrderKey).text().split(",").toVector();
    d->locationsOrder
        = documentNode.firstChildElement(kLocationsOrderKey).text().split(",").toVector();
    QVector<QString> storyLines;
    if (const auto storyLinesNode = documentNode.firstChildElement(kStoryLinesKey);
        !storyLinesNode.isNull()) {
        for (int index = 0; index < storyLinesNode.childNodes().size(); ++index) {
            const auto child = storyLinesNode.childNodes().at(index);
            storyLines.append(TextHelper::fromHtmlEscaped(child.toElement().text()));
        }
    }
    d->storyLines = storyLines;
}

void ScreenplayInformationModel::clearDocument()
{
    d.reset(new Implementation);
}

QByteArray ScreenplayInformationModel::toXml() const
{
    if (document() == nullptr) {
        return {};
    }

    QByteArray xml = "<?xml version=\"1.0\"?>\n";
    xml += QString("<%1 mime-type=\"%2\" version=\"1.0\">\n")
               .arg(kDocumentKey, Domain::mimeTypeFor(document()->type()))
               .toUtf8();
    auto writeTag = [&xml](const QString& _key, const QString& _value) {
        xml += QString("<%1><![CDATA[%2]]></%1>\n").arg(_key, _value).toUtf8();
    };
    auto writeBoolTag = [&writeTag](const QString& _key, bool _value) {
        writeTag(_key, _value ? "true" : "false");
    };
    writeTag(kNameKey, d->name);
    writeTag(kTaglineKey, d->tagline);
    writeTag(kLoglineKey, d->logline);
    writeBoolTag(kTitlePageVisibleKey, d->titlePageVisible);
    writeBoolTag(kSynopsisVisibleKey, d->synopsisVisible);
    writeBoolTag(kTreatmentVisibleKey, d->treatmentVisible);
    writeBoolTag(kScreenplayTextVisibleKey, d->screenplayTextVisible);
    writeBoolTag(kScreenplayStatisticsVisibleKey, d->screenplayStatisticsVisible);
    writeTag(kHeaderKey, d->header);
    writeBoolTag(kPrintHeaderOnTitlePageKey, d->printHeaderOnTitlePage);
    writeTag(kFooterKey, d->footer);
    writeBoolTag(kPrintFooterOnTitlePageKey, d->printFooterOnTitlePage);
    writeTag(kScenesNumbersTemplateKey, d->scenesNumbersTemplate);
    writeTag(kScenesNumberingStartAtKey, QString::number(d->scenesNumberingStartAt));
    writeBoolTag(kIsScenesNumberingLockedKey, d->isScenesNumbersLocked);
    writeTag(kCharactersOrderKey, d->charactersOrder.toList().join(','));
    writeTag(kLocationsOrderKey, d->locationsOrder.toList().join(','));
    xml += QString("<%1>\n").arg(kStoryLinesKey).toUtf8();
    for (const auto& storyLine : d->storyLines) {
        writeTag(kStoryLineKey, TextHelper::toHtmlEscaped(storyLine));
    }
    xml += QString("</%1>\n").arg(kStoryLinesKey).toUtf8();
    xml += QString("</%1>").arg(kDocumentKey).toUtf8();
    return xml;
}

ChangeCursor ScreenplayInformationModel::applyPatch(const QByteArray& _patch)
{
    //
    // Определить область изменения в xml
    //
    auto changes = dmpController().changedXml(toXml(), _patch);
    if (changes.first.xml.isEmpty() && changes.second.xml.isEmpty()) {
        Log::warning("Screenplay information model patch don't lead to any changes");
        return {};
    }

    changes.second.xml = xml::prepareXml(changes.second.xml);

    QDomDocument domDocument;
    domDocument.setContent(changes.second.xml);
    const auto documentNode = domDocument.firstChildElement(kDocumentKey);
    auto setText
        = [&documentNode](const QString& _key, std::function<void(const QString&)> _setter) {
              const auto node = documentNode.firstChildElement(_key);
              if (!node.isNull()) {
                  _setter(node.text());
              }
          };
    auto setBool = [&documentNode](const QString& _key, std::function<void(bool)> _setter) {
        const auto node = documentNode.firstChildElement(_key);
        if (!node.isNull()) {
            _setter(node.text() == "true");
        }
    };
    auto setInt = [&documentNode](const QString& _key, std::function<void(int)> _setter) {
        const auto node = documentNode.firstChildElement(_key);
        if (!node.isNull()) {
            _setter(node.text().toInt());
        }
    };
    auto setVector
        = [&documentNode](const QString& _key, std::function<void(const QVector<QString>&)> _setter,
                          char _separator) {
              const auto node = documentNode.firstChildElement(_key);
              if (!node.isNull()) {
                  _setter(node.text().split(_separator).toVector());
              }
          };
    auto setHtmlEscapedVector =
        [&documentNode](const QString& _key, std::function<void(const QVector<QString>&)> _setter) {
            const auto node = documentNode.firstChildElement(_key);
            QVector<QString> value;
            if (!node.isNull()) {
                for (int index = 0; index < node.childNodes().size(); ++index) {
                    const auto child = node.childNodes().at(index);
                    value.append(TextHelper::fromHtmlEscaped(child.toElement().text()));
                }
            }
            _setter(value);
        };

    using M = ScreenplayInformationModel;
    const auto _1 = std::placeholders::_1;
    setText(kNameKey, std::bind(&M::setName, this, _1));
    setText(kTaglineKey, std::bind(&M::setTagline, this, _1));
    setText(kLoglineKey, std::bind(&M::setLogline, this, _1));
    setBool(kTitlePageVisibleKey, std::bind(&M::setTitlePageVisible, this, _1));
    setBool(kSynopsisVisibleKey, std::bind(&M::setSynopsisVisible, this, _1));
    setBool(kTreatmentVisibleKey, std::bind(&M::setTreatmentVisible, this, _1));
    setBool(kScreenplayTextVisibleKey, std::bind(&M::setScreenplayTextVisible, this, _1));
    setBool(kScreenplayStatisticsVisibleKey,
            std::bind(&M::setScreenplayStatisticsVisible, this, _1));
    setText(kHeaderKey, std::bind(&M::setHeader, this, _1));
    setBool(kPrintHeaderOnTitlePageKey, std::bind(&M::setPrintHeaderOnTitlePage, this, _1));
    setText(kFooterKey, std::bind(&M::setFooter, this, _1));
    setBool(kPrintFooterOnTitlePageKey, std::bind(&M::setPrintFooterOnTitlePage, this, _1));
    setText(kScenesNumbersTemplateKey, std::bind(&M::setScenesNumbersTemplate, this, _1));
    setInt(kScenesNumberingStartAtKey, std::bind(&M::setScenesNumberingStartAt, this, _1));
    setBool(kIsScenesNumberingLockedKey, std::bind(&M::setScenesNumbersLocked, this, _1));
    setVector(kCharactersOrderKey, std::bind(&M::setCharactersOrder, this, _1), ',');
    setVector(kLocationsOrderKey, std::bind(&M::setLocationsOrder, this, _1), ',');
    setHtmlEscapedVector(kStoryLinesKey, std::bind(&M::setStoryLines, this, _1));

    return {};
}

} // namespace BusinessLayer
