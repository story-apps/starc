#include "screenplay_series_information_model.h"

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
const QLatin1String kOverrideSystemSettingsKey("override_system_settings");
const QLatin1String kTemplateIdKey("template_id");
const QLatin1String kShowSceneNumbersKey("show_scenes_numbers");
const QLatin1String kShowSceneNumbersOnLeftKey("show_scenes_numbers_on_left");
const QLatin1String kShowScenesNumbersOnRightKey("show_scenes_numbers_on_right");
const QLatin1String kShowDialoguesNumbersKey("show_dialogues_numbers");
const QLatin1String kCharactersOrderKey("characters_order");
const QLatin1String kLocationsOrderKey("locations_order");
} // namespace

class ScreenplaySeriesInformationModel::Implementation
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
    bool overrideCommonSettings = false;
    QString templateId;
    bool showSceneNumbers = false;
    bool showSceneNumbersOnLeft = false;
    bool showSceneNumbersOnRight = false;
    bool showDialoguesNumbers = false;

    QVector<QString> charactersOrder;
    QVector<QString> locationsOrder;
};


// ****


ScreenplaySeriesInformationModel::ScreenplaySeriesInformationModel(QObject* _parent)
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
            kOverrideSystemSettingsKey,
            kTemplateIdKey,
            kShowSceneNumbersKey,
            kShowSceneNumbersOnLeftKey,
            kShowScenesNumbersOnRightKey,
            kShowDialoguesNumbersKey,
            kCharactersOrderKey,
            kLocationsOrderKey,
        },
        _parent)
    , d(new Implementation)
{
    connect(this, &ScreenplaySeriesInformationModel::nameChanged, this,
            &ScreenplaySeriesInformationModel::updateDocumentContent);
    connect(this, &ScreenplaySeriesInformationModel::taglineChanged, this,
            &ScreenplaySeriesInformationModel::updateDocumentContent);
    connect(this, &ScreenplaySeriesInformationModel::loglineChanged, this,
            &ScreenplaySeriesInformationModel::updateDocumentContent);
    connect(this, &ScreenplaySeriesInformationModel::titlePageVisibleChanged, this,
            &ScreenplaySeriesInformationModel::updateDocumentContent);
    connect(this, &ScreenplaySeriesInformationModel::synopsisVisibleChanged, this,
            &ScreenplaySeriesInformationModel::updateDocumentContent);
    connect(this, &ScreenplaySeriesInformationModel::treatmentVisibleChanged, this,
            &ScreenplaySeriesInformationModel::updateDocumentContent);
    connect(this, &ScreenplaySeriesInformationModel::screenplayTextVisibleChanged, this,
            &ScreenplaySeriesInformationModel::updateDocumentContent);
    connect(this, &ScreenplaySeriesInformationModel::screenplayStatisticsVisibleChanged, this,
            &ScreenplaySeriesInformationModel::updateDocumentContent);
    connect(this, &ScreenplaySeriesInformationModel::headerChanged, this,
            &ScreenplaySeriesInformationModel::updateDocumentContent);
    connect(this, &ScreenplaySeriesInformationModel::printHeaderOnTitlePageChanged, this,
            &ScreenplaySeriesInformationModel::updateDocumentContent);
    connect(this, &ScreenplaySeriesInformationModel::footerChanged, this,
            &ScreenplaySeriesInformationModel::updateDocumentContent);
    connect(this, &ScreenplaySeriesInformationModel::printFooterOnTitlePageChanged, this,
            &ScreenplaySeriesInformationModel::updateDocumentContent);
    connect(this, &ScreenplaySeriesInformationModel::overrideCommonSettingsChanged, this,
            &ScreenplaySeriesInformationModel::updateDocumentContent);
    connect(this, &ScreenplaySeriesInformationModel::templateIdChanged, this,
            &ScreenplaySeriesInformationModel::updateDocumentContent);
    connect(this, &ScreenplaySeriesInformationModel::showSceneNumbersChanged, this,
            &ScreenplaySeriesInformationModel::updateDocumentContent);
    connect(this, &ScreenplaySeriesInformationModel::showSceneNumbersOnLeftChanged, this,
            &ScreenplaySeriesInformationModel::updateDocumentContent);
    connect(this, &ScreenplaySeriesInformationModel::showSceneNumbersOnRightChanged, this,
            &ScreenplaySeriesInformationModel::updateDocumentContent);
    connect(this, &ScreenplaySeriesInformationModel::showDialoguesNumbersChanged, this,
            &ScreenplaySeriesInformationModel::updateDocumentContent);
    connect(this, &ScreenplaySeriesInformationModel::charactersOrderChanged, this,
            &ScreenplaySeriesInformationModel::updateDocumentContent);
    connect(this, &ScreenplaySeriesInformationModel::locationsOrderChanged, this,
            &ScreenplaySeriesInformationModel::updateDocumentContent);
}

ScreenplaySeriesInformationModel::~ScreenplaySeriesInformationModel() = default;

const QString& ScreenplaySeriesInformationModel::name() const
{
    return d->name;
}

void ScreenplaySeriesInformationModel::setName(const QString& _name)
{
    if (d->name == _name) {
        return;
    }

    d->name = _name;
    emit nameChanged(d->name);
    emit documentNameChanged(d->name);
}

QString ScreenplaySeriesInformationModel::documentName() const
{
    return name();
}

void ScreenplaySeriesInformationModel::setDocumentName(const QString& _name)
{
    setName(_name);
}

const QString& ScreenplaySeriesInformationModel::tagline() const
{
    return d->tagline;
}

void ScreenplaySeriesInformationModel::setTagline(const QString& _tagline)
{
    if (d->tagline == _tagline) {
        return;
    }

    d->tagline = _tagline;
    emit taglineChanged(d->tagline);
}

const QString& ScreenplaySeriesInformationModel::logline() const
{
    return d->logline;
}

void ScreenplaySeriesInformationModel::setLogline(const QString& _logline)
{
    if (d->logline == _logline) {
        return;
    }

    d->logline = _logline;
    emit loglineChanged(d->logline);
}

bool ScreenplaySeriesInformationModel::titlePageVisible() const
{
    return d->titlePageVisible;
}

void ScreenplaySeriesInformationModel::setTitlePageVisible(bool _visible)
{
    if (d->titlePageVisible == _visible) {
        return;
    }

    d->titlePageVisible = _visible;
    emit titlePageVisibleChanged(d->titlePageVisible);
}

bool ScreenplaySeriesInformationModel::synopsisVisible() const
{
    return d->synopsisVisible;
}

void ScreenplaySeriesInformationModel::setSynopsisVisible(bool _visible)
{
    if (d->synopsisVisible == _visible) {
        return;
    }

    d->synopsisVisible = _visible;
    emit synopsisVisibleChanged(d->synopsisVisible);
}

bool ScreenplaySeriesInformationModel::treatmentVisible() const
{
    return d->treatmentVisible;
}

void ScreenplaySeriesInformationModel::setTreatmentVisible(bool _visible)
{
    if (d->treatmentVisible == _visible) {
        return;
    }

    d->treatmentVisible = _visible;
    emit treatmentVisibleChanged(d->treatmentVisible);
}

bool ScreenplaySeriesInformationModel::screenplayTextVisible() const
{
    return d->screenplayTextVisible;
}

void ScreenplaySeriesInformationModel::setScreenplayTextVisible(bool _visible)
{
    if (d->screenplayTextVisible == _visible) {
        return;
    }

    d->screenplayTextVisible = _visible;
    emit screenplayTextVisibleChanged(d->screenplayTextVisible);
}

bool ScreenplaySeriesInformationModel::screenplayStatisticsVisible() const
{
    return d->screenplayStatisticsVisible;
}

void ScreenplaySeriesInformationModel::setScreenplayStatisticsVisible(bool _visible)
{
    if (d->screenplayStatisticsVisible == _visible) {
        return;
    }

    d->screenplayStatisticsVisible = _visible;
    emit screenplayStatisticsVisibleChanged(d->screenplayStatisticsVisible);
}

const QString& ScreenplaySeriesInformationModel::header() const
{
    return d->header;
}

void ScreenplaySeriesInformationModel::setHeader(const QString& _header)
{
    if (d->header == _header) {
        return;
    }

    d->header = _header;
    emit headerChanged(d->header);
}

bool ScreenplaySeriesInformationModel::printHeaderOnTitlePage() const
{
    return d->printHeaderOnTitlePage;
}

void ScreenplaySeriesInformationModel::setPrintHeaderOnTitlePage(bool _print)
{
    if (d->printHeaderOnTitlePage == _print) {
        return;
    }

    d->printHeaderOnTitlePage = _print;
    emit printHeaderOnTitlePageChanged(d->printHeaderOnTitlePage);
}

const QString& ScreenplaySeriesInformationModel::footer() const
{
    return d->footer;
}

void ScreenplaySeriesInformationModel::setFooter(const QString& _footer)
{
    if (d->footer == _footer) {
        return;
    }

    d->footer = _footer;
    emit footerChanged(d->footer);
}

bool ScreenplaySeriesInformationModel::printFooterOnTitlePage() const
{
    return d->printFooterOnTitlePage;
}

void ScreenplaySeriesInformationModel::setPrintFooterOnTitlePage(bool _print)
{
    if (d->printFooterOnTitlePage == _print) {
        return;
    }

    d->printFooterOnTitlePage = _print;
    emit printFooterOnTitlePageChanged(d->printFooterOnTitlePage);
}

bool ScreenplaySeriesInformationModel::overrideCommonSettings() const
{
    return d->overrideCommonSettings;
}

void ScreenplaySeriesInformationModel::setOverrideCommonSettings(bool _override)
{
    if (d->overrideCommonSettings == _override) {
        return;
    }

    d->overrideCommonSettings = _override;
    emit overrideCommonSettingsChanged(d->overrideCommonSettings);

    //
    // При включении/выключении кастомных параметров, сбрасываем до стандартных
    //
    setTemplateId(TemplatesFacade::screenplayTemplate().id());
    setShowSceneNumbers(
        settingsValue(DataStorageLayer::kComponentsScreenplayEditorShowSceneNumbersKey).toBool());
    setShowSceneNumbersOnLeft(
        settingsValue(DataStorageLayer::kComponentsScreenplayEditorShowSceneNumbersOnLeftKey)
            .toBool());
    setShowSceneNumbersOnRight(
        settingsValue(DataStorageLayer::kComponentsScreenplayEditorShowSceneNumbersOnRightKey)
            .toBool());
    setShowDialoguesNumbers(
        settingsValue(DataStorageLayer::kComponentsScreenplayEditorShowDialogueNumbersKey)
            .toBool());
}

QString ScreenplaySeriesInformationModel::templateId() const
{
    if (d->overrideCommonSettings) {
        return d->templateId;
    }

    return TemplatesFacade::screenplayTemplate().id();
}

void ScreenplaySeriesInformationModel::setTemplateId(const QString& _templateId)
{
    if (d->templateId == _templateId) {
        return;
    }

    d->templateId = _templateId;
    emit templateIdChanged(d->templateId);
}

bool ScreenplaySeriesInformationModel::showSceneNumbers() const
{
    if (d->overrideCommonSettings) {
        return d->showSceneNumbers;
    }

    return settingsValue(DataStorageLayer::kComponentsScreenplayEditorShowSceneNumbersKey).toBool();
}

void ScreenplaySeriesInformationModel::setShowSceneNumbers(bool _show)
{
    if (d->showSceneNumbers == _show) {
        return;
    }

    d->showSceneNumbers = _show;
    emit showSceneNumbersChanged(d->showSceneNumbers);
}

bool ScreenplaySeriesInformationModel::showSceneNumbersOnLeft() const
{
    if (d->overrideCommonSettings) {
        return d->showSceneNumbersOnLeft;
    }

    return settingsValue(DataStorageLayer::kComponentsScreenplayEditorShowSceneNumbersOnLeftKey)
        .toBool();
}

void ScreenplaySeriesInformationModel::setShowSceneNumbersOnLeft(bool _show)
{
    if (d->showSceneNumbersOnLeft == _show) {
        return;
    }

    d->showSceneNumbersOnLeft = _show;
    emit showSceneNumbersOnLeftChanged(d->showSceneNumbersOnLeft);
}

bool ScreenplaySeriesInformationModel::showSceneNumbersOnRight() const
{
    if (d->overrideCommonSettings) {
        return d->showSceneNumbersOnRight;
    }

    return settingsValue(DataStorageLayer::kComponentsScreenplayEditorShowSceneNumbersOnRightKey)
        .toBool();
}

