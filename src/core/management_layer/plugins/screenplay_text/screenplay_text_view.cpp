#include "screenplay_text_view.h"

#include "text/screenplay_text_edit.h"
#include "text/screenplay_text_edit_shortcuts_manager.h"
#include "text/screenplay_text_edit_toolbar.h"
#include "ui/compliance_check_result_view.h"
#include "ui/dictionaries_view.h"

#include <business_layer/document/text/text_block_data.h>
#include <business_layer/document/text/text_cursor.h>
#include <business_layer/document/screenplay/text/screenplay_text_document.h>
#include <business_layer/document/simple_text/simple_text_document.h>
#include <business_layer/model/characters/character_model.h>
#include <business_layer/model/characters/characters_model.h>
#include <business_layer/model/locations/locations_model.h>
#include <business_layer/model/screenplay/screenplay_dictionaries_model.h>
#include <business_layer/model/screenplay/screenplay_information_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_folder_item.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_scene_item.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_text_item.h>
#include <business_layer/model/simple_text/simple_text_model.h>
#include <business_layer/templates/screenplay_template.h>
#include <business_layer/templates/templates_facade.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <domain/document_object.h>
#include <domain/starcloud_api.h>
#include <interfaces/management_layer/i_document_manager.h>
#include <ui/design_system/design_system.h>
#include <ui/modules/ai_assistant/ai_assistant_view.h>
#include <ui/modules/bookmarks/bookmarks_model.h>
#include <ui/modules/bookmarks/bookmarks_view.h>
#include <ui/modules/cards/card_item_parameters_view.h>
#include <ui/modules/comments/comments_model.h>
#include <ui/modules/comments/comments_toolbar.h>
#include <ui/modules/comments/comments_view.h>
#include <ui/modules/fast_format_widget/fast_format_widget.h>
#include <ui/modules/search_toolbar/search_manager.h>
#include <ui/modules/text_scrollbar_manager/screenplay_text_scrollbar_manager.h>
#include <ui/widgets/floating_tool_bar/floating_toolbar_animator.h>
#include <ui/widgets/dialog/dialog.h>
#include <ui/widgets/shadow/shadow.h>
#include <ui/widgets/splitter/splitter.h>
#include <ui/widgets/stack_widget/stack_widget.h>
#include <ui/widgets/tab_bar/tab_bar.h>
#include <ui/widgets/task_bar/task_bar.h>
#include <ui/widgets/text_edit/completer/completer.h>
#include <ui/widgets/text_edit/scalable_wrapper/scalable_wrapper.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/measurement_helper.h>
#include <utils/helpers/text_helper.h>
#include <utils/helpers/ui_helper.h>
#include <utils/logging.h>
#include <utils/tools/debouncer.h>

#include <QAction>
#include <QDateTime>
#include <QElapsedTimer>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QInputDialog>
#include <QPointer>
#include <QRegularExpression>
#include <QSettings>
#include <QSet>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QTextBlock>
#include <QTimer>
#include <QVBoxLayout>

#include <optional>
#include <functional>

namespace Ui {

namespace {
const int kTypeDataRole = Qt::UserRole + 100;

enum {
    kFastFormatTabIndex = 0,
    kSceneParametersTabIndex,
    kCommentsTabIndex,
    kAiAssistantTabIndex,
    kBookmarksTabIndex,
    kDictionariesTabIndex,
    kComplianceCheckResultTabIndex,
};

const QString kSettingsKey = "screenplay-text";
const QString kScaleFactorKey = kSettingsKey + "/scale-factor";
const QString kSidebarStateKey = kSettingsKey + "/sidebar-state";
const QString kIsFastFormatPanelVisibleKey = kSettingsKey + "/is-fast-format-panel-visible";
const QString kIsBeatsVisibleKey = kSettingsKey + "/is-beats-visible";
const QString kIsCommentsModeEnabledKey = kSettingsKey + "/is-comments-mode-enabled";
const QString kIsAiAssistantEnabledKey = kSettingsKey + "/is-ai-assistant-enabled";
const QString kIsItemIsolationEnabledKey = kSettingsKey + "/is-item-isolation-enabled";
const QString kIsSceneParametersVisibleKey = kSettingsKey + "/is-scene-parameters-visible";
const QString kIsBookmarksListVisibleKey = kSettingsKey + "/is-bookmarks-list-visible";
const QString kIsDictionariesVisibleKey = kSettingsKey + "/is-dictionaries-visible";
const QString kIsComplianceCheckResultVisibleKey
    = kSettingsKey + "/is-compliance-check-result-visible";
const QString kSidebarPanelIndexKey = kSettingsKey + "/sidebar-panel-index";
const QString kStoryAssistantSkillSettingsKey = "codex/story-assistant/skill";

QString screenplayFountainSnapshot(BusinessLayer::ScreenplayTextModel* _model)
{
    if (_model == nullptr) {
        return {};
    }
    QStringList scenes;
    std::function<void(const QModelIndex&)> appendScenes;
    appendScenes = [&appendScenes, _model, &scenes](const QModelIndex& _parent) {
        for (int row = 0; row < _model->rowCount(_parent); ++row) {
            const auto index = _model->index(row, 0, _parent);
            const auto item = _model->itemForIndex(index);
            if (item->type() == BusinessLayer::TextModelItemType::Folder) {
                appendScenes(index);
            } else if (item->type() == BusinessLayer::TextModelItemType::Group
                       && item->subtype()
                           == static_cast<int>(BusinessLayer::TextGroupType::Scene)) {
                const auto scene
                    = static_cast<const BusinessLayer::ScreenplayTextModelSceneItem*>(item);
                scenes.append(scene->text().trimmed());
            }
        }
    };
    appendScenes({});
    return scenes.join("\n\n").trimmed();
}

QString simpleTextSnapshot(BusinessLayer::SimpleTextModel* _model)
{
    if (_model == nullptr) {
        return {};
    }
    BusinessLayer::SimpleTextDocument document;
    document.setModel(_model, false);
    return document.toPlainText().trimmed();
}

QStringList treatmentParagraphs(BusinessLayer::ScreenplayTextModel* _model)
{
    if (_model == nullptr) {
        return {};
    }
    BusinessLayer::ScreenplayTextDocument document;
    document.setTreatmentDocument(true);
    document.setModel(_model, false);
    QStringList paragraphs;
    for (auto block = document.begin(); block.isValid(); block = block.next()) {
        if (block.isVisible() && block.userData() != nullptr) {
            paragraphs.append(block.text());
        }
    }
    return paragraphs;
}

QString treatmentSnapshot(BusinessLayer::ScreenplayTextModel* _model)
{
    return treatmentParagraphs(_model).join('\n');
}

bool replaceSimpleText(BusinessLayer::SimpleTextModel* _model, const QString& _text)
{
    if (_model == nullptr) {
        return false;
    }
    BusinessLayer::SimpleTextDocument document;
    document.setModel(_model);
    QTextCursor cursor(&document);
    cursor.beginEditBlock();
    cursor.select(QTextCursor::Document);
    cursor.insertText(_text.trimmed());
    cursor.endEditBlock();
    document.setModel(nullptr);
    return simpleTextSnapshot(_model) == _text.trimmed();
}

bool replaceTreatmentParagraphs(BusinessLayer::ScreenplayTextModel* _model,
                                QString _replacement)
{
    if (_model == nullptr) {
        return false;
    }
    _replacement.replace("\r\n", "\n");
    _replacement.replace('\r', '\n');
    while (_replacement.endsWith('\n')) {
        _replacement.chop(1);
    }
    const auto replacements = _replacement.split('\n', Qt::KeepEmptyParts);

    BusinessLayer::ScreenplayTextDocument document;
    document.setTreatmentDocument(true);
    document.setModel(_model);
    QVector<QTextBlock> blocks;
    for (auto block = document.begin(); block.isValid(); block = block.next()) {
        if (block.isVisible() && block.userData() != nullptr) {
            blocks.append(block);
        }
    }
    if (blocks.isEmpty() || blocks.size() != replacements.size()) {
        document.setModel(nullptr);
        return false;
    }

    QTextCursor cursor(&document);
    cursor.beginEditBlock();
    for (int index = blocks.size() - 1; index >= 0; --index) {
        cursor.setPosition(blocks.at(index).position());
        cursor.setPosition(blocks.at(index).position() + blocks.at(index).length() - 1,
                           QTextCursor::KeepAnchor);
        cursor.insertText(replacements.at(index));
    }
    cursor.endEditBlock();
    document.setModel(nullptr);
    return treatmentSnapshot(_model) == replacements.join('\n');
}

QString characterRoleValue(BusinessLayer::CharacterStoryRole _role)
{
    switch (_role) {
    case BusinessLayer::CharacterStoryRole::Primary: return "primary";
    case BusinessLayer::CharacterStoryRole::Secondary: return "secondary";
    case BusinessLayer::CharacterStoryRole::Tertiary: return "tertiary";
    case BusinessLayer::CharacterStoryRole::Undefined: return "undefined";
    }
    return "undefined";
}

bool characterRoleFromValue(const QString& _value,
                            BusinessLayer::CharacterStoryRole* _role)
{
    if (_role == nullptr) return false;
    if (_value == "primary") * _role = BusinessLayer::CharacterStoryRole::Primary;
    else if (_value == "secondary") * _role = BusinessLayer::CharacterStoryRole::Secondary;
    else if (_value == "tertiary") * _role = BusinessLayer::CharacterStoryRole::Tertiary;
    else if (_value == "undefined") * _role = BusinessLayer::CharacterStoryRole::Undefined;
    else return false;
    return true;
}

const QSet<QString>& editableCharacterFields()
{
    static const QSet<QString> fields{
        "name", "story_role", "age", "nickname", "one_sentence_description",
        "long_description", "family", "personality", "motivation", "moral",
        "greatest_fear", "secrets", "short_term_goal", "long_term_goal",
        "initial_beliefs", "changed_beliefs", "plot_involvement", "conflict", "speech",
    };
    return fields;
}

QString characterFieldValue(BusinessLayer::CharacterModel* _character,
                            const QString& _field)
{
    if (_character == nullptr) return {};
    if (_field == "name") return _character->name();
    if (_field == "story_role") return characterRoleValue(_character->storyRole());
    if (_field == "age") return _character->age();
    if (_field == "nickname") return _character->nickname();
    if (_field == "one_sentence_description") return _character->oneSentenceDescription();
    if (_field == "long_description") return _character->longDescription();
    if (_field == "family") return _character->family();
    if (_field == "personality") return _character->personality();
    if (_field == "motivation") return _character->motivation();
    if (_field == "moral") return _character->moral();
    if (_field == "greatest_fear") return _character->greatestFear();
    if (_field == "secrets") return _character->secrets();
    if (_field == "short_term_goal") return _character->shortTermGoal();
    if (_field == "long_term_goal") return _character->longTermGoal();
    if (_field == "initial_beliefs") return _character->initialBeliefs();
    if (_field == "changed_beliefs") return _character->changedBeliefs();
    if (_field == "plot_involvement") return _character->plotInvolvement();
    if (_field == "conflict") return _character->conflict();
    if (_field == "speech") return _character->speech();
    return {};
}

bool setCharacterField(BusinessLayer::CharacterModel* _character, const QString& _field,
                       const QString& _value)
{
    if (_character == nullptr || !editableCharacterFields().contains(_field)) return false;
    if (_field == "name") _character->setName(_value);
    else if (_field == "story_role") {
        BusinessLayer::CharacterStoryRole role;
        if (!characterRoleFromValue(_value, &role)) return false;
        _character->setStoryRole(role);
    } else if (_field == "age") _character->setAge(_value);
    else if (_field == "nickname") _character->setNickname(_value);
    else if (_field == "one_sentence_description") _character->setOneSentenceDescription(_value);
    else if (_field == "long_description") _character->setLongDescription(_value);
    else if (_field == "family") _character->setFamily(_value);
    else if (_field == "personality") _character->setPersonality(_value);
    else if (_field == "motivation") _character->setMotivation(_value);
    else if (_field == "moral") _character->setMoral(_value);
    else if (_field == "greatest_fear") _character->setGreatestFear(_value);
    else if (_field == "secrets") _character->setSecrets(_value);
    else if (_field == "short_term_goal") _character->setShortTermGoal(_value);
    else if (_field == "long_term_goal") _character->setLongTermGoal(_value);
    else if (_field == "initial_beliefs") _character->setInitialBeliefs(_value);
    else if (_field == "changed_beliefs") _character->setChangedBeliefs(_value);
    else if (_field == "plot_involvement") _character->setPlotInvolvement(_value);
    else if (_field == "conflict") _character->setConflict(_value);
    else if (_field == "speech") _character->setSpeech(_value);
    return characterFieldValue(_character, _field) == (_field == "name" ? _value.simplified()
                                                                          : _value);
}

QString characterSnapshot(BusinessLayer::CharacterModel* _character)
{
    if (_character == nullptr || _character->document() == nullptr) return {};
    QJsonObject fields;
    QStringList orderedFields = editableCharacterFields().values();
    std::sort(orderedFields.begin(), orderedFields.end());
    for (const auto& field : orderedFields) {
        fields.insert(field, characterFieldValue(_character, field));
    }
    const QJsonObject snapshot{
        { "entityId", _character->document()->uuid().toString(QUuid::WithoutBraces) },
        { "entityName", _character->name() },
        { "fields", fields },
    };
    return QString::fromUtf8(QJsonDocument(snapshot).toJson(QJsonDocument::Indented)).trimmed();
}

bool restoreCharacterSnapshot(BusinessLayer::CharactersModel* _characters,
                              const QString& _snapshot)
{
    if (_characters == nullptr) return false;
    const auto snapshot = QJsonDocument::fromJson(_snapshot.toUtf8()).object();
    const auto character = _characters->character(QUuid(snapshot.value("entityId").toString()));
    const auto fields = snapshot.value("fields").toObject();
    if (character == nullptr || fields.isEmpty()) return false;
    const auto savedName = fields.value("name").toString().simplified();
    const auto nameOwner = _characters->character(savedName);
    if (nameOwner != nullptr && nameOwner != character) return false;
    for (auto it = fields.constBegin(); it != fields.constEnd(); ++it) {
        if (!setCharacterField(character, it.key(), it.value().toString())) return false;
    }
    return characterSnapshot(character) == _snapshot.trimmed();
}

QString relationshipSnapshot(BusinessLayer::CharacterModel* _character,
                             const QUuid& _relatedCharacter)
{
    if (_character == nullptr || _character->document() == nullptr) return {};
    const auto relation = _character->relation(_relatedCharacter);
    const QJsonObject snapshot{
        { "entityId", _character->document()->uuid().toString(QUuid::WithoutBraces) },
        { "relatedCharacterId", _relatedCharacter.toString(QUuid::WithoutBraces) },
        { "exists", relation.isValid() },
        { "feeling", relation.feeling },
        { "details", relation.details },
    };
    return QString::fromUtf8(QJsonDocument(snapshot).toJson(QJsonDocument::Indented)).trimmed();
}

bool restoreRelationshipSnapshot(BusinessLayer::CharactersModel* _characters,
                                 const QString& _snapshot)
{
    if (_characters == nullptr) return false;
    const auto snapshot = QJsonDocument::fromJson(_snapshot.toUtf8()).object();
    const auto character = _characters->character(QUuid(snapshot.value("entityId").toString()));
    const QUuid relatedCharacter(snapshot.value("relatedCharacterId").toString());
    if (character == nullptr || relatedCharacter.isNull()
        || _characters->character(relatedCharacter) == nullptr) return false;
    if (!snapshot.value("exists").toBool()) {
        character->removeRelationWith(relatedCharacter);
    } else {
        if (!character->relation(relatedCharacter).isValid()) character->createRelation(relatedCharacter);
        auto relation = character->relation(relatedCharacter);
        relation.feeling = snapshot.value("feeling").toString();
        relation.details = snapshot.value("details").toString();
        character->updateRelation(relation);
    }
    return relationshipSnapshot(character, relatedCharacter) == _snapshot.trimmed();
}

QString assistantEditHistoryKey(BusinessLayer::ScreenplayTextModel* _model)
{
    if (_model == nullptr || _model->document() == nullptr) {
        return {};
    }
    return QString("codex/edit-history/%1")
        .arg(_model->document()->uuid().toString(QUuid::WithoutBraces));
}

QString assistantStoryMemoryKey(BusinessLayer::ScreenplayTextModel* _model)
{
    if (_model == nullptr || _model->document() == nullptr) {
        return {};
    }
    return QString("codex/story-memory/%1")
        .arg(_model->document()->uuid().toString(QUuid::WithoutBraces));
}

QJsonObject loadAssistantStoryMemory(BusinessLayer::ScreenplayTextModel* _model)
{
    const auto key = assistantStoryMemoryKey(_model);
    if (key.isEmpty()) {
        return {};
    }
    const auto stored = QSettings().value(key).toByteArray();
    if (stored.isEmpty()) {
        return {};
    }
    auto json = qUncompress(stored);
    if (json.isEmpty()) {
        json = stored;
    }
    return QJsonDocument::fromJson(json).object();
}

void saveAssistantStoryMemory(BusinessLayer::ScreenplayTextModel* _model,
                              const QString& _content, bool _stale, bool _writerEdited)
{
    const auto key = assistantStoryMemoryKey(_model);
    if (key.isEmpty() || _content.trimmed().isEmpty()) {
        return;
    }
    const QJsonObject memory{
        { "version", 1 },
        { "updatedAt", QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs) },
        { "content", _content.trimmed() },
        { "stale", _stale },
        { "writerEdited", _writerEdited },
    };
    QSettings().setValue(key,
                         qCompress(QJsonDocument(memory).toJson(QJsonDocument::Compact), 9));
}

void markAssistantStoryMemoryStale(BusinessLayer::ScreenplayTextModel* _model)
{
    auto memory = loadAssistantStoryMemory(_model);
    if (memory.value("content").toString().isEmpty() || memory.value("stale").toBool()) {
        return;
    }
    memory.insert("stale", true);
    memory.insert("staleAt", QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
    QSettings().setValue(assistantStoryMemoryKey(_model),
                         qCompress(QJsonDocument(memory).toJson(QJsonDocument::Compact), 9));
}

int characterNameMentions(const QString& _text, const QString& _name)
{
    if (_text.isEmpty() || _name.isEmpty()) return 0;
    const QRegularExpression expression(
        QString("(?<![\\p{L}\\p{N}_])%1(?![\\p{L}\\p{N}_])")
            .arg(QRegularExpression::escape(_name)),
        QRegularExpression::CaseInsensitiveOption);
    int mentions = 0;
    auto matches = expression.globalMatch(_text);
    while (matches.hasNext()) {
        matches.next();
        ++mentions;
    }
    return mentions;
}

QJsonObject characterDependencySnapshot(BusinessLayer::ScreenplayTextModel* _model,
                                        BusinessLayer::CharacterModel* _character)
{
    if (_model == nullptr || _character == nullptr || _character->document() == nullptr) {
        return {};
    }
    const auto name = _character->name();
    _character->updateDialogues();
    QJsonArray dialogueDocuments;
    int dialogueCount = 0;
    for (const auto& dialogues : _character->dialogues()) {
        dialogueCount += dialogues.dialoguesIndexes.size();
        dialogueDocuments.append(QJsonObject{
            { "document", dialogues.documentName },
            { "count", dialogues.dialoguesIndexes.size() },
        });
    }

    QJsonArray outboundRelationships;
    for (const auto& relation : _character->relations()) {
        const auto other = _model->charactersModel()->character(relation.character);
        outboundRelationships.append(
            other != nullptr ? other->name() : relation.character.toString(QUuid::WithoutBraces));
    }
    QJsonArray inboundRelationships;
    const auto characterId = _character->document()->uuid();
    auto characters = _model->charactersModel();
    for (int row = 0; characters != nullptr && row < characters->rowCount(); ++row) {
        const auto other = characters->character(row);
        if (other == nullptr || other == _character) continue;
        if (other->relation(characterId).isValid()) inboundRelationships.append(other->name());
    }

    return QJsonObject{
        { "entityId", characterId.toString(QUuid::WithoutBraces) },
        { "entityName", name },
        { "currentScreenplayMentions",
          characterNameMentions(screenplayFountainSnapshot(_model), name) },
        { "dialogueLinesAcrossProject", dialogueCount },
        { "dialogueDocuments", dialogueDocuments },
        { "synopsisMentions",
          characterNameMentions(simpleTextSnapshot(_model->synopsisModel()), name) },
        { "treatmentMentions", characterNameMentions(treatmentSnapshot(_model), name) },
        { "storyMemoryMentions",
          characterNameMentions(loadAssistantStoryMemory(_model).value("content").toString(),
                                name) },
        { "outboundRelationships", outboundRelationships },
        { "inboundRelationships", inboundRelationships },
        { "photos", _character->photos().size() },
    };
}

bool hasCharacterDependencies(const QJsonObject& _snapshot)
{
    return _snapshot.value("currentScreenplayMentions").toInt() > 0
        || _snapshot.value("dialogueLinesAcrossProject").toInt() > 0
        || _snapshot.value("synopsisMentions").toInt() > 0
        || _snapshot.value("treatmentMentions").toInt() > 0
        || _snapshot.value("storyMemoryMentions").toInt() > 0
        || !_snapshot.value("outboundRelationships").toArray().isEmpty()
        || !_snapshot.value("inboundRelationships").toArray().isEmpty();
}

QString characterDependencyReport(const QJsonObject& _snapshot)
{
    QStringList dialogueDocuments;
    for (const auto& value : _snapshot.value("dialogueDocuments").toArray()) {
        const auto document = value.toObject();
        dialogueDocuments.append(QString("%1 (%2)")
                                     .arg(document.value("document").toString())
                                     .arg(document.value("count").toInt()));
    }
    auto stringArray = [](const QJsonArray& _array) {
        QStringList result;
        for (const auto& value : _array) result.append(value.toString());
        return result.isEmpty() ? QObject::tr("None") : result.join(", ");
    };
    return QObject::tr(
               "DEPENDENCY REPORT\n"
               "Current screenplay name mentions: %1\n"
               "Dialogue lines across project scripts: %2\n"
               "Dialogue documents: %3\n"
               "Synopsis mentions: %4\n"
               "Treatment mentions: %5\n"
               "Story Memory mentions: %6\n"
               "Relationships from this character: %7\n"
               "Relationships pointing to this character: %8\n"
               "Attached photos preserved in Recycle Bin: %9\n\n"
               "Removal moves only the native character profile to STARC's Recycle Bin. "
               "Screenplay, synopsis, treatment, and Story Memory text will remain unchanged. "
               "Relationship links remain stored and can become active again if the character "
               "is restored.")
        .arg(_snapshot.value("currentScreenplayMentions").toInt())
        .arg(_snapshot.value("dialogueLinesAcrossProject").toInt())
        .arg(dialogueDocuments.isEmpty() ? QObject::tr("None") : dialogueDocuments.join(", "))
        .arg(_snapshot.value("synopsisMentions").toInt())
        .arg(_snapshot.value("treatmentMentions").toInt())
        .arg(_snapshot.value("storyMemoryMentions").toInt())
        .arg(stringArray(_snapshot.value("outboundRelationships").toArray()))
        .arg(stringArray(_snapshot.value("inboundRelationships").toArray()))
        .arg(_snapshot.value("photos").toInt());
}

bool hasMeaningfulCharacterFieldValue(const QString& _field, const QString& _value)
{
    return !_value.trimmed().isEmpty() && !(_field == "story_role" && _value == "undefined");
}

bool mergeFieldPlanIsComplete(BusinessLayer::CharacterModel* _survivor,
                              BusinessLayer::CharacterModel* _source,
                              const QMap<QString, QString>& _changes)
{
    if (_survivor == nullptr || _source == nullptr) return false;
    for (const auto& field : editableCharacterFields()) {
        if (field == "name") continue;
        const auto sourceValue = characterFieldValue(_source, field);
        const auto survivorValue = characterFieldValue(_survivor, field);
        if (hasMeaningfulCharacterFieldValue(field, sourceValue)
            && sourceValue != survivorValue && !_changes.contains(field)) {
            return false;
        }
    }
    return true;
}

QString mergedRelationshipText(const QString& _survivorValue, const QString& _sourceValue)
{
    if (_sourceValue.trimmed().isEmpty() || _survivorValue.contains(_sourceValue)) {
        return _survivorValue;
    }
    if (_survivorValue.trimmed().isEmpty()) return _sourceValue;
    return QString("%1\n\n%2").arg(_survivorValue, _sourceValue);
}

QJsonObject characterMergePlan(BusinessLayer::ScreenplayTextModel* _model,
                               BusinessLayer::CharacterModel* _survivor,
                               BusinessLayer::CharacterModel* _source,
                               const QMap<QString, QString>& _changes)
{
    if (_model == nullptr || _survivor == nullptr || _source == nullptr
        || _survivor->document() == nullptr || _source->document() == nullptr) return {};
    QJsonArray fields;
    for (auto it = _changes.constBegin(); it != _changes.constEnd(); ++it) {
        if (!editableCharacterFields().contains(it.key())) continue;
        fields.append(QJsonObject{
            { "field", it.key() },
            { "survivorValue", characterFieldValue(_survivor, it.key()) },
            { "duplicateValue", characterFieldValue(_source, it.key()) },
            { "finalValue", it.value() },
        });
    }

    QSet<QUuid> survivorPhotos;
    for (const auto& photo : _survivor->photos()) survivorPhotos.insert(photo.uuid);
    int photosToTransfer = 0;
    for (const auto& photo : _source->photos()) {
        if (!survivorPhotos.contains(photo.uuid)) ++photosToTransfer;
    }

    QJsonArray outgoingTransfers;
    for (const auto& relation : _source->relations()) {
        if (relation.character == _survivor->document()->uuid()) continue;
        const auto other = _model->charactersModel()->character(relation.character);
        outgoingTransfers.append(other != nullptr
                                     ? other->name()
                                     : relation.character.toString(QUuid::WithoutBraces));
    }
    QJsonArray incomingTransfers;
    auto characters = _model->charactersModel();
    for (int row = 0; characters != nullptr && row < characters->rowCount(); ++row) {
        const auto other = characters->character(row);
        if (other == nullptr || other == _survivor || other == _source) continue;
        if (other->relation(_source->document()->uuid()).isValid()) {
            incomingTransfers.append(other->name());
        }
    }

    return QJsonObject{
        { "survivorId", _survivor->document()->uuid().toString(QUuid::WithoutBraces) },
        { "survivorName", _survivor->name() },
        { "sourceId", _source->document()->uuid().toString(QUuid::WithoutBraces) },
        { "sourceName", _source->name() },
        { "sourceDependencies", characterDependencySnapshot(_model, _source) },
        { "fieldResolutions", fields },
        { "photosToTransfer", photosToTransfer },
        { "outgoingRelationshipTransfers", outgoingTransfers },
        { "incomingRelationshipTransfers", incomingTransfers },
    };
}

QString characterMergePlanReport(const QJsonObject& _plan)
{
    QStringList fields;
    for (const auto& value : _plan.value("fieldResolutions").toArray()) {
        const auto field = value.toObject();
        auto label = field.value("field").toString();
        label.replace('_', ' ');
        fields.append(QString("%1\n− Survivor: %2\n− Duplicate: %3\n+ Final: %4")
                          .arg(label.toUpper(),
                               field.value("survivorValue").toString().isEmpty()
                                   ? QObject::tr("(empty)")
                                   : field.value("survivorValue").toString(),
                               field.value("duplicateValue").toString().isEmpty()
                                   ? QObject::tr("(empty)")
                                   : field.value("duplicateValue").toString(),
                               field.value("finalValue").toString().isEmpty()
                                   ? QObject::tr("(empty)")
                                   : field.value("finalValue").toString()));
    }
    auto names = [](const QJsonArray& _array) {
        QStringList result;
        for (const auto& value : _array) result.append(value.toString());
        return result.isEmpty() ? QObject::tr("None") : result.join(", ");
    };
    return QObject::tr(
               "MERGE PLAN\n"
               "Surviving character: %1\n"
               "Duplicate moved to Recycle Bin: %2\n"
               "Photos transferred: %3\n"
               "Outgoing relationships transferred: %4\n"
               "Incoming relationships reassigned: %5\n\n"
               "PROFILE FIELD RESOLUTIONS\n%6\n\n%7\n\n"
               "All native character cues named %2 will be reassigned to %1 across screenplay, "
               "comic-book, audioplay, and stageplay documents. Ordinary action and dialogue "
               "prose will not be search-and-replaced. The duplicate's complete original profile "
               "is preserved in Recycle Bin.")
        .arg(_plan.value("survivorName").toString(), _plan.value("sourceName").toString())
        .arg(_plan.value("photosToTransfer").toInt())
        .arg(names(_plan.value("outgoingRelationshipTransfers").toArray()))
        .arg(names(_plan.value("incomingRelationshipTransfers").toArray()))
        .arg(fields.isEmpty() ? QObject::tr("No survivor profile fields change.")
                              : fields.join("\n\n"))
        .arg(characterDependencyReport(_plan.value("sourceDependencies").toObject()));
}

void transferCharacterRelationships(BusinessLayer::CharactersModel* _characters,
                                    BusinessLayer::CharacterModel* _survivor,
                                    BusinessLayer::CharacterModel* _source)
{
    if (_characters == nullptr || _survivor == nullptr || _source == nullptr
        || _survivor->document() == nullptr || _source->document() == nullptr) return;
    const auto survivorId = _survivor->document()->uuid();
    const auto sourceId = _source->document()->uuid();
    for (const auto& sourceRelation : _source->relations()) {
        if (sourceRelation.character == survivorId) continue;
        if (!_survivor->relation(sourceRelation.character).isValid()) {
            _survivor->createRelation(sourceRelation.character);
        }
        auto survivorRelation = _survivor->relation(sourceRelation.character);
        survivorRelation.feeling
            = mergedRelationshipText(survivorRelation.feeling, sourceRelation.feeling);
        survivorRelation.details
            = mergedRelationshipText(survivorRelation.details, sourceRelation.details);
        _survivor->updateRelation(survivorRelation);
    }
    for (int row = 0; row < _characters->rowCount(); ++row) {
        const auto other = _characters->character(row);
        if (other == nullptr || other == _survivor || other == _source) continue;
        const auto sourceRelation = other->relation(sourceId);
        if (!sourceRelation.isValid()) continue;
        if (!other->relation(survivorId).isValid()) other->createRelation(survivorId);
        auto survivorRelation = other->relation(survivorId);
        survivorRelation.feeling
            = mergedRelationshipText(survivorRelation.feeling, sourceRelation.feeling);
        survivorRelation.details
            = mergedRelationshipText(survivorRelation.details, sourceRelation.details);
        other->updateRelation(survivorRelation);
    }
}

QJsonArray loadAssistantEditHistory(BusinessLayer::ScreenplayTextModel* _model)
{
    const auto key = assistantEditHistoryKey(_model);
    if (key.isEmpty()) {
        return {};
    }
    const auto stored = QSettings().value(key).toByteArray();
    if (stored.isEmpty()) {
        return {};
    }
    auto json = qUncompress(stored);
    if (json.isEmpty()) {
        json = stored;
    }
    return QJsonDocument::fromJson(json).object().value("entries").toArray();
}

QString characterMergeLatestTransactionKey(const QUuid& _sourceId)
{
    return QString("codex/character-merge/latest/%1")
        .arg(_sourceId.toString(QUuid::WithoutBraces));
}

QString characterMergeTransactionKey(const QString& _transactionId)
{
    return QString("codex/character-merge/transactions/%1").arg(_transactionId);
}

QJsonObject characterMergeTransaction(const QString& _transactionId)
{
    if (_transactionId.isEmpty()) return {};
    const auto stored = QSettings().value(characterMergeTransactionKey(_transactionId)).toByteArray();
    if (stored.isEmpty()) return {};
    auto json = qUncompress(stored);
    if (json.isEmpty()) json = stored;
    return QJsonDocument::fromJson(json).object();
}

void saveAssistantEditHistory(BusinessLayer::ScreenplayTextModel* _model,
                              const QJsonArray& _entries)
{
    const auto key = assistantEditHistoryKey(_model);
    if (key.isEmpty()) {
        return;
    }
    const QJsonObject state{ { "version", 1 }, { "entries", _entries } };
    QSettings().setValue(key,
                         qCompress(QJsonDocument(state).toJson(QJsonDocument::Compact), 9));
}

void recordAssistantEdit(BusinessLayer::ScreenplayTextModel* _model, const QString& _action,
                         const QString& _instruction, const QString& _summary,
                         const QString& _before, const QString& _after,
                         const QString& _target = QStringLiteral("screenplay"),
                         const QString& _impactSummary = {},
                         const QJsonArray& _continuityChecks = {})
{
    if (_model == nullptr || _before == _after) {
        return;
    }
    auto entries = loadAssistantEditHistory(_model);
    entries.append(QJsonObject{
        { "timestamp", QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs) },
        { "action", _action },
        { "instruction", _instruction },
        { "summary", _summary },
        { "target", _target },
        { "impactSummary", _impactSummary },
        { "continuityChecks", _continuityChecks },
        { "storyMethod", QSettings().value(kStoryAssistantSkillSettingsKey, "edit-story").toString() },
        { "before", _before },
        { "after", _after },
    });
    constexpr int maximumHistoryEntries = 100;
    while (entries.size() > maximumHistoryEntries) {
        entries.removeFirst();
    }
    saveAssistantEditHistory(_model, entries);
}

QString readableActionName(const QString& _action)
{
    if (_action == "insert_screenplay") return QObject::tr("Inserted screenplay writing");
    if (_action == "replace_selection") return QObject::tr("Replaced a selection");
    if (_action == "delete_selection") return QObject::tr("Deleted a selection");
    if (_action == "clear_screenplay") return QObject::tr("Cleared the screenplay");
    if (_action == "update_logline") return QObject::tr("Updated the logline");
    if (_action == "replace_synopsis") return QObject::tr("Replaced the synopsis");
    if (_action == "revise_treatment") return QObject::tr("Revised the treatment outline");
    if (_action == "create_character") return QObject::tr("Created a character profile");
    if (_action == "update_character") return QObject::tr("Updated a character profile");
    if (_action == "remove_character") return QObject::tr("Moved a character to Recycle Bin");
    if (_action == "merge_character") return QObject::tr("Merged duplicate characters");
    if (_action == "update_character_relationship") return QObject::tr("Updated a relationship");
    if (_action == "restore") return QObject::tr("Restored an earlier version");
    return QObject::tr("Edited the screenplay");
}

QString screenplayLineDiff(const QString& _before, const QString& _after)
{
    auto linesFor = [](const QString& _text) {
        return _text.isEmpty() ? QStringList{} : _text.split('\n');
    };
    const auto beforeLines = linesFor(_before);
    const auto afterLines = linesFor(_after);
    if (beforeLines.size() * afterLines.size() > 250000) {
        return QObject::tr("BEFORE\n%1\n\nAFTER\n%2").arg(_before, _after);
    }

    QVector<QVector<int>> lcs(beforeLines.size() + 1,
                              QVector<int>(afterLines.size() + 1));
    for (int before = beforeLines.size() - 1; before >= 0; --before) {
        for (int after = afterLines.size() - 1; after >= 0; --after) {
            lcs[before][after] = beforeLines.at(before) == afterLines.at(after)
                ? lcs.at(before + 1).at(after + 1) + 1
                : qMax(lcs.at(before + 1).at(after), lcs.at(before).at(after + 1));
        }
    }

    QStringList diff;
    int before = 0;
    int after = 0;
    while (before < beforeLines.size() || after < afterLines.size()) {
        if (before < beforeLines.size() && after < afterLines.size()
            && beforeLines.at(before) == afterLines.at(after)) {
            diff.append("  " + beforeLines.at(before));
            ++before;
            ++after;
        } else if (after < afterLines.size()
                   && (before == beforeLines.size()
                       || lcs.at(before).at(after + 1) >= lcs.at(before + 1).at(after))) {
            diff.append("+ " + afterLines.at(after++));
        } else {
            diff.append(QString::fromUtf8("− ") + beforeLines.at(before++));
        }
    }
    return diff.join('\n');
}

QString focusedScreenplayLineDiff(const QString& _before, const QString& _after)
{
    const auto beforeLines = _before.isEmpty() ? QStringList{} : _before.split('\n');
    const auto afterLines = _after.isEmpty() ? QStringList{} : _after.split('\n');
    int prefix = 0;
    while (prefix < beforeLines.size() && prefix < afterLines.size()
           && beforeLines.at(prefix) == afterLines.at(prefix)) {
        ++prefix;
    }
    int suffix = 0;
    while (suffix < beforeLines.size() - prefix && suffix < afterLines.size() - prefix
           && beforeLines.at(beforeLines.size() - suffix - 1)
               == afterLines.at(afterLines.size() - suffix - 1)) {
        ++suffix;
    }
    constexpr int contextLines = 3;
    const int beforeStart = qMax(0, prefix - contextLines);
    const int afterStart = qMax(0, prefix - contextLines);
    const int beforeEnd = qMin(beforeLines.size(), beforeLines.size() - suffix + contextLines);
    const int afterEnd = qMin(afterLines.size(), afterLines.size() - suffix + contextLines);
    const auto beforeFocus = beforeLines.mid(beforeStart, beforeEnd - beforeStart).join('\n');
    const auto afterFocus = afterLines.mid(afterStart, afterEnd - afterStart).join('\n');
    QString result = screenplayLineDiff(beforeFocus, afterFocus);
    if (beforeStart > 0 || afterStart > 0) {
        result.prepend(QObject::tr("  … earlier screenplay unchanged …\n"));
    }
    if (beforeEnd < beforeLines.size() || afterEnd < afterLines.size()) {
        result.append(QObject::tr("\n  … later screenplay unchanged …"));
    }
    return result;
}

bool hasCriticalContinuityFinding(const QJsonArray& _checks)
{
    return std::any_of(_checks.cbegin(), _checks.cend(), [](const QJsonValue& _value) {
        return _value.toObject().value("severity").toString() == "critical";
    });
}

QString continuityGateReport(const QString& _impactSummary, const QJsonArray& _checks)
{
    QStringList report{ QObject::tr("CONTINUITY GATE") };
    if (!_impactSummary.trimmed().isEmpty()) {
        report.append(QObject::tr("Impact: %1").arg(_impactSummary.trimmed()));
    }
    if (_checks.isEmpty()) {
        report.append(QObject::tr("No continuity conflicts were identified by Codex. This is an "
                                  "advisory check; review the writing yourself before approval."));
        return report.join('\n');
    }

    for (const auto& value : _checks) {
        const auto check = value.toObject();
        const auto severity = check.value("severity").toString();
        const auto severityLabel = severity == "critical"
            ? QObject::tr("CRITICAL")
            : severity == "caution" ? QObject::tr("CAUTION") : QObject::tr("SUGGESTION");
        report.append(QString("\n%1 — %2\n%3\n%4")
                          .arg(severityLabel,
                               check.value("category").toString().trimmed().toUpper(),
                               check.value("issue").toString().trimmed(),
                               QObject::tr("Evidence: %1")
                                   .arg(check.value("evidence").toString().trimmed())));
    }
    return report.join('\n');
}

QString continuityRevisionRequest(const QString& _action, const QString& _target,
                                  const QString& _instruction, const QString& _content,
                                  const QString& _gateReport)
{
    return QString(
               "Revise the previous proposal so it still fulfills the ORIGINAL WRITER "
               "INSTRUCTION but resolves the Continuity Gate findings. Return the same structured "
               "action type and target unless safety requires request_clarification. Re-run the "
               "Continuity Gate self-audit on the revision. Do not apply anything or explain the "
               "revision conversationally.\n\nORIGINAL ACTION: %1\nTARGET: %2\nORIGINAL WRITER "
               "INSTRUCTION:\n%3\n\nPROPOSED CONTENT:\n%4\n\nCONTINUITY GATE:\n%5")
        .arg(_action, _target, _instruction, _content, _gateReport);
}

QString entityRevisionRequest(const QJsonObject& _proposal, const QString& _instruction,
                              const QString& _gateReport)
{
    return QString(
               "Revise the previous structured character proposal so it still fulfills the "
               "ORIGINAL WRITER INSTRUCTION but resolves the Continuity Gate findings. Keep the "
               "same action and stable entity IDs unless safety requires request_clarification. "
               "Return exactly one schema-valid action object and re-run the Continuity Gate. "
               "Do not apply anything or explain conversationally.\n\nORIGINAL WRITER "
               "INSTRUCTION:\n%1\n\nPROPOSED STRUCTURED ACTION:\n%2\n\nCONTINUITY GATE:\n%3")
        .arg(_instruction,
             QString::fromUtf8(QJsonDocument(_proposal).toJson(QJsonDocument::Indented)),
             _gateReport);
}

QMap<QString, QString> fieldChangesMap(const QJsonArray& _changes, bool* _valid = nullptr)
{
    QMap<QString, QString> changes;
    bool valid = true;
    for (const auto& value : _changes) {
        const auto change = value.toObject();
        const auto field = change.value("field").toString();
        if (!value.isObject() || field.isEmpty() || !change.value("value").isString()
            || changes.contains(field)) {
            valid = false;
            break;
        }
        changes.insert(field, change.value("value").toString());
    }
    if (_valid != nullptr) *_valid = valid;
    return changes;
}

QString characterChangesPreview(BusinessLayer::CharacterModel* _character,
                                const QString& _newCharacterName,
                                const QMap<QString, QString>& _changes)
{
    QStringList lines;
    for (auto it = _changes.constBegin(); it != _changes.constEnd(); ++it) {
        const auto before = _character == nullptr ? QString() : characterFieldValue(_character, it.key());
        auto fieldLabel = it.key();
        fieldLabel.replace('_', ' ');
        lines.append(QString("%1\n− %2\n+ %3")
                         .arg(fieldLabel.toUpper(),
                              before.isEmpty() ? QObject::tr("(empty)") : before,
                              it.value().isEmpty() ? QObject::tr("(empty)") : it.value()));
    }
    if (_character == nullptr) {
        lines.prepend(QObject::tr("NEW CHARACTER\n+ %1").arg(_newCharacterName));
    }
    return lines.join("\n\n");
}

void applyAfterCriticalContinuityConfirmation(
    QWidget* _parent, const QJsonArray& _checks, const std::function<void()>& _apply,
    const std::function<void()>& _cancel)
{
    if (!hasCriticalContinuityFinding(_checks)) {
        _apply();
        return;
    }
    auto dialog = new Dialog(_parent);
    dialog->setContentFixedWidth(Ui::DesignSystem::dialog().maximumWidth());
    dialog->enableSupportingTextScrolling();
    dialog->setDismissOnOutsideClick(false);
    dialog->setRejectOnEscape(false);
    dialog->showDialog(
        QObject::tr("Confirmed-canon conflict"),
        QObject::tr("The Continuity Gate found at least one direct conflict with confirmed canon. "
                    "Approving may intentionally retcon the story. Review the evidence before "
                    "continuing.\n\n%1")
            .arg(continuityGateReport({}, _checks)),
        { { 0, QObject::tr("Do not apply"), Dialog::RejectButton },
          { 1, QObject::tr("Approve intentional conflict"),
            Dialog::AcceptCriticalButton } });
    QObject::connect(dialog, &Dialog::finished, dialog,
                     [dialog, _apply, _cancel](const Dialog::ButtonInfo& _button) {
        dialog->hideDialog();
        if (_button.type == Dialog::AcceptCriticalButton) {
            _apply();
        } else {
            _cancel();
        }
    });
    QObject::connect(dialog, &Dialog::disappeared, dialog, &Dialog::deleteLater);
}

void applyAfterCharacterDependencyConfirmation(
    QWidget* _parent, const QString& _characterName, const QString& _dependencyReport,
    bool _hasDependencies, const std::function<void()>& _apply,
    const std::function<void()>& _cancel)
{
    if (!_hasDependencies) {
        _apply();
        return;
    }
    auto dialog = new Dialog(_parent);
    dialog->setContentFixedWidth(Ui::DesignSystem::dialog().maximumWidth());
    dialog->enableSupportingTextScrolling();
    dialog->setDismissOnOutsideClick(false);
    dialog->setRejectOnEscape(false);
    dialog->showDialog(
        QObject::tr("Move referenced character to Recycle Bin?"),
        QObject::tr("%1 is still referenced by the story. Review the dependency report again. "
                    "The references will not be rewritten or deleted.\n\n%2")
            .arg(_characterName, _dependencyReport),
        { { 0, QObject::tr("Keep character"), Dialog::RejectButton },
          { 1, QObject::tr("Move profile to Recycle Bin"),
            Dialog::AcceptCriticalButton } });
    QObject::connect(dialog, &Dialog::finished, dialog,
                     [dialog, _apply, _cancel](const Dialog::ButtonInfo& _button) {
        dialog->hideDialog();
        if (_button.type == Dialog::AcceptCriticalButton) _apply();
        else _cancel();
    });
    QObject::connect(dialog, &Dialog::disappeared, dialog, &Dialog::deleteLater);
}
} // namespace

class ScreenplayTextView::Implementation
{
public:
    explicit Implementation(ScreenplayTextView* _q);

    /**
     * @brief Переконфигурировать представление
     */
    void reconfigureTemplate(bool _withModelReinitialization = true);
    void reconfigureSceneNumbersVisibility();
    void reconfigureDialoguesNumbersVisibility();

    /**
     * @brief Обновить переводы дополнительных действий
     */
    void updateOptionsTranslations();

    /**
     * @brief Обновить настройки UI панели инструментов
     */
    void updateToolbarUi();
    void updateToolbarPositon();

    /**
     * @brief Обновить текущий отображаемый тип абзаца в панели инструментов
     */
    void updateToolBarCurrentParagraphTypeName();

    /**
     * @brief Обновить компоновку страницы
     */
    void updateTextEditPageMargins();

    /**
     * @brief Обновить параметры режима автоматических редакторских заметок
     */
    void updateTextEditAutoReviewMode();

    /**
     * @brief Обновить видимость и положение панели инструментов рецензирования
     */
    void updateCommentsToolbar(bool _force = false);

    /**
     * @brief Обновить видимость боковой панели (показана, если показана хотя бы одна из вложенных
     * панелей)
     */
    void updateSideBarVisibility(QWidget* _container);

    /**
     * @brief Отобразить параметры заданной сцены
     */
    void showParametersFor(BusinessLayer::TextModelItem* _item);

    /**
     * @brief Добавить редакторскую заметку для текущего выделения
     */
    void addReviewMark(const QColor& _textColor, const QColor& _backgroundColor,
                       const QString& _comment, bool _isRevision, bool _isAddition,
                       bool _isRemoval);


    ScreenplayTextView* q = nullptr;

    //
    // Модели
    //
    QPointer<BusinessLayer::ScreenplayTextModel> model;
    BusinessLayer::TextModelItem* lastSelectedItem = nullptr;
    BusinessLayer::CommentsModel* commentsModel = nullptr;
    BusinessLayer::BookmarksModel* bookmarksModel = nullptr;

    //
    // Редактор сценария
    //
    ScreenplayTextEdit* textEdit = nullptr;
    ScreenplayTextEditShortcutsManager shortcutsManager;
    ScalableWrapper* scalableWrapper = nullptr;
    ScreenplayTextScrollBarManager* screenplayTextScrollbarManager = nullptr;
    std::optional<int> pendingCursorPosition;
    int aiEditSelectionStart = -1;
    int aiEditSelectionEnd = -1;
    QString aiEditSelectionText;
    int aiEditInsertionPosition = -1;
    QString pendingCharacterMergeRollbackId;
    int aiRequestCursorPosition = -1;
    int aiEditDocumentRevision = -1;
    bool aiEditApplyConfirmed = false;
    bool aiActionProtocolPending = false;
    QString aiRequestInstruction;
    QString aiRequestLogline;
    QString aiRequestSynopsis;
    QString aiRequestTreatment;
    int aiRequestTreatmentParagraphCount = 0;
    quint64 storySourceRevision = 0;
    quint64 aiRequestStorySourceRevision = 0;
    QString aiRequestStoryMemory;
    QString aiPendingAction;
    QString aiPendingTarget;
    QString aiPendingSummary;
    QString aiPendingImpactSummary;
    QJsonArray aiPendingContinuityChecks;
    bool aiAssistantInProgress = false;
    QTimer writersRoomIdleTimer;
    QElapsedTimer writersRoomCooldown;
    int writersRoomChangeEvents = 0;
    int writersRoomBaselineTextLength = 0;

    //
    // Панели инструментов
    //
    ScreenplayTextEditToolbar* toolbar = nullptr;
    BusinessLayer::SearchManager* searchManager = nullptr;
    FloatingToolbarAnimator* toolbarAnimation = nullptr;
    BusinessLayer::TextParagraphType currentParagraphType
        = BusinessLayer::TextParagraphType::Undefined;
    QStandardItemModel* paragraphTypesModel = nullptr;
    //
    CommentsToolbar* commentsToolbar = nullptr;

    //
    // Сайдбар
    //
    Shadow* sidebarShadow = nullptr;
    //
    Widget* sidebarWidget = nullptr;
    TabBar* sidebarTabs = nullptr;
    StackWidget* sidebarContent = nullptr;
    FastFormatWidget* fastFormatWidget = nullptr;
    CardItemParametersView* itemParametersView = nullptr;
    CommentsView* commentsView = nullptr;
    AiAssistantView* aiAssistantView = nullptr;
    BookmarksView* bookmarksView = nullptr;
    DictionariesView* dictionariesView = nullptr;
    ComplianceCheckResultView* complianceCheckResultView = nullptr;
    //
    Splitter* splitter = nullptr;

    //
    // Действия опций редактора
    //
    QAction* showSceneParametersAction = nullptr;
    QAction* showBookmarksAction = nullptr;
    QAction* showDictionariesAction = nullptr;
    QAction* showComplianceCheckResultAction = nullptr;

    /**
     * @brief Группируем события об изменении положения курсора, чтобы сильно не спамить сервер
     */
    Debouncer cursorChangeNotificationsDebounser;
};

ScreenplayTextView::Implementation::Implementation(ScreenplayTextView* _q)
    : q(_q)
    , commentsModel(new BusinessLayer::CommentsModel(_q))
    , bookmarksModel(new BusinessLayer::BookmarksModel(_q))
    , textEdit(new ScreenplayTextEdit(_q))
    , shortcutsManager(textEdit)
    , scalableWrapper(new ScalableWrapper(textEdit, _q))
    , toolbar(new ScreenplayTextEditToolbar(scalableWrapper))
    , searchManager(new BusinessLayer::SearchManager(scalableWrapper, textEdit))
    , toolbarAnimation(new FloatingToolbarAnimator(_q))
    , paragraphTypesModel(new QStandardItemModel(toolbar))
    , commentsToolbar(new CommentsToolbar(_q))
    , sidebarShadow(new Shadow(Qt::RightEdge, scalableWrapper))
    , sidebarWidget(new Widget(_q))
    , sidebarTabs(new TabBar(_q))
    , sidebarContent(new StackWidget(_q))
    , fastFormatWidget(new FastFormatWidget(_q))
    , itemParametersView(new CardItemParametersView(_q))
    , commentsView(new CommentsView(_q))
    , aiAssistantView(new AiAssistantView(_q))
    , bookmarksView(new BookmarksView(_q))
    , dictionariesView(new DictionariesView(_q))
    , complianceCheckResultView(new ComplianceCheckResultView(_q))
    , splitter(new Splitter(_q))
    , showSceneParametersAction(new QAction(_q))
    , showBookmarksAction(new QAction(_q))
    , showDictionariesAction(new QAction(_q))
    , showComplianceCheckResultAction(new QAction(_q))
    , cursorChangeNotificationsDebounser(500)

{
    toolbar->setParagraphTypesModel(paragraphTypesModel);
    toolbar->setOptions({
        showSceneParametersAction,
        showBookmarksAction,
        showDictionariesAction,
        showComplianceCheckResultAction,
    });

    commentsToolbar->hide();

    shortcutsManager.setShortcutsContext(scalableWrapper);
    scalableWrapper->initScrollBarsSyncing();
    screenplayTextScrollbarManager = new ScreenplayTextScrollBarManager(scalableWrapper);
    screenplayTextScrollbarManager->initScrollBarsSyncing();
    UiHelper::setupScrolling(scalableWrapper, true);

    textEdit->setUsePageMode(true);

    writersRoomIdleTimer.setSingleShot(true);

    sidebarWidget->hide();
    sidebarTabs->setFixed(false);
    sidebarTabs->addTab({}); // fastformat
    sidebarTabs->setTabVisible(kFastFormatTabIndex, false);
    sidebarTabs->addTab({}); // scene parameters
    sidebarTabs->setTabVisible(kSceneParametersTabIndex, false);
    sidebarTabs->addTab({}); // comments
    sidebarTabs->setTabVisible(kCommentsTabIndex, false);
    sidebarTabs->addTab({}); // ai assistant
    sidebarTabs->setTabVisible(kAiAssistantTabIndex, false);
    sidebarTabs->addTab({}); // bookmarks
    sidebarTabs->setTabVisible(kBookmarksTabIndex, false);
    sidebarTabs->addTab({}); // dictionaries
    sidebarTabs->setTabVisible(kDictionariesTabIndex, false);
    sidebarTabs->addTab({}); // compliance checking
    sidebarTabs->setTabVisible(kComplianceCheckResultTabIndex, false);
    sidebarContent->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    sidebarContent->setAnimationType(StackWidget::AnimationType::Slide);
    sidebarContent->addWidget(fastFormatWidget);
    sidebarContent->addWidget(itemParametersView);
    sidebarContent->addWidget(commentsView);
    sidebarContent->addWidget(aiAssistantView);
    sidebarContent->addWidget(bookmarksView);
    sidebarContent->addWidget(dictionariesView);
    sidebarContent->addWidget(complianceCheckResultView);
    fastFormatWidget->hide();
    fastFormatWidget->setParagraphTypesModel(paragraphTypesModel);
    itemParametersView->setStartDateTimeVisible(false);
    itemParametersView->setStampVisible(false);
    itemParametersView->hide();
    commentsView->setModel(commentsModel);
    commentsView->hide();
    aiAssistantView->hide();
    aiAssistantView->setSynopsisGenerationAvaiable(true);
    aiAssistantView->setNovelGenerationAvaiable(true);
    bookmarksView->setModel(bookmarksModel);
    bookmarksView->hide();
    dictionariesView->hide();
    complianceCheckResultView->hide();

    showSceneParametersAction->setCheckable(true);
    showSceneParametersAction->setIconText(u8"\U000F1A7D");
    showBookmarksAction->setCheckable(true);
    showBookmarksAction->setIconText(u8"\U000F0E16");
    showDictionariesAction->setCheckable(true);
    showDictionariesAction->setIconText(u8"\U000F0EBF");
    showComplianceCheckResultAction->setCheckable(true);
    showComplianceCheckResultAction->setIconText(u8"\U000F05C7");
}

void ScreenplayTextView::Implementation::reconfigureTemplate(bool _withModelReinitialization)
{
    using namespace BusinessLayer;

    paragraphTypesModel->clear();

    //
    // Настраиваем список доступных для работы типов блоков
    //
    QVector<TextParagraphType> types = {
        TextParagraphType::SceneHeading,
        TextParagraphType::SceneCharacters,
        TextParagraphType::BeatHeading,
        TextParagraphType::Action,
        TextParagraphType::Character,
        TextParagraphType::Parenthetical,
        TextParagraphType::Dialogue,
        TextParagraphType::Lyrics,
        TextParagraphType::Shot,
        TextParagraphType::Transition,
        TextParagraphType::InlineNote,
        TextParagraphType::UnformattedText,
        TextParagraphType::SequenceHeading,
        TextParagraphType::SequenceFooter,
        TextParagraphType::ActHeading,
        TextParagraphType::ActFooter,
    };
    if (!toolbar->isBeatsVisible()) {
        types.removeOne(TextParagraphType::BeatHeading);
    }

    //
    // Настраиваем фильтры моделей
    //
    commentsModel->setParagraphTypesFiler(types);
    bookmarksModel->setParagraphTypesFiler(types);

    //
    // Убираем типы окончаний, для списка форматов редактора текста
    //
    types.removeOne(TextParagraphType::SequenceFooter);
    types.removeOne(TextParagraphType::ActFooter);
    const auto& usedTemplate = BusinessLayer::TemplatesFacade::screenplayTemplate(
        model && model->informationModel() ? model->informationModel()->templateId() : "");
    for (const auto type : std::as_const(types)) {
        if (!usedTemplate.paragraphStyle(type).isActive()) {
            continue;
        }

        auto typeItem = new QStandardItem(toDisplayString(type));
        typeItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        typeItem->setData(shortcutsManager.shortcut(type), Qt::ToolTipRole);
        typeItem->setData(static_cast<int>(type), kTypeDataRole);
        paragraphTypesModel->appendRow(typeItem);
    }

    shortcutsManager.reconfigure();

    if (_withModelReinitialization) {
        textEdit->reinit();
    }
}

void ScreenplayTextView::Implementation::reconfigureSceneNumbersVisibility()
{
    textEdit->setShowSceneNumber(
        settingsValue(DataStorageLayer::kComponentsScreenplayEditorShowSceneNumbersKey).toBool(),
        settingsValue(DataStorageLayer::kComponentsScreenplayEditorShowSceneNumbersOnLeftKey)
            .toBool(),
        settingsValue(DataStorageLayer::kComponentsScreenplayEditorShowSceneNumbersOnRightKey)
            .toBool());
}

void ScreenplayTextView::Implementation::reconfigureDialoguesNumbersVisibility()
{
    textEdit->setShowDialogueNumber(
        settingsValue(DataStorageLayer::kComponentsScreenplayEditorShowDialogueNumbersKey)
            .toBool());
}

void ScreenplayTextView::Implementation::updateOptionsTranslations()
{
    showSceneParametersAction->setText(showSceneParametersAction->isChecked()
                                           ? tr("Hide scene parameters")
                                           : tr("Show scene parameters"));
    showBookmarksAction->setText(showBookmarksAction->isChecked() ? tr("Hide bookmarks list")
                                                                  : tr("Show bookmarks list"));
    showDictionariesAction->setText(showDictionariesAction->isChecked()
                                        ? tr("Hide screenplay dictionaries")
                                        : tr("Show screenplay dictionaries"));
    showComplianceCheckResultAction->setText(showComplianceCheckResultAction->isChecked()
                                                 ? tr("Hide compliance check results")
                                                 : tr("Show compliance check results"));
}

void ScreenplayTextView::Implementation::updateToolbarUi()
{
    updateToolbarPositon();
    toolbar->setBackgroundColor(ColorHelper::nearby(Ui::DesignSystem::color().background()));
    toolbar->setTextColor(Ui::DesignSystem::color().onBackground());
    toolbar->raise();

    searchManager->toolbar()->setBackgroundColor(
        ColorHelper::nearby(Ui::DesignSystem::color().background()));
    searchManager->toolbar()->setTextColor(Ui::DesignSystem::color().onBackground());
    searchManager->toolbar()->raise();

    toolbarAnimation->setBackgroundColor(
        ColorHelper::nearby(Ui::DesignSystem::color().background()));
    toolbarAnimation->setTextColor(Ui::DesignSystem::color().onBackground());

    commentsToolbar->setBackgroundColor(
        ColorHelper::nearby(Ui::DesignSystem::color().background()));
    commentsToolbar->setTextColor(Ui::DesignSystem::color().onBackground());
    commentsToolbar->raise();
    updateCommentsToolbar();
}

void ScreenplayTextView::Implementation::updateToolbarPositon()
{
    toolbar->move(QPointF((scalableWrapper->width() - toolbar->width()) / 2.0,
                          -Ui::DesignSystem::card().shadowMargins().top())
                      .toPoint());
    searchManager->toolbar()->move(
        QPointF((scalableWrapper->width() - searchManager->toolbar()->width()) / 2.0,
                -Ui::DesignSystem::card().shadowMargins().top())
            .toPoint());
}

void ScreenplayTextView::Implementation::updateToolBarCurrentParagraphTypeName()
{
    auto paragraphType = textEdit->currentParagraphType();
    if (currentParagraphType == paragraphType) {
        return;
    }

    currentParagraphType = paragraphType;

    if (paragraphType == BusinessLayer::TextParagraphType::ActFooter) {
        paragraphType = BusinessLayer::TextParagraphType::ActHeading;
        toolbar->setParagraphTypesEnabled(false);
        fastFormatWidget->setEnabled(false);
    } else if (paragraphType == BusinessLayer::TextParagraphType::SequenceFooter) {
        paragraphType = BusinessLayer::TextParagraphType::SequenceHeading;
        toolbar->setParagraphTypesEnabled(false);
        fastFormatWidget->setEnabled(false);
    } else {
        toolbar->setParagraphTypesEnabled(!textEdit->isReadOnly() && true);
        fastFormatWidget->setEnabled(!textEdit->isReadOnly() && true);
    }

    for (int itemRow = 0; itemRow < paragraphTypesModel->rowCount(); ++itemRow) {
        const auto item = paragraphTypesModel->item(itemRow);
        const auto itemType
            = static_cast<BusinessLayer::TextParagraphType>(item->data(kTypeDataRole).toInt());
        if (itemType == paragraphType) {
            toolbar->setCurrentParagraphType(paragraphTypesModel->index(itemRow, 0));
            fastFormatWidget->setCurrentParagraphType(paragraphTypesModel->index(itemRow, 0));
            return;
        }
    }
}

void ScreenplayTextView::Implementation::updateTextEditPageMargins()
{
    if (textEdit->usePageMode()) {
        return;
    }

    const QMarginsF pageMargins
        = QMarginsF{ 15, 20 / scalableWrapper->zoomRange(), 12 / scalableWrapper->zoomRange(), 5 };
    textEdit->setPageMarginsMm(pageMargins);
}

void ScreenplayTextView::Implementation::updateTextEditAutoReviewMode()
{
    switch (commentsToolbar->commentsType()) {
    case Ui::CommentsToolbar::CommentsType::Review: {
        textEdit->setAutoReviewModeEnabled(false);
        break;
    }

    case Ui::CommentsToolbar::CommentsType::Changes: {
        textEdit->setAutoReviewModeEnabled(toolbar->isCommentsModeEnabled() && true);
        textEdit->setAutoReviewMode({}, commentsToolbar->color(), false, true);
        break;
    }

    case Ui::CommentsToolbar::CommentsType::Revision: {
        textEdit->setAutoReviewModeEnabled(toolbar->isCommentsModeEnabled() && true);
        textEdit->setAutoReviewMode(commentsToolbar->color(), {}, true, false);
        break;
    }
    }
}

void ScreenplayTextView::Implementation::updateCommentsToolbar(bool _force)
{
    if (!q->isVisible()) {
        return;
    }

    if (commentsView->isReadOnly() || !toolbar->isCommentsModeEnabled()) {
        commentsToolbar->hideToolbar();
        return;
    }

    //
    // Настроим список доступных действий панели рецензирования
    //
    if (!textEdit->textCursor().hasSelection() && commentsView->currentIndex().isValid()) {
        commentsToolbar->setMode(CommentsToolbar::Mode::EditReview);
        const auto currentIndex = commentsView->currentIndex();
        commentsToolbar->setCurrentCommentState(
            currentIndex.data(BusinessLayer::CommentsModel::ReviewMarkIsDoneRole).toBool(),
            currentIndex.data(BusinessLayer::CommentsModel::ReviewMarkIsAdditionRole).toBool()
                || currentIndex.data(BusinessLayer::CommentsModel::ReviewMarkIsRemovalRole)
                       .toBool(),
            currentIndex.data(BusinessLayer::CommentsModel::ReviewMarkIsRevisionRole).toBool());
    } else {
        commentsToolbar->setMode(CommentsToolbar::Mode::AddReview);
    }

    //
    // Настроим доступность действий добавления редакторских заметок
    //
    commentsToolbar->setAddingAvailable(textEdit->textCursor().hasSelection());

    const auto cursorRect = textEdit->cursorRect();
    const auto globalCursorCenter = textEdit->mapToGlobal(cursorRect.center());
    const auto localCursorCenter
        = commentsToolbar->parentWidget()->mapFromGlobal(globalCursorCenter);
    //
    // ... если курсор не виден на экране, то тулбар нужно скрыть
    //
    const bool isToolbarVisible = localCursorCenter.y() >= 0
        && localCursorCenter.y()
            < scalableWrapper->height() - screenplayTextScrollbarManager->scrollBarHeight();

    //
    // Определеим положение тулбара, с учётом края экрана
    //
    auto toolbarYPos = localCursorCenter.y() - commentsToolbar->width();
    if (toolbarYPos + commentsToolbar->height()
        > scalableWrapper->height() - screenplayTextScrollbarManager->scrollBarHeight()) {
        toolbarYPos = scalableWrapper->height() - commentsToolbar->height()
            - screenplayTextScrollbarManager->scrollBarHeight();
    }

    //
    // Если вьюпорт вмещается аккурат в видимую область, или не влезает,
    //
    if (textEdit->width() <= textEdit->viewport()->width() + commentsToolbar->width()) {
        commentsToolbar->setCurtain(true, q->isLeftToRight() ? Qt::RightEdge : Qt::LeftEdge);
        //
        // ... то позиционируем панель рецензирования по краю панели комментариев
        //
        commentsToolbar->moveToolbar(
            QPoint(q->isLeftToRight() ? (scalableWrapper->width() - commentsToolbar->width()
                                         + DesignSystem::layout().px(3))
                                      : (sidebarWidget->width() - DesignSystem::layout().px(3)),
                   toolbarYPos),
            _force);
    }
    //
    // В противном случае позиционируем её по краю листа
    //
    else {
        commentsToolbar->setCurtain(true, q->isLeftToRight() ? Qt::LeftEdge : Qt::RightEdge);
        //
        // ... определяем точку на границе страницы
        //
        const auto textEditWidth = scalableWrapper->zoomRange() * textEdit->width();
        const auto textEditViewportWidth
            = scalableWrapper->zoomRange() * textEdit->viewport()->width();
        const auto pos = q->isLeftToRight()
            ? ((textEditWidth - textEditViewportWidth) / 2.0 + textEditViewportWidth
               - (scalableWrapper->zoomRange()
                      * (DesignSystem::card().shadowMargins().left()
                         + DesignSystem::card().shadowMargins().right()
                         - DesignSystem::layout().px8())
                  + DesignSystem::floatingToolBar().shadowMargins().left()))
            : ((textEditWidth - textEditViewportWidth) / 2.0 + sidebarWidget->width()
               - (scalableWrapper->zoomRange()
                      * (DesignSystem::card().shadowMargins().left()
                         + DesignSystem::card().shadowMargins().right()
                         - DesignSystem::layout().px8())
                  + DesignSystem::floatingToolBar().shadowMargins().left()));
        //
        // ... и смещаем панель рецензирования к этой точке
        //
        commentsToolbar->moveToolbar(QPoint(pos, toolbarYPos), _force);
    }

    //
    // Если панель ещё не была показана, отобразим её
    //
    if (isToolbarVisible) {
        commentsToolbar->showToolbar();
    } else {
        commentsToolbar->hideToolbar();
    }
}

void ScreenplayTextView::Implementation::updateSideBarVisibility(QWidget* _container)
{
    const bool isSidebarShouldBeVisible = toolbar->isFastFormatPanelVisible()
        || toolbar->isCommentsModeEnabled() || toolbar->isAiAssistantEnabled()
        || showSceneParametersAction->isChecked() || showBookmarksAction->isChecked()
        || showDictionariesAction->isChecked() || showComplianceCheckResultAction->isChecked();
    if (sidebarWidget->isVisible() == isSidebarShouldBeVisible) {
        return;
    }

    sidebarShadow->setVisible(isSidebarShouldBeVisible);
    sidebarWidget->setVisible(isSidebarShouldBeVisible);

    if (isSidebarShouldBeVisible && splitter->sizes().constLast() == 0) {
        const auto sideBarWidth = sidebarContent->sizeHint().width();
        splitter->setSizes({ _container->width() - sideBarWidth, sideBarWidth });
    }
}

void ScreenplayTextView::Implementation::showParametersFor(BusinessLayer::TextModelItem* _item)
{
    if (_item == nullptr
        || (_item->type() != BusinessLayer::TextModelItemType::Folder
            && _item->type() != BusinessLayer::TextModelItemType::Group)) {
        return;
    }

    //
    // На время установки данных о другом элемента, блокируем сигналы сайдбара
    //
    QSignalBlocker signalBlocker(itemParametersView);

    lastSelectedItem = _item;

    switch (_item->type()) {
    case BusinessLayer::TextModelItemType::Folder: {
        itemParametersView->setItemType(Ui::CardItemType::Folder);

        auto folderItem = static_cast<BusinessLayer::TextModelFolderItem*>(lastSelectedItem);
        itemParametersView->setColor(folderItem->color());
        itemParametersView->setTitle(folderItem->heading());
        itemParametersView->setDescription(folderItem->description());
        itemParametersView->setStamp(folderItem->stamp());
        break;
    }

    case BusinessLayer::TextModelItemType::Group: {
        const auto groupItem = static_cast<BusinessLayer::TextModelGroupItem*>(lastSelectedItem);
        if (groupItem->groupType() != BusinessLayer::TextGroupType::Scene) {
            return;
        }

        itemParametersView->setItemType(Ui::CardItemType::Scene);

        const auto sceneItem = static_cast<BusinessLayer::ScreenplayTextModelSceneItem*>(groupItem);
        itemParametersView->setColor(sceneItem->color());
        itemParametersView->setTitle(sceneItem->title());
        itemParametersView->setHeading(sceneItem->heading());
        itemParametersView->setBeats(sceneItem->beats());
        itemParametersView->setStoryDay(sceneItem->storyDay(),
                                        model->dictionariesModel()->storyDays());
        itemParametersView->setStamp(sceneItem->stamp());
        if (const auto sceneNumber = sceneItem->number(); sceneNumber.has_value()) {
            itemParametersView->setNumber(sceneNumber->followNumber + sceneNumber->value,
                                          sceneNumber->isCustom, sceneNumber->isEatNumber,
                                          sceneNumber->isLocked);
        } else {
            itemParametersView->setNumber({}, false, true, false);
        }
        itemParametersView->setTags(sceneItem->tags(), model->dictionariesModel()->tags());
        break;
    }

    default: {
        break;
        ;
    }
    }
}

void ScreenplayTextView::Implementation::addReviewMark(const QColor& _textColor,
                                                       const QColor& _backgroundColor,
                                                       const QString& _comment, bool _isRevision,
                                                       bool _isAddition, bool _isRemoval)
{
    //
    // Добавим заметку
    //
    const auto textColor
        = _textColor.isValid() ? _textColor : ColorHelper::contrasted(_backgroundColor);
    textEdit->addReviewMark(textColor, _backgroundColor, _comment, _isRevision, _isAddition,
                            _isRemoval);

    //
    // Снимем выделение, чтобы пользователь получил обратную связь от приложения, что выделение
    // добавлено
    //
    BusinessLayer::TextCursor cursor(textEdit->textCursor());
    const auto selectionInterval = cursor.selectionInterval();
    //
    // ... делаем танец с бубном, чтобы получить сигнал об обновлении позиции курсора
    //     и выделить новую заметку в общем списке
    //
    cursor.setPosition(selectionInterval.from);
    textEdit->setTextCursorAndKeepScrollBars(cursor);
    cursor.setPosition(selectionInterval.to);
    textEdit->setTextCursorAndKeepScrollBars(cursor);

    //
    // Фокусируем редактор сценария, чтобы пользователь мог продолжать работать с ним
    //
    scalableWrapper->setFocus();
}


// ****


ScreenplayTextView::ScreenplayTextView(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation(this))
{
    setFocusProxy(d->scalableWrapper);
    d->scalableWrapper->installEventFilter(this);

    QVBoxLayout* sidebarLayout = new QVBoxLayout(d->sidebarWidget);
    sidebarLayout->setContentsMargins({});
    sidebarLayout->setSpacing(0);
    sidebarLayout->addWidget(d->sidebarTabs);
    sidebarLayout->addWidget(d->sidebarContent);

    d->splitter->setWidgets(d->scalableWrapper, d->sidebarWidget);
    d->splitter->setSizes({ 1, 0 });
    d->splitter->setHidePanelButtonAvailable(true, false);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(d->splitter);

    connect(d->toolbar, &ScreenplayTextEditToolbar::undoPressed, d->textEdit,
            &ScreenplayTextEdit::undo);
    connect(d->toolbar, &ScreenplayTextEditToolbar::redoPressed, d->textEdit,
            &ScreenplayTextEdit::redo);
    connect(d->toolbar, &ScreenplayTextEditToolbar::paragraphTypeChanged, this,
            [this](const QModelIndex& _index) {
                if (!_index.isValid()) {
                    return;
                }

                const auto type = static_cast<BusinessLayer::TextParagraphType>(
                    _index.data(kTypeDataRole).toInt());
                d->textEdit->setCurrentParagraphType(type);
                d->scalableWrapper->setFocus();
            });
    connect(d->toolbar, &ScreenplayTextEditToolbar::fastFormatPanelVisibleChanged, this,
            [this](bool _visible) {
                d->sidebarTabs->setTabVisible(kFastFormatTabIndex, _visible);
                d->fastFormatWidget->setVisible(_visible);
                if (_visible) {
                    d->sidebarTabs->setCurrentTab(kFastFormatTabIndex);
                    d->sidebarContent->setCurrentWidget(d->fastFormatWidget);
                }
                d->updateSideBarVisibility(this);
                d->updateToolbarPositon();
            });
    connect(d->toolbar, &ScreenplayTextEditToolbar::beatsVisibleChanged, this,
            [this](bool _visible) {
                d->textEdit->setBeatsVisible(_visible);
                const bool withModelReinitialization = false;
                d->reconfigureTemplate(withModelReinitialization);
            });
    connect(d->toolbar, &ScreenplayTextEditToolbar::commentsModeEnabledChanged, this,
            [this](bool _enabled) {
                d->sidebarTabs->setTabVisible(kCommentsTabIndex, _enabled);
                d->commentsView->setVisible(_enabled);
                if (_enabled) {
                    d->sidebarTabs->setCurrentTab(kCommentsTabIndex);
                    d->sidebarContent->setCurrentWidget(d->commentsView);
                }
                d->updateTextEditAutoReviewMode();
                d->updateCommentsToolbar();
                d->updateSideBarVisibility(this);
            });
    connect(d->toolbar, &ScreenplayTextEditToolbar::aiAssistantEnabledChanged, this,
            [this](bool _enabled) {
                d->sidebarTabs->setTabVisible(kAiAssistantTabIndex, _enabled);
                d->aiAssistantView->setVisible(_enabled);
                if (_enabled) {
                    d->sidebarTabs->setCurrentTab(kAiAssistantTabIndex);
                    d->sidebarContent->setCurrentWidget(d->aiAssistantView);
                }
                d->updateSideBarVisibility(this);
            });
    connect(d->toolbar, &ScreenplayTextEditToolbar::itemIsolationEnabledChanged, this,
            [this](bool _enabled) {
                d->textEdit->setVisibleTopLevelItemIndex(_enabled ? d->textEdit->currentModelIndex()
                                                                  : QModelIndex());

                const bool animate = false;
                d->screenplayTextScrollbarManager->setScrollBarVisible(!_enabled, animate);
                d->textEdit->ensureCursorVisible(d->textEdit->textCursor(), animate);
            });
    connect(d->toolbar, &ScreenplayTextEditToolbar::searchPressed, this, [this] {
        d->toolbarAnimation->switchToolbars(d->toolbar->searchIcon(),
                                            d->toolbar->searchIconPosition(), d->toolbar,
                                            d->searchManager->toolbar());
        d->searchManager->activateSearhToolbar();
    });
    //
    connect(d->searchManager, &BusinessLayer::SearchManager::hideToolbarRequested, this,
            [this] { d->toolbarAnimation->switchToolbarsBack(); });
    //
    connect(d->commentsToolbar, &CommentsToolbar::commentsTypeChanged, this,
            [this] { d->updateTextEditAutoReviewMode(); });
    connect(d->commentsToolbar, &CommentsToolbar::colorChanged, this,
            [this] { d->updateTextEditAutoReviewMode(); });
    connect(
        d->commentsToolbar, &CommentsToolbar::textColorChangeRequested, this,
        [this](const QColor& _color) { d->addReviewMark(_color, {}, {}, false, false, false); });
    connect(
        d->commentsToolbar, &CommentsToolbar::textBackgoundColorChangeRequested, this,
        [this](const QColor& _color) { d->addReviewMark({}, _color, {}, false, false, false); });
    connect(d->commentsToolbar, &CommentsToolbar::commentAddRequested, this,
            [this](const QColor& _color) {
                d->sidebarTabs->setCurrentTab(kCommentsTabIndex);
                d->commentsView->showAddCommentView(
                    _color, {},
                    d->commentsView
                        ->mapFromGlobal(d->textEdit->viewport()->mapToGlobal(
                            d->textEdit->cursorRect().topLeft()))
                        .y());
            });
    connect(d->commentsToolbar, &CommentsToolbar::changeAdditionAddRequested, this,
            [this](const QColor& _color) { d->addReviewMark({}, _color, {}, false, true, false); });
    connect(d->commentsToolbar, &CommentsToolbar::changeRemovalAddRequested, this,
            [this](const QColor& _color) { d->addReviewMark({}, _color, {}, false, false, true); });
    connect(d->commentsToolbar, &CommentsToolbar::revisionMarkAddRequested, this,
            [this](const QColor& _color) { d->addReviewMark(_color, {}, {}, true, false, false); });
    connect(d->commentsToolbar, &CommentsToolbar::markAsDoneRequested, this, [this](bool _checked) {
        QSignalBlocker blocker(d->commentsView);
        const auto commentIndex = d->commentsView->currentIndex();
        if (d->commentsModel->isChange(commentIndex)) {
            d->commentsModel->applyChanges({ commentIndex });
        } else {
            if (_checked) {
                d->commentsModel->markAsDone({ commentIndex });
            } else {
                d->commentsModel->markAsUndone({ commentIndex });
            }
        }
    });
    connect(d->commentsToolbar, &CommentsToolbar::removeRequested, this, [this] {
        QSignalBlocker blocker(d->commentsView);
        const auto commentIndex = d->commentsView->currentIndex();
        if (d->commentsModel->isChange(commentIndex)) {
            d->commentsModel->cancelChanges({ commentIndex });
        } else {
            d->commentsModel->remove({ d->commentsView->currentIndex() });
        }
        d->commentsToolbar->setMode(CommentsToolbar::Mode::AddReview);
    });
    //
    connect(d->scalableWrapper->verticalScrollBar(), &QScrollBar::valueChanged, this,
            [this] { d->updateCommentsToolbar(true); });
    connect(d->scalableWrapper->horizontalScrollBar(), &QScrollBar::valueChanged, this,
            [this] { d->updateCommentsToolbar(true); });
    connect(
        d->scalableWrapper, &ScalableWrapper::zoomRangeChanged, this,
        [this] {
            d->updateTextEditPageMargins();
            d->updateCommentsToolbar();
        },
        Qt::QueuedConnection);
    //
    auto findCurrentModelItem = [this]() -> BusinessLayer::TextModelItem* {
        if (d->model.isNull()) {
            return nullptr;
        }

        const auto currentModelIndex = this->currentModelIndex();
        if (!currentModelIndex.isValid()) {
            return nullptr;
        }

        auto currentItem = d->model->itemForIndex(currentModelIndex.parent());
        if (currentItem->type() == BusinessLayer::TextModelItemType::Folder
            && static_cast<BusinessLayer::TextFolderType>(currentItem->subtype())
                == BusinessLayer::TextFolderType::Root) {
            return nullptr;
        }

        if (currentItem->type() == BusinessLayer::TextModelItemType::Group
            && static_cast<BusinessLayer::TextGroupType>(currentItem->subtype())
                == BusinessLayer::TextGroupType::Beat) {
            currentItem = currentItem->parent();
        }
        return currentItem;
    };
    auto handleCursorPositionChanged = [this, findCurrentModelItem] {
        //
        // Обновим состояние панелей форматов
        //
        d->updateToolBarCurrentParagraphTypeName();

        //
        // Уведомим навигатор клиентов, о смене текущего элемента
        //
        const auto screenplayModelIndex = d->textEdit->currentModelIndex();
        if (hasFocus() || d->searchManager->toolbar()->hasFocus()) {
            emit currentModelIndexChanged(screenplayModelIndex);
        }

        //
        // Отобразим параметры сцены
        //
        auto currentItem = findCurrentModelItem();
        if (currentItem != nullptr && currentItem != d->lastSelectedItem) {
            d->showParametersFor(currentItem);
        }

        //
        // Если необходимо выберем соответствующий комментарий
        //
        const auto positionInBlock = d->textEdit->textCursor().positionInBlock();
        const auto commentModelIndex
            = d->commentsModel->mapFromModel(screenplayModelIndex, positionInBlock);
        d->commentsView->setCurrentIndex(commentModelIndex);

        //
        // После того, как комментарий был выбран, скорректируем состояние панели рецензирования
        //
        d->updateCommentsToolbar();

        //
        // Выберем закладку, если курсор в блоке с закладкой
        //
        const auto bookmarkModelIndex = d->bookmarksModel->mapFromModel(screenplayModelIndex);
        d->bookmarksView->setCurrentIndex(bookmarkModelIndex);

        //
        // Запланируем уведомление внешних клиентов о смене позиции курсора
        //
        d->cursorChangeNotificationsDebounser.orderWork();
    };
    connect(d->textEdit, &ScreenplayTextEdit::paragraphTypeChanged, this,
            handleCursorPositionChanged);
    connect(d->textEdit, &ScreenplayTextEdit::cursorPositionChanged, this,
            handleCursorPositionChanged);
    connect(d->textEdit, &ScreenplayTextEdit::selectionChanged, this, handleCursorPositionChanged);
    connect(d->aiAssistantView, &AiAssistantView::writersRoomModeChanged, this,
            [this](bool _enabled) {
                d->writersRoomIdleTimer.stop();
                d->writersRoomChangeEvents = 0;
                d->writersRoomBaselineTextLength
                    = d->textEdit->document()->characterCount();
                if (!_enabled) {
                    d->writersRoomCooldown.invalidate();
                }
            });
    connect(d->textEdit, &ScreenplayTextEdit::textChanged, this, [this] {
        if (!d->aiAssistantView->isWritersRoomEnabled() || !d->textEdit->hasFocus()) {
            return;
        }
        ++d->writersRoomChangeEvents;
        d->writersRoomIdleTimer.start(45000);
    });
    connect(&d->writersRoomIdleTimer, &QTimer::timeout, this, [this] {
        if (!d->aiAssistantView->isWritersRoomEnabled() || d->model == nullptr) {
            return;
        }
        if (d->aiAssistantInProgress) {
            d->writersRoomIdleTimer.start(30000);
            return;
        }

        constexpr qint64 minimumCooldownMs = 5 * 60 * 1000;
        if (d->writersRoomCooldown.isValid()
            && d->writersRoomCooldown.elapsed() < minimumCooldownMs) {
            d->writersRoomIdleTimer.start(
                static_cast<int>(minimumCooldownMs - d->writersRoomCooldown.elapsed()));
            return;
        }

        const auto currentTextLength = d->textEdit->document()->characterCount();
        const auto textLengthDelta
            = qAbs(currentTextLength - d->writersRoomBaselineTextLength);
        if (d->writersRoomChangeEvents < 8 && textLengthDelta < 80) {
            return;
        }

        d->writersRoomChangeEvents = 0;
        d->writersRoomBaselineTextLength = currentTextLength;
        d->writersRoomCooldown.restart();
        emit generateTextRequested(
            QStringLiteral(
                "Quietly assess the latest screenplay progress as a collaborative story-room "
                "partner. Reply in at most 120 words with: (1) the strongest recent development, "
                "(2) one continuity or structural watchpoint, and (3) one concrete possibility "
                "for the story's next turn. Apply the selected story method. Offer advice only; "
                "do not alter screenplay text. Begin with 'Room note:'."),
            {});
    });
    connect(d->textEdit, &ScreenplayTextEdit::addBookmarkRequested, this, [this] {
        //
        // Если список закладок показан, добавляем новую через него
        //
        if (d->showBookmarksAction->isChecked()) {
            d->sidebarTabs->setCurrentTab(kBookmarksTabIndex);
            d->bookmarksView->showAddBookmarkView(
                {},
                d->bookmarksView
                    ->mapFromGlobal(
                        d->textEdit->viewport()->mapToGlobal(d->textEdit->cursorRect().topLeft()))
                    .y());
        }
        //
        // В противном случае, через диалог
        //
        else {
            emit addBookmarkRequested();
        }
    });
    connect(d->textEdit, &ScreenplayTextEdit::editBookmarkRequested, this, [this] {
        //
        // Если список закладок показан, редактируем через него
        //
        if (d->showBookmarksAction->isChecked()) {
            d->sidebarTabs->setCurrentTab(kBookmarksTabIndex);
            d->bookmarksView->showAddBookmarkView(
                d->bookmarksModel->mapFromModel(currentModelIndex()),
                d->bookmarksView
                    ->mapFromGlobal(
                        d->textEdit->viewport()->mapToGlobal(d->textEdit->cursorRect().topLeft()))
                    .y());
        }
        //
        // В противном случае, через диалог
        //
        else {
            emit addBookmarkRequested();
        }
    });
    connect(d->textEdit, &ScreenplayTextEdit::removeBookmarkRequested, this,
            &ScreenplayTextView::removeBookmarkRequested);
    connect(d->textEdit, &ScreenplayTextEdit::showBookmarksRequested, d->showBookmarksAction,
            &QAction::toggle);
    //
    connect(d->sidebarTabs, &TabBar::currentIndexChanged, this, [this](int _currentIndex) {
        switch (_currentIndex) {
        case kFastFormatTabIndex: {
            d->sidebarContent->setCurrentWidget(d->fastFormatWidget);
            break;
        }

        case kSceneParametersTabIndex: {
            d->sidebarContent->setCurrentWidget(d->itemParametersView);
            break;
        }

        case kCommentsTabIndex: {
            d->sidebarContent->setCurrentWidget(d->commentsView);
            break;
        }

        case kAiAssistantTabIndex: {
            d->sidebarContent->setCurrentWidget(d->aiAssistantView);
            break;
        }

        case kBookmarksTabIndex: {
            d->sidebarContent->setCurrentWidget(d->bookmarksView);
            break;
        }

        case kDictionariesTabIndex: {
            d->sidebarContent->setCurrentWidget(d->dictionariesView);
            break;
        }

        case kComplianceCheckResultTabIndex: {
            d->sidebarContent->setCurrentWidget(d->complianceCheckResultView);
            break;
        }
        }
    });
    //
    connect(d->fastFormatWidget, &FastFormatWidget::paragraphTypeChanged, this,
            [this](const QModelIndex& _index) {
                if (!_index.isValid()) {
                    return;
                }

                const auto type = static_cast<BusinessLayer::TextParagraphType>(
                    _index.data(kTypeDataRole).toInt());
                d->textEdit->setCurrentParagraphType(type);
                d->scalableWrapper->setFocus();
            });
    //
    connect(d->itemParametersView, &CardItemParametersView::colorChanged, this,
            [this, findCurrentModelItem](const QColor& _color) {
                auto item = findCurrentModelItem();
                if (item == nullptr) {
                    return;
                }

                switch (item->type()) {
                case BusinessLayer::TextModelItemType::Folder: {
                    auto folderItem = static_cast<BusinessLayer::TextModelFolderItem*>(item);
                    folderItem->setColor(_color);
                    break;
                }

                case BusinessLayer::TextModelItemType::Group: {
                    auto groupItem = static_cast<BusinessLayer::TextModelGroupItem*>(item);
                    groupItem->setColor(_color);
                    break;
                }

                default: {
                    Q_ASSERT(false);
                }
                }

                d->model->updateItem(item);
            });
    connect(d->itemParametersView, &CardItemParametersView::titleChanged, this,
            [this, findCurrentModelItem](const QString& _title) {
                auto item = findCurrentModelItem();
                if (item == nullptr) {
                    return;
                }

                switch (item->type()) {
                case BusinessLayer::TextModelItemType::Folder: {
                    auto textItem
                        = static_cast<BusinessLayer::TextModelTextItem*>(item->childAt(0));
                    textItem->setText(_title);
                    item = textItem;
                    break;
                }

                case BusinessLayer::TextModelItemType::Group: {
                    auto groupItem = static_cast<BusinessLayer::TextModelGroupItem*>(item);
                    groupItem->setTitle(_title);
                    break;
                }

                default: {
                    Q_ASSERT(false);
                }
                }

                d->model->updateItem(item);
            });
    connect(d->itemParametersView, &CardItemParametersView::headingChanged, this,
            [this, findCurrentModelItem](const QString& _heading) {
                auto item = findCurrentModelItem();
                if (item == nullptr || item->type() != BusinessLayer::TextModelItemType::Group) {
                    return;
                }

                auto textItem = static_cast<BusinessLayer::TextModelTextItem*>(item->childAt(0));
                textItem->setText(_heading);
                d->model->updateItem(textItem);
            });
    connect(d->itemParametersView, &CardItemParametersView::descriptionChanged, this,
            [this, findCurrentModelItem](const QString& _description) {
                auto item = findCurrentModelItem();
                if (item == nullptr || item->type() != BusinessLayer::TextModelItemType::Folder) {
                    return;
                }

                auto folderItem = static_cast<BusinessLayer::TextModelFolderItem*>(item);
                folderItem->setDescription(_description);
                d->model->updateItem(folderItem);
            });
    connect(d->itemParametersView, &CardItemParametersView::beatAdded, this,
            [this, findCurrentModelItem](int _beatIndex) {
                auto item = findCurrentModelItem();
                if (item == nullptr || item->type() != BusinessLayer::TextModelItemType::Group) {
                    return;
                }

                //
                // Определим элемент бита, после которого нужно вставить новый
                //
                int currentBeatIndex = 0;
                for (int childIndex = 1; childIndex < item->childCount(); ++childIndex) {
                    auto childItem = item->childAt(childIndex);
                    if (childItem->type() != BusinessLayer::TextModelItemType::Group) {
                        continue;
                    }

                    if (currentBeatIndex != _beatIndex - 1) {
                        ++currentBeatIndex;
                        continue;
                    }

                    //
                    // ... и вставляем новый бит после обнаруженного
                    //
                    auto beatHeadingItem = d->model->createTextItem();
                    beatHeadingItem->setParagraphType(
                        BusinessLayer::TextParagraphType::BeatHeading);
                    auto beatItem = d->model->createGroupItem(BusinessLayer::TextGroupType::Beat);
                    beatItem->appendItems({ beatHeadingItem });
                    d->model->insertItem(beatItem, childItem);
                    break;
                }
            });
    connect(
        d->itemParametersView, &CardItemParametersView::beatChanged, this,
        [this, findCurrentModelItem](int _beatIndex, const QString& _beat) {
            auto item = findCurrentModelItem();
            if (item == nullptr || item->type() != BusinessLayer::TextModelItemType::Group) {
                return;
            }

            //
            // Определим элемент изменяемого бита
            //
            int currentBeatIndex = 0;
            BusinessLayer::TextModelTextItem* beatHeadingItem = nullptr;
            for (int childIndex = 1; childIndex < item->childCount(); ++childIndex) {
                auto child = item->childAt(childIndex);
                if (child->type() != BusinessLayer::TextModelItemType::Group) {
                    continue;
                }

                if (currentBeatIndex != _beatIndex) {
                    ++currentBeatIndex;
                    continue;
                }

                beatHeadingItem = static_cast<BusinessLayer::TextModelTextItem*>(child->childAt(0));
                break;
            }
            //
            // Если не удалось найти бит (обычно это происходит в ситуации когда не было ни
            // одного бита в сцене, и пользователь добавляет описание на карточку
            //
            if (beatHeadingItem == nullptr) {
                beatHeadingItem = d->model->createTextItem();
                beatHeadingItem->setParagraphType(BusinessLayer::TextParagraphType::BeatHeading);
                auto beatItem = d->model->createGroupItem(BusinessLayer::TextGroupType::Beat);
                beatItem->appendItems({ beatHeadingItem });
                d->model->appendItem(beatItem, item);
            }
            //
            // Обновляем текст заголовка бита
            //
            beatHeadingItem->setText(_beat);
            d->model->updateItem(beatHeadingItem);
        });
    connect(d->itemParametersView, &CardItemParametersView::beatRemoved, this,
            [this, findCurrentModelItem](int _beatIndex) {
                auto item = findCurrentModelItem();
                if (item == nullptr || item->type() != BusinessLayer::TextModelItemType::Group) {
                    return;
                }

                //
                // Определим элемент бита, который нужно удалить
                //
                int currentBeatIndex = 0;
                for (int childIndex = 1; childIndex < item->childCount(); ++childIndex) {
                    auto beatItem = item->childAt(childIndex);
                    if (beatItem->type() != BusinessLayer::TextModelItemType::Group) {
                        continue;
                    }

                    if (currentBeatIndex != _beatIndex) {
                        ++currentBeatIndex;
                        continue;
                    }

                    //
                    // ... извлечём всех детей и перенесём их по назначению
                    //
                    if (beatItem->hasChildren() && beatItem->childCount() > 1) {
                        QVector<BusinessLayer::TextModelItem*> beatChildren;
                        while (beatItem->childCount() > 1) {
                            auto beatChildItem = beatItem->childAt(1);
                            d->model->takeItem(beatChildItem);
                            beatChildren.append(beatChildItem);
                        }

                        const int beatItemIndex = beatItem->parent()->rowOfChild(beatItem);
                        if (beatItemIndex == 0) {
                            d->model->prependItems(beatChildren);
                        } else {
                            auto beforeBeatItem = beatItem->parent()->childAt(beatItemIndex - 1);
                            Q_ASSERT(beforeBeatItem);
                            if (beforeBeatItem->type() == BusinessLayer::TextModelItemType::Group) {
                                d->model->appendItems(beatChildren, beforeBeatItem);
                            } else {
                                d->model->insertItems(beatChildren, beforeBeatItem);
                            }
                        }
                    }

                    //
                    // ... и удалим его
                    //
                    d->model->removeItem(beatItem);
                    break;
                }
            });
    connect(d->itemParametersView, &CardItemParametersView::storyDayChanged, this,
            [this, findCurrentModelItem](const QString& _storyDay) {
                auto item = findCurrentModelItem();
                if (item == nullptr || item->type() != BusinessLayer::TextModelItemType::Group) {
                    return;
                }

                auto groupItem = static_cast<BusinessLayer::TextModelGroupItem*>(item);

                d->model->dictionariesModel()->removeStoryDay(groupItem->storyDay());
                d->model->dictionariesModel()->addStoryDay(_storyDay);
                //
                groupItem->setStoryDay(_storyDay);
                d->model->updateItem(groupItem);
            });
    connect(
        d->itemParametersView, &CardItemParametersView::numberChanged, this,
        [this, findCurrentModelItem](const QString& _number, bool _isCustom, bool _isEatNumber) {
            auto item = findCurrentModelItem();
            if (item == nullptr || item->type() != BusinessLayer::TextModelItemType::Group) {
                return;
            }

            auto groupItem = static_cast<BusinessLayer::TextModelGroupItem*>(item);
            if (_isCustom) {
                groupItem->setCustomNumber(_number, _isEatNumber);
            } else {
                groupItem->resetNumber();
            }
            d->model->updateItem(groupItem);
            d->model->updateNumbering();
        });
    connect(d->itemParametersView, &CardItemParametersView::tagsChanged, this,
            [this, findCurrentModelItem](const QVector<QPair<QString, QColor>>& _tags) {
                auto item = findCurrentModelItem();
                if (item == nullptr || item->type() != BusinessLayer::TextModelItemType::Group) {
                    return;
                }

                auto groupItem = static_cast<BusinessLayer::TextModelGroupItem*>(item);

                d->model->dictionariesModel()->removeTags(groupItem->tags());
                d->model->dictionariesModel()->addTags(_tags);

                groupItem->setTags(_tags);
                d->model->updateItem(groupItem);
            });
    //
    connect(d->commentsView, &CommentsView::addReviewMarkRequested, this,
            [this](const QColor& _color, const QString& _comment) {
                d->addReviewMark({}, _color, _comment, false, false, false);
            });
    connect(d->commentsView, &CommentsView::changeReviewMarkRequested, this,
            [this](const QModelIndex& _index, const QString& _comment) {
                QSignalBlocker blocker(d->commentsView);
                d->commentsModel->setComment(_index, _comment);
            });
    connect(d->commentsView, &CommentsView::addReviewMarkReplyRequested, this,
            [this](const QModelIndex& _index, const QString& _reply) {
                QSignalBlocker blocker(d->commentsView);
                d->commentsModel->addReply(_index, _reply);
            });
    connect(d->commentsView, &CommentsView::editReviewMarkReplyRequested, this,
            [this](const QModelIndex& _index, int _replyIndex, const QString& _reply) {
                QSignalBlocker blocker(d->commentsView);
                d->commentsModel->editReply(_index, _replyIndex, _reply);
            });
    connect(d->commentsView, &CommentsView::removeReviewMarkReplyRequested, this,
            [this](const QModelIndex& _index, int _replyIndex) {
                QSignalBlocker blocker(d->commentsView);
                d->commentsModel->removeReply(_index, _replyIndex);
            });
    connect(d->commentsView, &CommentsView::commentSelected, this,
            [this](const QModelIndex& _index) {
                const auto positionHint = d->commentsModel->mapToModel(_index);

                if (d->toolbar->isItemIsolationEnabled()) {
                    d->textEdit->setVisibleTopLevelItemIndex(positionHint.index);
                }

                const auto position = d->textEdit->positionForModelIndex(positionHint.index)
                    + positionHint.blockPosition;
                auto cursor = d->textEdit->textCursor();
                cursor.setPosition(position);
                d->textEdit->ensureCursorVisible(cursor);
                d->scalableWrapper->setFocus();
            });
    connect(d->commentsView, &CommentsView::markAsDoneRequested, this,
            [this](const QModelIndexList& _indexes) {
                QSignalBlocker blocker(d->commentsView);
                d->commentsModel->markAsDone(_indexes);
            });
    connect(d->commentsView, &CommentsView::markAsUndoneRequested, this,
            [this](const QModelIndexList& _indexes) {
                QSignalBlocker blocker(d->commentsView);
                d->commentsModel->markAsUndone(_indexes);
            });
    connect(d->commentsView, &CommentsView::applyChangeRequested, this,
            [this](const QModelIndexList& _indexes) {
                QSignalBlocker blocker(d->commentsView);
                d->commentsModel->applyChanges(_indexes);
            });
    connect(d->commentsView, &CommentsView::cancelChangeRequested, this,
            [this](const QModelIndexList& _indexes) {
                QSignalBlocker blocker(d->commentsView);
                d->commentsModel->cancelChanges(_indexes);
            });
    connect(d->commentsView, &CommentsView::removeRequested, this,
            [this](const QModelIndexList& _indexes) {
                QSignalBlocker blocker(d->commentsView);
                d->commentsModel->remove(_indexes);
            });
    //
    connect(d->aiAssistantView, &AiAssistantView::rephraseRequested, this,
            &ScreenplayTextView::rephraseTextRequested);
    connect(d->aiAssistantView, &AiAssistantView::expandRequested, this,
            &ScreenplayTextView::expandTextRequested);
    connect(d->aiAssistantView, &AiAssistantView::shortenRequested, this,
            &ScreenplayTextView::shortenTextRequested);
    connect(d->aiAssistantView, &AiAssistantView::insertRequested, this,
            &ScreenplayTextView::insertTextRequested);
    connect(d->aiAssistantView, &AiAssistantView::summarizeRequested, this,
            &ScreenplayTextView::summarizeTextRequested);
    connect(d->aiAssistantView, &AiAssistantView::translateRequested, this,
            &ScreenplayTextView::translateTextRequested);
    connect(d->aiAssistantView, &AiAssistantView::translateDocumentRequested, this,
            &ScreenplayTextView::translateDocumentRequested);
    connect(d->aiAssistantView, &AiAssistantView::generateSynopsisRequested, this,
            &ScreenplayTextView::generateSynopsisRequested);
    connect(d->aiAssistantView, &AiAssistantView::generateNovelRequested, this,
            &ScreenplayTextView::generateNovelRequested);
    d->aiAssistantView->setEditHistoryAvailable(true);
    d->aiAssistantView->setStoryMemoryAvailable(true);
    connect(d->aiAssistantView, &AiAssistantView::generateChatTextRequested, this,
            [this](const QString& _text, const QString& _conversationContext) {
                d->aiRequestInstruction = _text;
                emit generateTextRequested(_text, _conversationContext);
            });
    connect(d->aiAssistantView, &AiAssistantView::editHistoryRequested, this,
            [this] { showAssistantEditHistory(); });
    connect(d->aiAssistantView, &AiAssistantView::storyMemoryRequested, this,
            &ScreenplayTextView::showAssistantStoryMemory);
    connect(d->aiAssistantView, &AiAssistantView::cancelGenerationRequested, this, [this] {
        clearAssistantSelection();
        emit cancelAssistantRequested();
    });
    connect(d->aiAssistantView, &AiAssistantView::insertTextRequested, this,
            [this](const QString& _text) { d->textEdit->insertPlainText(_text); });
    connect(d->aiAssistantView, &AiAssistantView::buyCreditsPressed, this,
            &ScreenplayTextView::buyCreditsRequested);
    //
    connect(d->bookmarksView, &BookmarksView::addBookmarkRequested, this,
            &ScreenplayTextView::createBookmarkRequested);
    connect(d->bookmarksView, &BookmarksView::changeBookmarkRequested, this,
            [this](const QModelIndex& _index, const QString& _text, const QColor& _color) {
                emit changeBookmarkRequested(d->bookmarksModel->mapToModel(_index), _text, _color);
            });
    connect(d->bookmarksView, &BookmarksView::bookmarkSelected, this,
            [this](const QModelIndex& _index) {
                const auto index = d->bookmarksModel->mapToModel(_index);

                if (d->toolbar->isItemIsolationEnabled()) {
                    d->textEdit->setVisibleTopLevelItemIndex(index);
                }

                const auto position = d->textEdit->positionForModelIndex(index);
                auto cursor = d->textEdit->textCursor();
                cursor.setPosition(position);
                d->textEdit->ensureCursorVisible(cursor);
                d->scalableWrapper->setFocus();
            });
    connect(d->bookmarksView, &BookmarksView::removeRequested, this,
            [this](const QModelIndexList& _indexes) {
                QSignalBlocker blocker(d->commentsView);
                d->bookmarksModel->remove(_indexes);
            });
    //
    connect(d->complianceCheckResultView, &ComplianceCheckResultView::sceneSelected, this,
            [this](const QUuid& _sceneUuid) {
                const auto index = d->model->indexForUuid(_sceneUuid);

                if (d->toolbar->isItemIsolationEnabled()) {
                    d->textEdit->setVisibleTopLevelItemIndex(index);
                }

                const auto position = d->textEdit->positionForModelIndex(index);
                auto cursor = d->textEdit->textCursor();
                cursor.setPosition(position);
                d->textEdit->ensureCursorVisible(cursor);
                d->scalableWrapper->setFocus();
            });
    //
    connect(d->showSceneParametersAction, &QAction::toggled, this, [this](bool _checked) {
        d->updateOptionsTranslations();
        d->sidebarTabs->setTabVisible(kSceneParametersTabIndex, _checked);
        d->itemParametersView->setVisible(_checked);
        if (_checked) {
            d->sidebarTabs->setCurrentTab(kSceneParametersTabIndex);
            d->sidebarContent->setCurrentWidget(d->itemParametersView);
        }
        d->updateSideBarVisibility(this);
    });
    //
    connect(d->showBookmarksAction, &QAction::toggled, this, [this](bool _checked) {
        d->updateOptionsTranslations();
        d->sidebarTabs->setTabVisible(kBookmarksTabIndex, _checked);
        d->bookmarksView->setVisible(_checked);
        if (_checked) {
            d->sidebarTabs->setCurrentTab(kBookmarksTabIndex);
            d->sidebarContent->setCurrentWidget(d->bookmarksView);
        }
        d->updateSideBarVisibility(this);
    });
    //
    connect(d->showDictionariesAction, &QAction::toggled, this, [this](bool _checked) {
        d->updateOptionsTranslations();
        d->sidebarTabs->setTabVisible(kDictionariesTabIndex, _checked);
        d->dictionariesView->setVisible(_checked);
        if (_checked) {
            d->sidebarTabs->setCurrentTab(kDictionariesTabIndex);
            d->sidebarContent->setCurrentWidget(d->dictionariesView);
        }
        d->updateSideBarVisibility(this);
    });
    //
    connect(d->showComplianceCheckResultAction, &QAction::toggled, this, [this](bool _checked) {
        d->updateOptionsTranslations();
        d->sidebarTabs->setTabVisible(kComplianceCheckResultTabIndex, _checked);
        d->complianceCheckResultView->setVisible(_checked);
        if (_checked) {
            d->sidebarTabs->setCurrentTab(kComplianceCheckResultTabIndex);
            d->sidebarContent->setCurrentWidget(d->complianceCheckResultView);
        }
        d->updateSideBarVisibility(this);
    });
    //
    connect(&d->cursorChangeNotificationsDebounser, &Debouncer::gotWork, this, [this] {
        emit cursorChanged(QString::number(d->textEdit->textCursor().position()).toUtf8());
    });

    reconfigure({});
}

ScreenplayTextView::~ScreenplayTextView() = default;

QWidget* ScreenplayTextView::asQWidget()
{
    return this;
}

void ScreenplayTextView::toggleFullScreen(bool _isFullScreen)
{
    d->toolbar->setVisible(!_isFullScreen);
    d->screenplayTextScrollbarManager->setScrollBarVisible(!_isFullScreen);
}

void ScreenplayTextView::setEditingMode(ManagementLayer::DocumentEditingMode _mode)
{
    const auto readOnly = _mode != ManagementLayer::DocumentEditingMode::Edit;
    d->textEdit->setReadOnly(readOnly);
    d->toolbar->setReadOnly(readOnly);
    d->searchManager->setReadOnly(readOnly);
    if (readOnly && d->commentsToolbar->isVisible()) {
        d->commentsToolbar->hideToolbar();
    }
    d->itemParametersView->setReadOnly(readOnly);
    d->commentsView->setReadOnly(_mode == ManagementLayer::DocumentEditingMode::Read);
    d->aiAssistantView->setReadOnly(_mode == ManagementLayer::DocumentEditingMode::Read);
    d->bookmarksView->setReadOnly(readOnly);
    d->dictionariesView->setReadOnly(readOnly);
    const auto enabled = !readOnly;
    d->shortcutsManager.setEnabled(enabled);
    d->fastFormatWidget->setEnabled(enabled);
}

void ScreenplayTextView::setCursors(const QVector<Domain::CursorInfo>& _cursors)
{
    d->textEdit->setCollaboratorsCursors(_cursors);
}

void ScreenplayTextView::setCurrentCursor(const Domain::CursorInfo& _cursor)
{
    setCursorPosition(_cursor.cursorData.toInt());
}

void ScreenplayTextView::setCurrentModelIndex(const QModelIndex& _index)
{
    if (d->toolbar->isItemIsolationEnabled()) {
        d->textEdit->setVisibleTopLevelItemIndex(_index);
    }

    d->textEdit->setCurrentModelIndex(_index);
}

void ScreenplayTextView::setAvailableCredits(int _credits)
{
    d->aiAssistantView->setAvailableWords(_credits);
}

void ScreenplayTextView::setAiAssistantInProgress(bool _inProgress)
{
    d->aiAssistantInProgress = _inProgress;
    d->aiAssistantView->setGenerationInProgress(_inProgress);
}

void ScreenplayTextView::setAiAssistantStatus(const QString& _status)
{
    d->aiAssistantView->setGenerationStatus(_status);
}

void ScreenplayTextView::setRephrasedText(const QString& _text)
{
    d->aiAssistantView->setRephraseResult(_text);
}

void ScreenplayTextView::setExpandedText(const QString& _text)
{
    d->aiAssistantView->setExpandResult(_text);
}

void ScreenplayTextView::setShortenedText(const QString& _text)
{
    d->aiAssistantView->setShortenResult(_text);
}

void ScreenplayTextView::setInsertedText(const QString& _text)
{
    d->aiAssistantView->setInsertResult(_text);
}

void ScreenplayTextView::setSummarizedText(const QString& _text)
{
    d->aiAssistantView->setSummarizeResult(_text);
}

void ScreenplayTextView::setTranslatedText(const QString& _text)
{
    d->aiAssistantView->setTransateResult(_text);
}

void ScreenplayTextView::setTranslatedDocument(const QVector<QString>& _text)
{
    auto lines = _text;
    std::function<void(const QModelIndex&)> updateLines;
    updateLines = [this, &updateLines, &lines](const QModelIndex& _parentItemIndex) {
        for (int row = 0; row < d->model->rowCount(_parentItemIndex); ++row) {
            const auto itemIndex = d->model->index(row, 0, _parentItemIndex);
            const auto item = d->model->itemForIndex(itemIndex);
            switch (item->type()) {
            case BusinessLayer::TextModelItemType::Folder: {
                updateLines(itemIndex);
                break;
            }

            case BusinessLayer::TextModelItemType::Group: {
                updateLines(itemIndex);
                break;
            }

            case BusinessLayer::TextModelItemType::Text: {
                auto textItem = static_cast<BusinessLayer::ScreenplayTextModelTextItem*>(item);
                if (!textItem->text().isEmpty()) {
                    textItem->setText(lines.takeFirst());
                    textItem->setFormats({});
                    d->model->updateItem(textItem);
                }
                break;
            }

            default: {
                break;
            }
            }
        }
    };
    updateLines({});
}

void ScreenplayTextView::setGeneratedSynopsis(const QString& _text)
{
    d->aiAssistantView->setGenerateSynopsisResult(_text);
}

void ScreenplayTextView::setGeneratedText(const QString& _text)
{
    auto normalizedText = _text.trimmed();
    if (normalizedText.startsWith("```")) {
        const auto firstLineEnd = normalizedText.indexOf('\n');
        const auto lastFence = normalizedText.lastIndexOf("```");
        if (firstLineEnd >= 0 && lastFence > firstLineEnd) {
            normalizedText
                = normalizedText.mid(firstLineEnd + 1, lastFence - firstLineEnd - 1).trimmed();
        }
    }

    QJsonParseError protocolParseError;
    const auto protocolDocument
        = QJsonDocument::fromJson(normalizedText.toUtf8(), &protocolParseError);
    const auto protocolObject = protocolDocument.object();
    const bool isProtocolResponse = protocolParseError.error == QJsonParseError::NoError
        && protocolDocument.isObject() && protocolObject.value("version").toInt() == 3
        && protocolObject.value("action").isString();
    if (d->aiActionProtocolPending && !isProtocolResponse) {
        clearAssistantSelection();
        d->aiAssistantView->appendAssistantMessage(
            tr("Codex returned an invalid action response, so STARC did not change the screenplay. "
               "Please retry the request."));
        return;
    }

    if (isProtocolResponse) {
        const auto action = protocolObject.value("action").toString();
        const auto target = protocolObject.value("target").toString();
        const auto content = protocolObject.value("content").toString();
        const auto summary = protocolObject.value("summary").toString();
        const bool requiresApproval = protocolObject.value("requiresApproval").toBool();
        const auto entityId = protocolObject.value("entityId").toString();
        const auto entityName = protocolObject.value("entityName").toString();
        const auto fieldChanges = protocolObject.value("fieldChanges").toArray();
        const auto impactSummary = protocolObject.value("impactSummary").toString();
        const auto continuityChecks = protocolObject.value("continuityChecks").toArray();
        const bool validContinuityChecks
            = protocolObject.value("continuityChecks").isArray()
            && std::all_of(continuityChecks.cbegin(), continuityChecks.cend(),
                           [](const QJsonValue& _value) {
                const auto check = _value.toObject();
                const auto severity = check.value("severity").toString();
                return _value.isObject()
                    && (severity == "critical" || severity == "caution"
                        || severity == "suggestion")
                    && check.value("category").isString() && check.value("issue").isString()
                    && check.value("evidence").isString();
            });
        const auto hasCapturedSelection
            = d->aiEditSelectionStart >= 0 && d->aiEditSelectionEnd > d->aiEditSelectionStart;
        const bool isEditorAction
            = action == "insert_screenplay" || action == "replace_selection"
            || action == "delete_selection" || action == "clear_screenplay"
            || action == "update_logline" || action == "replace_synopsis"
            || action == "revise_treatment" || action == "create_character"
            || action == "update_character" || action == "remove_character"
            || action == "merge_character"
            || action == "update_character_relationship"
            || action == "update_story_memory";
        const bool validConversationAction
            = (action == "answer" || action == "suggest_ideas"
               || action == "request_clarification")
            && target == "none" && !requiresApproval;
        const bool validInsertAction
            = action == "insert_screenplay"
            && (target == "cursor" || target == "beginning" || target == "end")
            && requiresApproval && !content.trimmed().isEmpty();
        const bool validSelectionAction
            = (action == "replace_selection" || action == "delete_selection")
            && target == "selection" && requiresApproval;
        const bool validClearAction
            = action == "clear_screenplay" && target == "none" && requiresApproval;
        const bool validProjectDocumentAction
            = ((action == "update_logline" && target == "logline")
               || (action == "replace_synopsis" && target == "synopsis")
               || (action == "revise_treatment" && target == "treatment"))
            && requiresApproval && !content.trimmed().isEmpty();
        const bool validStoryMemoryAction
            = action == "update_story_memory" && target == "story_memory"
            && requiresApproval && !content.trimmed().isEmpty();
        bool validFieldChanges = false;
        const auto changes = fieldChangesMap(fieldChanges, &validFieldChanges);
        const QSet<QString> relationshipFields{ "related_character_id", "feeling", "details" };
        QSet<QString> mergeFields = editableCharacterFields();
        mergeFields.insert("merge_source_character_id");
        const bool characterFieldsOnly
            = std::all_of(changes.keyBegin(), changes.keyEnd(), [](const QString& _field) {
                return editableCharacterFields().contains(_field);
            });
        const bool relationshipFieldsOnly
            = std::all_of(changes.keyBegin(), changes.keyEnd(),
                          [&relationshipFields](const QString& _field) {
                return relationshipFields.contains(_field);
            });
        const bool mergeFieldsOnly
            = std::all_of(changes.keyBegin(), changes.keyEnd(), [&mergeFields](const QString& _field) {
                return mergeFields.contains(_field);
            });
        const bool validCreateCharacterAction
            = action == "create_character" && target == "characters" && requiresApproval
            && entityId.isEmpty() && !entityName.simplified().isEmpty() && validFieldChanges
            && characterFieldsOnly && !changes.contains("name");
        const bool validUpdateCharacterAction
            = action == "update_character" && target == "characters" && requiresApproval
            && !QUuid(entityId).isNull() && !entityName.simplified().isEmpty()
            && validFieldChanges && !changes.isEmpty() && characterFieldsOnly;
        const bool validRemoveCharacterAction
            = action == "remove_character" && target == "characters" && requiresApproval
            && !QUuid(entityId).isNull() && !entityName.simplified().isEmpty()
            && content.isEmpty() && validFieldChanges && changes.isEmpty();
        const bool validMergeCharacterAction
            = action == "merge_character" && target == "characters" && requiresApproval
            && !QUuid(entityId).isNull() && !entityName.simplified().isEmpty()
            && content.isEmpty() && validFieldChanges && mergeFieldsOnly
            && changes.contains("merge_source_character_id")
            && !QUuid(changes.value("merge_source_character_id")).isNull()
            && !changes.contains("name");
        const bool validRelationshipAction
            = action == "update_character_relationship" && target == "character_relationships"
            && requiresApproval && !QUuid(entityId).isNull()
            && !entityName.simplified().isEmpty() && validFieldChanges
            && relationshipFieldsOnly && changes.contains("related_character_id")
            && !QUuid(changes.value("related_character_id")).isNull();
        if (!validContinuityChecks
            || (!validConversationAction && !validInsertAction && !validSelectionAction
             && !validClearAction && !validProjectDocumentAction && !validStoryMemoryAction
             && !validCreateCharacterAction && !validUpdateCharacterAction
             && !validRemoveCharacterAction && !validMergeCharacterAction
             && !validRelationshipAction)
            || (isEditorAction && !requiresApproval)) {
            clearAssistantSelection();
            d->aiAssistantView->appendAssistantMessage(
                tr("Codex proposed an unsupported or unsafe action, so STARC did not change the "
                   "screenplay. Please rephrase the request."));
            return;
        }

        if (validConversationAction) {
            clearAssistantSelection();
            d->aiAssistantView->appendAssistantMessage(
                content.trimmed().isEmpty()
                    ? tr("I need a little more detail before I can answer safely.")
                    : content);
            return;
        }

        if ((action == "replace_selection" || action == "delete_selection")
            && !hasCapturedSelection) {
            clearAssistantSelection();
            d->aiAssistantView->appendAssistantMessage(
                tr("Select the exact screenplay passage first, then ask me to change or delete "
                   "that selection."));
            return;
        }

        d->aiPendingAction = action;
        d->aiPendingTarget = target;
        d->aiPendingSummary = summary;
        d->aiPendingImpactSummary = impactSummary;
        d->aiPendingContinuityChecks = continuityChecks;

        if (validCreateCharacterAction || validUpdateCharacterAction
            || validRemoveCharacterAction
            || validMergeCharacterAction
            || validRelationshipAction) {
            auto characters = d->model != nullptr ? d->model->charactersModel() : nullptr;
            const QUuid characterId(entityId);
            auto character = characters != nullptr ? characters->character(characterId) : nullptr;
            if (validCreateCharacterAction) {
                if (characters == nullptr || characters->exists(entityName)) {
                    clearAssistantSelection();
                    d->aiAssistantView->appendAssistantMessage(
                        tr("A character with that name already exists, so STARC did not create a "
                           "duplicate. Ask Codex to update the existing character instead."));
                    return;
                }
            } else if (character == nullptr || character->name() != entityName) {
                clearAssistantSelection();
                d->aiAssistantView->appendAssistantMessage(
                    tr("The proposed character target no longer matches the native Character tab, "
                       "so STARC did not edit anything. Please retry from the current project."));
                return;
            }

            QUuid relatedCharacterId;
            BusinessLayer::CharacterModel* relatedCharacter = nullptr;
            if (validRelationshipAction) {
                relatedCharacterId = QUuid(changes.value("related_character_id"));
                relatedCharacter = characters->character(relatedCharacterId);
                if (relatedCharacter == nullptr || relatedCharacterId == characterId) {
                    clearAssistantSelection();
                    d->aiAssistantView->appendAssistantMessage(
                        tr("The proposed relationship did not identify two different live "
                           "characters, so STARC did not edit anything."));
                    return;
                }
            }
            const QUuid mergeSourceId
                = validMergeCharacterAction
                ? QUuid(changes.value("merge_source_character_id")) : QUuid();
            auto mergeSource = validMergeCharacterAction
                ? characters->character(mergeSourceId) : nullptr;
            if (validMergeCharacterAction
                && (mergeSource == nullptr || mergeSource == character
                    || !mergeFieldPlanIsComplete(character, mergeSource, changes))) {
                clearAssistantSelection();
                d->aiAssistantView->appendAssistantMessage(
                    mergeSource == nullptr || mergeSource == character
                        ? tr("The merge plan did not identify two different live characters, so "
                             "STARC did not edit anything.")
                        : tr("The merge plan left one or more profile differences unresolved. "
                             "Tell Codex which character's conflicting information to keep, then "
                             "retry."));
                return;
            }

            if (changes.contains("name") && characters->exists(changes.value("name"))
                && changes.value("name").simplified() != character->name()) {
                clearAssistantSelection();
                d->aiAssistantView->appendAssistantMessage(
                    tr("That character name is already in use, so STARC did not apply the rename."));
                return;
            }
            if (changes.contains("story_role")) {
                BusinessLayer::CharacterStoryRole ignoredRole;
                if (!characterRoleFromValue(changes.value("story_role"), &ignoredRole)) {
                    clearAssistantSelection();
                    d->aiAssistantView->appendAssistantMessage(
                        tr("Codex proposed an invalid character story role, so nothing was edited."));
                    return;
                }
            }

            const auto before = validCreateCharacterAction
                ? QString()
                : validRelationshipAction
                ? relationshipSnapshot(character, relatedCharacterId)
                : characterSnapshot(character);
            const auto dependencySnapshot = validRemoveCharacterAction
                ? characterDependencySnapshot(d->model, character) : QJsonObject();
            const auto dependencyReport = validRemoveCharacterAction
                ? characterDependencyReport(dependencySnapshot) : QString();
            const auto mergePlan = validMergeCharacterAction
                ? characterMergePlan(d->model, character, mergeSource, changes) : QJsonObject();
            const auto mergePlanReport = validMergeCharacterAction
                ? characterMergePlanReport(mergePlan) : QString();
            QString preview;
            if (validMergeCharacterAction) {
                preview = mergePlanReport;
            } else if (validRemoveCharacterAction) {
                preview = tr("CHARACTER PROFILE\n− %1\n\n%2")
                              .arg(character->name(), dependencyReport);
            } else if (validRelationshipAction) {
                const auto relation = character->relation(relatedCharacterId);
                preview = tr("RELATIONSHIP\n%1 → %2\n\nFEELING\n− %3\n+ %4\n\nDETAILS\n− %5\n+ %6")
                              .arg(character->name(), relatedCharacter->name(),
                                   relation.feeling.isEmpty() ? tr("(empty)") : relation.feeling,
                                   changes.contains("feeling")
                                       ? (changes.value("feeling").isEmpty() ? tr("(empty)")
                                                                             : changes.value("feeling"))
                                       : relation.feeling,
                                   relation.details.isEmpty() ? tr("(empty)") : relation.details,
                                   changes.contains("details")
                                       ? (changes.value("details").isEmpty() ? tr("(empty)")
                                                                             : changes.value("details"))
                                       : relation.details);
            } else {
                preview = characterChangesPreview(character, entityName, changes);
            }

            const auto instruction = d->aiRequestInstruction;
            const auto editSummary = d->aiPendingSummary;
            const auto gateReport = continuityGateReport(impactSummary, continuityChecks);
            const auto requestRevision = d->aiRequestStorySourceRevision;
            auto dialog = new Dialog(this);
            dialog->setContentFixedWidth(Ui::DesignSystem::dialog().maximumWidth());
            const auto availableReviewHeight
                = qMax(240, height() - static_cast<int>(Ui::DesignSystem::layout().px48() * 2));
            dialog->setContentFixedHeight(qMin(720, availableReviewHeight));
            dialog->enableSupportingTextScrolling();
            dialog->enableDiffHighlighting();
            dialog->setDismissOnOutsideClick(false);
            dialog->setRejectOnEscape(false);
            dialog->showDialog(
                validMergeCharacterAction ? tr("Review Codex character merge")
                : validRelationshipAction ? tr("Review Codex relationship change")
                                        : tr("Review Codex character change"),
                tr("Review the native STARC field changes below. Nothing will change until you "
                   "approve.\n\n%1\n\n%2")
                    .arg(preview, gateReport),
                { { 0, tr("Discard"), Dialog::RejectButton },
                  { 2, tr("Revise proposal"), Dialog::NormalButton },
                  { 1, validRemoveCharacterAction ? tr("Review removal")
                       : validMergeCharacterAction ? tr("Approve merge plan")
                                                  : tr("Apply to Character tabs"),
                    Dialog::AcceptButton } }, false);
            d->aiActionProtocolPending = false;
            connect(dialog, &Dialog::finished, this,
                    [this, dialog, protocolObject, instruction, editSummary, action, entityId,
                     entityName, changes, before, relatedCharacterId, requestRevision, gateReport,
                     continuityChecks, impactSummary, dependencySnapshot,
                     dependencyReport, mergeSourceId, mergePlan,
                     mergePlanReport](const Dialog::ButtonInfo& _button) {
                dialog->hideDialog();
                if (_button.id == 2) {
                    emit generateTextRequested(
                        entityRevisionRequest(protocolObject, instruction, gateReport), {});
                    return;
                }
                if (_button.type != Dialog::AcceptButton) {
                    clearAssistantSelection();
                    d->aiAssistantView->appendAssistantMessage(
                        tr("The proposed character change was discarded. Nothing was edited."));
                    return;
                }

                auto applyChange = [this, instruction, editSummary, action, entityId, entityName,
                                    changes, before, relatedCharacterId, requestRevision,
                                    continuityChecks, impactSummary, dependencySnapshot,
                                    mergeSourceId, mergePlan] {
                    if (requestRevision != d->storySourceRevision) {
                        clearAssistantSelection();
                        d->aiAssistantView->appendAssistantMessage(
                            tr("The story changed while Codex was working, so STARC did not "
                               "overwrite newer character information. Please retry."));
                        return;
                    }
                    auto characters = d->model != nullptr ? d->model->charactersModel() : nullptr;
                    auto character = characters != nullptr
                        ? characters->character(QUuid(entityId)) : nullptr;
                    bool applied = false;
                    QString targetName;
                    if (action == "create_character") {
                        if (characters == nullptr || characters->exists(entityName)) {
                            clearAssistantSelection();
                            d->aiAssistantView->appendAssistantMessage(
                                tr("The character now exists, so STARC did not create a duplicate."));
                            return;
                        }
                        characters->createCharacter(entityName);
                        character = characters->character(entityName);
                        if (character != nullptr) {
                            applied = true;
                            for (auto it = changes.constBegin(); it != changes.constEnd(); ++it) {
                                applied = setCharacterField(character, it.key(), it.value()) && applied;
                            }
                            targetName = QString("character:%1")
                                             .arg(character->document()->uuid().toString(
                                                 QUuid::WithoutBraces));
                        }
                    } else if (action == "remove_character") {
                        if (character == nullptr || characterSnapshot(character) != before
                            || characterDependencySnapshot(d->model, character)
                                != dependencySnapshot) {
                            clearAssistantSelection();
                            d->aiAssistantView->appendAssistantMessage(
                                tr("That character's dependencies changed before approval, so "
                                   "STARC did not remove the profile. Please review it again."));
                            return;
                        }
                        targetName = QString("character:%1").arg(entityId);
                        emit character->removeRequested();
                        applied = characters->character(QUuid(entityId)) == nullptr;
                    } else if (action == "merge_character") {
                        auto source = characters != nullptr
                            ? characters->character(mergeSourceId) : nullptr;
                        if (character == nullptr || source == nullptr
                            || characterSnapshot(character) != before
                            || characterMergePlan(d->model, character, source, changes)
                                != mergePlan) {
                            clearAssistantSelection();
                            d->aiAssistantView->appendAssistantMessage(
                                tr("One of the character profiles or dependencies changed before "
                                   "approval, so STARC did not perform the merge. Please review a "
                                   "fresh plan."));
                            return;
                        }
                        source->saveChanges();
                        const auto survivorName = character->name();
                        source->setName(survivorName);
                        const bool sourceMovedToRecycleBin
                            = characters->character(mergeSourceId) == nullptr;
                        const auto transactionId
                            = QSettings().value(characterMergeLatestTransactionKey(mergeSourceId))
                                  .toString();
                        if (!sourceMovedToRecycleBin || transactionId.isEmpty()) {
                            clearAssistantSelection();
                            d->aiAssistantView->appendAssistantMessage(
                                tr("STARC could not create the merge safety journal, so it stopped "
                                   "before changing the survivor profile."));
                            return;
                        }

                        applied = true;
                        for (auto it = changes.constBegin(); it != changes.constEnd(); ++it) {
                            if (!editableCharacterFields().contains(it.key())) continue;
                            applied = setCharacterField(character, it.key(), it.value()) && applied;
                        }
                        QSet<QUuid> survivorPhotos;
                        for (const auto& photo : character->photos()) {
                            survivorPhotos.insert(photo.uuid);
                        }
                        for (const auto& photo : source->photos()) {
                            if (!survivorPhotos.contains(photo.uuid)) {
                                character->addPhoto(photo);
                                survivorPhotos.insert(photo.uuid);
                            }
                        }
                        transferCharacterRelationships(characters, character, source);
                        applied = applied && sourceMovedToRecycleBin
                            && characters->character(QUuid(entityId)) == character;
                        targetName = QString("merge:%1:%2:%3")
                                         .arg(entityId,
                                              mergeSourceId.toString(QUuid::WithoutBraces),
                                              transactionId);
                        if (!applied) {
                            d->pendingCharacterMergeRollbackId = transactionId;
                            emit characterMergeRollbackRequested(transactionId);
                        }
                    } else if (action == "update_character_relationship") {
                        if (character == nullptr
                            || relationshipSnapshot(character, relatedCharacterId) != before) {
                            clearAssistantSelection();
                            d->aiAssistantView->appendAssistantMessage(
                                tr("That relationship changed before approval, so STARC kept the "
                                   "newer version."));
                            return;
                        }
                        if (!character->relation(relatedCharacterId).isValid()) {
                            character->createRelation(relatedCharacterId);
                        }
                        auto relation = character->relation(relatedCharacterId);
                        if (changes.contains("feeling")) relation.feeling = changes.value("feeling");
                        if (changes.contains("details")) relation.details = changes.value("details");
                        character->updateRelation(relation);
                        applied = character->relation(relatedCharacterId).isValid()
                            && character->relation(relatedCharacterId).feeling == relation.feeling
                            && character->relation(relatedCharacterId).details == relation.details;
                        targetName = QString("relationship:%1:%2")
                                         .arg(entityId,
                                              relatedCharacterId.toString(QUuid::WithoutBraces));
                    } else {
                        if (character == nullptr || characterSnapshot(character) != before) {
                            clearAssistantSelection();
                            d->aiAssistantView->appendAssistantMessage(
                                tr("That character changed before approval, so STARC kept the newer "
                                   "version."));
                            return;
                        }
                        applied = true;
                        for (auto it = changes.constBegin(); it != changes.constEnd(); ++it) {
                            applied = setCharacterField(character, it.key(), it.value()) && applied;
                        }
                        targetName = QString("character:%1").arg(entityId);
                    }

                    const auto after = action == "remove_character"
                        ? tr("Moved %1 to STARC Recycle Bin; story text was preserved.")
                              .arg(entityName)
                        : action == "merge_character"
                        ? QString("%1\n\nMERGED DUPLICATE\n%2 moved to Recycle Bin")
                              .arg(characterSnapshot(character),
                                   mergePlan.value("sourceName").toString())
                        : action == "update_character_relationship"
                        ? relationshipSnapshot(character, relatedCharacterId)
                        : characterSnapshot(character);
                    if (applied) {
                        recordAssistantEdit(d->model, action, instruction, editSummary, before,
                                            after, targetName, impactSummary, continuityChecks);
                        markAssistantStoryMemoryStale(d->model);
                    }
                    clearAssistantSelection();
                    d->aiAssistantView->appendAssistantMessage(
                        applied
                            ? (action == "remove_character"
                                   ? tr("Moved the approved character profile to STARC's Recycle "
                                        "Bin. Story text and relationship data were preserved.")
                                   : action == "merge_character"
                                   ? tr("Merged the approved duplicate into the surviving "
                                        "character. Native script cues, profile fields, photos, "
                                        "and relationship links were consolidated; the original "
                                        "duplicate profile is in Recycle Bin.")
                                   : tr("Applied the approved change directly to the native "
                                        "Character tabs. It is recorded in Codex edit history."))
                            : tr("STARC could not safely apply that character change, so nothing "
                                 "was edited."));
                };
                auto applyAfterDependencies
                    = [this, action, entityName, dependencySnapshot, dependencyReport,
                       mergePlan, mergePlanReport, applyChange] {
                    const auto dependencyName = action == "merge_character"
                        ? mergePlan.value("sourceName").toString() : entityName;
                    const auto dependencyDetails = action == "merge_character"
                        ? mergePlanReport : dependencyReport;
                    const bool needsDependencyConfirmation = action == "merge_character"
                        || (action == "remove_character"
                            && hasCharacterDependencies(dependencySnapshot));
                    applyAfterCharacterDependencyConfirmation(
                        this, dependencyName, dependencyDetails,
                        needsDependencyConfirmation,
                        applyChange, [this] {
                            clearAssistantSelection();
                            d->aiAssistantView->appendAssistantMessage(
                                tr("The referenced character was kept. Nothing was edited."));
                        });
                };
                applyAfterCriticalContinuityConfirmation(
                    this, continuityChecks, applyAfterDependencies, [this] {
                        clearAssistantSelection();
                        d->aiAssistantView->appendAssistantMessage(
                            tr("The character change with a confirmed-canon conflict was not "
                               "applied."));
                    });
            });
            connect(dialog, &Dialog::disappeared, dialog, &Dialog::deleteLater);
            return;
        }

        if (validStoryMemoryAction) {
            const auto proposedMemory = content.trimmed();
            const QStringList requiredSections{
                "CHARACTERS & RELATIONSHIPS", "CHARACTER KNOWLEDGE", "TIMELINE",
                "PLOT THREADS", "SETUPS & PAYOFFS", "WORLD RULES", "VOICE & STYLE",
                "CONTINUITY RISKS",
            };
            const bool hasRequiredSections
                = std::all_of(requiredSections.cbegin(), requiredSections.cend(),
                              [&proposedMemory](const QString& _section) {
                    return proposedMemory.contains(_section, Qt::CaseInsensitive);
                });
            if (!hasRequiredSections) {
                clearAssistantSelection();
                d->aiAssistantView->appendAssistantMessage(
                    tr("Codex returned an incomplete Story Memory structure, so STARC did not "
                       "save it. Please refresh Story Memory again."));
                return;
            }

            const auto previousMemory = d->aiRequestStoryMemory;
            const auto requestRevision = d->aiRequestStorySourceRevision;
            auto dialog = new Dialog(this);
            dialog->setContentFixedWidth(Ui::DesignSystem::dialog().maximumWidth());
            const auto availableReviewHeight
                = qMax(240, height() - static_cast<int>(Ui::DesignSystem::layout().px48() * 2));
            dialog->setContentFixedHeight(qMin(720, availableReviewHeight));
            dialog->enableSupportingTextScrolling();
            dialog->enableDiffHighlighting();
            dialog->setDismissOnOutsideClick(false);
            dialog->setRejectOnEscape(false);
            dialog->showDialog(
                tr("Review refreshed Story Memory"),
                tr("This is working continuity analysis, not automatic canon. Review it before "
                   "saving. Green + lines are new; red − lines are removed.\n\n%1")
                    .arg(screenplayLineDiff(previousMemory, proposedMemory)),
                { { 0, tr("Discard"), Dialog::RejectButton },
                  { 1, tr("Save Story Memory"), Dialog::AcceptButton } });
            d->aiActionProtocolPending = false;
            connect(dialog, &Dialog::finished, this,
                    [this, dialog, proposedMemory,
                     requestRevision](const Dialog::ButtonInfo& _button) {
                dialog->hideDialog();
                if (_button.type != Dialog::AcceptButton) {
                    clearAssistantSelection();
                    d->aiAssistantView->appendAssistantMessage(
                        tr("The proposed Story Memory refresh was discarded."));
                    return;
                }
                if (requestRevision != d->storySourceRevision) {
                    clearAssistantSelection();
                    d->aiAssistantView->appendAssistantMessage(
                        tr("The story changed while Codex was building Story Memory, so the stale "
                           "analysis was not saved. Refresh it again from the current draft."));
                    return;
                }
                saveAssistantStoryMemory(d->model, proposedMemory, false, false);
                clearAssistantSelection();
                d->aiAssistantView->appendAssistantMessage(
                    tr("Story Memory was refreshed from the current screenplay and linked tabs."));
            });
            connect(dialog, &Dialog::disappeared, dialog, &Dialog::deleteLater);
            return;
        }

        if (validProjectDocumentAction) {
            QString proposedContent = content.trimmed();
            if (action == "update_logline") {
                proposedContent = proposedContent.simplified();
            } else if (action == "revise_treatment") {
                proposedContent.replace("\r\n", "\n");
                proposedContent.replace('\r', '\n');
                while (proposedContent.endsWith('\n')) {
                    proposedContent.chop(1);
                }
                if (d->aiRequestTreatmentParagraphCount == 0
                    || proposedContent.split('\n', Qt::KeepEmptyParts).size()
                        != d->aiRequestTreatmentParagraphCount) {
                    clearAssistantSelection();
                    d->aiAssistantView->appendAssistantMessage(
                        tr("The proposed treatment revision did not preserve the existing scene "
                           "and beat paragraph structure, so STARC did not apply it. Ask Codex to "
                           "revise the existing treatment paragraphs without adding or removing "
                           "scenes."));
                    return;
                }
            }

            const auto before = action == "update_logline"
                ? d->aiRequestLogline
                : action == "replace_synopsis" ? d->aiRequestSynopsis
                                                  : d->aiRequestTreatment;
            const auto instruction = d->aiRequestInstruction;
            const auto editSummary = d->aiPendingSummary;
            const auto targetName = target;
            const auto continuityChecksForChange = d->aiPendingContinuityChecks;
            const auto impactSummaryForChange = d->aiPendingImpactSummary;
            const auto gateReport
                = continuityGateReport(impactSummaryForChange, continuityChecksForChange);
            auto dialog = new Dialog(this);
            dialog->setContentFixedWidth(Ui::DesignSystem::dialog().maximumWidth());
            const auto availableReviewHeight
                = qMax(240, height() - static_cast<int>(Ui::DesignSystem::layout().px48() * 2));
            dialog->setContentFixedHeight(qMin(720, availableReviewHeight));
            dialog->enableSupportingTextScrolling();
            dialog->enableDiffHighlighting();
            dialog->setDismissOnOutsideClick(false);
            dialog->setRejectOnEscape(false);
            const auto documentName = action == "update_logline"
                ? tr("logline")
                : action == "replace_synopsis" ? tr("synopsis") : tr("treatment outline");
            dialog->showDialog(
                tr("Review Codex %1 change").arg(documentName),
                tr("Review the proposed %1 change. Green + lines will be added; red − lines "
                   "will be removed. STARC will apply it only after approval.\n\n%2\n\n%3")
                    .arg(documentName, screenplayLineDiff(before, proposedContent), gateReport),
                { { 0, tr("Discard"), Dialog::RejectButton },
                  { 2, tr("Revise proposal"), Dialog::NormalButton },
                  { 1, tr("Apply change"), Dialog::AcceptButton } }, false);
            d->aiActionProtocolPending = false;
            connect(dialog, &Dialog::finished, this,
                    [this, dialog, action, proposedContent, before, instruction, editSummary,
                     targetName, gateReport,
                     continuityChecksForChange,
                     impactSummaryForChange](const Dialog::ButtonInfo& _button) {
                dialog->hideDialog();
                if (_button.id == 2) {
                    emit generateTextRequested(
                        continuityRevisionRequest(action, targetName, instruction,
                                                  proposedContent, gateReport),
                        {});
                    return;
                }
                if (_button.type != Dialog::AcceptButton) {
                    clearAssistantSelection();
                    d->aiAssistantView->appendAssistantMessage(
                        tr("The proposed project change was discarded. Nothing was edited."));
                    return;
                }

                auto applyChange = [this, action, proposedContent, before, instruction,
                                    editSummary, targetName, continuityChecksForChange,
                                    impactSummaryForChange] {
                    const auto current = action == "update_logline"
                        ? (d->model != nullptr && d->model->informationModel() != nullptr
                               ? d->model->informationModel()->logline()
                               : QString())
                        : action == "replace_synopsis"
                        ? simpleTextSnapshot(d->model != nullptr ? d->model->synopsisModel()
                                                                 : nullptr)
                        : treatmentSnapshot(d->model);
                    if (current != before) {
                        clearAssistantSelection();
                        d->aiAssistantView->appendAssistantMessage(
                            tr("That project document changed while Codex was working, so STARC "
                               "did not overwrite the newer version. Please run the request "
                               "again."));
                        return;
                    }

                    bool applied = false;
                    if (action == "update_logline" && d->model != nullptr
                        && d->model->informationModel() != nullptr) {
                        d->model->informationModel()->setLogline(proposedContent);
                        applied = d->model->informationModel()->logline() == proposedContent;
                    } else if (action == "replace_synopsis" && d->model != nullptr) {
                        applied = replaceSimpleText(d->model->synopsisModel(), proposedContent);
                    } else if (action == "revise_treatment") {
                        applied = replaceTreatmentParagraphs(d->model, proposedContent);
                    }
                    const auto after = action == "update_logline"
                        ? (d->model != nullptr && d->model->informationModel() != nullptr
                               ? d->model->informationModel()->logline()
                               : QString())
                        : action == "replace_synopsis"
                        ? simpleTextSnapshot(d->model != nullptr ? d->model->synopsisModel()
                                                                 : nullptr)
                        : treatmentSnapshot(d->model);
                    if (applied) {
                        recordAssistantEdit(d->model, action, instruction, editSummary, before,
                                            after, targetName, impactSummaryForChange,
                                            continuityChecksForChange);
                    }
                    clearAssistantSelection();
                    d->aiAssistantView->appendAssistantMessage(
                        applied
                            ? tr("Applied the approved %1 change directly to its native STARC tab. "
                                 "It is recorded in Codex edit history.")
                                  .arg(targetName)
                            : tr("STARC could not safely apply that project change, so nothing was "
                                 "edited."));
                };
                applyAfterCriticalContinuityConfirmation(
                    this, continuityChecksForChange, applyChange, [this] {
                        clearAssistantSelection();
                        d->aiAssistantView->appendAssistantMessage(
                            tr("The change with a confirmed-canon conflict was not applied."));
                    });
            });
            connect(dialog, &Dialog::disappeared, dialog, &Dialog::deleteLater);
            return;
        }

        if (action == "clear_screenplay") {
            d->aiActionProtocolPending = false;
            requestAssistantClearScreenplay();
            return;
        }

        if (action == "delete_selection") {
            const int selectionStart = d->aiEditSelectionStart;
            const int selectionEnd = d->aiEditSelectionEnd;
            const auto selectionText = d->aiEditSelectionText;
            const int documentRevision = d->aiEditDocumentRevision;
            const auto beforeSnapshot = screenplayFountainSnapshot(d->model);
            const auto instruction = d->aiRequestInstruction;
            const auto editSummary = d->aiPendingSummary;
            const auto continuityChecksForChange = d->aiPendingContinuityChecks;
            const auto impactSummaryForChange = d->aiPendingImpactSummary;
            const auto gateReport
                = continuityGateReport(impactSummaryForChange, continuityChecksForChange);
            auto preview = selectionText;
            preview.replace(QChar::ParagraphSeparator, '\n');
            d->aiActionProtocolPending = false;
            auto dialog = new Dialog(this);
            dialog->setContentFixedWidth(Ui::DesignSystem::dialog().maximumWidth());
            dialog->enableSupportingTextScrolling();
            dialog->setDismissOnOutsideClick(false);
            dialog->setRejectOnEscape(false);
            dialog->showDialog(
                tr("Review Codex screenplay deletion"),
                tr("The following selected screenplay passage will be removed as one undoable "
                   "editor action:\n\n%1\n\n%2")
                    .arg(preview, gateReport),
                { { 0, tr("Keep selection"), Dialog::RejectButton },
                  { 1, tr("Delete selection"), Dialog::AcceptButton } });
            connect(dialog, &Dialog::finished, this,
                    [this, dialog, selectionStart, selectionEnd, selectionText,
                     documentRevision, beforeSnapshot, instruction,
                     editSummary,
                     continuityChecksForChange,
                     impactSummaryForChange](const Dialog::ButtonInfo& _button) {
                dialog->hideDialog();
                if (_button.type != Dialog::AcceptButton) {
                    clearAssistantSelection();
                    d->aiAssistantView->appendAssistantMessage(
                        tr("The selected screenplay passage was kept."));
                    return;
                }
                auto applyDeletion = [this, selectionStart, selectionEnd, selectionText,
                                      documentRevision, beforeSnapshot, instruction, editSummary,
                                      continuityChecksForChange, impactSummaryForChange] {
                    QTextCursor cursor(d->textEdit->document());
                    cursor.setPosition(selectionStart);
                    cursor.setPosition(selectionEnd, QTextCursor::KeepAnchor);
                    if (d->textEdit->document()->revision() != documentRevision
                        || cursor.selectedText() != selectionText) {
                        clearAssistantSelection();
                        d->aiAssistantView->appendAssistantMessage(
                            tr("The screenplay changed while Codex was working, so nothing was "
                               "deleted. Select the passage again and retry."));
                        return;
                    }
                    cursor.beginEditBlock();
                    cursor.removeSelectedText();
                    cursor.endEditBlock();
                    d->textEdit->setTextCursor(cursor);
                    recordAssistantEdit(d->model, "delete_selection", instruction, editSummary,
                                        beforeSnapshot, screenplayFountainSnapshot(d->model),
                                        "screenplay", impactSummaryForChange,
                                        continuityChecksForChange);
                    clearAssistantSelection();
                    d->aiAssistantView->appendAssistantMessage(
                        tr("Deleted the approved screenplay selection. You can restore it with "
                           "Undo."));
                };
                applyAfterCriticalContinuityConfirmation(
                    this, continuityChecksForChange, applyDeletion, [this] {
                        clearAssistantSelection();
                        d->aiAssistantView->appendAssistantMessage(
                            tr("The deletion with a confirmed-canon conflict was not applied."));
                    });
            });
            connect(dialog, &Dialog::disappeared, dialog, &Dialog::deleteLater);
            return;
        }

        d->aiActionProtocolPending = false;
        d->aiEditApplyConfirmed = false;
        if (action == "replace_selection") {
            d->aiEditInsertionPosition = -1;
        } else {
            d->aiEditSelectionStart = -1;
            d->aiEditSelectionEnd = -1;
            d->aiEditSelectionText.clear();
            if (target == "beginning") {
                d->aiEditInsertionPosition = 0;
            } else if (target == "end") {
                d->aiEditInsertionPosition = d->textEdit->document()->characterCount() - 1;
            } else {
                d->aiEditInsertionPosition = d->aiRequestCursorPosition;
            }
        }
        setGeneratedText(content);
        return;
    }

    auto fountainText = normalizedText;

    const auto hasEditTarget
        = d->aiEditSelectionStart >= 0 && d->aiEditSelectionEnd > d->aiEditSelectionStart;
    const auto hasInsertionTarget = d->aiEditInsertionPosition >= 0;
    const auto isScreenplayChange = hasEditTarget || hasInsertionTarget;
    if (!isScreenplayChange) {
        d->aiAssistantView->appendAssistantMessage(_text);
        return;
    }
    if (fountainText.isEmpty()) {
        clearAssistantSelection();
        d->aiAssistantView->appendAssistantMessage(
            tr("Codex did not return usable screenplay text, so nothing was changed."));
        return;
    }

    // Markdown section labels are not screenplay scene headings and the Fountain importer would
    // otherwise treat them as ordinary action. Refuse the malformed result instead of silently
    // damaging screenplay structure.
    static const QRegularExpression markdownSceneLabel(
        QStringLiteral("(?im)^\\s*(?:#{1,6}\\s*)?(?:scene|sequence)\\s*"
                       "(?:#?\\d+|[IVXLCDM]+)(?:\\s*[:.\\-–—].*)?\\s*$"));
    if (markdownSceneLabel.match(fountainText).hasMatch()) {
        clearAssistantSelection();
        d->aiAssistantView->appendAssistantMessage(
            tr("Codex returned a Markdown scene label instead of a valid Fountain scene heading, "
               "so nothing was changed. Please retry; new scenes must begin with a heading like "
               "INT. LOCATION - TIME or EXT. LOCATION - TIME."));
        return;
    }
    if (!d->aiEditApplyConfirmed) {
        auto dialog = new Dialog(this);
        dialog->setContentFixedWidth(Ui::DesignSystem::dialog().maximumWidth());
        const auto availableReviewHeight
            = qMax(240, height() - static_cast<int>(Ui::DesignSystem::layout().px48() * 2));
        dialog->setContentFixedHeight(qMin(720, availableReviewHeight));
        dialog->enableSupportingTextScrolling();
        dialog->enableDiffHighlighting();
        // A screenplay review must end with an explicit button choice. Accidental clicks outside
        // the dialog or Escape presses must not silently discard a long generation.
        dialog->setDismissOnOutsideClick(false);
        dialog->setRejectOnEscape(false);
        const auto originalPreview
            = QString(d->aiEditSelectionText).replace(QChar::ParagraphSeparator, '\n');
        const auto actionForRevision = d->aiPendingAction;
        const auto targetForRevision = d->aiPendingTarget;
        const auto instructionForRevision = d->aiRequestInstruction;
        const auto continuityChecksForChange = d->aiPendingContinuityChecks;
        const auto gateReport
            = continuityGateReport(d->aiPendingImpactSummary, continuityChecksForChange);
        const auto reviewMessage
            = tr("Review the screenplay changes below. Green + lines will be added; red − lines "
                 "will be removed. Unmarked lines are unchanged. Approving will preserve native "
                 "scene headings, action, characters, parentheticals, dialogue, shots, and "
                 "transitions.\n\n%1\n\n%2")
                  .arg(screenplayLineDiff(hasEditTarget ? originalPreview : QString(),
                                          fountainText),
                       gateReport);
        dialog->showDialog(
            tr("Review Codex screenplay change"), reviewMessage,
            { { 0, tr("Discard"), Dialog::RejectButton },
              { 2, tr("Revise proposal"), Dialog::NormalButton },
              { 1, hasEditTarget ? tr("Replace selection") : tr("Apply to screenplay"),
                Dialog::AcceptButton } }, false);
        connect(dialog, &Dialog::finished, this,
                [this, dialog, text = fountainText, actionForRevision, targetForRevision,
                 instructionForRevision, continuityChecksForChange,
                 gateReport](const Dialog::ButtonInfo& _button) {
                    dialog->hideDialog();
                    if (_button.id == 2) {
                        emit generateTextRequested(
                            continuityRevisionRequest(actionForRevision, targetForRevision,
                                                      instructionForRevision, text, gateReport),
                            {});
                        return;
                    }
                    if (_button.type == Dialog::AcceptButton) {
                        applyAfterCriticalContinuityConfirmation(
                            this, continuityChecksForChange,
                            [this, text] {
                                d->aiEditApplyConfirmed = true;
                                setGeneratedText(text);
                            },
                            [this] {
                                clearAssistantSelection();
                                d->aiAssistantView->appendAssistantMessage(
                                    tr("The screenplay change with a confirmed-canon conflict was "
                                       "not applied."));
                            });
                    } else {
                        clearAssistantSelection();
                        d->aiAssistantView->appendAssistantMessage(
                            tr("The proposed screenplay change was discarded. Nothing was edited."));
                    }
                });
        connect(dialog, &Dialog::disappeared, dialog, &Dialog::deleteLater);
        return;
    }

    if (hasEditTarget) {
        QTextCursor editCursor(d->textEdit->document());
        editCursor.setPosition(d->aiEditSelectionStart);
        editCursor.setPosition(d->aiEditSelectionEnd, QTextCursor::KeepAnchor);
        if (editCursor.selectedText() != d->aiEditSelectionText) {
            d->aiEditSelectionStart = -1;
            d->aiEditSelectionEnd = -1;
            d->aiEditSelectionText.clear();
            d->aiEditInsertionPosition = -1;
            d->aiEditDocumentRevision = -1;
            d->aiEditApplyConfirmed = false;
            auto dialog = new Dialog(this);
            dialog->showDialog(tr("Codex edit not applied"),
                               tr("The selected passage changed while Codex was working. Select it "
                                  "again and retry so no newer writing is overwritten."),
                               { { 0, tr("OK"), Dialog::AcceptButton } });
            connect(dialog, &Dialog::finished, dialog, [dialog] { dialog->hideDialog(); });
            connect(dialog, &Dialog::disappeared, dialog, &Dialog::deleteLater);
            return;
        }
        d->textEdit->setTextCursor(editCursor);
    } else {
        if (d->textEdit->document()->revision() != d->aiEditDocumentRevision
            || d->aiEditInsertionPosition > d->textEdit->document()->characterCount() - 1) {
            clearAssistantSelection();
            auto dialog = new Dialog(this);
            dialog->showDialog(
                tr("Codex change not applied"),
                tr("The screenplay changed while Codex was working. Run the request again so the "
                   "new writing is inserted at the correct place."),
                { { 0, tr("OK"), Dialog::AcceptButton } });
            connect(dialog, &Dialog::finished, dialog, [dialog] { dialog->hideDialog(); });
            connect(dialog, &Dialog::disappeared, dialog, &Dialog::deleteLater);
            return;
        }
        QTextCursor insertionCursor(d->textEdit->document());
        insertionCursor.setPosition(d->aiEditInsertionPosition);
        d->textEdit->setTextCursor(insertionCursor);
    }

    const QLatin1String textWritingTaskKey("text-writing-task");
    TaskBar::addTask(textWritingTaskKey);
    TaskBar::setTaskTitle(textWritingTaskKey, tr("Applying Codex screenplay change"));
    d->textEdit->setCompleterActive(true);
    const auto beforeSnapshot = screenplayFountainSnapshot(d->model);
    const auto action = d->aiPendingAction;
    const auto instruction = d->aiRequestInstruction;
    const auto summary = d->aiPendingSummary;
    const auto impactSummary = d->aiPendingImpactSummary;
    const auto continuityChecks = d->aiPendingContinuityChecks;
    const auto applied = d->textEdit->insertFountainText(fountainText);
    if (applied) {
        recordAssistantEdit(d->model,
                            action.isEmpty()
                                ? (hasEditTarget ? QString("replace_selection")
                                                 : QString("insert_screenplay"))
                                : action,
                            instruction, summary, beforeSnapshot,
                            screenplayFountainSnapshot(d->model), "screenplay", impactSummary,
                            continuityChecks);
    }
    clearAssistantSelection();
    TaskBar::finishTask(textWritingTaskKey);
    d->aiAssistantView->appendAssistantMessage(
        applied ? (hasEditTarget
                       ? tr("Applied the approved replacement directly to the screenplay with "
                            "native formatting. You can restore it with Undo.")
                       : tr("Applied the approved writing directly to the screenplay with native "
                            "formatting. You can restore it with Undo."))
                : tr("STARC could not apply the proposed screenplay change, so nothing was edited."));
}

DictionariesView* ScreenplayTextView::dictionariesView() const
{
    return d->dictionariesView;
}

void ScreenplayTextView::setComplianceCheckResultAvailable(bool _available)
{
    //
    // Если недоступно, то форсируем скрытие панели с проверками
    //
    if (!_available) {
        d->showComplianceCheckResultAction->setChecked(false);
        d->updateSideBarVisibility(this);
    }

    d->showComplianceCheckResultAction->setVisible(_available);
}

ComplianceCheckResultView* ScreenplayTextView::complianceCheckResultView() const
{
    return d->complianceCheckResultView;
}

void ScreenplayTextView::reconfigure(const QStringList& _changedSettingsKeys)
{
    UiHelper::initSpellingFor(d->textEdit);

    auto contains = [_changedSettingsKeys](const QString& _key) {
        return _changedSettingsKeys.contains(_key);
    };
    auto toBool = [](const QString& _key) { return settingsValue(_key).toBool(); };

    using namespace DataStorageLayer;

    if (_changedSettingsKeys.isEmpty() || contains(kApplicationAiAssistantEnabledKey)) {
        d->toolbar->setAiAssistantVisible(
            settingsValue(kApplicationAiAssistantEnabledKey).toBool());
        d->updateToolbarUi();
    }

    if (_changedSettingsKeys.isEmpty() || contains(kComponentsScreenplayEditorDefaultTemplateKey)) {
        d->reconfigureTemplate();
    }

    if (_changedSettingsKeys.isEmpty()
        || contains(kComponentsScreenplayEditorShowSceneNumbersKey)) {
        d->reconfigureSceneNumbersVisibility();
    }
    if (_changedSettingsKeys.isEmpty()
        || contains(kComponentsScreenplayEditorShowDialogueNumbersKey)) {
        d->reconfigureDialoguesNumbersVisibility();
    }
    if (_changedSettingsKeys.isEmpty() || contains(kComponentsScreenplayEditorContinueDialogueKey)
        || contains(kComponentsScreenplayEditorCorrectTextOnPageBreaksKey)) {
        d->textEdit->setCorrectionOptions(
            toBool(kComponentsScreenplayEditorContinueDialogueKey),
            toBool(kComponentsScreenplayEditorCorrectTextOnPageBreaksKey));
    }
    if (_changedSettingsKeys.isEmpty()
        || contains(kComponentsScreenplayEditorShowCharacterSuggestionsInEmptyBlockKey)) {
        d->textEdit->setShowSuggestionsInEmptyBlocks(
            toBool(kComponentsScreenplayEditorShowCharacterSuggestionsInEmptyBlockKey));
    }
    if (_changedSettingsKeys.isEmpty() || contains(kComponentsScreenplayEditorShortcutsKey)) {
        d->shortcutsManager.reconfigure();
    }

    if (_changedSettingsKeys.isEmpty() || contains(kApplicationShowDocumentsPagesKey)) {
        const auto usePageMode = toBool(kApplicationShowDocumentsPagesKey);
        d->textEdit->setUsePageMode(usePageMode);
        if (usePageMode) {
            d->textEdit->reinit();
        } else {
            d->updateTextEditPageMargins();
        }
    }
    if (_changedSettingsKeys.isEmpty() || contains(kApplicationHighlightCurrentLineKey)) {
        d->textEdit->setHighlightCurrentLine(toBool(kApplicationHighlightCurrentLineKey));
    }
    if (_changedSettingsKeys.isEmpty() || contains(kApplicationFocusCurrentParagraphKey)) {
        d->textEdit->setFocusCurrentParagraph(toBool(kApplicationFocusCurrentParagraphKey));
    }
    if (_changedSettingsKeys.isEmpty() || contains(kApplicationUseTypewriterScrollingKey)) {
        d->textEdit->setUseTypewriterScrolling(toBool(kApplicationUseTypewriterScrollingKey));
    }
    if (_changedSettingsKeys.isEmpty() || contains(kApplicationCorrectDoubleCapitalsKey)) {
        d->textEdit->setCorrectDoubleCapitals(toBool(kApplicationCorrectDoubleCapitalsKey));
    }
    if (_changedSettingsKeys.isEmpty() || contains(kApplicationCapitalizeSingleILetterKey)) {
        d->textEdit->setCapitalizeSingleILetter(toBool(kApplicationCapitalizeSingleILetterKey));
    }
    if (_changedSettingsKeys.isEmpty() || contains(kApplicationReplaceThreeDotsWithEllipsisKey)) {
        d->textEdit->setReplaceThreeDots(toBool(kApplicationReplaceThreeDotsWithEllipsisKey));
    }
    if (_changedSettingsKeys.isEmpty() || contains(kApplicationSmartQuotesKey)) {
        d->textEdit->setUseSmartQuotes(toBool(kApplicationSmartQuotesKey));
    }
    if (_changedSettingsKeys.isEmpty() || contains(kApplicationReplaceTwoDashesWithEmDashKey)) {
        d->textEdit->setReplaceTwoDashes(toBool(kApplicationReplaceTwoDashesWithEmDashKey));
    }
    if (_changedSettingsKeys.isEmpty() || contains(kApplicationAvoidMultipleSpacesKey)) {
        d->textEdit->setAvoidMultipleSpaces(toBool(kApplicationAvoidMultipleSpacesKey));
    }

    if (_changedSettingsKeys.isEmpty() || contains(kComponentsScreenplayDurationUseEighthsKey)) {
        const auto useEighths = toBool(kComponentsScreenplayDurationUseEighthsKey);
        d->screenplayTextScrollbarManager->setScrollBarType(useEighths ? ScrollBarType::Pageline
                                                                       : ScrollBarType::Timeline);
        d->complianceCheckResultView->setUseEighths(useEighths);
    }
}

void ScreenplayTextView::loadViewSettings()
{
    using namespace DataStorageLayer;

    const auto scaleFactor = settingsValue(kScaleFactorKey, 1.0).toReal();
    d->scalableWrapper->setZoomRange(scaleFactor);

    const auto isItemIsolationEnabled = settingsValue(kIsItemIsolationEnabledKey, false).toBool();
    d->toolbar->setItemIsolationEnabled(isItemIsolationEnabled);
    const auto isCommentsModeEnabled = settingsValue(kIsCommentsModeEnabledKey, false).toBool();
    d->toolbar->setCommentsModeEnabled(isCommentsModeEnabled);
    const auto isAiAssistantEnabled = settingsValue(kIsAiAssistantEnabledKey, false).toBool();
    d->toolbar->setAiAssistantEnabled(isAiAssistantEnabled);
    const auto isFastFormatPanelVisible
        = settingsValue(kIsFastFormatPanelVisibleKey, false).toBool();
    d->toolbar->setFastFormatPanelVisible(isFastFormatPanelVisible);
    const auto isBeatsVisible = settingsValue(kIsBeatsVisibleKey, false).toBool();
    d->toolbar->setBeatsVisible(isBeatsVisible);
    const auto isSceneParametersVisible
        = settingsValue(kIsSceneParametersVisibleKey, false).toBool();
    d->showSceneParametersAction->setChecked(isSceneParametersVisible);
    const auto isBookmarksListVisible = settingsValue(kIsBookmarksListVisibleKey, false).toBool();
    d->showBookmarksAction->setChecked(isBookmarksListVisible);
    const auto isDictionariesVisible = settingsValue(kIsDictionariesVisibleKey, false).toBool();
    d->showDictionariesAction->setChecked(isDictionariesVisible);
    const auto isComplianceCheckResultVisible
        = settingsValue(kIsComplianceCheckResultVisibleKey, false).toBool();
    d->showComplianceCheckResultAction->setChecked(isComplianceCheckResultVisible);
    const auto sidebarPanelIndex = settingsValue(kSidebarPanelIndexKey, 0).toInt();
    d->sidebarTabs->setCurrentTab(sidebarPanelIndex);

    const auto sidebarState = settingsValue(kSidebarStateKey);
    if (sidebarState.isValid()) {
        d->splitter->restoreState(sidebarState.toByteArray());
    }
}

void ScreenplayTextView::saveViewSettings()
{
    setSettingsValue(kScaleFactorKey, d->scalableWrapper->zoomRange());

    setSettingsValue(kIsFastFormatPanelVisibleKey, d->toolbar->isFastFormatPanelVisible());
    setSettingsValue(kIsBeatsVisibleKey, d->toolbar->isBeatsVisible());
    setSettingsValue(kIsCommentsModeEnabledKey, d->toolbar->isCommentsModeEnabled());
    setSettingsValue(kIsAiAssistantEnabledKey, d->toolbar->isAiAssistantEnabled());
    setSettingsValue(kIsItemIsolationEnabledKey, d->toolbar->isItemIsolationEnabled());
    setSettingsValue(kIsSceneParametersVisibleKey, d->showSceneParametersAction->isChecked());
    setSettingsValue(kIsBookmarksListVisibleKey, d->showBookmarksAction->isChecked());
    setSettingsValue(kIsDictionariesVisibleKey, d->showDictionariesAction->isChecked());
    setSettingsValue(kIsComplianceCheckResultVisibleKey,
                     d->showComplianceCheckResultAction->isChecked());
    setSettingsValue(kSidebarPanelIndexKey, d->sidebarTabs->currentTab());

    setSettingsValue(kSidebarStateKey, d->splitter->saveState());
}

void ScreenplayTextView::setModel(BusinessLayer::ScreenplayTextModel* _model)
{
    Log::debug("[ScreenplayTextView] Set model");

    if (d->model) {
        Log::trace("[ScreenplayTextView] Disconnect previous model");
        if (d->model->synopsisModel()) {
            d->model->synopsisModel()->disconnect(this);
        }
        if (d->model->charactersModel()) {
            d->model->charactersModel()->disconnect(this);
        }
        if (d->model->locationsModel()) {
            d->model->locationsModel()->disconnect(this);
        }
        d->model->disconnect(this);
        if (d->model->informationModel()) {
            d->model->informationModel()->disconnect(this);
        }
    }

    d->model = _model;
    d->storySourceRevision = 0;
    d->aiAssistantView->setConversationStorageKey(
        d->model != nullptr && d->model->document() != nullptr
            ? d->model->document()->uuid().toString(QUuid::WithoutBraces)
            : QString());

    //
    // Отслеживаем изменения некоторых параметров
    //
    if (d->model && d->model->informationModel()) {
        Log::trace("[ScreenplayTextView] Reconfigure template");
        const bool reinitModel = true;
        d->reconfigureTemplate(!reinitModel);
        Log::trace("[ScreenplayTextView] Reconfigure scene numbers visibility");
        d->reconfigureSceneNumbersVisibility();
        Log::trace("[ScreenplayTextView] Reconfigure dialogue numbers visibility");
        d->reconfigureDialoguesNumbersVisibility();

        Log::trace("[ScreenplayTextView] Connect model signals");
        connect(d->model, &BusinessLayer::ScreenplayTextModel::dataChanged, this,
                [this](const QModelIndex& _topLeft) {
                    auto updatedItem = d->model->itemForIndex(_topLeft);
                    if (updatedItem != d->lastSelectedItem) {
                        return;
                    }

                    d->showParametersFor(updatedItem);
                });

        //
        // Обновляем стоимость генерации при изменении модели
        //
        auto updateGenerationPrice = [this] {
            d->aiAssistantView->setTranslationDocumentOption(
                tr("Document translation will take %n word(s)", 0, d->model->wordsCount()));
            d->aiAssistantView->setGenerationSynopsisOptions(
                tr("Synopsis generation will take %n word(s)", 0, d->model->wordsCount()));
            d->aiAssistantView->setGenerationNovelOptions(
                tr("Novel generation will take %n word(s)", 0, d->model->wordsCount()));
        };
        connect(d->model, &BusinessLayer::ScreenplayTextModel::modelReset, this,
                updateGenerationPrice);
        connect(d->model, &BusinessLayer::ScreenplayTextModel::dataChanged, this,
                updateGenerationPrice);
        connect(d->model, &BusinessLayer::ScreenplayTextModel::rowsInserted, this,
                updateGenerationPrice);
        connect(d->model, &BusinessLayer::ScreenplayTextModel::rowsMoved, this,
                updateGenerationPrice);
        connect(d->model, &BusinessLayer::ScreenplayTextModel::rowsRemoved, this,
                updateGenerationPrice);

        auto storySourceChanged = [this] {
            ++d->storySourceRevision;
            markAssistantStoryMemoryStale(d->model);
        };
        auto trackStoryModel = [this, storySourceChanged](QAbstractItemModel* _source) {
            if (_source == nullptr) {
                return;
            }
            connect(_source, &QAbstractItemModel::dataChanged, this,
                    [storySourceChanged] { storySourceChanged(); });
            connect(_source, &QAbstractItemModel::modelReset, this, storySourceChanged);
            connect(_source, &QAbstractItemModel::rowsInserted, this,
                    [storySourceChanged] { storySourceChanged(); });
            connect(_source, &QAbstractItemModel::rowsMoved, this,
                    [storySourceChanged] { storySourceChanged(); });
            connect(_source, &QAbstractItemModel::rowsRemoved, this,
                    [storySourceChanged] { storySourceChanged(); });
        };
        trackStoryModel(d->model);
        trackStoryModel(d->model->synopsisModel());
        trackStoryModel(d->model->charactersModel());
        trackStoryModel(d->model->locationsModel());
        auto trackCharacter = [this, storySourceChanged](BusinessLayer::CharacterModel* _character) {
            if (_character == nullptr) return;
            connect(_character, &BusinessLayer::CharacterModel::contentsChanged, this,
                    [storySourceChanged] { storySourceChanged(); });
        };
        if (d->model->charactersModel() != nullptr) {
            for (int row = 0; row < d->model->charactersModel()->rowCount(); ++row) {
                trackCharacter(d->model->charactersModel()->character(row));
            }
            connect(d->model->charactersModel(), &QAbstractItemModel::rowsInserted, this,
                    [this, trackCharacter](const QModelIndex&, int _first, int _last) {
                if (d->model == nullptr || d->model->charactersModel() == nullptr) return;
                for (int row = _first; row <= _last; ++row) {
                    trackCharacter(d->model->charactersModel()->character(row));
                }
            });
        }
        connect(d->model->informationModel(),
                &BusinessLayer::ScreenplayInformationModel::nameChanged, this,
                [storySourceChanged] { storySourceChanged(); });
        connect(d->model->informationModel(),
                &BusinessLayer::ScreenplayInformationModel::taglineChanged, this,
                [storySourceChanged] { storySourceChanged(); });
        connect(d->model->informationModel(),
                &BusinessLayer::ScreenplayInformationModel::loglineChanged, this,
                [storySourceChanged] { storySourceChanged(); });
        connect(d->model->informationModel(),
                &BusinessLayer::ScreenplayInformationModel::storyLinesChanged, this,
                [storySourceChanged] { storySourceChanged(); });

        //
        // Перед началом сброса документа запоминаем текущую позицию курсора
        //
        connect(d->model, &BusinessLayer::ScreenplayTextModel::modelAboutToBeReset, this, [this] {
            if (!d->pendingCursorPosition.has_value()) {
                d->pendingCursorPosition = cursorPosition();
            }
        });
        //
        // ... после завершения сброса, отложенно возвращаем курсор на место
        //
        connect(
            d->model, &BusinessLayer::ScreenplayTextModel::modelReset, this,
            [this] {
                if (!d->pendingCursorPosition.has_value()) {
                    return;
                }

                //
                // Извлечём позицию для установки
                //
                const int position = d->pendingCursorPosition.value();
                //
                // ... затем сбрасываем буфер, чтобы позиция установилась внутрь редактора текста
                //
                d->pendingCursorPosition.reset();
                //
                // ... устанавливаем позицию в редактор
                //
                setCursorPosition(position);
            },
            Qt::QueuedConnection);
    }

    Log::trace("[ScreenplayTextView] Reset collaborators cursors");
    d->textEdit->setCollaboratorsCursors({});
    Log::trace("[ScreenplayTextView] Initialize text editor model");
    d->textEdit->initWithModel(d->model);
    d->writersRoomIdleTimer.stop();
    d->writersRoomChangeEvents = 0;
    d->writersRoomBaselineTextLength = d->textEdit->document()->characterCount();
    d->writersRoomCooldown.invalidate();
    Log::trace("[ScreenplayTextView] Initialize scrollbar model");
    d->screenplayTextScrollbarManager->setModel(d->model);
    Log::trace("[ScreenplayTextView] Initialize comments model");
    d->commentsModel->setTextModel(d->model);
    Log::trace("[ScreenplayTextView] Initialize bookmarks model");
    d->bookmarksModel->setTextModel(d->model);

    Log::trace("[ScreenplayTextView] Update toolbar paragraph type");
    d->updateToolBarCurrentParagraphTypeName();

    Log::debug("[ScreenplayTextView] Model set");
}

QModelIndex ScreenplayTextView::currentModelIndex() const
{
    return d->textEdit->currentModelIndex();
}

int ScreenplayTextView::cursorPosition() const
{
    return d->textEdit->textCursor().position();
}

QString ScreenplayTextView::selectedTextForAssistant() const
{
    auto text = d->textEdit->textCursor().selectedText();
    text.replace(QChar::ParagraphSeparator, '\n');
    return text.trimmed();
}

void ScreenplayTextView::captureAssistantRequestContext()
{
    const auto cursor = d->textEdit->textCursor();
    d->aiRequestCursorPosition = cursor.position();
    d->aiEditDocumentRevision = d->textEdit->document()->revision();
    d->aiEditApplyConfirmed = false;
    d->aiActionProtocolPending = true;
    d->aiEditInsertionPosition = -1;
    d->aiRequestLogline
        = d->model != nullptr && d->model->informationModel() != nullptr
        ? d->model->informationModel()->logline()
        : QString();
    d->aiRequestSynopsis
        = d->model != nullptr ? simpleTextSnapshot(d->model->synopsisModel()) : QString();
    const auto treatmentAtRequest = treatmentParagraphs(d->model);
    d->aiRequestTreatment = treatmentAtRequest.join('\n');
    d->aiRequestTreatmentParagraphCount = treatmentAtRequest.size();
    d->aiRequestStorySourceRevision = d->storySourceRevision;
    d->aiRequestStoryMemory
        = loadAssistantStoryMemory(d->model).value("content").toString();
    if (cursor.hasSelection()) {
        d->aiEditSelectionStart = cursor.selectionStart();
        d->aiEditSelectionEnd = cursor.selectionEnd();
        d->aiEditSelectionText = cursor.selectedText();
    } else {
        d->aiEditSelectionStart = -1;
        d->aiEditSelectionEnd = -1;
        d->aiEditSelectionText.clear();
    }
}

void ScreenplayTextView::captureAssistantSelection()
{
    const auto cursor = d->textEdit->textCursor();
    if (!cursor.hasSelection()) {
        d->aiEditSelectionStart = -1;
        d->aiEditSelectionEnd = -1;
        d->aiEditSelectionText.clear();
        return;
    }

    d->aiEditSelectionStart = cursor.selectionStart();
    d->aiEditSelectionEnd = cursor.selectionEnd();
    d->aiEditSelectionText = cursor.selectedText();
    d->aiEditInsertionPosition = -1;
    d->aiRequestCursorPosition = cursor.position();
    d->aiEditDocumentRevision = d->textEdit->document()->revision();
    d->aiEditApplyConfirmed = false;
    d->aiActionProtocolPending = false;
}

void ScreenplayTextView::captureAssistantInsertionPoint(bool _atBeginning, bool _atEnd)
{
    auto cursor = d->textEdit->textCursor();
    if (_atBeginning) {
        cursor.movePosition(QTextCursor::Start);
    } else if (_atEnd) {
        cursor.movePosition(QTextCursor::End);
    }
    d->aiEditSelectionStart = -1;
    d->aiEditSelectionEnd = -1;
    d->aiEditSelectionText.clear();
    d->aiEditInsertionPosition = cursor.position();
    d->aiRequestCursorPosition = cursor.position();
    d->aiEditDocumentRevision = d->textEdit->document()->revision();
    d->aiEditApplyConfirmed = false;
    d->aiActionProtocolPending = false;
}

void ScreenplayTextView::clearAssistantSelection()
{
    d->aiEditSelectionStart = -1;
    d->aiEditSelectionEnd = -1;
    d->aiEditSelectionText.clear();
    d->aiEditInsertionPosition = -1;
    d->aiRequestCursorPosition = -1;
    d->aiEditDocumentRevision = -1;
    d->aiEditApplyConfirmed = false;
    d->aiActionProtocolPending = false;
    d->aiPendingAction.clear();
    d->aiPendingTarget.clear();
    d->aiPendingSummary.clear();
    d->aiPendingImpactSummary.clear();
    d->aiPendingContinuityChecks = {};
    d->aiRequestLogline.clear();
    d->aiRequestSynopsis.clear();
    d->aiRequestTreatment.clear();
    d->aiRequestTreatmentParagraphCount = 0;
    d->aiRequestStorySourceRevision = 0;
    d->aiRequestStoryMemory.clear();
}

void ScreenplayTextView::showAssistantNotice(const QString& _message)
{
    auto dialog = new Dialog(this);
    dialog->showDialog(tr("Codex needs an edit target"), _message,
                       { { 0, tr("OK"), Dialog::AcceptButton } });
    connect(dialog, &Dialog::finished, dialog, [dialog] { dialog->hideDialog(); });
    connect(dialog, &Dialog::disappeared, dialog, &Dialog::deleteLater);
}

void ScreenplayTextView::handleCharacterMergeRollbackFinished(
    const QString& _transactionId, bool _success, const QString& _message)
{
    if (_transactionId.isEmpty() || _transactionId != d->pendingCharacterMergeRollbackId) {
        return;
    }
    d->pendingCharacterMergeRollbackId.clear();
    if (_success) {
        markAssistantStoryMemoryStale(d->model);
    }
    d->aiAssistantView->appendAssistantMessage(
        !_message.trimmed().isEmpty()
            ? _message
            : (_success ? tr("The character merge was rolled back completely.")
                        : tr("STARC could not roll back that character merge safely.")));
}

void ScreenplayTextView::requestAssistantClearScreenplay()
{
    if (d->model == nullptr || d->textEdit->document()->isEmpty()) {
        clearAssistantSelection();
        d->aiAssistantView->appendAssistantMessage(
            tr("The screenplay is already empty, so no edit was needed."));
        return;
    }

    const int wordsToRemove = d->model->wordsCount();
    const auto beforeSnapshot = screenplayFountainSnapshot(d->model);
    const auto instruction = d->aiRequestInstruction;
    const auto summary = d->aiPendingSummary;
    const auto impactSummaryForChange = d->aiPendingImpactSummary;
    const auto continuityChecksForChange = d->aiPendingContinuityChecks;
    const auto gateReport
        = continuityGateReport(impactSummaryForChange, continuityChecksForChange);
    auto dialog = new Dialog(this);
    dialog->setDismissOnOutsideClick(false);
    dialog->setRejectOnEscape(false);
    dialog->showDialog(
        tr("Clear the entire screenplay?"),
        tr("Codex will remove all %n word(s) from the screenplay as one undoable editor action. "
           "Synopsis, treatment, characters, locations, and project metadata will not be "
           "removed.\n\n%1",
           nullptr, wordsToRemove)
            .arg(gateReport),
        { { 0, tr("Cancel"), Dialog::RejectButton },
          { 1, tr("Clear screenplay"), Dialog::AcceptButton } });
    connect(dialog, &Dialog::finished, this,
            [this, dialog, wordsToRemove, beforeSnapshot, instruction,
             summary, impactSummaryForChange,
             continuityChecksForChange](const Dialog::ButtonInfo& _button) {
                dialog->hideDialog();
                if (_button.type != Dialog::AcceptButton) {
                    clearAssistantSelection();
                    d->aiAssistantView->appendAssistantMessage(
                        tr("I left the screenplay unchanged."));
                    return;
                }

                auto applyClear = [this, wordsToRemove, beforeSnapshot, instruction, summary,
                                   impactSummaryForChange, continuityChecksForChange] {
                    auto cursor = d->textEdit->textCursor();
                    cursor.beginEditBlock();
                    cursor.select(QTextCursor::Document);
                    cursor.removeSelectedText();
                    cursor.endEditBlock();
                    d->textEdit->setTextCursor(cursor);
                    recordAssistantEdit(d->model, "clear_screenplay", instruction, summary,
                                        beforeSnapshot, screenplayFountainSnapshot(d->model),
                                        "screenplay", impactSummaryForChange,
                                        continuityChecksForChange);
                    clearAssistantSelection();
                    d->aiAssistantView->appendAssistantMessage(
                        tr("Removed all %n screenplay word(s). This was one editor action, so you "
                           "can restore it with Undo.",
                           nullptr, wordsToRemove));
                };
                applyAfterCriticalContinuityConfirmation(
                    this, continuityChecksForChange, applyClear, [this] {
                        clearAssistantSelection();
                        d->aiAssistantView->appendAssistantMessage(
                            tr("The full-screenplay deletion with a confirmed-canon conflict was "
                               "not applied."));
                    });
            });
    connect(dialog, &Dialog::disappeared, dialog, &Dialog::deleteLater);
}

void ScreenplayTextView::showAssistantEditHistory(int _entryOffset)
{
    const auto entries = loadAssistantEditHistory(d->model);
    if (entries.isEmpty()) {
        d->aiAssistantView->appendAssistantMessage(
            tr("No approved Codex screenplay edits have been recorded for this screenplay yet."));
        return;
    }

    const int entryOffset = qBound(0, _entryOffset, entries.size() - 1);
    const auto entry = entries.at(entries.size() - entryOffset - 1).toObject();
    auto timestamp
        = QDateTime::fromString(entry.value("timestamp").toString(), Qt::ISODateWithMs);
    if (timestamp.isValid()) {
        timestamp = timestamp.toLocalTime();
    }
    const auto method = entry.value("storyMethod").toString() == "eric-edson-story-skill"
        ? tr("Eric Edson method")
        : tr("Story continuity");
    const auto instruction = entry.value("instruction").toString().trimmed();
    const auto summary = entry.value("summary").toString().trimmed();
    const auto before = entry.value("before").toString();
    const auto after = entry.value("after").toString();
    const auto historyImpact = entry.value("impactSummary").toString();
    const auto historyContinuityChecks = entry.value("continuityChecks").toArray();
    const auto historyGate = historyImpact.isEmpty() && historyContinuityChecks.isEmpty()
        ? QString()
        : QString("\n\n%1").arg(continuityGateReport(historyImpact, historyContinuityChecks));
    const auto historyTarget = entry.value("target").toString("screenplay");
    const auto historyAction = entry.value("action").toString();
    const bool isCharacterTarget = historyTarget.startsWith("character:");
    const bool isRelationshipTarget = historyTarget.startsWith("relationship:");
    const bool isMergeTarget = historyTarget.startsWith("merge:");
    const auto historyTargetParts = historyTarget.split(':');
    const auto mergeTransactionId
        = isMergeTarget && historyTargetParts.size() == 4 ? historyTargetParts.at(3) : QString();
    const auto mergeTransactionState = characterMergeTransaction(mergeTransactionId);
    const bool mergeCanRollback
        = isMergeTarget && mergeTransactionState.value("status").toString() == "committed";
    const auto targetLabel = historyTarget == "logline"
        ? tr("logline")
        : historyTarget == "synopsis"
        ? tr("synopsis")
        : historyTarget == "treatment"
        ? tr("treatment outline")
        : isCharacterTarget ? tr("character profile")
        : isRelationshipTarget ? tr("character relationship")
        : isMergeTarget ? tr("character merge") : tr("screenplay");
    const auto details
        = tr("Edit %1 of %2 (newest first)\n%3\n%4\n\nInstruction\n%5\n\nSummary\n%6\n\n"
             "CHANGE\n%7%8")
              .arg(entryOffset + 1)
              .arg(entries.size())
              .arg(timestamp.isValid() ? timestamp.toString("yyyy-MM-dd h:mm AP")
                                       : tr("Unknown date"),
                   method,
                   instruction.isEmpty() ? tr("No instruction recorded") : instruction,
                   summary.isEmpty() ? readableActionName(entry.value("action").toString())
                                     : summary,
                   focusedScreenplayLineDiff(before, after), historyGate);

    QVector<Dialog::ButtonInfo> buttons{ { 0, tr("Close"), Dialog::RejectButton } };
    if (entryOffset + 1 < entries.size()) {
        buttons.append({ 1, tr("Older"), Dialog::NormalButton });
    }
    if (entryOffset > 0) {
        buttons.append({ 2, tr("Newer"), Dialog::NormalButton });
    }
    if (historyAction != "create_character" && historyAction != "remove_character"
        && (historyAction != "merge_character" || mergeCanRollback)) {
        buttons.append({ 3, tr("Restore earlier %1").arg(targetLabel),
                         Dialog::AcceptCriticalButton });
    }

    auto dialog = new Dialog(this);
    dialog->setContentFixedWidth(Ui::DesignSystem::dialog().maximumWidth());
    const auto availableHeight
        = qMax(240, height() - static_cast<int>(Ui::DesignSystem::layout().px48() * 2));
    dialog->setContentFixedHeight(qMin(720, availableHeight));
    dialog->enableSupportingTextScrolling();
    dialog->enableDiffHighlighting();
    dialog->setDismissOnOutsideClick(false);
    dialog->setRejectOnEscape(true);
    dialog->showDialog(tr("Codex project edit history"), details, buttons, false);
    connect(dialog, &Dialog::finished, this,
            [this, dialog, entryOffset, before, instruction, historyTarget,
             targetLabel](const Dialog::ButtonInfo& _button) {
        dialog->hideDialog();
        if (_button.id == 1) {
            showAssistantEditHistory(entryOffset + 1);
            return;
        }
        if (_button.id == 2) {
            showAssistantEditHistory(entryOffset - 1);
            return;
        }
        if (_button.id != 3) {
            return;
        }

        auto confirmation = new Dialog(this);
        confirmation->setDismissOnOutsideClick(false);
        confirmation->setRejectOnEscape(false);
        confirmation->showDialog(
            tr("Restore this earlier %1?").arg(targetLabel),
            historyTarget.startsWith("merge:")
                ? tr("This rolls back the complete character merge transaction: the duplicate "
                     "profile returns from Recycle Bin, the survivor and relationships return to "
                     "their pre-merge state, and every affected script document restores its "
                     "pre-merge cues. Changes made to those documents after the merge will be "
                     "replaced.")
                : historyTarget == "screenplay"
                ? tr("This restores the entire screenplay to its state before the selected Codex "
                     "edit. Writing added afterward will be replaced. The restoration is also "
                     "recorded in edit history.")
                : tr("This restores the %1 to its state before the selected Codex edit. Newer "
                     "changes to that document will be replaced. The restoration is also "
                     "recorded in edit history.")
                      .arg(targetLabel),
            { { 0, tr("Cancel"), Dialog::RejectButton },
              { 1, tr("Restore %1").arg(targetLabel), Dialog::AcceptCriticalButton } });
        connect(confirmation, &Dialog::finished, this,
                [this, confirmation, before, instruction, historyTarget,
                 targetLabel](const Dialog::ButtonInfo& _choice) {
            confirmation->hideDialog();
            if (_choice.type != Dialog::AcceptCriticalButton) {
                return;
            }
            const bool isCharacterTarget = historyTarget.startsWith("character:");
            const bool isRelationshipTarget = historyTarget.startsWith("relationship:");
            const bool isMergeTarget = historyTarget.startsWith("merge:");
            const auto targetParts = historyTarget.split(':');
            if (isMergeTarget) {
                if (targetParts.size() != 4) {
                    d->aiAssistantView->appendAssistantMessage(
                        tr("This older merge was recorded before transaction-safe rollback was "
                           "available."));
                    return;
                }
                d->pendingCharacterMergeRollbackId = targetParts.at(3);
                emit characterMergeRollbackRequested(d->pendingCharacterMergeRollbackId);
                return;
            }
            auto characters = d->model != nullptr ? d->model->charactersModel() : nullptr;
            auto historyCharacter = characters != nullptr && isCharacterTarget
                && targetParts.size() == 2
                ? characters->character(QUuid(targetParts.at(1)))
                : characters != nullptr && isRelationshipTarget && targetParts.size() == 3
                ? characters->character(QUuid(targetParts.at(1))) : nullptr;
            const auto current = historyTarget == "logline"
                ? (d->model != nullptr && d->model->informationModel() != nullptr
                       ? d->model->informationModel()->logline()
                       : QString())
                : historyTarget == "synopsis"
                ? simpleTextSnapshot(d->model != nullptr ? d->model->synopsisModel() : nullptr)
                : historyTarget == "treatment" ? treatmentSnapshot(d->model)
                : isCharacterTarget ? characterSnapshot(historyCharacter)
                : isRelationshipTarget && targetParts.size() == 3
                ? relationshipSnapshot(historyCharacter, QUuid(targetParts.at(2)))
                                                 : screenplayFountainSnapshot(d->model);
            if (current == before) {
                d->aiAssistantView->appendAssistantMessage(
                    tr("The %1 is already at that saved version.").arg(targetLabel));
                return;
            }

            bool restored = false;
            if (historyTarget == "logline" && d->model != nullptr
                && d->model->informationModel() != nullptr) {
                d->model->informationModel()->setLogline(before);
                restored = d->model->informationModel()->logline() == before;
            } else if (historyTarget == "synopsis" && d->model != nullptr) {
                restored = replaceSimpleText(d->model->synopsisModel(), before);
            } else if (historyTarget == "treatment") {
                restored = replaceTreatmentParagraphs(d->model, before);
            } else if (isCharacterTarget) {
                restored = restoreCharacterSnapshot(characters, before);
            } else if (isRelationshipTarget) {
                restored = restoreRelationshipSnapshot(characters, before);
            } else {
                auto cursor = d->textEdit->textCursor();
                cursor.select(QTextCursor::Document);
                d->textEdit->setTextCursor(cursor);
                if (before.isEmpty()) {
                    cursor.beginEditBlock();
                    cursor.removeSelectedText();
                    cursor.endEditBlock();
                    d->textEdit->setTextCursor(cursor);
                    restored = true;
                } else {
                    restored = d->textEdit->insertFountainText(before);
                }
            }
            if (!restored) {
                d->aiAssistantView->appendAssistantMessage(
                    tr("STARC could not safely restore that saved %1 version.").arg(targetLabel));
                return;
            }
            const auto restoredSnapshot = historyTarget == "logline"
                ? d->model->informationModel()->logline()
                : historyTarget == "synopsis" ? simpleTextSnapshot(d->model->synopsisModel())
                : historyTarget == "treatment" ? treatmentSnapshot(d->model)
                : isCharacterTarget ? characterSnapshot(historyCharacter)
                : isRelationshipTarget && targetParts.size() == 3
                ? relationshipSnapshot(historyCharacter, QUuid(targetParts.at(2)))
                                                 : screenplayFountainSnapshot(d->model);
            recordAssistantEdit(
                d->model, "restore", tr("Restore before: %1").arg(instruction),
                tr("Restored an earlier %1 version from Codex edit history.").arg(targetLabel),
                current, restoredSnapshot, historyTarget);
            if (isCharacterTarget || isRelationshipTarget) {
                markAssistantStoryMemoryStale(d->model);
            }
            d->aiAssistantView->appendAssistantMessage(
                tr("Restored the saved %1 version.").arg(targetLabel));
        });
        connect(confirmation, &Dialog::disappeared, confirmation, &Dialog::deleteLater);
    });
    connect(dialog, &Dialog::disappeared, dialog, &Dialog::deleteLater);
}

void ScreenplayTextView::showAssistantStoryMemory()
{
    const auto memory = loadAssistantStoryMemory(d->model);
    const auto content = memory.value("content").toString().trimmed();
    auto updatedAt
        = QDateTime::fromString(memory.value("updatedAt").toString(), Qt::ISODateWithMs);
    if (updatedAt.isValid()) {
        updatedAt = updatedAt.toLocalTime();
    }
    const auto status = content.isEmpty()
        ? tr("NOT BUILT")
        : memory.value("stale").toBool() ? tr("STALE — the story changed after this analysis")
                                          : tr("CURRENT");
    const auto source = memory.value("writerEdited").toBool()
        ? tr("Writer-corrected")
        : tr("Codex-derived; live STARC data remains authoritative");
    const auto details
        = tr("Status: %1\nSource: %2\nLast updated: %3\n\n%4")
              .arg(status, source,
                   updatedAt.isValid() ? updatedAt.toString("yyyy-MM-dd h:mm AP") : tr("Never"),
                   content.isEmpty()
                       ? tr("Story Memory has not been built yet. Refresh it with Codex to map "
                            "characters, knowledge, chronology, plot threads, setups and payoffs, "
                            "world rules, voice, and continuity risks.")
                       : content);

    QVector<Dialog::ButtonInfo> buttons{
        { 0, tr("Close"), Dialog::RejectButton },
        { 1, content.isEmpty() ? tr("Build with Codex") : tr("Refresh with Codex"),
          Dialog::AcceptButton },
        { 2, content.isEmpty() ? tr("Write manually") : tr("Edit / correct"),
          Dialog::NormalButton },
    };
    auto dialog = new Dialog(this);
    dialog->setContentFixedWidth(Ui::DesignSystem::dialog().maximumWidth());
    const auto availableHeight
        = qMax(240, height() - static_cast<int>(Ui::DesignSystem::layout().px48() * 2));
    dialog->setContentFixedHeight(qMin(720, availableHeight));
    dialog->enableSupportingTextScrolling();
    dialog->setDismissOnOutsideClick(false);
    dialog->showDialog(tr("Story Memory"), details, buttons, false);
    connect(dialog, &Dialog::finished, this,
            [this, dialog, content](const Dialog::ButtonInfo& _button) {
        dialog->hideDialog();
        if (_button.id == 1) {
            requestAssistantStoryMemoryRefresh();
            return;
        }
        if (_button.id != 2) {
            return;
        }

        const auto templateText = QString(
            "CHARACTERS & RELATIONSHIPS\n\nCHARACTER KNOWLEDGE\n\nTIMELINE\n\n"
            "PLOT THREADS\n\nSETUPS & PAYOFFS\n\nWORLD RULES\n\nVOICE & STYLE\n\n"
            "CONTINUITY RISKS");
        bool accepted = false;
        const auto corrected = QInputDialog::getMultiLineText(
                                   this, tr("Edit Story Memory"),
                                   tr("Correct the working memory below. Live STARC tabs remain "
                                      "the source of truth."),
                                   content.isEmpty() ? templateText : content, &accepted)
                                   .trimmed();
        if (!accepted || corrected.isEmpty()) {
            return;
        }
        const QStringList requiredSections{
            "CHARACTERS & RELATIONSHIPS", "CHARACTER KNOWLEDGE", "TIMELINE",
            "PLOT THREADS", "SETUPS & PAYOFFS", "WORLD RULES", "VOICE & STYLE",
            "CONTINUITY RISKS",
        };
        const bool hasRequiredSections
            = std::all_of(requiredSections.cbegin(), requiredSections.cend(),
                          [&corrected](const QString& _section) {
                return corrected.contains(_section, Qt::CaseInsensitive);
            });
        if (!hasRequiredSections) {
            d->aiAssistantView->appendAssistantMessage(
                tr("Story Memory was not saved because one or more required continuity sections "
                   "were removed."));
            return;
        }
        saveAssistantStoryMemory(d->model, corrected, false, true);
        d->aiAssistantView->appendAssistantMessage(
            tr("Your Story Memory corrections were saved and will guide future requests."));
    });
    connect(dialog, &Dialog::disappeared, dialog, &Dialog::deleteLater);
}

void ScreenplayTextView::requestAssistantStoryMemoryRefresh()
{
    if (d->aiAssistantInProgress) {
        d->aiAssistantView->appendAssistantMessage(
            tr("Wait for the current Codex request to finish before refreshing Story Memory."));
        return;
    }
    d->aiRequestInstruction = tr("Refresh structured Story Memory from the current draft");
    captureAssistantRequestContext();
    emit generateTextRequested(
        QString(
            "Build or refresh Story Memory from the live STORY PACKAGE CANON and approved "
            "screenplay. Return update_story_memory targeting story_memory. Cover exactly these "
            "sections: CHARACTERS & RELATIONSHIPS, CHARACTER KNOWLEDGE, TIMELINE, PLOT THREADS, "
            "SETUPS & PAYOFFS, WORLD RULES, VOICE & STYLE, CONTINUITY RISKS. Under each section, "
            "separate confirmed canon from reasonable inference, cite scene headings or linked "
            "STARC tabs as evidence, preserve the writer's voice, and explicitly label unknowns. "
            "Do not propose or apply screenplay edits."),
        {});
}

void ScreenplayTextView::setCursorPosition(int _position)
{
    if (d->pendingCursorPosition.has_value()) {
        d->pendingCursorPosition = _position;
        return;
    }

    auto cursor = d->textEdit->textCursor();
    cursor.setPosition(_position);
    d->textEdit->ensureCursorVisible(cursor, false);
}

int ScreenplayTextView::verticalScroll() const
{
    return d->textEdit->verticalScroll();
}

void ScreenplayTextView::setVerticalScroll(int _value)
{
    d->textEdit->setVerticalScroll(_value);
}

bool ScreenplayTextView::eventFilter(QObject* _target, QEvent* _event)
{
    if (_target == d->scalableWrapper) {
        if (_event->type() == QEvent::Resize) {
            QTimer::singleShot(0, this, [this] {
                d->updateToolbarPositon();
                d->updateCommentsToolbar();
            });
        } else if (_event->type() == QEvent::KeyPress && d->searchManager->toolbar()->isVisible()
                   && d->scalableWrapper->hasFocus()) {
            auto keyEvent = static_cast<QKeyEvent*>(_event);
            if (keyEvent->key() == Qt::Key_Escape) {
                d->toolbarAnimation->switchToolbarsBack();
            }
        }
    }

    return Widget::eventFilter(_target, _event);
}

void ScreenplayTextView::resizeEvent(QResizeEvent* _event)
{
    Widget::resizeEvent(_event);

    d->updateToolbarPositon();
    d->updateCommentsToolbar();
}

void ScreenplayTextView::updateTranslations()
{
    d->sidebarTabs->setTabName(kFastFormatTabIndex, tr("Formatting"));
    d->sidebarTabs->setTabName(kSceneParametersTabIndex, tr("Scene parameters"));
    d->sidebarTabs->setTabName(kCommentsTabIndex, tr("Comments"));
    d->sidebarTabs->setTabName(kAiAssistantTabIndex, tr("AI assistant"));
    d->sidebarTabs->setTabName(kBookmarksTabIndex, tr("Bookmarks"));
    d->sidebarTabs->setTabName(kDictionariesTabIndex, tr("Dictionaries"));
    d->sidebarTabs->setTabName(kComplianceCheckResultTabIndex, tr("Checklist"));

    d->aiAssistantView->setGenerationPromptHint(
        tr("Ask Codex to write screenplay text, or create storyboard artifacts from the complete "
           "screenplay. Try \"Create a nine-beat storyboard for this screenplay\"."));

    d->updateOptionsTranslations();

    //
    // Обновить список форматов в выпадающем меню
    //
    const auto withModelReinitialization = false;
    d->reconfigureTemplate(withModelReinitialization);
    //
    // ... и текст текущего формата
    //
    d->currentParagraphType = BusinessLayer::TextParagraphType::Undefined;
    d->updateToolBarCurrentParagraphTypeName();

    d->searchManager->setSearchInBlockTypes(
        { { tr("In the whole text"), BusinessLayer::TextParagraphType::Undefined },
          { tr("In scene heading"), BusinessLayer::TextParagraphType::SceneHeading },
          { tr("In cast list"), BusinessLayer::TextParagraphType::SceneCharacters },
          { tr("In action"), BusinessLayer::TextParagraphType::Action },
          { tr("In character"), BusinessLayer::TextParagraphType::Character },
          { tr("In dialogue"), BusinessLayer::TextParagraphType::Dialogue } });
}

void ScreenplayTextView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    setBackgroundColor(Ui::DesignSystem::color().surface());

    d->updateToolbarUi();

    d->textEdit->setPageSpacing(Ui::DesignSystem::layout().px24());
    QPalette palette;
    palette.setColor(QPalette::Window, Ui::DesignSystem::color().surface());
    palette.setColor(QPalette::Base, Ui::DesignSystem::color().textEditor());
    palette.setColor(QPalette::Text, Ui::DesignSystem::color().onTextEditor());
    palette.setColor(QPalette::Highlight, Ui::DesignSystem::color().accent());
    palette.setColor(QPalette::HighlightedText, Ui::DesignSystem::color().onAccent());
    d->scalableWrapper->setPalette(palette);
    d->textEdit->setPalette(palette);
    palette.setColor(QPalette::Base, Qt::transparent);
    d->textEdit->viewport()->setPalette(palette);
    d->textEdit->completer()->setTextColor(Ui::DesignSystem::color().onBackground());
    d->textEdit->completer()->setBackgroundColor(Ui::DesignSystem::color().background());

    d->splitter->setBackgroundColor(Ui::DesignSystem::color().surface());

    d->sidebarTabs->setTextColor(Ui::DesignSystem::color().onPrimary());
    d->sidebarTabs->setBackgroundColor(Ui::DesignSystem::color().primary());
    d->sidebarContent->setBackgroundColor(Ui::DesignSystem::color().primary());
}

} // namespace Ui
