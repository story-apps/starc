#pragma once

#include "../simple_text/simple_text_model.h"


namespace BusinessLayer {

class NovelInformationModel;

/**
 * @brief Модель синопсиса
 */
class CORE_LIBRARY_EXPORT NovelSynopsisModel : public SimpleTextModel
{
    Q_OBJECT

public:
    explicit NovelSynopsisModel(QObject* _parent = nullptr);
    ~NovelSynopsisModel() override;

    /**
     * @brief Название документа
     */
    QString documentName() const override;

    /**
     * @brief Игнорируем установку названия документа
     */
    void setDocumentName(const QString& _name) override;

    /**
     * @brief Задать модель информации
     */
    void setInformationModel(NovelInformationModel* _model);
    NovelInformationModel* informationModel() const;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer
