#pragma once

#include <business_layer/model/text/text_model.h>


namespace BusinessLayer {

/**
 * @brief Модель текстового документа
 */
class CORE_LIBRARY_EXPORT SimpleTextModel : public TextModel
{
    Q_OBJECT

public:
    explicit SimpleTextModel(QObject* _parent = nullptr);
    ~SimpleTextModel() override;

    /**
     * @brief Создать элементы модели
     */
    TextModelFolderItem* createFolderItem(TextFolderType _type) const override;
    TextModelGroupItem* createGroupItem(TextGroupType _type) const override;
    TextModelTextItem* createTextItem() const override;

    /**
     * @brief Название текстового документа
     */
    QString name() const;
    void setName(const QString& _name);
    Q_SIGNAL void nameChanged(const QString& _name);
    QString documentName() const override;
    void setDocumentName(const QString& _name) override;

    /**
     * @brief Определим список майм типов для модели
     */
    QStringList mimeTypes() const override;

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
     * @brief Обновить название документа
     */
    virtual void updateDisplayName(const QModelIndex& _index);

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

    /**
     * @brief Восстановить xml после сравнения документов
     */
    QByteArray restoreAfterComparison(const QByteArray& _xml) const override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer
