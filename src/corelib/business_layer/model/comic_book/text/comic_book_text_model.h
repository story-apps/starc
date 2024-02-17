#pragma once

#include <business_layer/model/base/script_text_model.h>


namespace BusinessLayer {

class ComicBookDictionariesModel;
class ComicBookInformationModel;

/**
 * @brief Модель текста комикса
 */
class CORE_LIBRARY_EXPORT ComicBookTextModel : public ScriptTextModel
{
    Q_OBJECT

public:
    explicit ComicBookTextModel(QObject* _parent = nullptr);
    ~ComicBookTextModel() override;

    /**
     * @brief Создать элементы модели
     */
    TextModelFolderItem* createFolderItem(TextFolderType _type) const override;
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
     * @brief Обновить имя персонажа
     */
    void updateCharacterName(const QString& _oldName, const QString& _newName) override;

    /**
     * @brief Получить список реплик персонажа
     */
    QVector<QModelIndex> characterDialogues(const QString& _name) const override;

    /**
     * @brief Найти всех персонажей сценария
     */
    QVector<QString> findCharactersFromText() const override;

    /**
     * @brief Обновить название локации
     */
    void updateLocationName(const QString& _oldName, const QString& _newName) override;

    /**
     * @brief Получить список сцен локации
     */
    QVector<QModelIndex> locationScenes(const QString& _name) const override;

    /**
     * @brief Найти все локации сценария
     */
    QVector<QString> findLocationsFromText() const override;

    /**
     * @brief Настроить справочники, которые собираются во время работы приложения
     */
    void updateRuntimeDictionaries() override;

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
    ChangeCursor applyPatch(const QByteArray& _patch) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer
