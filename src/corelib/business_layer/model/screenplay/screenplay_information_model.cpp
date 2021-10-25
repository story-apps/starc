#include "screenplay_information_model.h"

#include <business_layer/model/abstract_image_wrapper.h>
#include <business_layer/templates/screenplay_template.h>
#include <business_layer/templates/templates_facade.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <domain/document_object.h>
#include <utils/helpers/image_helper.h>
#include <utils/helpers/text_helper.h>

#include <QDomDocument>


namespace BusinessLayer {

namespace {
const QString kDocumentKey = "document";
const QString kNameKey = "name";
const QString kTaglineKey = "tagline";
const QString kLoglineKey = "logline";
const QString kTitlePageVisibleKey = "title_page_visible";
const QString kSynopsisVisibleKey = "synopsis_visible";
const QString kTreatmentVisibleKey = "treatment_visible";
const QString kScreenplayTextVisibleKey = "screenplay_text_visible";
const QString kScreenplayStatisticsVisibleKey = "screenplay_statistics_visible";
const QString kHeaderKey = "header";
const QString kPrintHeaderOnTitlePageKey = "print_header_on_title";
const QString kFooterKey = "footer";
const QString kPrintFooterOnTitlePageKey = "print_footer_on_title";
const QString kScenesNumbersPrefixKey = "scenes_numbers_prefix";
const QString kScenesNumberingStartAtKey = "scenes_numbering_start_at";
const QString kOverrideSystemSettingsKey = "override_system_settings";
const QString kTemplateIdKey = "template_id";
const QString kShowScenesNumbersKey = "show_scenes_numbers";
const QString kShowScenesNumbersOnLeftKey = "show_scenes_numbers_on_left";
const QString kShowScenesNumbersOnRightKey = "show_scenes_numbers_on_right";
const QString kShowDialoguesNumbersKey = "show_dialogues_numbers";
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
    QString scenesNumbersPrefix;
    int scenesNumberingStartAt = 1;
    bool overrideCommonSettings = false;
    QString templateId;
    bool showSceneNumbers = false;
    bool showSceneNumbersOnLeft = false;
    bool showSceneNumbersOnRight = false;
    bool showDialoguesNumbers = false;
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
            kScenesNumbersPrefixKey,
            kScenesNumberingStartAtKey,
            kOverrideSystemSettingsKey,
            kTemplateIdKey,
            kShowScenesNumbersKey,
            kShowScenesNumbersOnLeftKey,
            kShowScenesNumbersOnRightKey,
            kShowDialoguesNumbersKey,
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
    connect(this, &ScreenplayInformationModel::scenesNumbersPrefixChanged, this,
            &ScreenplayInformationModel::updateDocumentContent);
    connect(this, &ScreenplayInformationModel::scenesNumberingStartAtChanged, this,
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

const QString& ScreenplayInformationModel::scenesNumbersPrefix() const
{
    return d->scenesNumbersPrefix;
}

void ScreenplayInformationModel::setScenesNumbersPrefix(const QString& _prefix)
{
    if (d->scenesNumbersPrefix == _prefix) {
        return;
    }

    d->scenesNumbersPrefix = _prefix;
    emit scenesNumbersPrefixChanged(d->scenesNumbersPrefix);
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
    d->scenesNumbersPrefix = documentNode.firstChildElement(kScenesNumbersPrefixKey).text();
    const auto scenesNumberingStartAtNode
        = documentNode.firstChildElement(kScenesNumberingStartAtKey);
    if (!scenesNumberingStartAtNode.isNull()) {
        d->scenesNumberingStartAt = scenesNumberingStartAtNode.text().toInt();
    }
    d->overrideCommonSettings
        = documentNode.firstChildElement(kOverrideSystemSettingsKey).text() == "true";
    d->templateId = documentNode.firstChildElement(kTemplateIdKey).text();
    d->showSceneNumbers = documentNode.firstChildElement(kShowScenesNumbersKey).text() == "true";
    d->showSceneNumbersOnLeft
        = documentNode.firstChildElement(kShowScenesNumbersOnLeftKey).text() == "true";
    d->showSceneNumbersOnRight
        = documentNode.firstChildElement(kShowScenesNumbersOnRightKey).text() == "true";
    d->showDialoguesNumbers
        = documentNode.firstChildElement(kShowDialoguesNumbersKey).text() == "true";
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
    writeTag(kScenesNumbersPrefixKey, d->scenesNumbersPrefix);
    writeTag(kScenesNumberingStartAtKey, QString::number(d->scenesNumberingStartAt));
    writeBoolTag(kOverrideSystemSettingsKey, d->overrideCommonSettings);
    writeTag(kTemplateIdKey, d->templateId);
    writeBoolTag(kShowScenesNumbersKey, d->showSceneNumbers);
    writeBoolTag(kShowScenesNumbersOnLeftKey, d->showSceneNumbersOnLeft);
    writeBoolTag(kShowScenesNumbersOnRightKey, d->showSceneNumbersOnRight);
    writeBoolTag(kShowDialoguesNumbersKey, d->showDialoguesNumbers);
    xml += QString("</%1>").arg(kDocumentKey).toUtf8();
    return xml;
}

} // namespace BusinessLayer
