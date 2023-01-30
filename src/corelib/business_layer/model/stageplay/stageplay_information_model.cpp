#include "stageplay_information_model.h"

#include <business_layer/model/abstract_image_wrapper.h>
#include <business_layer/model/abstract_model_xml.h>
#include <business_layer/templates/stageplay_template.h>
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
const QLatin1String kStageplayTextVisibleKey("stageplay_text_visible");
const QLatin1String kStageplayStatisticsVisibleKey("stageplay_statistics_visible");
const QLatin1String kHeaderKey("header");
const QLatin1String kPrintHeaderOnTitlePageKey("print_header_on_title");
const QLatin1String kFooterKey("footer");
const QLatin1String kPrintFooterOnTitlePageKey("print_footer_on_title");
const QLatin1String kOverrideSystemSettingsKey("override_system_settings");
const QLatin1String kTemplateIdKey("template_id");
} // namespace

class StageplayInformationModel::Implementation
{
public:
    QString name;
    QString tagline;
    QString logline;
    bool titlePageVisible = true;
    bool synopsisVisible = true;
    bool stageplayTextVisible = true;
    bool stageplayStatisticsVisible = true;
    QString header;
    bool printHeaderOnTitlePage = false;
    QString footer;
    bool printFooterOnTitlePage = false;

    bool overrideCommonSettings = false;
    QString templateId;
};


// ****


StageplayInformationModel::StageplayInformationModel(QObject* _parent)
    : AbstractModel(
        {
            kDocumentKey,
            kNameKey,
            kTaglineKey,
            kLoglineKey,
            kTitlePageVisibleKey,
            kSynopsisVisibleKey,
            kStageplayTextVisibleKey,
            kStageplayStatisticsVisibleKey,
            kHeaderKey,
            kPrintHeaderOnTitlePageKey,
            kFooterKey,
            kPrintFooterOnTitlePageKey,
            kOverrideSystemSettingsKey,
            kTemplateIdKey,
        },
        _parent)
    , d(new Implementation)
{
    connect(this, &StageplayInformationModel::nameChanged, this,
            &StageplayInformationModel::updateDocumentContent);
    connect(this, &StageplayInformationModel::taglineChanged, this,
            &StageplayInformationModel::updateDocumentContent);
    connect(this, &StageplayInformationModel::loglineChanged, this,
            &StageplayInformationModel::updateDocumentContent);
    connect(this, &StageplayInformationModel::titlePageVisibleChanged, this,
            &StageplayInformationModel::updateDocumentContent);
    connect(this, &StageplayInformationModel::synopsisVisibleChanged, this,
            &StageplayInformationModel::updateDocumentContent);
    connect(this, &StageplayInformationModel::stageplayTextVisibleChanged, this,
            &StageplayInformationModel::updateDocumentContent);
    connect(this, &StageplayInformationModel::stageplayStatisticsVisibleChanged, this,
            &StageplayInformationModel::updateDocumentContent);
    connect(this, &StageplayInformationModel::headerChanged, this,
            &StageplayInformationModel::updateDocumentContent);
    connect(this, &StageplayInformationModel::printHeaderOnTitlePageChanged, this,
            &StageplayInformationModel::updateDocumentContent);
    connect(this, &StageplayInformationModel::footerChanged, this,
            &StageplayInformationModel::updateDocumentContent);
    connect(this, &StageplayInformationModel::printFooterOnTitlePageChanged, this,
            &StageplayInformationModel::updateDocumentContent);
    connect(this, &StageplayInformationModel::overrideCommonSettingsChanged, this,
            &StageplayInformationModel::updateDocumentContent);
    connect(this, &StageplayInformationModel::templateIdChanged, this,
            &StageplayInformationModel::updateDocumentContent);
}

StageplayInformationModel::~StageplayInformationModel() = default;

const QString& StageplayInformationModel::name() const
{
    return d->name;
}

void StageplayInformationModel::setName(const QString& _name)
{
    if (d->name == _name) {
        return;
    }

    d->name = _name;
    emit nameChanged(d->name);
    emit documentNameChanged(d->name);
}

QString StageplayInformationModel::documentName() const
{
    return name();
}

void StageplayInformationModel::setDocumentName(const QString& _name)
{
    setName(_name);
}

const QString& StageplayInformationModel::tagline() const
{
    return d->tagline;
}

void StageplayInformationModel::setTagline(const QString& _tagline)
{
    if (d->tagline == _tagline) {
        return;
    }

    d->tagline = _tagline;
    emit taglineChanged(d->tagline);
}

const QString& StageplayInformationModel::logline() const
{
    return d->logline;
}

void StageplayInformationModel::setLogline(const QString& _logline)
{
    if (d->logline == _logline) {
        return;
    }

    d->logline = _logline;
    emit loglineChanged(d->logline);
}

bool StageplayInformationModel::titlePageVisible() const
{
    return d->titlePageVisible;
}

void StageplayInformationModel::setTitlePageVisible(bool _visible)
{
    if (d->titlePageVisible == _visible) {
        return;
    }

    d->titlePageVisible = _visible;
    emit titlePageVisibleChanged(d->titlePageVisible);
}

bool StageplayInformationModel::synopsisVisible() const
{
    return d->synopsisVisible;
}

void StageplayInformationModel::setSynopsisVisible(bool _visible)
{
    if (d->synopsisVisible == _visible) {
        return;
    }

    d->synopsisVisible = _visible;
    emit synopsisVisibleChanged(d->synopsisVisible);
}

bool StageplayInformationModel::stageplayTextVisible() const
{
    return d->stageplayTextVisible;
}

void StageplayInformationModel::setStageplayTextVisible(bool _visible)
{
    if (d->stageplayTextVisible == _visible) {
        return;
    }

    d->stageplayTextVisible = _visible;
    emit stageplayTextVisibleChanged(d->stageplayTextVisible);
}

bool StageplayInformationModel::stageplayStatisticsVisible() const
{
    return d->stageplayStatisticsVisible;
}

void StageplayInformationModel::setStageplayStatisticsVisible(bool _visible)
{
    if (d->stageplayStatisticsVisible == _visible) {
        return;
    }

    d->stageplayStatisticsVisible = _visible;
    emit stageplayStatisticsVisibleChanged(d->stageplayStatisticsVisible);
}

const QString& StageplayInformationModel::header() const
{
    return d->header;
}

void StageplayInformationModel::setHeader(const QString& _header)
{
    if (d->header == _header) {
        return;
    }

    d->header = _header;
    emit headerChanged(d->header);
}

bool StageplayInformationModel::printHeaderOnTitlePage() const
{
    return d->printHeaderOnTitlePage;
}

void StageplayInformationModel::setPrintHeaderOnTitlePage(bool _print)
{
    if (d->printHeaderOnTitlePage == _print) {
        return;
    }

    d->printHeaderOnTitlePage = _print;
    emit printHeaderOnTitlePageChanged(d->printHeaderOnTitlePage);
}

const QString& StageplayInformationModel::footer() const
{
    return d->footer;
}

void StageplayInformationModel::setFooter(const QString& _footer)
{
    if (d->footer == _footer) {
        return;
    }

    d->footer = _footer;
    emit footerChanged(d->footer);
}

bool StageplayInformationModel::printFooterOnTitlePage() const
{
    return d->printFooterOnTitlePage;
}

void StageplayInformationModel::setPrintFooterOnTitlePage(bool _print)
{
    if (d->printFooterOnTitlePage == _print) {
        return;
    }

    d->printFooterOnTitlePage = _print;
    emit printFooterOnTitlePageChanged(d->printFooterOnTitlePage);
}

bool StageplayInformationModel::overrideCommonSettings() const
{
    return d->overrideCommonSettings;
}

void StageplayInformationModel::setOverrideCommonSettings(bool _override)
{
    if (d->overrideCommonSettings == _override) {
        return;
    }

    d->overrideCommonSettings = _override;
    emit overrideCommonSettingsChanged(d->overrideCommonSettings);

    //
    // При включении/выключении кастомных параметров, сбрасываем до стандартных
    //
    setTemplateId(TemplatesFacade::stageplayTemplate().id());
}

QString StageplayInformationModel::templateId() const
{
    if (d->overrideCommonSettings) {
        return d->templateId;
    }

    return TemplatesFacade::stageplayTemplate().id();
}

void StageplayInformationModel::setTemplateId(const QString& _templateId)
{
    if (d->templateId == _templateId) {
        return;
    }

    d->templateId = _templateId;
    emit templateIdChanged(d->templateId);
}

void StageplayInformationModel::initDocument()
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
    d->stageplayTextVisible
        = documentNode.firstChildElement(kStageplayTextVisibleKey).text() == "true";
    d->stageplayStatisticsVisible
        = documentNode.firstChildElement(kStageplayStatisticsVisibleKey).text() == "true";
    d->header = documentNode.firstChildElement(kHeaderKey).text();
    d->printHeaderOnTitlePage
        = documentNode.firstChildElement(kPrintHeaderOnTitlePageKey).text() == "true";
    d->footer = documentNode.firstChildElement(kFooterKey).text();
    d->printFooterOnTitlePage
        = documentNode.firstChildElement(kPrintFooterOnTitlePageKey).text() == "true";
    d->overrideCommonSettings
        = documentNode.firstChildElement(kOverrideSystemSettingsKey).text() == "true";
    d->templateId = documentNode.firstChildElement(kTemplateIdKey).text();
}

void StageplayInformationModel::clearDocument()
{
    d.reset(new Implementation);
}

QByteArray StageplayInformationModel::toXml() const
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
    writeBoolTag(kStageplayTextVisibleKey, d->stageplayTextVisible);
    writeBoolTag(kStageplayStatisticsVisibleKey, d->stageplayStatisticsVisible);
    writeTag(kHeaderKey, d->header);
    writeBoolTag(kPrintHeaderOnTitlePageKey, d->printHeaderOnTitlePage);
    writeTag(kFooterKey, d->footer);
    writeBoolTag(kPrintFooterOnTitlePageKey, d->printFooterOnTitlePage);
    writeBoolTag(kOverrideSystemSettingsKey, d->overrideCommonSettings);
    writeTag(kTemplateIdKey, d->templateId);
    xml += QString("</%1>").arg(kDocumentKey).toUtf8();
    return xml;
}

ChangeCursor StageplayInformationModel::applyPatch(const QByteArray& _patch)
{
    //
    // Определить область изменения в xml
    //
    auto changes = dmpController().changedXml(toXml(), _patch);
    if (changes.first.xml.isEmpty() && changes.second.xml.isEmpty()) {
        Log::warning("Audioplay information model patch don't lead to any changes");
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
    using M = StageplayInformationModel;
    const auto _1 = std::placeholders::_1;
    setText(kNameKey, std::bind(&M::setName, this, _1));
    setText(kTaglineKey, std::bind(&M::setTagline, this, _1));
    setText(kLoglineKey, std::bind(&M::setLogline, this, _1));
    setBool(kTitlePageVisibleKey, std::bind(&M::setTitlePageVisible, this, _1));
    setBool(kSynopsisVisibleKey, std::bind(&M::setSynopsisVisible, this, _1));
    setBool(kStageplayTextVisibleKey, std::bind(&M::setStageplayTextVisible, this, _1));
    setBool(kStageplayStatisticsVisibleKey, std::bind(&M::setStageplayStatisticsVisible, this, _1));
    setText(kHeaderKey, std::bind(&M::setHeader, this, _1));
    setBool(kPrintHeaderOnTitlePageKey, std::bind(&M::setPrintHeaderOnTitlePage, this, _1));
    setText(kFooterKey, std::bind(&M::setFooter, this, _1));
    setBool(kPrintFooterOnTitlePageKey, std::bind(&M::setPrintFooterOnTitlePage, this, _1));
    setBool(kOverrideSystemSettingsKey, std::bind(&M::setOverrideCommonSettings, this, _1));
    setText(kTemplateIdKey, std::bind(&M::setTemplateId, this, _1));

    return {};
}

} // namespace BusinessLayer
