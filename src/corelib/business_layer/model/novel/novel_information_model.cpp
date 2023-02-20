#include "novel_information_model.h"

#include <business_layer/model/abstract_image_wrapper.h>
#include <business_layer/model/abstract_model_xml.h>
#include <business_layer/templates/novel_template.h>
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
const QLatin1String kNovelTextVisibleKey("novel_text_visible");
const QLatin1String kNovelStatisticsVisibleKey("novel_statistics_visible");
const QLatin1String kHeaderKey("header");
const QLatin1String kPrintHeaderOnTitlePageKey("print_header_on_title");
const QLatin1String kFooterKey("footer");
const QLatin1String kPrintFooterOnTitlePageKey("print_footer_on_title");
const QLatin1String kOverrideSystemSettingsKey("override_system_settings");
const QLatin1String kTemplateIdKey("template_id");
} // namespace

class NovelInformationModel::Implementation
{
public:
    QString name;
    QString tagline;
    QString logline;
    bool titlePageVisible = true;
    bool synopsisVisible = true;
    bool outlineVisible = true;
    bool novelTextVisible = true;
    bool novelStatisticsVisible = true;
    QString header;
    bool printHeaderOnTitlePage = false;
    QString footer;
    bool printFooterOnTitlePage = false;
    bool overrideCommonSettings = false;
    QString templateId;
};


// ****


