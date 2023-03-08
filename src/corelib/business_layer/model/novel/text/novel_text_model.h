#pragma once

#include <business_layer/model/text/text_model.h>


namespace BusinessLayer {

class CharacterModel;
class CharactersModel;
class LocationModel;
class LocationsModel;
class NovelDictionariesModel;
class NovelInformationModel;

/**
 * @brief Модель текста сценария
 */
class CORE_LIBRARY_EXPORT NovelTextModel : public TextModel
{
    Q_OBJECT

public:
    explicit NovelTextModel(QObject* _parent = nullptr);
    ~NovelTextModel() override;

    /**
     * @brief Название документа
     */
    QString documentName() const override;

    /**
     * @brief Создать элементы модели
     */
    TextModelFolderItem* createFolderItem(TextFolderType _type) const override;
    TextModelGroupItem* createGroupItem(TextGroupType _type) const override;
    TextModelTextItem* createTextItem() const override;

    /**
     * @brief Задать модель информации о сценарии
     */
    void setInformationModel(NovelInformationModel* _model);
    NovelInformationModel* informationModel() const;

    /**
     * @brief Задать модель справочников сценария
     */
    void setDictionariesModel(NovelDictionariesModel* _model);
    NovelDictionariesModel* dictionariesModel() const;

    /**
     * @brief Задать модель персонажей проекта
     */
    void setCharactersModel(CharactersModel* _model);
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
    void updateCharacterName(const QString& _oldName, const QString& _newName);

    /**
     * @brief Получить список реплик персонажа
     */
    QVector<QModelIndex> characterDialogues(const QString& _name) const;

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
     * @brief Количество страниц текста поэпизодника
     */
    int outlinePageCount() const;
    void setOutlinePageCount(int _count);

    /**
     * @brief Количество страниц текста сценария
     */
    int textPageCount() const;
    void setTextPageCount(int _count);

    /**
     * @brief Количество сцен
     */
    int scenesCount() const;

    /**
     * @brief Количество слов
     */
    int wordsCount() const;

    /**
     * @brief Количество символов
     */
    QPair<int, int> charactersCount() const;

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

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer
