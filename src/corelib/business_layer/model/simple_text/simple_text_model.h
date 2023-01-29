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
     * @brief Перезаписать содержимое документа
     */
    void setDocumentContent(const QByteArray& _content);

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
