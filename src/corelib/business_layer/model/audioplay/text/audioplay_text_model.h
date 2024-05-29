#pragma once

#include <business_layer/model/base/script_text_model.h>


namespace BusinessLayer {

class AudioplayInformationModel;

/**
 * @brief Модель текста аудиопостановки
 */
class CORE_LIBRARY_EXPORT AudioplayTextModel : public ScriptTextModel
{
    Q_OBJECT

public:
    explicit AudioplayTextModel(QObject* _parent = nullptr);
    ~AudioplayTextModel() override;

    /**
     * @brief Создать элементы модели
     */
    TextModelFolderItem* createFolderItem(TextFolderType _type) const override;
    TextModelGroupItem* createGroupItem(TextGroupType _type) const override;
    TextModelTextItem* createTextItem() const override;

    /**
     * @brief Задать модель информации
     */
    void setInformationModel(AudioplayInformationModel* _model);
    AudioplayInformationModel* informationModel() const;

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
     * @brief Пересчитать счетчики
     */
    void recalculateCounters();

    /**
     * @brief Настроить справочники сценария, которые собираются во время работы приложения
     */
    void updateRuntimeDictionaries() override;

    /**
     * @brief Определим список майм типов для модели
     */
    QStringList mimeTypes() const override;

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
     * @brief Количество страниц текста сценария
     */
    int textPageCount() const;
    void setTextPageCount(int _count);

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
