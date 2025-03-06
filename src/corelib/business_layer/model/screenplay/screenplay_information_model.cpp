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
const QLatin1String kScenesNumbersPrefixKey("scenes_numbers_prefix");
const QLatin1String kScenesNumbersTemplateKey("scenes_numbers_template");
const QLatin1String kScenesNumberingStartAtKey("scenes_numbering_start_at");
const QLatin1String kIsScenesNumberingLockedKey("is_scenes_numbering_locked");
const QLatin1String kCanOverrideSystemSettingsKey("can_override_system_settings");
const QLatin1String kOverrideSystemSettingsKey("override_system_settings");
const QLatin1String kTemplateIdKey("template_id");
const QLatin1String kShowSceneNumbersKey("show_scenes_numbers");
const QLatin1String kShowSceneNumbersOnLeftKey("show_scenes_numbers_on_left");
const QLatin1String kShowScenesNumbersOnRightKey("show_scenes_numbers_on_right");
const QLatin1String kShowDialoguesNumbersKey("show_dialogues_numbers");
const QLatin1String kChronomertyOptionsKey("chronmetry_options");
const QLatin1String kCharactersOrderKey("characters_order");
const QLatin1String kLocationsOrderKey("locations_order");
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
    bool canCommonSettingsBeOverridden = true;
    bool overrideCommonSettings = false;
    QString templateId;
    bool showSceneNumbers = false;
    bool showSceneNumbersOnLeft = false;
    bool showSceneNumbersOnRight = false;
    bool showDialoguesNumbers = false;
    ChronometerOptions chronometerOptions;


    QVector<QString> charactersOrder;
    QVector<QString> locationsOrder;
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
            kCanOverrideSystemSettingsKey,
            kOverrideSystemSettingsKey,
            kTemplateIdKey,
            kShowSceneNumbersKey,
            kShowSceneNumbersOnLeftKey,
            kShowScenesNumbersOnRightKey,
            kShowDialoguesNumbersKey,
            kChronomertyOptionsKey,
            kCharactersOrderKey,
            kLocationsOrderKey,
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
    connect(this, &ScreenplayInformationModel::canCommonSettingsBeOverriddenChanged, this,
            &ScreenplayInformationModel::updateDocumentContent);
    connect(this, &ScreenplayInformationModel::overrideCommonSettingsChanged, this,
            &ScreenplayInformationModel::updateDocumentContent);
    connect(this, &ScreenplayInformationModel::templateIdChanged, this,
            &ScreenplayInformationModel::updateDocumentContent);
    connect(this, &ScreenplayInformationModel::showSceneNumbersChanged, this,
            &ScreenplayInformationModel::updateDocumentContent);
    connect(this, &ScreenplayInformationModel::showSceneNumbersOnLeftChanged, this,
            &ScreenplayInformationModel::updateDocumentContent);
    connect(this, &ScreenplayInformationModel::showSceneNumbersOnRightChanged, this,
            &ScreenplayInformationModel::updateDocumentContent);
    connect(this, &ScreenplayInformationModel::showDialoguesNumbersChanged, this,
            &ScreenplayInformationModel::updateDocumentContent);
    connect(this, &ScreenplayInformationModel::chronometerOptionsChanged, this,
            &ScreenplayInformationModel::updateDocumentContent);
    connect(this, &ScreenplayInformationModel::charactersOrderChanged, this,
            &ScreenplayInformationModel::updateDocumentContent);
    connect(this, &ScreenplayInformationModel::locationsOrderChanged, this,
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

bool ScreenplayInformationModel::canCommonSettingsBeOverridden() const
{
    return d->canCommonSettingsBeOverridden;
}

void ScreenplayInformationModel::setCanCommonSettingsBeOverridden(bool _can)
{
    if (d->canCommonSettingsBeOverridden == _can) {
        return;
    }

    d->canCommonSettingsBeOverridden = _can;
    emit canCommonSettingsBeOverriddenChanged(d->canCommonSettingsBeOverridden);
}

bool ScreenplayInformationModel::overrideCommonSettings() const
{
    return d->overrideCommonSettings;
}

void ScreenplayInformationModel::setOverrideCommonSettings(bool _override)
{
    if (d->overrideCommonSettings == _override) {
        return;
    }

    d->overrideCommonSettings = _override;
    emit overrideCommonSettingsChanged(d->overrideCommonSettings);

    //
    // При включении/выключении кастомных параметров, сбрасываем до стандартных
    //
    using namespace DataStorageLayer;
    setTemplateId(TemplatesFacade::screenplayTemplate().id());
    setShowSceneNumbers(settingsValue(kComponentsScreenplayEditorShowSceneNumbersKey).toBool());
    setShowSceneNumbersOnLeft(
        settingsValue(kComponentsScreenplayEditorShowSceneNumbersOnLeftKey).toBool());
    setShowSceneNumbersOnRight(
        settingsValue(kComponentsScreenplayEditorShowSceneNumbersOnRightKey).toBool());
    setShowDialoguesNumbers(
        settingsValue(kComponentsScreenplayEditorShowDialogueNumbersKey).toBool());
    //
    // ...  хронометраж
    //
    ChronometerOptions options;
    options.type
        = static_cast<ChronometerType>(settingsValue(kComponentsScreenplayDurationTypeKey).toInt());
    options.page.seconds = settingsValue(kComponentsScreenplayDurationByPageDurationKey).toInt();
    options.characters.characters
        = settingsValue(kComponentsScreenplayDurationByCharactersCharactersKey).toInt();
    options.characters.considerSpaces
        = settingsValue(kComponentsScreenplayDurationByCharactersIncludeSpacesKey).toBool();
    options.characters.seconds
        = settingsValue(kComponentsScreenplayDurationByCharactersDurationKey).toInt();
    options.sophocles.secsPerAction
        = settingsValue(kComponentsScreenplayDurationConfigurableSecondsPerParagraphForActionKey)
              .toDouble();
    options.sophocles.secsPerEvery50Action
        = settingsValue(kComponentsScreenplayDurationConfigurableSecondsPerEvery50ForActionKey)
              .toDouble();
    options.sophocles.secsPerDialogue
        = settingsValue(kComponentsScreenplayDurationConfigurableSecondsPerParagraphForDialogueKey)
              .toDouble();
    options.sophocles.secsPerEvery50Dialogue
        = settingsValue(kComponentsScreenplayDurationConfigurableSecondsPerEvery50ForDialogueKey)
              .toDouble();
    options.sophocles.secsPerSceneHeading
        = settingsValue(
              kComponentsScreenplayDurationConfigurableSecondsPerParagraphForSceneHeadingKey)
              .toDouble();
    options.sophocles.secsPerEvery50SceneHeading
        = settingsValue(
              kComponentsScreenplayDurationConfigurableSecondsPerEvery50ForSceneHeadingKey)
              .toDouble();
    setChronometerOptions(options);
}

QString ScreenplayInformationModel::templateId() const
{
    if (d->overrideCommonSettings) {
        return d->templateId;
    }

    return TemplatesFacade::screenplayTemplate().id();
}

void ScreenplayInformationModel::setTemplateId(const QString& _templateId)
{
    if (d->templateId == _templateId) {
        return;
    }

    d->templateId = _templateId;
    emit templateIdChanged(d->templateId);
}

bool ScreenplayInformationModel::showSceneNumbers() const
{
    if (d->overrideCommonSettings) {
        return d->showSceneNumbers;
    }

    return settingsValue(DataStorageLayer::kComponentsScreenplayEditorShowSceneNumbersKey).toBool();
}

void ScreenplayInformationModel::setShowSceneNumbers(bool _show)
{
    if (d->showSceneNumbers == _show) {
        return;
    }

    d->showSceneNumbers = _show;
    emit showSceneNumbersChanged(d->showSceneNumbers);
}

bool ScreenplayInformationModel::showSceneNumbersOnLeft() const
{
    if (d->overrideCommonSettings) {
        return d->showSceneNumbersOnLeft;
    }

    return settingsValue(DataStorageLayer::kComponentsScreenplayEditorShowSceneNumbersOnLeftKey)
        .toBool();
}

void ScreenplayInformationModel::setShowSceneNumbersOnLeft(bool _show)
{
    if (d->showSceneNumbersOnLeft == _show) {
        return;
    }

    d->showSceneNumbersOnLeft = _show;
    emit showSceneNumbersOnLeftChanged(d->showSceneNumbersOnLeft);
}

bool ScreenplayInformationModel::showSceneNumbersOnRight() const
{
    if (d->overrideCommonSettings) {
        return d->showSceneNumbersOnRight;
    }

    return settingsValue(DataStorageLayer::kComponentsScreenplayEditorShowSceneNumbersOnRightKey)
        .toBool();
}

void ScreenplayInformationModel::setShowSceneNumbersOnRight(bool _show)
{
    if (d->showSceneNumbersOnRight == _show) {
        return;
    }

    d->showSceneNumbersOnRight = _show;
    emit showSceneNumbersOnRightChanged(d->showSceneNumbersOnRight);
}

bool ScreenplayInformationModel::showDialoguesNumbers() const
{
    if (d->overrideCommonSettings) {
        return d->showDialoguesNumbers;
    }

    return settingsValue(DataStorageLayer::kComponentsScreenplayEditorShowDialogueNumbersKey)
        .toBool();
}

void ScreenplayInformationModel::setShowDialoguesNumbers(bool _show)
{
    if (d->showDialoguesNumbers == _show) {
        return;
    }

    d->showDialoguesNumbers = _show;
    emit showDialoguesNumbersChanged(d->showDialoguesNumbers);
}

ChronometerOptions ScreenplayInformationModel::chronometerOptions() const
{
    if (d->overrideCommonSettings) {
        return d->chronometerOptions;
    }

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

void ScreenplayInformationModel::setChronometerOptions(const ChronometerOptions& _options)
{
    if (d->chronometerOptions == _options) {
        return;
    }

    d->chronometerOptions = _options;
    emit chronometerOptionsChanged(_options);
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

void ScreenplayInformationModel::initDocument()
{
    if (document() == nullptr) {
        return;
    }

    QDomDocument domDocument;
    const bool isContentValid = domDocument.setContent(document()->content());
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
    //
    // TODO: выпилить в одной из будущих версий
    //
    if (!documentNode.firstChildElement(kScenesNumbersPrefixKey).isNull()) {
        d->scenesNumbersTemplate
            = documentNode.firstChildElement(kScenesNumbersPrefixKey).text() + "#.";
    } else {
        d->scenesNumbersTemplate = documentNode.firstChildElement(kScenesNumbersTemplateKey).text();
    }
    const auto scenesNumberingStartAtNode
        = documentNode.firstChildElement(kScenesNumberingStartAtKey);
    if (!scenesNumberingStartAtNode.isNull()) {
        d->scenesNumberingStartAt = scenesNumberingStartAtNode.text().toInt();
    }
    d->isScenesNumbersLocked
        = documentNode.firstChildElement(kIsScenesNumberingLockedKey).text() == "true";
    //
    // TODO: выпилить в одной из будущих версий
    //
    if (!documentNode.firstChildElement(kCanOverrideSystemSettingsKey).isNull()) {
        d->canCommonSettingsBeOverridden
            = documentNode.firstChildElement(kCanOverrideSystemSettingsKey).text() == "true";
    }
    d->overrideCommonSettings
        = documentNode.firstChildElement(kOverrideSystemSettingsKey).text() == "true";
    d->templateId = documentNode.firstChildElement(kTemplateIdKey).text();
    d->showSceneNumbers = documentNode.firstChildElement(kShowSceneNumbersKey).text() == "true";
    d->showSceneNumbersOnLeft
        = documentNode.firstChildElement(kShowSceneNumbersOnLeftKey).text() == "true";
    d->showSceneNumbersOnRight
        = documentNode.firstChildElement(kShowScenesNumbersOnRightKey).text() == "true";
    d->showDialoguesNumbers
        = documentNode.firstChildElement(kShowDialoguesNumbersKey).text() == "true";
    //
    // TODO: выпилить в одной из будущих версий
    //
    if (const auto node = documentNode.firstChildElement(kChronomertyOptionsKey); !node.isNull()) {
        d->chronometerOptions.type = static_cast<ChronometerType>(node.attribute("type").toInt());
        switch (d->chronometerOptions.type) {
        default:
        case ChronometerType::Page: {
            d->chronometerOptions.page.seconds = node.attribute("seconds").toInt();
            break;
        }

        case ChronometerType::Characters: {
            d->chronometerOptions.characters.seconds = node.attribute("characters").toInt();
            d->chronometerOptions.characters.seconds = node.attribute("consider_spaces") == "true";
            d->chronometerOptions.characters.seconds = node.attribute("seconds").toInt();
            break;
        }

        case ChronometerType::Sophocles: {
            d->chronometerOptions.sophocles.secsPerAction = node.attribute("spa").toDouble();
            d->chronometerOptions.sophocles.secsPerEvery50Action
                = node.attribute("sp50a").toDouble();
            d->chronometerOptions.sophocles.secsPerDialogue = node.attribute("spd").toDouble();
            d->chronometerOptions.sophocles.secsPerEvery50Dialogue
                = node.attribute("sp50d").toDouble();
            d->chronometerOptions.sophocles.secsPerSceneHeading = node.attribute("spsh").toDouble();
            d->chronometerOptions.sophocles.secsPerEvery50SceneHeading
                = node.attribute("sp50sh").toDouble();
            break;
        }
        }
    }
    d->charactersOrder
        = documentNode.firstChildElement(kCharactersOrderKey).text().split(",").toVector();
    d->locationsOrder
        = documentNode.firstChildElement(kLocationsOrderKey).text().split(",").toVector();
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
    auto writeChronometerOptions = [&xml](const QString& _key, const ChronometerOptions& _options) {
        QString attributes;
        switch (_options.type) {
        default:
        case ChronometerType::Page: {
            attributes = QString("seconds=\"%1\"").arg(_options.page.seconds);
            break;
        }

        case ChronometerType::Characters: {
            attributes = QString("characters=\"%1\" consider_spaces=\"%2\" seconds=\"%3\"")
                             .arg(_options.characters.characters)
                             .arg(_options.characters.considerSpaces ? "true" : "false")
                             .arg(_options.characters.seconds);
            break;
        }

        case ChronometerType::Sophocles: {
            attributes
                = QString(
                      "spa=\"%1\" sp50a=\"%2\" spd=\"%3\" sp50d=\"%4\" spsh=\"%5\" sp50sh=\"%6\"")
                      .arg(_options.sophocles.secsPerAction)
                      .arg(_options.sophocles.secsPerEvery50Action)
                      .arg(_options.sophocles.secsPerDialogue)
                      .arg(_options.sophocles.secsPerEvery50Dialogue)
                      .arg(_options.sophocles.secsPerSceneHeading)
                      .arg(_options.sophocles.secsPerEvery50SceneHeading);
            break;
        }
        }

        xml += QString("<%1 type=\"%2\" %3 />")
                   .arg(_key, QString::number(static_cast<int>(_options.type)), attributes)
                   .toUtf8();
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
    writeBoolTag(kCanOverrideSystemSettingsKey, d->canCommonSettingsBeOverridden);
    writeBoolTag(kOverrideSystemSettingsKey, d->overrideCommonSettings);
    writeTag(kTemplateIdKey, d->templateId);
    writeBoolTag(kShowSceneNumbersKey, d->showSceneNumbers);
    writeBoolTag(kShowSceneNumbersOnLeftKey, d->showSceneNumbersOnLeft);
    writeBoolTag(kShowScenesNumbersOnRightKey, d->showSceneNumbersOnRight);
    writeBoolTag(kShowDialoguesNumbersKey, d->showDialoguesNumbers);
    writeChronometerOptions(kChronomertyOptionsKey, d->chronometerOptions);
    writeTag(kCharactersOrderKey, d->charactersOrder.toList().join(','));
    writeTag(kLocationsOrderKey, d->locationsOrder.toList().join(','));
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
    auto setChronometerOptions
        = [&documentNode](const QString& _key,
                          std::function<void(const ChronometerOptions&)> _setter) {
              const auto node = documentNode.firstChildElement(_key);
              if (!node.isNull()) {
                  ChronometerOptions options;
                  options.type = static_cast<ChronometerType>(node.attribute("type").toInt());
                  switch (options.type) {
                  default:
                  case ChronometerType::Page: {
                      options.page.seconds = node.attribute("seconds").toInt();
                      break;
                  }

                  case ChronometerType::Characters: {
                      options.characters.seconds = node.attribute("characters").toInt();
                      options.characters.seconds = node.attribute("consider_spaces") == "true";
                      options.characters.seconds = node.attribute("seconds").toInt();
                      break;
                  }

                  case ChronometerType::Sophocles: {
                      options.sophocles.secsPerAction = node.attribute("spa").toDouble();
                      options.sophocles.secsPerEvery50Action = node.attribute("sp50a").toDouble();
                      options.sophocles.secsPerDialogue = node.attribute("spd").toDouble();
                      options.sophocles.secsPerEvery50Dialogue = node.attribute("sp50d").toDouble();
                      options.sophocles.secsPerSceneHeading = node.attribute("spsh").toDouble();
                      options.sophocles.secsPerEvery50SceneHeading
                          = node.attribute("sp50sh").toDouble();
                      break;
                  }
                  }
                  _setter(options);
              }
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
    setBool(kCanOverrideSystemSettingsKey,
            std::bind(&M::setCanCommonSettingsBeOverridden, this, _1));
    setBool(kOverrideSystemSettingsKey, std::bind(&M::setOverrideCommonSettings, this, _1));
    setText(kTemplateIdKey, std::bind(&M::setTemplateId, this, _1));
    setBool(kShowSceneNumbersKey, std::bind(&M::setShowSceneNumbers, this, _1));
    setBool(kShowSceneNumbersOnLeftKey, std::bind(&M::setShowSceneNumbersOnLeft, this, _1));
    setBool(kShowScenesNumbersOnRightKey, std::bind(&M::setShowSceneNumbersOnRight, this, _1));
    setBool(kShowDialoguesNumbersKey, std::bind(&M::setShowDialoguesNumbers, this, _1));
    setChronometerOptions(kChronomertyOptionsKey, std::bind(&M::setChronometerOptions, this, _1));
    setVector(kCharactersOrderKey, std::bind(&M::setCharactersOrder, this, _1), ',');
    setVector(kLocationsOrderKey, std::bind(&M::setLocationsOrder, this, _1), ',');

    return {};
}

} // namespace BusinessLayer
