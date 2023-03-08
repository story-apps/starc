#pragma once

#include <business_layer/model/text/text_model.h>


namespace BusinessLayer {

class CharacterModel;
class CharactersModel;
class AudioplayInformationModel;

/**
 * @brief Модель текста аудиопостановки
 */
class CORE_LIBRARY_EXPORT AudioplayTextModel : public TextModel
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
    ChangeCursor applyPatch(const QByteArray& _patch) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer
