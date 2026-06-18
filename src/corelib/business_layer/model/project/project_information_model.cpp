#include "project_information_model.h"

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
const QLatin1String kLoglineKey("logline");
const QLatin1String kCoverKey("cover");
const QLatin1String kOverrideSystemSettingsKey("override_system_settings");
const QLatin1String kTemplateIdKey("template_id");
const QLatin1String kShowSceneNumbersKey("show_scenes_numbers");
const QLatin1String kShowSceneNumbersOnLeftKey("show_scenes_numbers_on_left");
const QLatin1String kShowScenesNumbersOnRightKey("show_scenes_numbers_on_right");
const QLatin1String kShowDialoguesNumbersKey("show_dialogues_numbers");
const QLatin1String kChronomertyOptionsKey("chronmetry_options");
} // namespace

class ProjectInformationModel::Implementation
{
public:
    /**
     * @brief Обновим параметры сессии в соответствии с настройками модели
     */
    void updateSessionSettings();


    QString name;
    QString logline;
    Domain::DocumentImage cover;

    struct {
        bool overrideCommonSettings = false;
        QString templateId;
        bool showSceneNumbers = false;
        bool showSceneNumbersOnLeft = false;
        bool showSceneNumbersOnRight = false;
        bool showDialoguesNumbers = false;
        ChronometerOptions chronometerOptions;
    } screenplay;

    StructureModel* structureModel = nullptr;

    QVector<Domain::ProjectCollaboratorInfo> collaborators;
    QVector<Domain::TeamMemberInfo> teammates;
};

void ProjectInformationModel::Implementation::updateSessionSettings()
{
    using namespace DataStorageLayer;

    if (screenplay.overrideCommonSettings) {
        //
        // Шаблон
        //
        setSettingsValueForSession(kComponentsScreenplayEditorDefaultTemplateKey,
                                   screenplay.templateId);
        BusinessLayer::TemplatesFacade::setDefaultScreenplayTemplate(screenplay.templateId);
        //
        // Нумерация
        //
        setSettingsValueForSession(kComponentsScreenplayEditorShowSceneNumbersKey,
                                   screenplay.showSceneNumbers);
        setSettingsValueForSession(kComponentsScreenplayEditorShowSceneNumbersOnLeftKey,
                                   screenplay.showSceneNumbersOnLeft);
        setSettingsValueForSession(kComponentsScreenplayEditorShowSceneNumbersOnRightKey,
                                   screenplay.showSceneNumbersOnRight);
        setSettingsValueForSession(kComponentsScreenplayEditorShowDialogueNumbersKey,
                                   screenplay.showDialoguesNumbers);
        //
        // Хронометраж
        //
        const auto& chrono = screenplay.chronometerOptions;
        setSettingsValueForSession(kComponentsScreenplayDurationTypeKey,
                                   static_cast<int>(chrono.type));
        setSettingsValueForSession(kComponentsScreenplayDurationByPageDurationKey,
                                   chrono.page.seconds);
        setSettingsValueForSession(kComponentsScreenplayDurationByCharactersCharactersKey,
                                   chrono.characters.characters);
        setSettingsValueForSession(kComponentsScreenplayDurationByCharactersIncludeSpacesKey,
                                   chrono.characters.considerSpaces);
        setSettingsValueForSession(kComponentsScreenplayDurationByCharactersDurationKey,
                                   chrono.characters.seconds);
        setSettingsValueForSession(
            kComponentsScreenplayDurationConfigurableSecondsPerParagraphForActionKey,
            chrono.sophocles.secsPerAction);
        setSettingsValueForSession(
            kComponentsScreenplayDurationConfigurableSecondsPerEvery50ForActionKey,
            chrono.sophocles.secsPerEvery50Action);
        setSettingsValueForSession(
            kComponentsScreenplayDurationConfigurableSecondsPerParagraphForDialogueKey,
            chrono.sophocles.secsPerDialogue);
        setSettingsValueForSession(
            kComponentsScreenplayDurationConfigurableSecondsPerEvery50ForDialogueKey,
            chrono.sophocles.secsPerEvery50Dialogue);
        setSettingsValueForSession(
            kComponentsScreenplayDurationConfigurableSecondsPerParagraphForSceneHeadingKey,
            chrono.sophocles.secsPerSceneHeading);
        setSettingsValueForSession(
            kComponentsScreenplayDurationConfigurableSecondsPerEvery50ForSceneHeadingKey,
            chrono.sophocles.secsPerEvery50SceneHeading);

    } else {
        for (const auto& key : {
                 kComponentsScreenplayEditorDefaultTemplateKey,
                 kComponentsScreenplayEditorShowSceneNumbersKey,
                 kComponentsScreenplayEditorShowSceneNumbersOnLeftKey,
                 kComponentsScreenplayEditorShowSceneNumbersOnRightKey,
                 kComponentsScreenplayEditorShowDialogueNumbersKey,
                 kComponentsScreenplayDurationTypeKey,
                 kComponentsScreenplayDurationByPageDurationKey,
                 kComponentsScreenplayDurationByCharactersCharactersKey,
                 kComponentsScreenplayDurationByCharactersIncludeSpacesKey,
                 kComponentsScreenplayDurationByCharactersDurationKey,
                 kComponentsScreenplayDurationConfigurableSecondsPerParagraphForActionKey,
                 kComponentsScreenplayDurationConfigurableSecondsPerEvery50ForActionKey,
                 kComponentsScreenplayDurationConfigurableSecondsPerParagraphForDialogueKey,
                 kComponentsScreenplayDurationConfigurableSecondsPerEvery50ForDialogueKey,
                 kComponentsScreenplayDurationConfigurableSecondsPerParagraphForSceneHeadingKey,
                 kComponentsScreenplayDurationConfigurableSecondsPerEvery50ForSceneHeadingKey,
             }) {
            setSettingsValueForSession(key, {});
        }

        //
        // ... восстановим шаблон оформления
        //
        BusinessLayer::TemplatesFacade::setDefaultScreenplayTemplate(
            settingsValue(kComponentsScreenplayEditorDefaultTemplateKey).toString());
    }
}


// ****


ProjectInformationModel::ProjectInformationModel(QObject* _parent)
    : AbstractModel(
          {
              kDocumentKey,
              kNameKey,
              kLoglineKey,
              kCoverKey,
              kOverrideSystemSettingsKey,
              kTemplateIdKey,
              kShowSceneNumbersKey,
              kShowSceneNumbersOnLeftKey,
              kShowScenesNumbersOnRightKey,
              kShowDialoguesNumbersKey,
              kChronomertyOptionsKey,
          },
          _parent)
    , d(new Implementation)
{
    connect(this, &ProjectInformationModel::nameChanged, this,
            &ProjectInformationModel::updateDocumentContent);
    connect(this, &ProjectInformationModel::loglineChanged, this,
            &ProjectInformationModel::updateDocumentContent);
    connect(this, &ProjectInformationModel::coverChanged, this,
            &ProjectInformationModel::updateDocumentContent);
    connect(this, &ProjectInformationModel::overrideCommonSettingsForScreenplayChanged, this,
            &ProjectInformationModel::updateDocumentContent);
    connect(this, &ProjectInformationModel::templateIdForScreenplayChanged, this,
            &ProjectInformationModel::updateDocumentContent);
    connect(this, &ProjectInformationModel::showSceneNumbersForScreenplayChanged, this,
            &ProjectInformationModel::updateDocumentContent);
    connect(this, &ProjectInformationModel::showSceneNumbersOnLeftForScreenplayChanged, this,
            &ProjectInformationModel::updateDocumentContent);
    connect(this, &ProjectInformationModel::showSceneNumbersOnRightForScreenplayChanged, this,
            &ProjectInformationModel::updateDocumentContent);
    connect(this, &ProjectInformationModel::showDialoguesNumbersForScreenplayChanged, this,
            &ProjectInformationModel::updateDocumentContent);
    connect(this, &ProjectInformationModel::chronometerOptionsForScreenplayChanged, this,
            &ProjectInformationModel::updateDocumentContent);
}

ProjectInformationModel::~ProjectInformationModel() = default;

const QString& ProjectInformationModel::name() const
{
    return d->name;
}

void ProjectInformationModel::setName(const QString& _name)
{
    if (d->name == _name) {
        return;
    }

    d->name = _name;
    emit nameChanged(d->name);
    emit documentNameChanged(d->name);
}

QString ProjectInformationModel::documentName() const
{
    return name();
}

void ProjectInformationModel::setDocumentName(const QString& _name)
{
    setName(_name);
}

const QString& ProjectInformationModel::logline() const
{
    return d->logline;
}

void ProjectInformationModel::setLogline(const QString& _logline)
{
    if (d->logline == _logline) {
        return;
    }

    d->logline = _logline;
    emit loglineChanged(d->logline);
}

const QPixmap& ProjectInformationModel::cover() const
{
    return d->cover.image;
}

void ProjectInformationModel::setCover(const QPixmap& _cover)
{
    //
    // Если изображения одинаковые, или если оба пусты, то ничего не делаем
    //
    if (_cover.cacheKey() == d->cover.image.cacheKey()
        || (_cover.isNull() && d->cover.image.isNull())) {
        return;
    }

    //
    // Если ранее обложка была задана, то удалим её
    //
    if (!d->cover.uuid.isNull()) {
        imageWrapper()->remove(d->cover.uuid);
        d->cover = {};
    }

    //
    // Если задана новая обложка, то добавляем её
    //
    if (!_cover.isNull()) {
        d->cover.uuid = imageWrapper()->save(_cover);
        d->cover.image = _cover;
    }

    emit coverChanged(d->cover.image);
}

void ProjectInformationModel::setCover(const QUuid& _uuid, const QPixmap& _cover)
{
    if (d->cover.uuid == _uuid && _cover.cacheKey() == d->cover.image.cacheKey()) {
        return;
    }

    d->cover.image = _cover;
    if (d->cover.uuid != _uuid) {
        d->cover.uuid = _uuid;
    }
    emit coverChanged(d->cover.image);
}

bool ProjectInformationModel::overrideCommonSettingsForScreenplay() const
{
    return d->screenplay.overrideCommonSettings;
}

