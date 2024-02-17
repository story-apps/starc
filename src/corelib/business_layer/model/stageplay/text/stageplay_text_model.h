#pragma once

#include <business_layer/model/base/script_text_model.h>


namespace BusinessLayer {

class StageplayInformationModel;

/**
 * @brief Модель текста пьесы
 */
class CORE_LIBRARY_EXPORT StageplayTextModel : public ScriptTextModel
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
     * @brief Обновить имя персонажа
     */
    void updateCharacterName(const QString& _oldName, const QString& _newName) override;

    /**
     * @brief Получить список реплик персонажа
     */
    QVector<QModelIndex> characterDialogues(const QString& _name) const override;

    /**
     * @brief Найти всех персонажей пьесы
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
     * @brief Настроить справочники сценария, которые собираются во время работы приложения
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
