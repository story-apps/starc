#include "screenplay_dictionaries_model.h"

#include <domain/document_object.h>
#include <utils/helpers/text_helper.h>

#include <QDomDocument>


namespace BusinessLayer {

namespace {

const QLatin1String kDocumentKey("document");
const QLatin1String kPageIntrosKey("scene_intros");
const QLatin1String kSceneTimesKey("scene_times");
const QLatin1String kCharacterExtensionsKey("character_extensions");
const QLatin1String kTransitionsKey("transitions");
const QLatin1String kStoryDaysKey("story_days");
const QLatin1String kTagsKey("tags");
const QLatin1String kResourceCategoriesKey("resource_categories");
const QLatin1String kResourcesKey("resources");
const QLatin1String kItemKey("v");
const QLatin1String kItemParentUuidAttribute("parent_uuid");
const QLatin1String kItemUuidAttribute("uuid");
const QLatin1String kItemNameAttribute("name");
const QLatin1String kItemColorAttribute("color");
const QLatin1String kItemIconAttribute("icon");
const QLatin1String kItemCountAttribute("count");
const QLatin1String kItemHasIdsAttribute("has_ids");

struct ColorsComparator {
    bool operator()(const QPair<QString, QColor>& _lhs, const QPair<QString, QColor>& _rhs) const
    {
        return _lhs.first < _rhs.first && _lhs.second.name() < _rhs.second.name();
    }
};

} // namespace


bool operator==(const BreakdownSceneResource& _lhs, const BreakdownSceneResource& _rhs)
{
    return _lhs.uuid == _rhs.uuid;
}


class ScreenplayDictionariesModel::Implementation
{
public:
    QVector<QString> sceneIntros;
    QVector<QString> sceneTimes;
    QVector<QString> characterExtensions;
    QVector<QString> transitions;

    /**
     * @brief Дни истории <название, кол-во>
     */
    std::map<QString, int> storyDays;

    /**
     * @brief Тэги карточек <тэг, кол-вл>
     */
    std::map<QPair<QString, QColor>, int, ColorsComparator> tags;

    /**
     * @brief Категории ресурсов
     */
    QVector<BreakdownResourceCategory> resourceCategories;

    /**
     * @brief Ресурсы
     */
    QVector<BreakdownResource> resources;
};


// ****


ScreenplayDictionariesModel::ScreenplayDictionariesModel(QObject* _parent)
    : AbstractModel({ kDocumentKey, kPageIntrosKey, kSceneTimesKey, kStoryDaysKey,
                      kCharacterExtensionsKey, kTransitionsKey, kItemKey },
                    _parent)
    , d(new Implementation)
{
    connect(this, &ScreenplayDictionariesModel::sceneIntrosChanged, this,
            &ScreenplayDictionariesModel::updateDocumentContent);
    connect(this, &ScreenplayDictionariesModel::sceneTimesChanged, this,
            &ScreenplayDictionariesModel::updateDocumentContent);
    connect(this, &ScreenplayDictionariesModel::storyDaysChanged, this,
            &ScreenplayDictionariesModel::updateDocumentContent);
    connect(this, &ScreenplayDictionariesModel::charactersExtensionsChanged, this,
            &ScreenplayDictionariesModel::updateDocumentContent);
    connect(this, &ScreenplayDictionariesModel::transitionsChanged, this,
            &ScreenplayDictionariesModel::updateDocumentContent);
    connect(this, &ScreenplayDictionariesModel::tagsChanged, this,
            &ScreenplayDictionariesModel::updateDocumentContent);
    connect(this, &ScreenplayDictionariesModel::resourceCategoriesChanged, this,
            &ScreenplayDictionariesModel::updateDocumentContent);
    connect(this, &ScreenplayDictionariesModel::resourcesChanged, this,
            &ScreenplayDictionariesModel::updateDocumentContent);
}

ScreenplayDictionariesModel::~ScreenplayDictionariesModel() = default;

const QVector<QString>& ScreenplayDictionariesModel::sceneIntros() const
{
    return d->sceneIntros;
}

void ScreenplayDictionariesModel::addSceneIntro(const QString& _intro)
{
    const auto introCorrected = TextHelper::smartToUpper(_intro);
    if (d->sceneIntros.contains(introCorrected)) {
        return;
    }

    d->sceneIntros.append(introCorrected);
    emit sceneIntrosChanged();
}

void ScreenplayDictionariesModel::setSceneIntro(int _index, const QString& _intro)
{
    const auto introCorrected = TextHelper::smartToUpper(_intro);
    if (d->sceneIntros.contains(introCorrected)) {
        return;
    }

    if (_index < 0 || d->sceneIntros.size() <= _index) {
        return;
    }

    d->sceneIntros[_index] = introCorrected;
    emit sceneIntrosChanged();
}

void ScreenplayDictionariesModel::removeSceneIntro(int _index)
{
    if (_index < 0 || d->sceneIntros.size() <= _index) {
        return;
    }

    d->sceneIntros.removeAt(_index);
    emit sceneIntrosChanged();
}

const QVector<QString>& ScreenplayDictionariesModel::sceneTimes() const
{
    return d->sceneTimes;
}

void ScreenplayDictionariesModel::addSceneTime(const QString& _time)
{
    const auto timeCorrected = TextHelper::smartToUpper(_time);
    if (d->sceneTimes.contains(timeCorrected)) {
        return;
    }

    d->sceneTimes.append(timeCorrected);
    emit sceneTimesChanged();
}

void ScreenplayDictionariesModel::setSceneTime(int _index, const QString& _time)
{
    const auto timeCorrected = TextHelper::smartToUpper(_time);
    if (d->sceneTimes.contains(timeCorrected)) {
        return;
    }

    if (_index < 0 || d->sceneTimes.size() <= _index) {
        return;
    }

    d->sceneTimes[_index] = timeCorrected;
    emit sceneTimesChanged();
}

void ScreenplayDictionariesModel::removeSceneTime(int _index)
{
    if (_index < 0 || d->sceneTimes.size() <= _index) {
        return;
    }

    d->sceneTimes.removeAt(_index);
    emit sceneTimesChanged();
}

const QVector<QString>& ScreenplayDictionariesModel::characterExtensions() const
{
    return d->characterExtensions;
}

void ScreenplayDictionariesModel::addCharacterExtension(const QString& _extension)
{
    const auto extensionCorrected = TextHelper::smartToUpper(_extension);
    if (d->characterExtensions.contains(extensionCorrected)) {
        return;
    }

    d->characterExtensions.append(extensionCorrected);
    emit charactersExtensionsChanged();
}

void ScreenplayDictionariesModel::setCharacterExtension(int _index, const QString& _extension)
{
    const auto extensionCorrected = TextHelper::smartToUpper(_extension);
    if (d->characterExtensions.contains(extensionCorrected)) {
        return;
    }

    if (_index < 0 || d->characterExtensions.size() <= _index) {
        return;
    }

    d->characterExtensions[_index] = extensionCorrected;
    emit charactersExtensionsChanged();
}

void ScreenplayDictionariesModel::removeCharacterExtension(int _index)
{
    if (_index < 0 || d->characterExtensions.size() <= _index) {
        return;
    }

    d->characterExtensions.removeAt(_index);
    emit charactersExtensionsChanged();
}

const QVector<QString>& ScreenplayDictionariesModel::transitions() const
{
    return d->transitions;
}

void ScreenplayDictionariesModel::addTransition(const QString& _transition)
{
    const auto transitionsCorrected = TextHelper::smartToUpper(_transition);
    if (d->transitions.contains(transitionsCorrected)) {
        return;
    }

    d->transitions.append(transitionsCorrected);
    emit transitionsChanged();
}

void ScreenplayDictionariesModel::setTransition(int _index, const QString& _transition)
{
    const auto transitionCorrected = TextHelper::smartToUpper(_transition);
    if (d->transitions.contains(transitionCorrected)) {
        return;
    }

    if (_index < 0 || d->transitions.size() <= _index) {
        return;
    }

    d->transitions[_index] = transitionCorrected;
    emit transitionsChanged();
}

void ScreenplayDictionariesModel::removeTransition(int _index)
{
    if (_index < 0 || d->transitions.size() <= _index) {
        return;
    }

    d->transitions.removeAt(_index);
    emit transitionsChanged();
}

QVector<QString> ScreenplayDictionariesModel::storyDays() const
{
    QVector<QString> storyDays;
    for (const auto& [storyDay, count] : d->storyDays) {
        storyDays.append(storyDay);
    }
    return storyDays;
}

void ScreenplayDictionariesModel::addStoryDay(const QString& _day)
{
    if (_day.isEmpty()) {
        return;
    }

    auto iter = d->storyDays.find(_day);
    if (iter == d->storyDays.end()) {
        d->storyDays.emplace(_day, 1);
    } else {
        ++d->storyDays[iter->first];
    }
    emit storyDaysChanged();
}

void ScreenplayDictionariesModel::removeStoryDay(const QString& _day)
{
    auto iter = d->storyDays.find(_day);
    if (iter == d->storyDays.end()) {
        return;
    }

    if (iter->second == 1) {
        d->storyDays.erase(iter);
    } else {
        --d->storyDays[iter->first];
    }
    emit storyDaysChanged();
}

QVector<QPair<QString, QColor>> ScreenplayDictionariesModel::tags() const
{
    QVector<QPair<QString, QColor>> tags;
    for (const auto& [tag, count] : d->tags) {
        tags.append(tag);
    }
    return tags;
}

void ScreenplayDictionariesModel::addTags(const QVector<QPair<QString, QColor>>& _tags)
{
    for (const auto& tag : _tags) {
        if (tag.first.isEmpty()) {
            continue;
        }

        auto iter = d->tags.find(tag);
        if (iter == d->tags.end()) {
            d->tags.emplace(tag, 1);
        } else {
            ++d->tags[iter->first];
        }
    }
    emit tagsChanged();
}

void ScreenplayDictionariesModel::removeTags(const QVector<QPair<QString, QColor>>& _tags)
{
    for (const auto& tag : _tags) {
        auto iter = d->tags.find(tag);
        if (iter == d->tags.end()) {
            continue;
        }

        if (iter->second == 1) {
            d->tags.erase(iter);
        } else {
            --d->tags[iter->first];
        }
    }
    emit tagsChanged();
}

QVector<BreakdownResourceCategory> ScreenplayDictionariesModel::resourceCategories() const
{
    return d->resourceCategories;
}

void ScreenplayDictionariesModel::addResourceCategory(const QString& _name, const QString& _icon,
                                                      const QColor& _color, bool _hasIds)
{
    if (_name.isEmpty()) {
        return;
    }

    d->resourceCategories.append({ QUuid::createUuid(), _name, _icon, _color, _hasIds });
    emit resourceCategoriesChanged();
}

void ScreenplayDictionariesModel::setResourceCategory(const QUuid& _uuid, const QString& _name,
                                                      const QString& _icon, const QColor& _color,
                                                      bool _hasIds)
{
    Q_UNUSED(_icon)

    for (auto& category : d->resourceCategories) {
        if (category.uuid != _uuid) {
            continue;
        }

        category.name = _name;
        //        category.icon = _icon;
        category.color = _color;
        category.hasIds = _hasIds;
        emit resourceCategoriesChanged();
        break;
    }
}

void ScreenplayDictionariesModel::moveResourceCategory(const QUuid& _uuid, int _index)
{
    if (_index < 0 || _index >= d->resourceCategories.size()) {
        return;
    }

    for (int index = 0; index < d->resourceCategories.size(); ++index) {
        if (d->resourceCategories.at(index).uuid != _uuid) {
            continue;
        }

        d->resourceCategories.move(index, _index);
        emit resourceCategoriesChanged();
        break;
    }
}

void ScreenplayDictionariesModel::removeResourceCategory(const QUuid& _uuid)
{
    for (int index = 0; index < d->resourceCategories.size(); ++index) {
        if (d->resourceCategories.at(index).uuid != _uuid) {
            continue;
        }

        d->resourceCategories.removeAt(index);

        bool isResourcesChanged = false;
        for (int resourceIndex = 0; resourceIndex < d->resources.size(); ++resourceIndex) {
            if (d->resources.at(resourceIndex).categoryUuid == _uuid) {
                d->resources.removeAt(resourceIndex);
                --resourceIndex;
                isResourcesChanged = true;
            }
        }

        emit resourceCategoriesChanged();
        if (isResourcesChanged) {
            emit resourcesChanged();
        }
        break;
    }
}

QVector<BreakdownResource> ScreenplayDictionariesModel::resources() const
{
    return d->resources;
}

void ScreenplayDictionariesModel::addResource(const QUuid& _categoryUuid, const QString& _name,
                                              const QString& _description)
{
    if (_categoryUuid.isNull() || _name.isEmpty()) {
        return;
    }

    //
    // Проверяем, существует ли указанная категория
    //
    bool isResourceCategoryExists = false;
    for (const auto& resourceCategory : std::as_const(d->resourceCategories)) {
        if (resourceCategory.uuid == _categoryUuid) {
            isResourceCategoryExists = true;
            break;
        }
    }
    if (!isResourceCategoryExists) {
        return;
    }

    //
    // Добавляем ресурс и уведомляем клиентов
    //
    d->resources.append({ QUuid::createUuid(), _categoryUuid, _name, _description });
    emit resourcesChanged();
}

void ScreenplayDictionariesModel::setResource(const QUuid& _uuid, const QUuid& _categoryUuid,
                                              const QString& _name, const QString& _description)
{
    for (auto& resource : d->resources) {
        if (resource.uuid != _uuid) {
            continue;
        }

        resource.categoryUuid = _categoryUuid;
        resource.name = _name;
        resource.description = _description;
        emit resourcesChanged();
        break;
    }
}

void ScreenplayDictionariesModel::setResourceCategory(const QUuid& _uuid,
                                                      const QUuid& _categoryUuid)
{
    for (auto& resource : d->resources) {
        if (resource.uuid != _uuid) {
            continue;
        }

        if (resource.categoryUuid != _categoryUuid) {
            resource.categoryUuid = _categoryUuid;
            emit resourcesChanged();
        }
        break;
    }
}

void ScreenplayDictionariesModel::moveResource(const QUuid& _uuid, int _index)
{
    if (_index < 0 || _index >= d->resources.size()) {
        return;
    }

    for (int index = 0; index < d->resources.size(); ++index) {
        if (d->resources.at(index).uuid != _uuid) {
            continue;
        }

        d->resources.move(index, _index);
        emit resourcesChanged();
        break;
    }
}

void ScreenplayDictionariesModel::removeResource(const QUuid& _uuid)
{
    for (int index = 0; index < d->resources.size(); ++index) {
        if (d->resources.at(index).uuid == _uuid) {
            d->resources.removeAt(index);
            emit resourcesChanged();
            break;
        }
    }
}

void ScreenplayDictionariesModel::initDocument()
{
    if (document() == nullptr) {
        return;
    }

    const auto shouldBeInitialized = document()->content().isEmpty();

    QDomDocument domDocument;
    domDocument.setContent(document()->content());
    const auto documentNode = domDocument.firstChildElement(kDocumentKey);
    auto fillDictionary = [documentNode, shouldBeInitialized](const QString& _key,
                                                              const QVector<QString>& _defaultItems,
                                                              QVector<QString>& _dictionary) {
        if (shouldBeInitialized) {
            _dictionary.append(_defaultItems);
            return;
        }

        const auto dictionaryNode = documentNode.firstChildElement(_key);
        auto itemNode = dictionaryNode.firstChildElement();
        while (!itemNode.isNull()) {
            _dictionary.append(TextHelper::fromHtmlEscaped(itemNode.text()));
            itemNode = itemNode.nextSiblingElement();
        }
    };
    const QVector<QString> defaultSceneIntros = {
        tr("INT."),
        tr("EXT."),
        tr("INT./EXT."),
        tr("EXT./INT."),
    };
    fillDictionary(kPageIntrosKey, defaultSceneIntros, d->sceneIntros);
    //
    const QVector<QString> defaultSceneTimes = {
        tr("DAY"),   tr("NIGHT"),         tr("MORNING"),    tr("AFTERNOON"),    tr("EVENING"),
        tr("LATER"), tr("MOMENTS LATER"), tr("CONTINUOUS"), tr("THE NEXT DAY"),
    };
    fillDictionary(kSceneTimesKey, defaultSceneTimes, d->sceneTimes);
    //
    const QVector<QString> defaultCharacterExtensions = {
        tr("V.O."), tr("O.S."), tr("O.C."), tr("SUBTITLE"), tr("CONT'D"),
    };
    fillDictionary(kCharacterExtensionsKey, defaultCharacterExtensions, d->characterExtensions);
    //
    const QVector<QString> defaultTransitions = {
        tr("CUT TO:"),       tr("FADE IN:"),     tr("FADE OUT"),
        tr("FADE TO:"),      tr("DISSOLVE TO:"), tr("BACK TO:"),
        tr("MATCH CUT TO:"), tr("JUMP CUT TO:"), tr("FADE TO BLACK"),
    };
    fillDictionary(kTransitionsKey, defaultTransitions, d->transitions);
    //
    if (!shouldBeInitialized) {
        const auto storyDaysNode = documentNode.firstChildElement(kStoryDaysKey);
        auto storyDayNode = storyDaysNode.firstChildElement();
        while (!storyDayNode.isNull()) {
            const auto storyDay = TextHelper::fromHtmlEscaped(storyDayNode.text());
            const auto count = storyDayNode.hasAttribute(kItemCountAttribute)
                ? storyDayNode.attributeNode(kItemCountAttribute).value().toInt()
                : 1;
            d->storyDays.emplace(storyDay, count);
            storyDayNode = storyDayNode.nextSiblingElement();
        }
    }
    //
    if (!shouldBeInitialized) {
        const auto tagsNode = documentNode.firstChildElement(kTagsKey);
        auto tagNode = tagsNode.firstChildElement();
        while (!tagNode.isNull()) {
            const auto tag = qMakePair(TextHelper::fromHtmlEscaped(tagNode.text()),
                                       tagNode.attributeNode(kItemColorAttribute).value());
            const auto count = tagNode.hasAttribute(kItemCountAttribute)
                ? tagNode.attributeNode(kItemCountAttribute).value().toInt()
                : 1;
            d->tags.emplace(tag, count);
            tagNode = tagNode.nextSiblingElement();
        }
    }
    //
    if (shouldBeInitialized) {
        //
        // Хардкодим UUID'ы чтобы дефолтный xml всегда был одинаковым
        //
        // clang-format off
        for (const auto& resourceCategory : std::vector<BreakdownResourceCategory>{
                 { QUuid("{ef2711ff-f92e-4fb0-9f53-101acd0b382c}"), tr("Backgroun actors (atmosphere)"), u8"\U000F05CB", "#006724" },
                 { QUuid("{0f4a8d40-82b2-4d10-bebc-40cefb5a99f9}"), tr("Backgroun actors (silent)"), u8"\U000F0849", "#009434" },
                 { QUuid("{a8ebbb8c-f6fc-4c83-b154-e1d8bf88ba47}"), tr("Backgroun actors (special)"), u8"\U000F0017", "#2db75e" },
                 { QUuid("{79c87ced-e531-4c92-ae1f-89e90c00b6d2}"), tr("Stunts"), u8"\U000F1A41", "#d61530" },
                 { QUuid("{6a913b68-6245-441b-a034-e14e9810de3d}"), tr("Vehicles"), u8"\U000F010B", "#00acbe" },
                 { QUuid("{331c6ed2-bad6-4305-bf2c-1b25b69430da}"), tr("Props"), u8"\U000F0E10", "#a56334" },
                 { QUuid("{afb07d68-ad59-417b-989f-512c687577c6}"), tr("Camera"), u8"\U000F0567", "#c0da61" },
                 { QUuid("{6cc4a061-6a9c-42b1-82f6-61287feb96d5}"), tr("Special effects"), u8"\U000F0F35", "#78a9af" },
                 { QUuid("{7700078e-fbca-417a-a596-e5f00fec534c}"), tr("Wardrobe"), u8"\U000F0A7B", "#ff6500" },
                 { QUuid("{420da819-7233-47db-a6b3-9319d5b29b31}"), tr("Makeup/hair"), u8"\U000F1077", "#f400ee" },
                 { QUuid("{11548941-c278-4e7a-862b-45b8c1b01bfc}"), tr("Animals"), u8"\U000F1A61", "#abbb18" },
                 { QUuid("{1e7212d1-e50b-48d5-a669-385b4c8c4c14}"), tr("Animal handler"), u8"\U000F0E9B", "#dead00" },
                 { QUuid("{82874cf3-8222-49d1-9222-33681bf10262}"), tr("Music"), u8"\U000F075A", "#007880" },
                 { QUuid("{a9e55c5a-4373-426a-bdd4-19b52f24eb03}"), tr("Sound"), u8"\U000F057E", "#a10f81" },
                 { QUuid("{4babab66-164f-4dfc-bd8a-732bf674a889}"), tr("Set dressing"), u8"\U000F011A", "#00ef47" },
                 { QUuid("{f61bf97e-08cb-4591-b8c4-ddb72979eae0}"), tr("Greenery"), u8"\U000F0531", "#00ba69" },
                 { QUuid("{c4efceaf-1ed6-4094-a5ef-d1b81e872b43}"), tr("Special equipment"), u8"\U000F0841", "#ff0000" },
                 { QUuid("{6799a18c-4a5d-4599-9707-4f5467727dbd}"), tr("Security"), u8"\U000F0483", {} },
                 { QUuid("{4166f271-1f2d-411a-86fd-b932e4c3fda8}"), tr("Additional labor"), u8"\U000F05B5", {} },
                 { QUuid("{c508420d-d473-45ba-8d0f-40be453d9e10}"), tr("Optical FX (Visual FX)"), u8"\U000F086D", {} },
                 { QUuid("{9e85ca3a-3319-491b-8504-5d83363fc05c}"), tr("Mechanical FX"), u8"\U000F0210", {} },
             }) {
            d->resourceCategories.append(resourceCategory);
        }
        // clang-format on
    } else {
        const auto resourceCategoriesNode = documentNode.firstChildElement(kResourceCategoriesKey);
        auto resourceCategoryNode = resourceCategoriesNode.firstChildElement();
        while (!resourceCategoryNode.isNull()) {
            BreakdownResourceCategory resourceCategory;
            resourceCategory.uuid = resourceCategoryNode.attributeNode(kItemUuidAttribute).value();
            resourceCategory.name = TextHelper::fromHtmlEscaped(resourceCategoryNode.text());
            resourceCategory.icon = resourceCategoryNode.attributeNode(kItemIconAttribute).value();
            if (resourceCategoryNode.hasAttribute(kItemColorAttribute)) {
                resourceCategory.color
                    = resourceCategoryNode.attributeNode(kItemColorAttribute).value();
            }
            resourceCategory.hasIds = resourceCategoryNode.hasAttribute(kItemHasIdsAttribute);
            d->resourceCategories.append(resourceCategory);
            resourceCategoryNode = resourceCategoryNode.nextSiblingElement();
        }
    }
    //
    if (!shouldBeInitialized) {
        const auto resourcesNode = documentNode.firstChildElement(kResourcesKey);
        auto resourceNode = resourcesNode.firstChildElement();
        while (!resourceNode.isNull()) {
            BreakdownResource resource;
            resource.uuid = resourceNode.attributeNode(kItemUuidAttribute).value();
            resource.categoryUuid = resourceNode.attributeNode(kItemParentUuidAttribute).value();
            resource.name = resourceNode.attributeNode(kItemNameAttribute).value();
            resource.description = TextHelper::fromHtmlEscaped(resourceNode.text());
            d->resources.append(resource);
            resourceNode = resourceNode.nextSiblingElement();
        }
    }

    if (shouldBeInitialized) {
        updateDocumentContent();
    }
}

void ScreenplayDictionariesModel::clearDocument()
{
    QSignalBlocker signalBlocker(this);

    d->sceneIntros.clear();
    d->sceneTimes.clear();
    d->characterExtensions.clear();
    d->transitions.clear();
    d->storyDays.clear();
    d->tags.clear();
    d->resourceCategories.clear();
    d->resources.clear();
}

QByteArray ScreenplayDictionariesModel::toXml() const
{
    if (document() == nullptr) {
        return {};
    }

    QByteArray xml = "<?xml version=\"1.0\"?>\n";
    xml += QString("<%1 mime-type=\"%2\" version=\"1.0\">\n")
               .arg(kDocumentKey, Domain::mimeTypeFor(document()->type()))
               .toUtf8();
    auto writeDictionary = [&xml](const QString& _key, const QVector<QString>& _values) {
        xml += QString("<%1>\n").arg(_key).toUtf8();
        for (const auto& value : _values) {
            xml += QString("<%1><![CDATA[%2]]></%1>\n")
                       .arg(kItemKey, TextHelper::toHtmlEscaped(value))
                       .toUtf8();
        }
        xml += QString("</%1>\n").arg(_key).toUtf8();
    };
    writeDictionary(kPageIntrosKey, d->sceneIntros);
    writeDictionary(kSceneTimesKey, d->sceneTimes);
    writeDictionary(kCharacterExtensionsKey, d->characterExtensions);
    writeDictionary(kTransitionsKey, d->transitions);
    //
    xml += QString("<%1>\n").arg(kStoryDaysKey).toUtf8();
    for (const auto& [storyDay, count] : std::as_const(d->storyDays)) {
        xml += QString("<%1 %2=\"%3\"><![CDATA[%4]]></%1>\n")
                   .arg(kItemKey, kItemCountAttribute, QString::number(count),
                        TextHelper::toHtmlEscaped(storyDay))
                   .toUtf8();
    }
    xml += QString("</%1>\n").arg(kStoryDaysKey).toUtf8();
    //
    xml += QString("<%1>\n").arg(kTagsKey).toUtf8();
    for (const auto& [tag, count] : std::as_const(d->tags)) {
        xml += QString("<%1 %2=\"%3\" %4=\"%5\"><![CDATA[%6]]></%1>\n")
                   .arg(kItemKey, kItemColorAttribute, tag.second.name(), kItemCountAttribute,
                        QString::number(count), TextHelper::toHtmlEscaped(tag.first))
                   .toUtf8();
    }
    xml += QString("</%1>\n").arg(kTagsKey).toUtf8();
    //
    xml += QString("<%1>\n").arg(kResourceCategoriesKey).toUtf8();
    for (const auto& resourceCategory : std::as_const(d->resourceCategories)) {
        xml += QString("<%1 %2=\"%3\" %4=\"%5\" %6%7><![CDATA[%8]]></%1>\n")
                   .arg(kItemKey, kItemUuidAttribute, resourceCategory.uuid.toString(),
                        kItemIconAttribute, resourceCategory.icon,
                        (resourceCategory.color.isValid()
                             ? QString(" %1=\"%2\"")
                                   .arg(kItemColorAttribute, resourceCategory.color.name())
                             : ""),
                        (resourceCategory.hasIds ? QString(" %1=\"true\"").arg(kItemHasIdsAttribute)
                                                 : ""),
                        TextHelper::toHtmlEscaped(resourceCategory.name))
                   .toUtf8();
    }
    xml += QString("</%1>\n").arg(kResourceCategoriesKey).toUtf8();
    //
    xml += QString("<%1>\n").arg(kResourcesKey).toUtf8();
    for (const auto& resource : std::as_const(d->resources)) {
        xml += QString("<%1 %2=\"%3\" %4=\"%5\" %6=\"%7\"><![CDATA[%8]]></%1>\n")
                   .arg(kItemKey, kItemUuidAttribute, resource.uuid.toString(),
                        kItemParentUuidAttribute, resource.categoryUuid.toString(),
                        kItemNameAttribute, TextHelper::toHtmlEscaped(resource.name),
                        TextHelper::toHtmlEscaped(resource.description))
                   .toUtf8();
    }
    xml += QString("</%1>\n").arg(kResourcesKey).toUtf8();

    xml += QString("</%1>").arg(kDocumentKey).toUtf8();
    return xml;
}

} // namespace BusinessLayer