void ProjectInformationModel::setOverrideCommonSettingsForScreenplay(bool _override)
{
    if (d->screenplay.overrideCommonSettings == _override) {
        return;
    }

    d->screenplay.overrideCommonSettings = _override;

    //
    // При включении/выключении кастомных параметров, сбрасываем до стандартных
    //
    using namespace DataStorageLayer;
    setTemplateIdForScreenplay(TemplatesFacade::screenplayTemplate().id());
    setShowSceneNumbersForScreenplay(
        settingsValue(kComponentsScreenplayEditorShowSceneNumbersKey).toBool());
    setShowSceneNumbersOnLeftForScreenplay(
        settingsValue(kComponentsScreenplayEditorShowSceneNumbersOnLeftKey).toBool());
    setShowSceneNumbersOnRightForScreenplay(
        settingsValue(kComponentsScreenplayEditorShowSceneNumbersOnRightKey).toBool());
    setShowDialoguesNumbersForScreenplay(
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
    setChronometerOptionsForScreenplay(options);

    //
    // Обновим параметры сессии
    //
    d->updateSessionSettings();

    //
    // Уведомляем клиентов
    //
    emit overrideCommonSettingsForScreenplayChanged(d->screenplay.overrideCommonSettings);
}

QString ProjectInformationModel::templateIdForScreenplay() const
{
    if (d->screenplay.overrideCommonSettings) {
        return d->screenplay.templateId;
    }

    return TemplatesFacade::screenplayTemplate().id();
}

void ProjectInformationModel::setTemplateIdForScreenplay(const QString& _templateId)
{
    if (d->screenplay.templateId == _templateId) {
        return;
    }

    d->screenplay.templateId = _templateId;
    d->updateSessionSettings();
    emit templateIdForScreenplayChanged(d->screenplay.templateId);

    // TemplatesFacade::screenplayTemplate(d->templateId).saveToFile()
}

bool ProjectInformationModel::showSceneNumbersForScreenplay() const
{
    if (d->screenplay.overrideCommonSettings) {
        return d->screenplay.showSceneNumbers;
    }

    return settingsValue(DataStorageLayer::kComponentsScreenplayEditorShowSceneNumbersKey).toBool();
}

void ProjectInformationModel::setShowSceneNumbersForScreenplay(bool _show)
{
    if (d->screenplay.showSceneNumbers == _show) {
        return;
    }

    d->screenplay.showSceneNumbers = _show;
    d->updateSessionSettings();
    emit showSceneNumbersForScreenplayChanged(d->screenplay.showSceneNumbers);
}

bool ProjectInformationModel::showSceneNumbersOnLeftForScreenplay() const
{
    if (d->screenplay.overrideCommonSettings) {
        return d->screenplay.showSceneNumbersOnLeft;
    }

    return settingsValue(DataStorageLayer::kComponentsScreenplayEditorShowSceneNumbersOnLeftKey)
        .toBool();
}

void ProjectInformationModel::setShowSceneNumbersOnLeftForScreenplay(bool _show)
{
    if (d->screenplay.showSceneNumbersOnLeft == _show) {
        return;
    }

    d->screenplay.showSceneNumbersOnLeft = _show;
    d->updateSessionSettings();
    emit showSceneNumbersOnLeftForScreenplayChanged(d->screenplay.showSceneNumbersOnLeft);
}

bool ProjectInformationModel::showSceneNumbersOnRightForScreenplay() const
{
    if (d->screenplay.overrideCommonSettings) {
        return d->screenplay.showSceneNumbersOnRight;
    }

    return settingsValue(DataStorageLayer::kComponentsScreenplayEditorShowSceneNumbersOnRightKey)
        .toBool();
}

void ProjectInformationModel::setShowSceneNumbersOnRightForScreenplay(bool _show)
{
    if (d->screenplay.showSceneNumbersOnRight == _show) {
        return;
    }

    d->screenplay.showSceneNumbersOnRight = _show;
    d->updateSessionSettings();
    emit showSceneNumbersOnRightForScreenplayChanged(d->screenplay.showSceneNumbersOnRight);
}

bool ProjectInformationModel::showDialoguesNumbersForScreenplay() const
{
    if (d->screenplay.overrideCommonSettings) {
        return d->screenplay.showDialoguesNumbers;
    }

    return settingsValue(DataStorageLayer::kComponentsScreenplayEditorShowDialogueNumbersKey)
        .toBool();
}

void ProjectInformationModel::setShowDialoguesNumbersForScreenplay(bool _show)
{
    if (d->screenplay.showDialoguesNumbers == _show) {
        return;
    }

    d->screenplay.showDialoguesNumbers = _show;
    d->updateSessionSettings();
    emit showDialoguesNumbersForScreenplayChanged(d->screenplay.showDialoguesNumbers);
}

ChronometerOptions ProjectInformationModel::chronometerOptionsForScreenplay() const
{
    if (d->screenplay.overrideCommonSettings) {
        return d->screenplay.chronometerOptions;
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

void ProjectInformationModel::setChronometerOptionsForScreenplay(const ChronometerOptions& _options)
{
    if (d->screenplay.chronometerOptions == _options) {
        return;
    }

    d->screenplay.chronometerOptions = _options;
    d->updateSessionSettings();
    emit chronometerOptionsForScreenplayChanged(_options);
}

StructureModel* ProjectInformationModel::structureModel() const
{
    return d->structureModel;
}

void ProjectInformationModel::setStructureModel(StructureModel* _model)
{
    d->structureModel = _model;
}

QVector<Domain::ProjectCollaboratorInfo> ProjectInformationModel::collaborators() const
{
    return d->collaborators;
}

void ProjectInformationModel::setCollaborators(
    const QVector<Domain::ProjectCollaboratorInfo>& _collaborators)
{
    if (d->collaborators == _collaborators) {
        return;
    }

    d->collaborators = _collaborators;
    emit collaboratorsChanged(d->collaborators);
}

QVector<Domain::TeamMemberInfo> ProjectInformationModel::teammates() const
{
    return d->teammates;
}

void ProjectInformationModel::setTeammates(const QVector<Domain::TeamMemberInfo>& _teammates)
{
    if (d->teammates == _teammates) {
        return;
    }

    d->teammates = _teammates;
    emit teammatesChanged(d->teammates);
}

void ProjectInformationModel::initImageWrapper()
{
    connect(imageWrapper(), &AbstractImageWrapper::imageUpdated, this,
            [this](const QUuid& _uuid, const QPixmap& _image) {
                if (_uuid != d->cover.uuid) {
                    return;
                }

                setCover(_uuid, _image);
            });
}

void ProjectInformationModel::initDocument()
{
    if (document() == nullptr) {
        return;
    }

    QDomDocument domDocument;
    domDocument.setContent(document()->content());
    const auto documentNode = domDocument.firstChildElement(kDocumentKey);
    d->name = documentNode.firstChildElement(kNameKey).text();
    d->logline = documentNode.firstChildElement(kLoglineKey).text();
    d->cover.uuid = QUuid::fromString(documentNode.firstChildElement(kCoverKey).text());
    d->cover.image = imageWrapper()->load(d->cover.uuid);
    d->screenplay.overrideCommonSettings
        = documentNode.firstChildElement(kOverrideSystemSettingsKey).text() == "true";
    d->screenplay.templateId = documentNode.firstChildElement(kTemplateIdKey).text();
    d->screenplay.showSceneNumbers
        = documentNode.firstChildElement(kShowSceneNumbersKey).text() == "true";
    d->screenplay.showSceneNumbersOnLeft
        = documentNode.firstChildElement(kShowSceneNumbersOnLeftKey).text() == "true";
    d->screenplay.showSceneNumbersOnRight
        = documentNode.firstChildElement(kShowScenesNumbersOnRightKey).text() == "true";
    d->screenplay.showDialoguesNumbers
        = documentNode.firstChildElement(kShowDialoguesNumbersKey).text() == "true";
    //
    // TODO: выпилить в одной из будущих версий
    //
    if (const auto node = documentNode.firstChildElement(kChronomertyOptionsKey); !node.isNull()) {
        d->screenplay.chronometerOptions.type
            = static_cast<ChronometerType>(node.attribute("type").toInt());
        switch (d->screenplay.chronometerOptions.type) {
        default:
        case ChronometerType::Page: {
            d->screenplay.chronometerOptions.page.seconds = node.attribute("seconds").toInt();
            break;
        }

        case ChronometerType::Characters: {
            d->screenplay.chronometerOptions.characters.seconds
                = node.attribute("characters").toInt();
            d->screenplay.chronometerOptions.characters.seconds
                = node.attribute("consider_spaces") == "true";
            d->screenplay.chronometerOptions.characters.seconds = node.attribute("seconds").toInt();
            break;
        }

        case ChronometerType::Sophocles: {
            d->screenplay.chronometerOptions.sophocles.secsPerAction
                = node.attribute("spa").toDouble();
            d->screenplay.chronometerOptions.sophocles.secsPerEvery50Action
                = node.attribute("sp50a").toDouble();
            d->screenplay.chronometerOptions.sophocles.secsPerDialogue
                = node.attribute("spd").toDouble();
            d->screenplay.chronometerOptions.sophocles.secsPerEvery50Dialogue
                = node.attribute("sp50d").toDouble();
            d->screenplay.chronometerOptions.sophocles.secsPerSceneHeading
                = node.attribute("spsh").toDouble();
            d->screenplay.chronometerOptions.sophocles.secsPerEvery50SceneHeading
                = node.attribute("sp50sh").toDouble();
            break;
        }
        }
    }

    d->updateSessionSettings();
}

void ProjectInformationModel::clearDocument()
{
    //
    // Сохраняем указатель на модель структуры, чтобы после очистки восстановить его
    //
    auto structureModel = d->structureModel;

    d.reset(new Implementation);

    d->structureModel = structureModel;
}

QByteArray ProjectInformationModel::toXml() const
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
    writeTag(kLoglineKey, d->logline);
    writeTag(kCoverKey, d->cover.uuid.toString());
    writeBoolTag(kOverrideSystemSettingsKey, d->screenplay.overrideCommonSettings);
    writeTag(kTemplateIdKey, d->screenplay.templateId);
    writeBoolTag(kShowSceneNumbersKey, d->screenplay.showSceneNumbers);
    writeBoolTag(kShowSceneNumbersOnLeftKey, d->screenplay.showSceneNumbersOnLeft);
    writeBoolTag(kShowScenesNumbersOnRightKey, d->screenplay.showSceneNumbersOnRight);
    writeBoolTag(kShowDialoguesNumbersKey, d->screenplay.showDialoguesNumbers);
    writeChronometerOptions(kChronomertyOptionsKey, d->screenplay.chronometerOptions);
    xml += QString("</%1>").arg(kDocumentKey).toUtf8();
    return xml;
}

ChangeCursor ProjectInformationModel::applyPatch(const QByteArray& _patch)
{
    //
    // Определить область изменения в xml
    //
    auto changes = dmpController().changedXml(toXml(), _patch);
    if (changes.first.xml.isEmpty() && changes.second.xml.isEmpty()) {
        Log::warning("Project information model patch don't lead to any changes");
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
    if (auto nameNode = documentNode.firstChildElement(kNameKey); !nameNode.isNull()) {
        setName(nameNode.text());
    }
    if (auto loglineNode = documentNode.firstChildElement(kLoglineKey); !loglineNode.isNull()) {
        setLogline(loglineNode.text());
    }

    using M = ProjectInformationModel;
    const auto _1 = std::placeholders::_1;
    setText(kNameKey, std::bind(&M::setName, this, _1));
    setText(kLoglineKey, std::bind(&M::setLogline, this, _1));
    if (auto coverNode = documentNode.firstChildElement(kCoverKey); !coverNode.isNull()) {
        const auto coverUuid = QUuid(coverNode.text());
        setCover(coverUuid, imageWrapper()->load(coverUuid));
    }
    setBool(kOverrideSystemSettingsKey,
            std::bind(&M::setOverrideCommonSettingsForScreenplay, this, _1));
    setText(kTemplateIdKey, std::bind(&M::setTemplateIdForScreenplay, this, _1));
    setBool(kShowSceneNumbersKey, std::bind(&M::setShowSceneNumbersForScreenplay, this, _1));
    setBool(kShowSceneNumbersOnLeftKey,
            std::bind(&M::setShowSceneNumbersOnLeftForScreenplay, this, _1));
    setBool(kShowScenesNumbersOnRightKey,
            std::bind(&M::setShowSceneNumbersOnRightForScreenplay, this, _1));
    setBool(kShowDialoguesNumbersKey,
            std::bind(&M::setShowDialoguesNumbersForScreenplay, this, _1));
    setChronometerOptions(kChronomertyOptionsKey,
                          std::bind(&M::setChronometerOptionsForScreenplay, this, _1));


    d->updateSessionSettings();

    return {};
}

} // namespace BusinessLayer