void ScreenplaySeriesInformationModel::setShowSceneNumbersOnRight(bool _show)
{
    if (d->showSceneNumbersOnRight == _show) {
        return;
    }

    d->showSceneNumbersOnRight = _show;
    emit showSceneNumbersOnRightChanged(d->showSceneNumbersOnRight);
}

bool ScreenplaySeriesInformationModel::showDialoguesNumbers() const
{
    if (d->overrideCommonSettings) {
        return d->showDialoguesNumbers;
    }

    return settingsValue(DataStorageLayer::kComponentsScreenplayEditorShowDialogueNumbersKey)
        .toBool();
}

void ScreenplaySeriesInformationModel::setShowDialoguesNumbers(bool _show)
{
    if (d->showDialoguesNumbers == _show) {
        return;
    }

    d->showDialoguesNumbers = _show;
    emit showDialoguesNumbersChanged(d->showDialoguesNumbers);
}

QVector<QString> ScreenplaySeriesInformationModel::charactersOrder() const
{
    return d->charactersOrder;
}

void ScreenplaySeriesInformationModel::setCharactersOrder(const QVector<QString>& _characters)
{
    if (d->charactersOrder == _characters) {
        return;
    }

    d->charactersOrder = _characters;
    emit charactersOrderChanged(d->charactersOrder);
}

QVector<QString> ScreenplaySeriesInformationModel::locationsOrder() const
{
    return d->locationsOrder;
}

void ScreenplaySeriesInformationModel::setLocationsOrder(const QVector<QString>& _locations)
{
    if (d->locationsOrder == _locations) {
        return;
    }

    d->locationsOrder = _locations;
    emit locationsOrderChanged(d->locationsOrder);
}

void ScreenplaySeriesInformationModel::initDocument()
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
    d->charactersOrder
        = documentNode.firstChildElement(kCharactersOrderKey).text().split(",").toVector();
    d->locationsOrder
        = documentNode.firstChildElement(kLocationsOrderKey).text().split(",").toVector();
}

void ScreenplaySeriesInformationModel::clearDocument()
{
    d.reset(new Implementation);
}

QByteArray ScreenplaySeriesInformationModel::toXml() const
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
    writeBoolTag(kOverrideSystemSettingsKey, d->overrideCommonSettings);
    writeTag(kTemplateIdKey, d->templateId);
    writeBoolTag(kShowSceneNumbersKey, d->showSceneNumbers);
    writeBoolTag(kShowSceneNumbersOnLeftKey, d->showSceneNumbersOnLeft);
    writeBoolTag(kShowScenesNumbersOnRightKey, d->showSceneNumbersOnRight);
    writeBoolTag(kShowDialoguesNumbersKey, d->showDialoguesNumbers);
    writeTag(kCharactersOrderKey, d->charactersOrder.toList().join(','));
    writeTag(kLocationsOrderKey, d->locationsOrder.toList().join(','));
    xml += QString("</%1>").arg(kDocumentKey).toUtf8();
    return xml;
}

ChangeCursor ScreenplaySeriesInformationModel::applyPatch(const QByteArray& _patch)
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
    using M = ScreenplaySeriesInformationModel;
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
    setBool(kOverrideSystemSettingsKey, std::bind(&M::setOverrideCommonSettings, this, _1));
    setText(kTemplateIdKey, std::bind(&M::setTemplateId, this, _1));
    setBool(kShowSceneNumbersKey, std::bind(&M::setShowSceneNumbers, this, _1));
    setBool(kShowSceneNumbersOnLeftKey, std::bind(&M::setShowSceneNumbersOnLeft, this, _1));
    setBool(kShowScenesNumbersOnRightKey, std::bind(&M::setShowSceneNumbersOnRight, this, _1));
    setBool(kShowDialoguesNumbersKey, std::bind(&M::setShowDialoguesNumbers, this, _1));
    setVector(kCharactersOrderKey, std::bind(&M::setCharactersOrder, this, _1), ',');
    setVector(kLocationsOrderKey, std::bind(&M::setLocationsOrder, this, _1), ',');

    return {};
}

} // namespace BusinessLayer