NovelInformationModel::NovelInformationModel(QObject* _parent)
    : AbstractModel(
        {
            kDocumentKey,
            kNameKey,
            kTaglineKey,
            kLoglineKey,
            kTitlePageVisibleKey,
            kSynopsisVisibleKey,
            kTreatmentVisibleKey,
            kNovelTextVisibleKey,
            kNovelStatisticsVisibleKey,
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
    connect(this, &NovelInformationModel::nameChanged, this,
            &NovelInformationModel::updateDocumentContent);
    connect(this, &NovelInformationModel::taglineChanged, this,
            &NovelInformationModel::updateDocumentContent);
    connect(this, &NovelInformationModel::loglineChanged, this,
            &NovelInformationModel::updateDocumentContent);
    connect(this, &NovelInformationModel::titlePageVisibleChanged, this,
            &NovelInformationModel::updateDocumentContent);
    connect(this, &NovelInformationModel::synopsisVisibleChanged, this,
            &NovelInformationModel::updateDocumentContent);
    connect(this, &NovelInformationModel::outlineVisibleChanged, this,
            &NovelInformationModel::updateDocumentContent);
    connect(this, &NovelInformationModel::novelTextVisibleChanged, this,
            &NovelInformationModel::updateDocumentContent);
    connect(this, &NovelInformationModel::novelStatisticsVisibleChanged, this,
            &NovelInformationModel::updateDocumentContent);
    connect(this, &NovelInformationModel::headerChanged, this,
            &NovelInformationModel::updateDocumentContent);
    connect(this, &NovelInformationModel::printHeaderOnTitlePageChanged, this,
            &NovelInformationModel::updateDocumentContent);
    connect(this, &NovelInformationModel::footerChanged, this,
            &NovelInformationModel::updateDocumentContent);
    connect(this, &NovelInformationModel::printFooterOnTitlePageChanged, this,
            &NovelInformationModel::updateDocumentContent);
    connect(this, &NovelInformationModel::overrideCommonSettingsChanged, this,
            &NovelInformationModel::updateDocumentContent);
    connect(this, &NovelInformationModel::templateIdChanged, this,
            &NovelInformationModel::updateDocumentContent);
}

NovelInformationModel::~NovelInformationModel() = default;

const QString& NovelInformationModel::name() const
{
    return d->name;
}

void NovelInformationModel::setName(const QString& _name)
{
    if (d->name == _name) {
        return;
    }

    d->name = _name;
    emit nameChanged(d->name);
    emit documentNameChanged(d->name);
}

QString NovelInformationModel::documentName() const
{
    return name();
}

void NovelInformationModel::setDocumentName(const QString& _name)
{
    setName(_name);
}

const QString& NovelInformationModel::tagline() const
{
    return d->tagline;
}

void NovelInformationModel::setTagline(const QString& _tagline)
{
    if (d->tagline == _tagline) {
        return;
    }

    d->tagline = _tagline;
    emit taglineChanged(d->tagline);
}

const QString& NovelInformationModel::logline() const
{
    return d->logline;
}

void NovelInformationModel::setLogline(const QString& _logline)
{
    if (d->logline == _logline) {
        return;
    }

    d->logline = _logline;
    emit loglineChanged(d->logline);
}

bool NovelInformationModel::titlePageVisible() const
{
    return d->titlePageVisible;
}

void NovelInformationModel::setTitlePageVisible(bool _visible)
{
    if (d->titlePageVisible == _visible) {
        return;
    }

    d->titlePageVisible = _visible;
    emit titlePageVisibleChanged(d->titlePageVisible);
}

bool NovelInformationModel::synopsisVisible() const
{
    return d->synopsisVisible;
}

void NovelInformationModel::setSynopsisVisible(bool _visible)
{
    if (d->synopsisVisible == _visible) {
        return;
    }

    d->synopsisVisible = _visible;
    emit synopsisVisibleChanged(d->synopsisVisible);
}

bool NovelInformationModel::outlineVisible() const
{
    return d->outlineVisible;
}

void NovelInformationModel::setOutlineVisible(bool _visible)
{
    if (d->outlineVisible == _visible) {
        return;
    }

    d->outlineVisible = _visible;
    emit outlineVisibleChanged(d->outlineVisible);
}

bool NovelInformationModel::novelTextVisible() const
{
    return d->novelTextVisible;
}

void NovelInformationModel::setNovelTextVisible(bool _visible)
{
    if (d->novelTextVisible == _visible) {
        return;
    }

    d->novelTextVisible = _visible;
    emit novelTextVisibleChanged(d->novelTextVisible);
}

bool NovelInformationModel::novelStatisticsVisible() const
{
    return d->novelStatisticsVisible;
}

void NovelInformationModel::setNovelStatisticsVisible(bool _visible)
{
    if (d->novelStatisticsVisible == _visible) {
        return;
    }

    d->novelStatisticsVisible = _visible;
    emit novelStatisticsVisibleChanged(d->novelStatisticsVisible);
}

const QString& NovelInformationModel::header() const
{
    return d->header;
}

void NovelInformationModel::setHeader(const QString& _header)
{
    if (d->header == _header) {
        return;
    }

    d->header = _header;
    emit headerChanged(d->header);
}

bool NovelInformationModel::printHeaderOnTitlePage() const
{
    return d->printHeaderOnTitlePage;
}

void NovelInformationModel::setPrintHeaderOnTitlePage(bool _print)
{
    if (d->printHeaderOnTitlePage == _print) {
        return;
    }

    d->printHeaderOnTitlePage = _print;
    emit printHeaderOnTitlePageChanged(d->printHeaderOnTitlePage);
}

const QString& NovelInformationModel::footer() const
{
    return d->footer;
}

void NovelInformationModel::setFooter(const QString& _footer)
{
    if (d->footer == _footer) {
        return;
    }

    d->footer = _footer;
    emit footerChanged(d->footer);
}

bool NovelInformationModel::printFooterOnTitlePage() const
{
    return d->printFooterOnTitlePage;
}

void NovelInformationModel::setPrintFooterOnTitlePage(bool _print)
{
    if (d->printFooterOnTitlePage == _print) {
        return;
    }

    d->printFooterOnTitlePage = _print;
    emit printFooterOnTitlePageChanged(d->printFooterOnTitlePage);
}

bool NovelInformationModel::overrideCommonSettings() const
{
    return d->overrideCommonSettings;
}

void NovelInformationModel::setOverrideCommonSettings(bool _override)
{
    if (d->overrideCommonSettings == _override) {
        return;
    }

    d->overrideCommonSettings = _override;
    emit overrideCommonSettingsChanged(d->overrideCommonSettings);

    //
    // При включении/выключении кастомных параметров, сбрасываем до стандартных
    //
    setTemplateId(TemplatesFacade::novelTemplate().id());
}

QString NovelInformationModel::templateId() const
{
    if (d->overrideCommonSettings) {
        return d->templateId;
    }

    return TemplatesFacade::novelTemplate().id();
}

void NovelInformationModel::setTemplateId(const QString& _templateId)
{
    if (d->templateId == _templateId) {
        return;
    }

    d->templateId = _templateId;
    emit templateIdChanged(d->templateId);
}

void NovelInformationModel::initDocument()
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
    d->outlineVisible = documentNode.firstChildElement(kTreatmentVisibleKey).text() == "true";
    d->novelTextVisible = documentNode.firstChildElement(kNovelTextVisibleKey).text() == "true";
    d->novelStatisticsVisible
        = documentNode.firstChildElement(kNovelStatisticsVisibleKey).text() == "true";
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

void NovelInformationModel::clearDocument()
{
    d.reset(new Implementation);
}

QByteArray NovelInformationModel::toXml() const
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
    writeBoolTag(kTreatmentVisibleKey, d->outlineVisible);
    writeBoolTag(kNovelTextVisibleKey, d->novelTextVisible);
    writeBoolTag(kNovelStatisticsVisibleKey, d->novelStatisticsVisible);
    writeTag(kHeaderKey, d->header);
    writeBoolTag(kPrintHeaderOnTitlePageKey, d->printHeaderOnTitlePage);
    writeTag(kFooterKey, d->footer);
    writeBoolTag(kPrintFooterOnTitlePageKey, d->printFooterOnTitlePage);
    writeBoolTag(kOverrideSystemSettingsKey, d->overrideCommonSettings);
    writeTag(kTemplateIdKey, d->templateId);
    xml += QString("</%1>").arg(kDocumentKey).toUtf8();
    return xml;
}

ChangeCursor NovelInformationModel::applyPatch(const QByteArray& _patch)
{
    //
    // Определить область изменения в xml
    //
    auto changes = dmpController().changedXml(toXml(), _patch);
    if (changes.first.xml.isEmpty() && changes.second.xml.isEmpty()) {
        Log::warning("Novel information model patch don't lead to any changes");
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
    using M = NovelInformationModel;
    const auto _1 = std::placeholders::_1;
    setText(kNameKey, std::bind(&M::setName, this, _1));
    setText(kTaglineKey, std::bind(&M::setTagline, this, _1));
    setText(kLoglineKey, std::bind(&M::setLogline, this, _1));
    setBool(kTitlePageVisibleKey, std::bind(&M::setTitlePageVisible, this, _1));
    setBool(kSynopsisVisibleKey, std::bind(&M::setSynopsisVisible, this, _1));
    setBool(kTreatmentVisibleKey, std::bind(&M::setOutlineVisible, this, _1));
    setBool(kNovelTextVisibleKey, std::bind(&M::setNovelTextVisible, this, _1));
    setBool(kNovelStatisticsVisibleKey, std::bind(&M::setNovelStatisticsVisible, this, _1));
    setText(kHeaderKey, std::bind(&M::setHeader, this, _1));
    setBool(kPrintHeaderOnTitlePageKey, std::bind(&M::setPrintHeaderOnTitlePage, this, _1));
    setText(kFooterKey, std::bind(&M::setFooter, this, _1));
    setBool(kPrintFooterOnTitlePageKey, std::bind(&M::setPrintFooterOnTitlePage, this, _1));
    setBool(kOverrideSystemSettingsKey, std::bind(&M::setOverrideCommonSettings, this, _1));
    setText(kTemplateIdKey, std::bind(&M::setTemplateId, this, _1));

    return {};
}

} // namespace BusinessLayer
