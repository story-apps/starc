#pragma once

#include <business_layer/model/text/text_model.h>

class QStringListModel;


namespace BusinessLayer {

class CharacterModel;
class CharactersModel;
class LocationModel;
class LocationsModel;

/**
 * @brief Модель текста сценария
 */
class CORE_LIBRARY_EXPORT ScriptTextModel : public TextModel
{
    Q_OBJECT

public:
    explicit ScriptTextModel(QObject* _parent, TextModelFolderItem* _rootItem);
    ~ScriptTextModel() override;

    /**
     * @brief Задать модель персонажей проекта
     */
    void setCharactersModel(CharactersModel* _model);
    CharactersModel* charactersModel() const;

    /**
     * @brief Список персонажей
     */
    QAbstractItemModel* charactersList() const;

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
    virtual void updateCharacterName(const QString& _oldName, const QString& _newName) = 0;

    /**
     * @brief Получить список реплик персонажа
     */
    virtual QVector<QModelIndex> characterDialogues(const QString& _name) const = 0;

    /**
     * @brief Найти всех персонажей сценария
     */
    virtual QVector<QString> findCharactersFromText() const = 0;

    /**
     * @brief Модель локаций проекта
     */
    void setLocationsModel(LocationsModel* _model);
    LocationsModel* locationsModel() const;

    /**
     * @brief Список локаций
     */
    QAbstractItemModel* locationsList() const;

    /**
     * @brief Получить модель локации по заданному имени
     */
    BusinessLayer::LocationModel* location(const QString& _name) const;

    /**
     * @brief Создать локациюс заданным именем
     */
    void createLocation(const QString& _name);

    /**
     * @brief Обновить название локации
     */
    virtual void updateLocationName(const QString& _oldName, const QString& _newName) = 0;

    /**
     * @brief Получить список сцен локации
     */
    virtual QVector<QModelIndex> locationScenes(const QString& _name) const = 0;

    /**
     * @brief Найти все локации сценария
     */
    virtual QVector<QString> findLocationsFromText() const = 0;

    /**
     * @brief Настроить справочники сценария, которые собираются во время работы приложения
     */
    void updateRuntimeDictionariesIfNeeded();
    virtual void updateRuntimeDictionaries() = 0;

protected:
    /**
     * @brief Нужно обновить справочники, которые строятся в рантайме
     */
    void markNeedUpdateRuntimeDictionaries();

    /**
     * @brief Справочники, которые строятся в рантайме
     */
    QStringListModel* charactersModelFromText() const;
    QStringListModel* locationsModelFromText() const;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer
