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
    TextModelFolderItem* createFolderItem() const override;
    TextModelGroupItem* createGroupItem() const override;
    TextModelTextItem* createTextItem() const override;

    /**
     * @brief Название текстового документа
     */
    QString name() const;
    void setName(const QString& _name);
    void setDocumentName(const QString& _name) override;
    Q_SIGNAL void nameChanged(const QString& _name);

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

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer
