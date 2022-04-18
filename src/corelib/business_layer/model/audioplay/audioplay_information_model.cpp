#include "audioplay_information_model.h"

#include <business_layer/model/abstract_image_wrapper.h>
#include <business_layer/templates/audioplay_template.h>
#include <business_layer/templates/templates_facade.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <domain/document_object.h>
#include <utils/helpers/image_helper.h>
#include <utils/helpers/text_helper.h>

#include <QDomDocument>


namespace BusinessLayer {

namespace {
const QLatin1String kDocumentKey("document");
const QLatin1String kNameKey("name");
const QLatin1String kTaglineKey("tagline");
const QLatin1String kLoglineKey("logline");
const QLatin1String kTitlePageVisibleKey("title_page_visible");
const QLatin1String kSynopsisVisibleKey("synopsis_visible");
const QLatin1String kAudioplayTextVisibleKey("audioplay_text_visible");
const QLatin1String kAudioplayStatisticsVisibleKey("audioplay_statistics_visible");
const QLatin1String kHeaderKey("header");
const QLatin1String kPrintHeaderOnTitlePageKey("print_header_on_title");
const QLatin1String kFooterKey("footer");
const QLatin1String kPrintFooterOnTitlePageKey("print_footer_on_title");
const QLatin1String kOverrideSystemSettingsKey("override_system_settings");
const QLatin1String kTemplateIdKey("template_id");
const QLatin1String kShowBlockNumbersKey("show_block_numbers");
const QLatin1String kContinueBlockNumbersKey("continue_block_numbers");
} // namespace

class AudioplayInformationModel::Implementation
{
public:
    QString name;
    QString tagline;
    QString logline;
    bool titlePageVisible = true;
    bool synopsisVisible = true;
    bool audioplayTextVisible = true;
    bool audioplayStatisticsVisible = true;
    QString header;
    bool printHeaderOnTitlePage = false;
    QString footer;
    bool printFooterOnTitlePage = false;

    bool overrideCommonSettings = false;
    QString templateId;
    bool showBlockNumbers = false;
    bool continueBlockNumbers = false;
};


// ****


AudioplayInformationModel::AudioplayInformationModel(QObject* _parent)
    : AbstractModel(
        {
            kDocumentKey,
            kNameKey,
            kTaglineKey,
            kLoglineKey,
            kTitlePageVisibleKey,
            kSynopsisVisibleKey,
            kAudioplayTextVisibleKey,
            kAudioplayStatisticsVisibleKey,
            kHeaderKey,
            kPrintHeaderOnTitlePageKey,
            kFooterKey,
            kPrintFooterOnTitlePageKey,
            kOverrideSystemSettingsKey,
            kTemplateIdKey,
            kShowBlockNumbersKey,
            kContinueBlockNumbersKey,
        },
        _parent)
    , d(new Implementation)
{
    connect(this, &AudioplayInformationModel::nameChanged, this,
            &AudioplayInformationModel::updateDocumentContent);
    connect(this, &AudioplayInformationModel::taglineChanged, this,
            &AudioplayInformationModel::updateDocumentContent);
    connect(this, &AudioplayInformationModel::loglineChanged, this,
            &AudioplayInformationModel::updateDocumentContent);
    connect(this, &AudioplayInformationModel::titlePageVisibleChanged, this,
            &AudioplayInformationModel::updateDocumentContent);
    connect(this, &AudioplayInformationModel::synopsisVisibleChanged, this,
            &AudioplayInformationModel::updateDocumentContent);
    connect(this, &AudioplayInformationModel::audioplayTextVisibleChanged, this,
            &AudioplayInformationModel::updateDocumentContent);
    connect(this, &AudioplayInformationModel::audioplayStatisticsVisibleChanged, this,
            &AudioplayInformationModel::updateDocumentContent);
    connect(this, &AudioplayInformationModel::headerChanged, this,
            &AudioplayInformationModel::updateDocumentContent);
    connect(this, &AudioplayInformationModel::printHeaderOnTitlePageChanged, this,
            &AudioplayInformationModel::updateDocumentContent);
    connect(this, &AudioplayInformationModel::footerChanged, this,
            &AudioplayInformationModel::updateDocumentContent);
    connect(this, &AudioplayInformationModel::printFooterOnTitlePageChanged, this,
            &AudioplayInformationModel::updateDocumentContent);
    connect(this, &AudioplayInformationModel::overrideCommonSettingsChanged, this,
            &AudioplayInformationModel::updateDocumentContent);
    connect(this, &AudioplayInformationModel::templateIdChanged, this,
            &AudioplayInformationModel::updateDocumentContent);
    connect(this, &AudioplayInformationModel::showBlockNumbersChanged, this,
            &AudioplayInformationModel::updateDocumentContent);
    connect(this, &AudioplayInformationModel::continueBlockNumbersChanged, this,
            &AudioplayInformationModel::updateDocumentContent);
}

AudioplayInformationModel::~AudioplayInformationModel() = default;

const QString& AudioplayInformationModel::name() const
{
    return d->name;
}

void AudioplayInformationModel::setName(const QString& _name)
{
    if (d->name == _name) {
        return;
    }

    d->name = _name;
    emit nameChanged(d->name);
    emit documentNameChanged(d->name);
}

void AudioplayInformationModel::setDocumentName(const QString& _name)
{
    setName(_name);
}

const QString& AudioplayInformationModel::tagline() const
{
    return d->tagline;
}

void AudioplayInformationModel::setTagline(const QString& _tagline)
{
    if (d->tagline == _tagline) {
        return;
    }

    d->tagline = _tagline;
    emit taglineChanged(d->tagline);
}

const QString& AudioplayInformationModel::logline() const
{
    return d->logline;
}

void AudioplayInformationModel::setLogline(const QString& _logline)
{
    if (d->logline == _logline) {
        return;
    }

    d->logline = _logline;
    emit loglineChanged(d->logline);
}

bool AudioplayInformationModel::titlePageVisible() const
{
    return d->titlePageVisible;
}

void AudioplayInformationModel::setTitlePageVisible(bool _visible)
{
    if (d->titlePageVisible == _visible) {
        return;
    }

    d->titlePageVisible = _visible;
    emit titlePageVisibleChanged(d->titlePageVisible);
}

bool AudioplayInformationModel::synopsisVisible() const
{
    return d->synopsisVisible;
}

void AudioplayInformationModel::setSynopsisVisible(bool _visible)
{
    if (d->synopsisVisible == _visible) {
        return;
    }

    d->synopsisVisible = _visible;
    emit synopsisVisibleChanged(d->synopsisVisible);
}

bool AudioplayInformationModel::audioplayTextVisible() const
{
    return d->audioplayTextVisible;
}

void AudioplayInformationModel::setAudioplayTextVisible(bool _visible)
{
    if (d->audioplayTextVisible == _visible) {
        return;
    }

    d->audioplayTextVisible = _visible;
    emit audioplayTextVisibleChanged(d->audioplayTextVisible);
}

bool AudioplayInformationModel::audioplayStatisticsVisible() const
{
    return d->audioplayStatisticsVisible;
}

void AudioplayInformationModel::setAudioplayStatisticsVisible(bool _visible)
{
    if (d->audioplayStatisticsVisible == _visible) {
        return;
    }

    d->audioplayStatisticsVisible = _visible;
    emit audioplayStatisticsVisibleChanged(d->audioplayStatisticsVisible);
}

const QString& AudioplayInformationModel::header() const
{
    return d->header;
}

void AudioplayInformationModel::setHeader(const QString& _header)
{
    if (d->header == _header) {
        return;
    }

    d->header = _header;
    emit headerChanged(d->header);
}

bool AudioplayInformationModel::printHeaderOnTitlePage() const
{
    return d->printHeaderOnTitlePage;
}

void AudioplayInformationModel::setPrintHeaderOnTitlePage(bool _print)
{
    if (d->printHeaderOnTitlePage == _print) {
        return;
    }

    d->printHeaderOnTitlePage = _print;
    emit printHeaderOnTitlePageChanged(d->printHeaderOnTitlePage);
}

const QString& AudioplayInformationModel::footer() const
{
    return d->footer;
}

void AudioplayInformationModel::setFooter(const QString& _footer)
{
    if (d->footer == _footer) {
        return;
    }

    d->footer = _footer;
    emit footerChanged(d->footer);
}

bool AudioplayInformationModel::printFooterOnTitlePage() const
{
    return d->printFooterOnTitlePage;
}

void AudioplayInformationModel::setPrintFooterOnTitlePage(bool _print)
{
    if (d->printFooterOnTitlePage == _print) {
        return;
    }

    d->printFooterOnTitlePage = _print;
    emit printFooterOnTitlePageChanged(d->printFooterOnTitlePage);
}

bool AudioplayInformationModel::overrideCommonSettings() const
{
    return d->overrideCommonSettings;
}

void AudioplayInformationModel::setOverrideCommonSettings(bool _override)
{
    if (d->overrideCommonSettings == _override) {
        return;
    }

    d->overrideCommonSettings = _override;
    emit overrideCommonSettingsChanged(d->overrideCommonSettings);

    //
    // При включении/выключении кастомных параметров, сбрасываем до стандартных
    //
    setTemplateId(TemplatesFacade::audioplayTemplate().id());
    setShowBlockNumbers(
        settingsValue(DataStorageLayer::kComponentsAudioplayEditorShowBlockNumbersKey).toBool());
    setContinueBlockNumbers(
        settingsValue(DataStorageLayer::kComponentsAudioplayEditorContinueBlockNumbersKey)
            .toBool());
}

QString AudioplayInformationModel::templateId() const
{
    if (d->overrideCommonSettings) {
        return d->templateId;
    }

    return TemplatesFacade::audioplayTemplate().id();
}

void AudioplayInformationModel::setTemplateId(const QString& _templateId)
{
    if (d->templateId == _templateId) {
        return;
    }

    d->templateId = _templateId;
    emit templateIdChanged(d->templateId);
}

bool AudioplayInformationModel::showBlockNumbers() const
{
    if (d->overrideCommonSettings) {
        return d->showBlockNumbers;
    }

    return settingsValue(DataStorageLayer::kComponentsAudioplayEditorShowBlockNumbersKey).toBool();
}

void AudioplayInformationModel::setShowBlockNumbers(bool _show)
{
    if (d->showBlockNumbers == _show) {
        return;
    }

    d->showBlockNumbers = _show;
    emit showBlockNumbersChanged(d->showBlockNumbers);
}

bool AudioplayInformationModel::continueBlockNumbers() const
{
    if (d->overrideCommonSettings) {
        return d->continueBlockNumbers;
    }

    return settingsValue(DataStorageLayer::kComponentsAudioplayEditorContinueBlockNumbersKey)
        .toBool();
}

void AudioplayInformationModel::setContinueBlockNumbers(bool _continue)
{
    if (d->continueBlockNumbers == _continue) {
        return;
    }

    d->continueBlockNumbers = _continue;
    emit continueBlockNumbersChanged(d->continueBlockNumbers);
}

void AudioplayInformationModel::initDocument()
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
    d->audioplayTextVisible
        = documentNode.firstChildElement(kAudioplayTextVisibleKey).text() == "true";
    d->audioplayStatisticsVisible
        = documentNode.firstChildElement(kAudioplayStatisticsVisibleKey).text() == "true";
    d->header = documentNode.firstChildElement(kHeaderKey).text();
    d->printHeaderOnTitlePage
        = documentNode.firstChildElement(kPrintHeaderOnTitlePageKey).text() == "true";
    d->footer = documentNode.firstChildElement(kFooterKey).text();
    d->printFooterOnTitlePage
        = documentNode.firstChildElement(kPrintFooterOnTitlePageKey).text() == "true";
    d->overrideCommonSettings
        = documentNode.firstChildElement(kOverrideSystemSettingsKey).text() == "true";
    d->templateId = documentNode.firstChildElement(kTemplateIdKey).text();
    d->showBlockNumbers = documentNode.firstChildElement(kShowBlockNumbersKey).text() == "true";
    d->continueBlockNumbers
        = documentNode.firstChildElement(kContinueBlockNumbersKey).text() == "true";
}

void AudioplayInformationModel::clearDocument()
{
    d.reset(new Implementation);
}

QByteArray AudioplayInformationModel::toXml() const
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
    writeBoolTag(kAudioplayTextVisibleKey, d->audioplayTextVisible);
    writeBoolTag(kAudioplayStatisticsVisibleKey, d->audioplayStatisticsVisible);
    writeTag(kHeaderKey, d->header);
    writeBoolTag(kPrintHeaderOnTitlePageKey, d->printHeaderOnTitlePage);
    writeTag(kFooterKey, d->footer);
    writeBoolTag(kPrintFooterOnTitlePageKey, d->printFooterOnTitlePage);
    writeBoolTag(kOverrideSystemSettingsKey, d->overrideCommonSettings);
    writeTag(kTemplateIdKey, d->templateId);
    writeBoolTag(kShowBlockNumbersKey, d->showBlockNumbers);
    writeBoolTag(kContinueBlockNumbersKey, d->continueBlockNumbers);
    xml += QString("</%1>").arg(kDocumentKey).toUtf8();
    return xml;
}

} // namespace BusinessLayer
