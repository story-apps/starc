#pragma once

#include <business_layer/model/base/script_text_model.h>


namespace BusinessLayer {

class NovelDictionariesModel;
class NovelInformationModel;

/**
 * @brief Модель текста сценария
 */
class CORE_LIBRARY_EXPORT NovelTextModel : public ScriptTextModel
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
     * @brief Восстановить xml после сравнения документов
     */
    QByteArray restoreAfterComparison(const QByteArray& _xml) const override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer
