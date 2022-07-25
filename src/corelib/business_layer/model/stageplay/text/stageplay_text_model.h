#pragma once

#include <business_layer/model/text/text_model.h>


namespace BusinessLayer {

class CharacterModel;
class CharactersModel;
class StageplayInformationModel;

/**
 * @brief Модель текста пьесы
 */
class CORE_LIBRARY_EXPORT StageplayTextModel : public TextModel
{
    Q_OBJECT

public:
    explicit StageplayTextModel(QObject* _parent = nullptr);
    ~StageplayTextModel() override;

    /**
     * @brief Создать элементы модели
     */
    TextModelFolderItem* createFolderItem(TextFolderType _type) const override;
    TextModelGroupItem* createGroupItem(TextGroupType _type) const override;
    TextModelTextItem* createTextItem() const override;

    /**
     * @brief Задать модель информации
     */
    void setInformationModel(StageplayInformationModel* _model);
    StageplayInformationModel* informationModel() const;

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
     * @brief Найти всех персонажей пьесы
     */
    QSet<QString> findCharactersFromText() const;

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
