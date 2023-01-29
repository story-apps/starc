#pragma once

#include "../simple_text/simple_text_model.h"


namespace BusinessLayer {

class StageplayInformationModel;

/**
 * @brief Модель синопсиса аудиопостановки
 */
class CORE_LIBRARY_EXPORT StageplaySynopsisModel : public SimpleTextModel
{
    Q_OBJECT

public:
    explicit StageplaySynopsisModel(QObject* _parent = nullptr);
    ~StageplaySynopsisModel() override;

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
    void setInformationModel(StageplayInformationModel* _model);
    StageplayInformationModel* informationModel() const;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer
