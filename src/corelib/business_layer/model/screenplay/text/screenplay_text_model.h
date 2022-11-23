#pragma once

#include <business_layer/model/text/text_model.h>


namespace BusinessLayer {

class CharacterModel;
class CharactersModel;
class LocationModel;
class LocationsModel;
class ScreenplayDictionariesModel;
class ScreenplayInformationModel;

/**
 * @brief Модель текста сценария
 */
class CORE_LIBRARY_EXPORT ScreenplayTextModel : public TextModel
{
    Q_OBJECT

public:
    explicit ScreenplayTextModel(QObject* _parent = nullptr);
    ~ScreenplayTextModel() override;

    /**
     * @brief Создать элементы модели
     */
    TextModelFolderItem* createFolderItem(TextFolderType _type) const override;
    TextModelGroupItem* createGroupItem(TextGroupType _type) const override;
    TextModelTextItem* createTextItem() const override;

    /**
     * @brief Задать модель информации о сценарии
     */
    void setInformationModel(ScreenplayInformationModel* _model);
    ScreenplayInformationModel* informationModel() const;

    /**
     * @brief Задать модель справочников сценария
     */
    void setDictionariesModel(ScreenplayDictionariesModel* _model);
    ScreenplayDictionariesModel* dictionariesModel() const;

    /**
     * @brief Задать модель персонажей проекта
     */
    void setCharactersModel(CharactersModel* _model);
    QAbstractItemModel* charactersModel() const;

    /**
     * @brief Получить модель персонажа по заданному имени
     */
    BusinessLayer::CharacterModel* character(const QString& _name) const;

    /**
     * @brief Создать персонажа с заданным именем
     */
    void createCharacter(const QString& _name);

    /**
     * @brief Обновить имя персонажа
     */
    void updateCharacterName(const QString& _oldName, const QString& _newName);

    /**
     * @brief Найти всех персонажей сценария
     */
    QSet<QString> findCharactersFromText() const;

    /**
     * @brief Задать модель локаций проекта
     */
    void setLocationsModel(LocationsModel* _model);
    QAbstractItemModel* locationsModel() const;

    /**
     * @brief Получить модель локации по заданному имени
     */
    BusinessLayer::LocationModel* location(const QString& _name) const;

    /**
     * @brief Создать локациюс заданным именем
     */
    void createLocation(const QString& _name);

    /**
     * @brief Найти все локации сценария
     */
    QSet<QString> findLocationsFromText() const;

    /**
     * @brief Обновить название локации
     */
    void updateLocationName(const QString& _oldName, const QString& _newName);

    /**
     * @brief Длительность сценария
     */
    std::chrono::milliseconds duration() const;

    /**
     * @brief Получить цвета элементов сценария
     */
    std::map<std::chrono::milliseconds, QColor> itemsColors() const;

    /**
     * @brief Получить цвета заладок элементов сценария
     */
    std::map<std::chrono::milliseconds, QColor> itemsBookmarks() const;

    /**
     * @brief Обновить номера сцен и реплик
     */
    void updateNumbering();

    /**
     * @brief Задать блокировку номеров сцен
     */
    void setScenesNumbersLocked(bool _locked);

    /**
     * @brief Пересчитать хронометраж
     */
    void recalculateDuration();

    /**
     * @brief Настроить справочники сценария, которые собираются во время работы приложения
     */
    void updateRuntimeDictionariesIfNeeded();
    void updateRuntimeDictionaries();

    /**
     * @brief Определим список майм типов для модели
     */
    QStringList mimeTypes() const override;

protected:
    /**
     * @brief Инициилизировать пустой документ
     */
    void initEmptyDocument() override;

    /**
     * @brief Донастроить модель после её инициилизации
     */
    void finalizeInitialization() override;

    /**
     * @brief Добавляем дополнительную логику после применения патча в модели
     */
    void applyPatch(const QByteArray& _patch) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer
