#pragma once

#include "../simple_text/simple_text_model.h"


namespace BusinessLayer {

class ScreenplayInformationModel;

/**
 * @brief Модель
 */
class CORE_LIBRARY_EXPORT ScreenplaySynopsisModel : public SimpleTextModel
{
    Q_OBJECT

public:
    explicit ScreenplaySynopsisModel(QObject* _parent = nullptr);
    ~ScreenplaySynopsisModel() override;

    /**
     * @brief Название документа
     */
    QString documentName() const override;

    /**
     * @brief Игнорируем установку названия документа
     */
    void setDocumentName(const QString& _name) override;

    /**
     * @brief Задать модель информации о сценарии
     */
    void setInformationModel(ScreenplayInformationModel* _model);
    ScreenplayInformationModel* informationModel() const;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer
