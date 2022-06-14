#pragma once

#include <business_layer/model/text/text_model.h>


namespace BusinessLayer {

class CharacterModel;
class CharactersModel;
class ComicBookDictionariesModel;
class ComicBookInformationModel;

/**
 * @brief Модель текста комикса
 */
class CORE_LIBRARY_EXPORT ComicBookTextModel : public TextModel
{
    Q_OBJECT

public:
    explicit ComicBookTextModel(QObject* _parent = nullptr);
    ~ComicBookTextModel() override;

    /**
     * @brief Создать элементы модели
     */
    TextModelFolderItem* createFolderItem() const override;
    TextModelGroupItem* createGroupItem(TextGroupType _type) const override;
    TextModelTextItem* createTextItem() const override;

    /**
     * @brief Задать модель информации о комиксе
     */
    void setInformationModel(ComicBookInformationModel* _model);
    ComicBookInformationModel* informationModel() const;

    /**
     * @brief Задать модель справочников комикса
     */
    void setDictionariesModel(ComicBookDictionariesModel* _model);
    ComicBookDictionariesModel* dictionariesModel() const;

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
     * @brief Настроить справочники, которые собираются во время работы приложения
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

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer
